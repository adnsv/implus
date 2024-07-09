#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <imgui.h>
#include <implus/blocks.hpp>
#include <implus/menu.hpp>
#include <implus/selbox.hpp>
#include <optional>

namespace ImPlus {

char const* OverflowLabel = "...##menu-overflow";

enum placement_target {
    placement_front,
    placement_trail,
    placement_overflow, // overflow submenu is open
    placement_nowhere,  // overflow submenu is closed
};

struct menu_state {
    placement_target placement;
    unsigned level; // 0 = not in a bar, 1 = menu bar root, ...
    float front_pos;
    float trail_pos;
    float item_spacing; // in menus this is usually 2 * Style.ItemSpacing
    float overflow_width;
};

auto GetMenuState() -> menu_state*
{
    // ideally, this should stored within window context
    // for simplicity in current implementation, use global state
    static menu_state _ms;

    return &_ms;
}

auto BeginMainMenuBar() -> bool
{
    auto* ms = GetMenuState();
    if (!ms || ms->level != 0)
        return false;

    if (!ImGui::BeginMainMenuBar())
        return false;

    auto const* window = ImGui::GetCurrentWindowRead();
    auto const menu_rect = window->MenuBarRect();
    ms->level = 1; // ready to place items onto the root level
    ms->front_pos = menu_rect.Min.x + window->DC.MenuBarOffset.x;
    ms->trail_pos = menu_rect.Max.x;
    ms->item_spacing = GImGui->Style.ItemSpacing.x * 2.0f; // precalculate

    // pre-calculate the size of the overflow ("...") menu item
    ms->overflow_width = ImGui::CalcTextSize(OverflowLabel, nullptr, true).x + ms->item_spacing;

    return true;
}

void EndMainMenuBar()
{
    auto* ms = GetMenuState();
    IM_ASSERT(ms && ms->level == 1);
    if (ms->placement == placement_overflow)
        ImGui::EndMenu();
    ms->placement = placement_front;
    ms->level = 0;
    ImGui::EndMainMenuBar();
}

void MenuFrontPlacement()
{
    auto* ms = GetMenuState();
    if (ms && ms->placement == placement_trail)
        ms->placement = placement_front;
}
void MenuTrailPlacement()
{
    auto* ms = GetMenuState();
    if (ms && ms->placement == placement_front)
        ms->placement = placement_trail;
}

auto BeginMenu(char const* label, bool enabled) -> bool
{
    auto* ms = GetMenuState();
    if (!ms || ms->level == 0) {
        // outside of main menu bar
        return ImGui::BeginMenu(label, enabled);
    }

    if (ms->placement == placement_nowhere) {
        // overflow triggered, but not showing any items
        return false;
    }

    if (ms->level > 1 || ms->placement == placement_overflow) {
        // showing as submenu
        auto expanded = ImGui::BeginMenu(label, enabled);
        if (expanded)
            ++ms->level;
        return expanded;
    }

    auto const w = ImGui::CalcTextSize(label, nullptr, true).x + ms->item_spacing;

    auto available_size = ms->trail_pos - ms->front_pos;
    if (w + ms->overflow_width > available_size) {
        // trigger overflow
        auto overflow_expanded = ImGui::BeginMenu(OverflowLabel);
        ms->placement = overflow_expanded ? placement_overflow : placement_nowhere;
        if (!overflow_expanded)
            return false;
        auto expanded = ImGui::BeginMenu(label, enabled);
        if (expanded)
            ++ms->level;
        return expanded;
    }

    auto window = ImGui::GetCurrentWindow();

    if (ms->placement == placement_trail) {
        ms->trail_pos -= w;
        window->DC.CursorPos.x = ms->trail_pos;
        window->DC.CursorMaxPos.x = ImMax(window->DC.CursorMaxPos.x, window->DC.CursorPos.x);
        auto expanded = ImGui::BeginMenu(label, enabled);
        window->DC.CursorPos.x = ms->front_pos;
        if (expanded)
            ++ms->level;
        return expanded;
    }

    // front placement
    window->DC.CursorPos.x = ms->front_pos; // just in case, it should be there already
    auto expanded = ImGui::BeginMenu(label, enabled);
    ms->front_pos = window->DC.CursorPos.x;
    if (expanded)
        ++ms->level;
    return expanded;
}

void EndMenu()
{
    ImGui::EndMenu();

    auto* ms = GetMenuState();
    if (!ms || ms->level == 0)
        return;

    // keep tracking the hierarchy level to know when we return to the root
    // level
    IM_ASSERT(ms->level > 1);
    --ms->level;
}

static bool _IsRootOfOpenMenuSet()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    if ((g.OpenPopupStack.Size <= g.BeginPopupStack.Size) ||
        (window->Flags & ImGuiWindowFlags_ChildMenu))
        return false;
    const ImGuiPopupData* upper_popup = &g.OpenPopupStack[g.BeginPopupStack.Size];
    if (window->DC.NavLayerCurrent != upper_popup->ParentNavLayer)
        return false;
    return upper_popup->Window && (upper_popup->Window->Flags & ImGuiWindowFlags_ChildMenu) &&
           ImGui::IsWindowChildOf(upper_popup->Window, window, true);
}

