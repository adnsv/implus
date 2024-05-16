#include "implus/menu.hpp"

#include <imgui.h>
#include <imgui_internal.h>
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
    ms->overflow_width =
        ImGui::CalcTextSize(OverflowLabel, nullptr, true).x + ms->item_spacing;

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

    auto const w =
        ImGui::CalcTextSize(label, nullptr, true).x + ms->item_spacing;

    auto available_size = ms->trail_pos - ms->front_pos;
    if (w + ms->overflow_width > available_size) {
        // trigger overflow
        auto overflow_expanded = ImGui::BeginMenu(OverflowLabel);
        ms->placement =
            overflow_expanded ? placement_overflow : placement_nowhere;
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
        window->DC.CursorMaxPos.x =
            ImMax(window->DC.CursorMaxPos.x, window->DC.CursorPos.x);
        auto expanded = ImGui::BeginMenu(label, enabled);
        window->DC.CursorPos.x = ms->front_pos;
        if (expanded)
            ++ms->level;
        return expanded;
    }

    // front placement
    window->DC.CursorPos.x =
        ms->front_pos; // just in case, it should be there already
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

} // namespace ImPlus