auto MenuItem(ImPlus::ImID const& id, ImPlus::Icon const& icon, std::string_view caption,
    std::string_view shortcut, bool selected, bool enabled) -> bool
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImVec2 pos = window->DC.CursorPos;

    auto shortcut_blk = ImPlus::TextBlock(shortcut, {0, 0.5f});

    auto const menuset_is_open = _IsRootOfOpenMenuSet();
    if (menuset_is_open)
        ImGui::PushItemFlag(ImGuiItemFlags_NoWindowHoverableCheck, true);

    // We've been using the equivalent of ImGuiSelectableFlags_SetNavIdOnHover on all Selectable()
    // since early Nav system days (commit 43ee5d73), but I am unsure whether this should be kept at
    // all. For now moved it to be an opt-in feature used by menus only.
    bool pressed;
    ImGui::PushID(id);
    if (!enabled)
        ImGui::BeginDisabled();

    // We use ImGuiSelectableFlags_NoSetKeyOwner to allow down on one menu item, move, up on
    // another.
    const ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SelectOnRelease |
                                                  ImGuiSelectableFlags_NoSetKeyOwner |
                                                  ImGuiSelectableFlags_SetNavIdOnHover;
    const ImGuiMenuColumns* offsets = &window->DC.MenuColumns;
    if (window->DC.LayoutType == ImGuiLayoutType_Horizontal) {
        // Inside a horizontal menu
        auto caption_blk = ImPlus::TextBlock(caption, {0, 0.5f});
        float w = caption_blk.Size.x;
        window->DC.CursorPos.x += IM_TRUNC(style.ItemSpacing.x * 0.5f);
        ImVec2 text_pos(window->DC.CursorPos.x + offsets->OffsetLabel,
            window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
        ImGui::PushStyleVar(
            ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x * 2.0f, style.ItemSpacing.y));
        auto ir = ImPlus::SelectableBox("", "", selected, selectable_flags, {w, 0}, {},
            [&](ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max, ColorSet const& cs) {
                caption_blk.Render(dl, bb_min, bb_max, cs.Content);
            });
        pressed = ir.Pressed;

        ImGui::PopStyleVar();
        window->DC.CursorPos.x +=
            IM_TRUNC(style.ItemSpacing.x *
                     (-1.0f + 0.5f)); // -1 spacing to compensate the spacing added when
                                      // Selectable() did a SameLine(). It would also work to call
                                      // SameLine() ourselves after the PopStyleVar().
    }
    else {
        // Menu item inside a vertical menu
        auto caption_blk = ImPlus::TextBlock{caption, {0, 0.5f}};
        auto icon_blk = ImPlus::IconBlock{icon};
        auto shortcut_blk = ImPlus::TextBlock{shortcut, {0, 0.5f}};
        auto checkmark_w = ImPlus::to_pt<ImPlus::rounded>(1.2_em);

        float min_w = window->DC.MenuColumns.DeclColumns(icon_blk.Size.x, caption_blk.Size.x,
            shortcut_blk.Size.x, checkmark_w); // Feedback for next frame
        auto h = std::max({caption_blk.Size.y, icon_blk.Size.y, shortcut_blk.Size.y});
        h = std::max(h, ImPlus::to_pt<ImPlus::rounded>(Style::Menu::Vertical::MinItemHeight()));

        float stretch_w = std::max(0.0f, ImGui::GetContentRegionAvail().x - min_w);

        auto ir = ImPlus::SelectableBox("", "", selected,
            selectable_flags | ImGuiSelectableFlags_SpanAvailWidth, {min_w, h}, ColorSets_MenuItem,
            [&](ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max, ColorSet const& cs) {
                if (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible) {
                    auto x = bb_min.x + offsets->OffsetLabel;
                    caption_blk.Render(
                        dl, {x, bb_min.y}, {x + caption_blk.Size.x, bb_max.y}, cs.Content);
                    if (!icon_blk.Empty()) {
                        auto x = bb_min.x + offsets->OffsetIcon;
                        auto y = (bb_min.y + bb_max.y - icon_blk.Size.y) * 0.5f;
                        icon_blk.RenderXY(dl, {x, y}, cs.Content);
                    }

                    if (!shortcut_blk.Empty()) {
                        auto x = bb_min.x + offsets->OffsetShortcut;
                        shortcut_blk.Render(dl, {x, bb_min.y}, {x + shortcut_blk.Size.x, bb_max.y},
                            style.Colors[ImGuiCol_TextDisabled]);
                    }

                    if (selected) {
                        auto x = bb_min.x + offsets->OffsetMark + stretch_w + g.FontSize * 0.40f;
                        auto y = bb_min.y + g.FontSize * 0.134f * 0.5f;
                        ImGui::RenderCheckMark(
                            dl, {x, y}, ImGui::GetColorU32(cs.Content), g.FontSize * 0.866f);
                    }
                }
            });
        pressed = ir.Pressed;
    }
    IMGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label,
        g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable |
            (selected ? ImGuiItemStatusFlags_Checked : 0));
    if (!enabled)
        ImGui::EndDisabled();
    ImGui::PopID();
    if (menuset_is_open)
        ImGui::PopItemFlag();

    return pressed;
}

} // namespace ImPlus