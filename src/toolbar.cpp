#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/blocks.hpp"
#include "implus/button.hpp"
#include "implus/dropdown.hpp"
#include "implus/toolbar.hpp"

namespace ImPlus::Toolbar {

auto const overflow_content = ImPlus::ICD{ImPlus::Icon::Builtin::DotDotDot, ""};

enum placement_target {
    placement_not_in_toolbar, // not building toolbar yet
    placement_front,
    placement_trail,
    placement_overflow,         // overflow submenu is open
    placement_nowhere,          // overflow submenu is closed
    placement_pending_overflow, // the following items must go into the overflow
};

// toolbar_state is an internal state representation for a toolbar
//
// - all parameters are populated when BeginEx (Begin) is executed
//
// - changes to style after BeginEx will have no effect
//
// - in horizontal toolbars:
//   - item_size.x is the minimum button width
//   - item_size.y is the actual button height
//
// - in vertical toolbars:
//   - item_size.x is the actual button width
//   - item_size.y is the minimum button height
//
struct toolbar_state {
    placement_target placement = placement_not_in_toolbar;
    ImRect layout_box = {};
    Axis item_stacking = ImPlus::Axis::Horz;
    float item_spacing = 0.0f;
    ImVec2 item_padding = {};
    ImVec2 item_size = {};
    Content::Layout item_layout = Content::Layout::VertCenter;
    TextVisibility show_text = TextVisibility::Always;
    Text::CDOverflowPolicy item_overflow_policy;
    ImVec2 overflow_size = {};
    Placement::Options tooltip_placement;
    Placement::Options dropdown_placement;
    float line_after_thickness = 0.0f;
    ImU32 line_after_u32 = 0;
    bool remove_spacing_after = false;
    ImGuiWindow* window = nullptr;
    int next_button_index = 0; // used for ID generation
    // std::string popup_lbl;
    // ImGuiID overflow_popup_id;
};

// todo: make this an unordered map for different toolbar ids
static auto get_toolbar_state() -> toolbar_state&
{
    // ideally, this should stored within window context
    // for simplicity in current implementation, use global state
    static toolbar_state _ts;
    return _ts;
}

static void do_remove_spacing_after(ImPlus::Axis dir)
{
    if (dir == ImPlus::Axis::Horz)
        ImGui::GetCurrentWindow()->DC.CursorPos.y -= ImGui::GetStyle().ItemSpacing.y;
    else
        ImGui::GetCurrentWindow()->DC.CursorPos.x -= ImGui::GetStyle().ItemSpacing.x;
}

inline auto vround(float x, float y) -> ImVec2 { return {std::round(x), std::round(y)}; }
inline auto vround(ImVec2 const& v) -> ImVec2 { return vround(v.x, v.y); }

auto calc_effective_padding(Options const& opts) -> ImVec2
{
    if (auto b = std::get_if<bool>(&opts.Padding)) {
        if (*b)
            return GImGui->Style.WindowPadding;
    }
    else if (auto v = std::get_if<ImVec2>(&opts.Padding))
        return *v;

    return {0, 0};
}

auto BeginEx(ImID id, ImVec2 const& size, Axis item_stacking, Options const& opts) -> bool
{
    IM_ASSERT(id != 0);

    auto& ts = get_toolbar_state();
    IM_ASSERT_USER_ERROR(ts.placement == placement_not_in_toolbar,
        "Previous Toolbar::Begin() was not followed with "
        "Toolbar::End()");

    auto bg_color = opts.BackgroundColor.value_or(Style::Toolbar::BackgroundColor());
    auto const horz = item_stacking == Axis::Horz;
    auto along = [horz](auto&& sz) { return horz ? sz.x : sz.y; };
    auto across = [horz](auto&& sz) { return horz ? sz.y : sz.x; };

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_color);

    auto cflags = ImGuiChildFlags{ImGuiWindowFlags_None};
    auto wflags = ImGuiWindowFlags{ImGuiWindowFlags_NoScrollbar};
    auto const effective_padding = calc_effective_padding(opts);
    auto n_style_var = 0;

    if (effective_padding != ImVec2{0, 0}) {
        cflags |= ImGuiChildFlags_AlwaysUseWindowPadding;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, effective_padding);
        ++n_style_var;
    }

    auto ret = ImGui::BeginChildEx(nullptr, id, size, cflags, wflags);

    ImGui::PopStyleColor();
    if (n_style_var)
        ImGui::PopStyleVar(n_style_var);

    if (!ret) {
        ts.placement = placement_not_in_toolbar;
        ImGui::EndChild();
        if (opts.RemoveSpacingAfterToolbar)
            do_remove_spacing_after(item_stacking);
        return false;
    }

    auto pos = ImGui::GetCursorPos();

    ts.placement = placement_front;
    ts.layout_box.Min = ImGui::GetCursorPos();
    ts.layout_box.Max = ts.layout_box.Min + size - effective_padding * 2.0f;

    ts.item_stacking = horz ? Axis::Horz : Axis::Vert;
    ts.item_spacing = 0.0f;
    ts.item_padding = vround(GImGui->Style.FramePadding);
    ts.item_layout = opts.ItemLayout.value_or(
        horz ? Style::Toolbar::Horz::ItemLayout() : Style::Toolbar::Vert::ItemLayout());
    ts.show_text = opts.ShowText;
    ts.item_size.x =
        horz ? to_pt(Style::Toolbar::Horz::MinimumButtonWidth()) : ts.layout_box.GetWidth();
    ts.item_size.y =
        horz ? ts.layout_box.GetHeight() : to_pt(Style::Toolbar::Vert::MinimumButtonHeight());
    ts.tooltip_placement = horz
                               ? Placement::Options{.Direction = ImGuiDir_Down, .Alignment = 0.0f}
                               : Placement::Options{.Direction = ImGuiDir_Right, .Alignment = 0.5f};
    ts.dropdown_placement =
        horz ? Placement::Options{.Direction = ImGuiDir_Down, .Alignment = 1.0f}
             : Placement::Options{.Direction = ImGuiDir_Right, .Alignment = 0.0f};
    ts.remove_spacing_after = opts.RemoveSpacingAfterToolbar;
    ts.item_overflow_policy = horz ? Style::Toolbar::Horz::ItemOverflowPolicy()
                                   : Style::Toolbar::Vert::ItemOverflowPolicy();
    ts.overflow_size = ts.item_padding * 2.0f + ICDBlock{overflow_content, ts.item_layout}.Size;
    ts.window = ImGui::GetCurrentWindow();
    ts.next_button_index = 1;

    if (opts.WantLineAfter) {
        ts.line_after_u32 = ImGui::GetColorU32(Style::Toolbar::LineAfter::Color());
        ts.line_after_thickness = to_pt<rounded_up>(Style::Toolbar::LineAfter::Thickness());
    }
    else {
        ts.line_after_u32 = 0u;
        ts.line_after_thickness = 0.0f;
    }

    return ret;
}

auto Begin(ImID id, Axis item_stacking, Options const& opts) -> bool
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    auto const horz = item_stacking == Axis::Horz;
    auto sz = ImGui::GetContentRegionAvail();
    auto const effective_padding = calc_effective_padding(opts);
    auto const& button_padding = GImGui->Style.FramePadding;

    if (horz) {
        sz.y = to_pt<rounded_up>(Style::Toolbar::Horz::Height());
        if (sz.y == 0) {
            sz.y = button_padding.y * 2.0f + to_pt(em);
            if (opts.ShowText == TextVisibility::Always) {
                auto const item_layout =
                    opts.ItemLayout.value_or(Style::Toolbar::Horz::ItemLayout());
                if (item_layout == Content::Layout::VertNear ||
                    item_layout == Content::Layout::VertCenter) {
                    sz.y += to_pt<rounded>(em);
                    sz.y += to_pt<rounded>(Style::ICD::VertIconSpacing());
                }
            }
        }

        sz.y += effective_padding.y * 2.0f;
        sz.y = std::round(sz.y);
    }
    else {
        sz.x = to_pt<rounded_up>(Style::Toolbar::Vert::Width());
        if (sz.x == 0) {
            sz.x = button_padding.x * 2.0f + to_pt(em);
        }
        sz.x += effective_padding.x * 2.0f;
        sz.x = std::round(sz.x);
    }
    return BeginEx(id, sz, item_stacking, opts);
}

void End(bool remove_spacing)
{
    auto& ts = get_toolbar_state();
    IM_ASSERT_USER_ERROR(
        ts.placement != placement_not_in_toolbar, "Calling Toolbar::End() too many times");

    if (ts.placement == placement_overflow)
        ImGui::EndPopup();

    ts.placement = placement_not_in_toolbar;

    auto r = ImGui::GetCurrentWindowRead()->Rect();

    ImGui::EndChild();

    if (ts.item_stacking == ImPlus::Axis::Vert)
        ImGui::SameLine();

    if (ts.remove_spacing_after)
        do_remove_spacing_after(ts.item_stacking);

    auto const vert = ts.item_stacking == ImPlus::Axis::Vert;

    if (ts.line_after_thickness > 0) {
        auto wnd = ImGui::GetCurrentWindow();
        if (vert) {
            r.Min.x = r.Max.x;
            r.Max.x += ts.line_after_thickness;
            wnd->DC.CursorPos.x += ts.line_after_thickness;
        }
        else {
            r.Min.y = r.Max.y;
            r.Max.y += ts.line_after_thickness;
            wnd->DC.CursorPos.y += ts.line_after_thickness;
        }
        if (ts.line_after_u32)
            wnd->DrawList->AddRectFilled(r.Min, r.Max, ts.line_after_u32);
    }
}

void FrontPlacement()
{
    auto& ts = get_toolbar_state();
    IM_ASSERT_USER_ERROR(ts.placement != placement_not_in_toolbar,
        "Calling Toolbar::FrontPlacement() outside toolbar");
    if (ts.placement == placement_trail)
        ts.placement = placement_front;
}
void TrailPlacement()
{
    auto& ts = get_toolbar_state();
    IM_ASSERT_USER_ERROR(ts.placement != placement_not_in_toolbar,
        "Calling Toolbar::TrailPlacement() outside toolbar");
    if (ts.placement == placement_front)
        ts.placement = placement_trail;
}
void OverflowPlacement()
{
    auto& ts = get_toolbar_state();
    IM_ASSERT_USER_ERROR(ts.placement != placement_not_in_toolbar,
        "Calling Toolbar::OverflowPlacement() outside toolbar");
    if (ts.placement == placement_front || ts.placement == placement_trail)
        ts.placement = placement_pending_overflow;
}

auto same(ImVec4 const& a, ImVec4 const& b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

template <ButtonAppearance Appearance, bool Selected>
auto colorset_selector(InteractState const& st) -> ColorSet
{
    auto const active = st.Held && st.Hovered;
    auto const hovered = st.Hovered;

    auto cs = ColorSet{.Content = Color::FromStyle(ImGuiCol_Text), .Background = {0, 0, 0, 0}};

    if constexpr (Appearance == Regular) {
        cs.Content = Color::FromStyle(ImGuiCol_Text);
        if constexpr (Selected) {
            cs.Background = Color::FromStyle(ImGuiCol_ButtonActive);
        }
        else {

            cs.Background = active    ? Color::FromStyle(ImGuiCol_ButtonActive)
                            : hovered ? Color::FromStyle(ImGuiCol_ButtonHovered)
                                      : Color::FromStyle(ImGuiCol_Button);
        }
    }
    else if constexpr (Appearance == Flat) {
        if constexpr (Selected) {
            cs.Background = Color::FromStyle(ImGuiCol_ButtonActive);
        }
        else {
            cs.Background =
                active    ? Color::FromStyle(ImGuiCol_ButtonActive)
                : hovered ? Color::ModulateAlpha(Color::FromStyle(ImGuiCol_ButtonHovered), 0.5f)
                          : ImVec4{0, 0, 0, 0};
        }
    }
    else if constexpr (Appearance == Transparent) {
        cs.Content = Color::FromStyle(ImGuiCol_Text);
        auto bright = Selected || active || hovered;
        if (!bright)
            cs.Content.w *= 0.7f;
    }
    return cs;
}

auto choose_colorset_selector(ButtonAppearance appearance, bool selected)
{
    switch (appearance) {
    case Transparent:
        return selected ? colorset_selector<Transparent, true>
                        : colorset_selector<Transparent, false>;
    case Flat: return selected ? colorset_selector<Flat, true> : colorset_selector<Flat, false>;
    default: return selected ? colorset_selector<Regular, true> : colorset_selector<Regular, false>;
    }
}

auto display_btn(toolbar_state& ts, ImID id, ICDBlock const& c, ImVec2 const& sz, bool selected,
    ImGuiButtonFlags flags, ButtonOptions const& opts) -> bool
{
    auto* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return {};

    auto& g = *GImGui;
    auto& style = g.Style;
    auto const stacking = ts.item_layout;
    auto const op = ts.item_overflow_policy;

    auto const appearance = Style::Toolbar::Button::Appearance();

    auto effective_opts = opts;
    if (!effective_opts.ColorSet)
        effective_opts.ColorSet = choose_colorset_selector(appearance, selected);
    if (!effective_opts.Padding)
        effective_opts.Padding = ts.item_padding;

    auto const baseline_offset = to_pt<rounded>(effective_opts.Padding->y);

    auto on_content = [&](ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max,
                          ColorSet const& clr) { c.Render(dl, bb_min, bb_max, clr.Content); };

    auto cb = MakeButtonDrawCallback(effective_opts, on_content);

    auto dr = CustomButton(
        id, c.NameForTestEngine().c_str(), sz, baseline_offset, sz, flags, std::move(cb));

    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    return dr.Pressed;
};

auto do_begin_overflow(toolbar_state& ts) -> bool
{
    auto cp = ts.layout_box.Min;
    if (ts.item_stacking == Axis::Horz)
        cp.x = ts.layout_box.Max.x - ts.overflow_size.x;
    else
        cp.y = ts.layout_box.Max.y - ts.overflow_size.y;
    ImGui::SetCursorPos(cp);

    auto& g = *GImGui;
    auto save_flags = g.NextWindowData.Flags;
    g.NextWindowData.ClearFlags();
    if (ts.window->SkipItems)
        return false;

    auto popup_id = ts.window->GetID("##.TOOLBAR.OVERFLOW.POPUP.");
    auto button_id = ts.window->GetID("##.TOOLBAR.OVERFLOW.BTN.");

    auto overflow_expanded = ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None);

    auto btn_size = ts.item_stacking == Axis::Horz
                        ? ImVec2{ts.overflow_size.x, ts.layout_box.GetHeight()}
                        : ImVec2{ts.layout_box.GetWidth(), ts.overflow_size.y};
    auto btn_block = ICDBlock{overflow_content, ts.item_layout};

    auto pressed = display_btn(ts, button_id, btn_block, btn_size, overflow_expanded,
        ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_NoSetKeyOwner, {});

    if (pressed && !overflow_expanded) {
        ImGui::OpenPopup(popup_id, ImGuiPopupFlags_None);
        overflow_expanded = true;
    }

    if (!overflow_expanded) {
        ts.placement = placement_nowhere;
        return false;
    }

    auto const& r = g.LastItemData.Rect;
    g.NextWindowData.Flags = save_flags;
    overflow_expanded = ImPlus::BeginDropDownPopup(popup_id, r.Min, r.Max, ts.dropdown_placement);
    ts.placement = overflow_expanded ? placement_overflow : placement_nowhere;
    return overflow_expanded;
}

void Label(ImPlus::ICD_view const& content, float width, bool enabled)
{
    auto& ts = get_toolbar_state();
    IM_ASSERT_USER_ERROR(
        ts.placement != placement_not_in_toolbar, "Calling Toolbar::Label() outside toolbar");
    if (ts.placement == placement_nowhere)
        return;

    auto as_menu = [&] {
        ImGui::BeginDisabled(!enabled);
        ImGui::TextUnformatted(
            content.Caption.data(), content.Caption.data() + content.Caption.size());
        ImGui::EndDisabled();
    };

    if (ts.placement == placement_pending_overflow) {
        if (do_begin_overflow(ts))
            as_menu();
        return;
    }

    if (ts.placement == placement_overflow) {
        // showing overflow submenu
        as_menu();
        return;
    }

    auto const horz = ts.item_stacking == Axis::Horz;

    auto desired_ow = std::optional<float>{};
    if (!horz)
        desired_ow = ts.item_size.x - ts.item_padding.x * 2;

    auto block = ICDBlock{
        content,
        ts.item_layout,
        {},
        ts.item_overflow_policy,
        desired_ow,
    };

    auto sz = block.Size + ts.item_padding * 2.0f;
    if (horz)
        sz.y = ts.item_size.y;
    else
        sz.x = ts.item_size.x;

    auto overflows = horz ? sz.x + ts.overflow_size.x + ts.item_spacing > ts.layout_box.GetWidth()
                          : sz.y + ts.overflow_size.y + ts.item_spacing > ts.layout_box.GetHeight();

    if (overflows) {
        if (do_begin_overflow(ts))
            as_menu();
        return;
    }

    auto cp = ts.layout_box.Min;
    if (ts.placement == placement_trail) {
        if (horz)
            cp.x = ts.layout_box.Max.x - sz.x;
        else
            cp.y = ts.layout_box.Max.y - sz.y;
    }

    ImGui::SetCursorPos(cp);

    if (ts.placement == placement_trail) {
        if (horz)
            ts.layout_box.Max.x -= sz.x + ts.item_spacing;
        else
            ts.layout_box.Max.y -= sz.y + ts.item_spacing;
    }
    else {
        if (horz)
            ts.layout_box.Min.x += sz.x + ts.item_spacing;
        else
            ts.layout_box.Min.y += sz.y + ts.item_spacing;
    }

    ImGui::BeginDisabled(!enabled);
    {
        auto pos = ImGui::GetCursorScreenPos();
        auto bb = ImRect{pos, pos + sz};
        ImGui::ItemSize(sz, 0.0f);
        auto id = ts.window->GetID(ts.next_button_index++);
        if (ImGui::ItemAdd(bb, id)) {
            bb.Min += ts.item_padding;
            bb.Max -= ts.item_padding;
            auto clr = ImGui::GetStyleColorVec4(ImGuiCol_Text);
            block.Render(ImGui::GetWindowDrawList(), bb.Min, bb.Max, clr);
        }
    }
    ImGui::EndDisabled();
}

auto make_button(ImPlus::ICD_view const& content, bool selected, bool enabled,
    ImGuiButtonFlags flags, ButtonOptions const& opts, bool as_dropdown,
    bool with_dropdown_arrow = true) -> bool
{
    // normally this function returns true if the button was pressed.
    //
    // however, if the button is placed into an overflow menu, and as_dropdown =
    // true, then it returns true if its submenu is popped

    auto& ts = get_toolbar_state();
    IM_ASSERT_USER_ERROR(
        ts.placement != placement_not_in_toolbar, "Calling Toolbar::Button() outside toolbar");
    if (ts.placement == placement_nowhere)
        return false;

    auto id = ts.window->GetID(ts.next_button_index++);

    auto as_menu = [&] {
        // todo: more logic when label is invisible
        auto s = std::string{content.Caption};
        s += "##.TB.";
        s += std::to_string(id);
        if (!as_dropdown)
            return ImGui::MenuItem(s.c_str(), nullptr, selected, enabled);
        else
            return ImGui::BeginMenu(s.c_str(), enabled);
    };

    if (ts.placement == placement_pending_overflow) {
        if (do_begin_overflow(ts))
            return as_menu();
        else
            return false;
    }

    if (ts.placement == placement_overflow) {
        // showing overflow submenu
        return as_menu();
    }

    auto const horz = ts.item_stacking == Axis::Horz;
    auto along = [horz](auto&& sz) { return horz ? sz.x : sz.y; };
    auto across = [horz](auto&& sz) { return horz ? sz.y : sz.x; };

    auto effective_content = content;
    auto effective_options = ICDOptions{.WithDropdownArrow = as_dropdown && with_dropdown_arrow};
    auto effective_tooltip = ICD_view{};

    if (!effective_content.Icon.Empty()) {
        if (ts.show_text == TextVisibility::Auto) {
            // move caption into tooltip
            std::swap(effective_tooltip.Caption, effective_content.Caption);
        }
    }

    if (ts.show_text == TextVisibility::Auto || ts.show_text == TextVisibility::CaptionOnly) {
        // move description into tooltip
        std::swap(effective_tooltip.Descr, effective_content.Descr);
    }

    auto desired_ow = std::optional<float>{};
    if (!horz)
        desired_ow = ts.item_size.x - ts.item_padding.x * 2;

    auto block = ICDBlock{
        effective_content,
        ts.item_layout,
        effective_options,
        ts.item_overflow_policy,
        desired_ow,
    };

    auto sz = block.Size + ts.item_padding * 2.0f;
    sz.x = horz ? std::max(sz.x, ts.item_size.x) : ts.item_size.x;
    sz.y = horz ? ts.item_size.y : std::max(sz.y, ts.item_size.y);

    auto overflows = horz ? sz.x + ts.overflow_size.x + ts.item_spacing > ts.layout_box.GetWidth()
                          : sz.y + ts.overflow_size.y + ts.item_spacing > ts.layout_box.GetHeight();

    if (overflows) {
        if (do_begin_overflow(ts))
            return as_menu();
        else
            return false;
    }

    auto cp = ts.layout_box.Min;
    if (ts.placement == placement_trail) {
        if (horz)
            cp.x = ts.layout_box.Max.x - sz.x;
        else
            cp.y = ts.layout_box.Max.y - sz.y;
    }

    ImGui::SetCursorPos(cp);

    if (ts.placement == placement_trail) {
        if (horz)
            ts.layout_box.Max.x -= sz.x + ts.item_spacing;
        else
            ts.layout_box.Max.y -= sz.y + ts.item_spacing;
    }
    else {
        if (horz)
            ts.layout_box.Min.x += sz.x + ts.item_spacing;
        else
            ts.layout_box.Min.y += sz.y + ts.item_spacing;
    }

    ImGui::BeginDisabled(!enabled);
    auto clicked = display_btn(ts, id, block, sz, selected, flags, opts);
    ImGui::EndDisabled();

    if (!ImPlus::MouseSourceIsTouchScreen() &&
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_AllowWhenDisabled) &&
        (!effective_tooltip.Caption.empty() || !effective_tooltip.Descr.empty())) {
        auto bb = ImPlus::Rect{ImGui::GetItemRectMin(), ImGui::GetItemRectMax()};
        auto d = std::round(ImPlus::GetFontHeight() * 0.2f);
        bb.Expand(d, d);

        ImPlus::Tooltip(bb, ts.tooltip_placement, effective_tooltip);
    }

    return clicked;
}

auto Button(ImPlus::ICD_view const& content, bool selected, bool enabled, ImGuiButtonFlags flags,
    ButtonOptions const& opts) -> bool
{
    return make_button(content, selected, enabled, flags, opts, false);
}

void Spacing(ImPlus::length sz)
{
    auto& ts = get_toolbar_state();
    IM_ASSERT_USER_ERROR(
        ts.placement != placement_not_in_toolbar, "Calling Toolbar::Separator() outside toolbar");
    if (ts.placement == placement_nowhere)
        return;

    if (ts.placement == placement_overflow) {
        // ignore
        return;
    }
    else if (ts.placement == placement_pending_overflow) {
        // avoid separators at the top of overflow submenu
        return;
    }

    auto pt_sz = ImPlus::to_pt(sz);
    auto const horz = ts.item_stacking == Axis::Horz;

    if (horz) {
        if (pt_sz + ts.overflow_size.x + ts.item_spacing > ts.layout_box.GetWidth()) {
            // trigger overflow, but do not yet insert anything into it to avoid
            // having a submenu that starts with a separator
            ts.placement = placement_pending_overflow;
            return;
        }
    }
    else {
        if (pt_sz + ts.overflow_size.y + ts.item_spacing > ts.layout_box.GetHeight()) {
            // trigger overflow, but do not yet insert anything into it to avoid
            // having a submenu that starts with a separator
            ts.placement = placement_pending_overflow;
            return;
        }
    }

    if (ts.placement == placement_trail) {
        if (horz)
            ts.layout_box.Max.x -= pt_sz + ts.item_spacing;
        else
            ts.layout_box.Max.y -= pt_sz + ts.item_spacing;
    }
    else {
        if (horz)
            ts.layout_box.Min.x += pt_sz + ts.item_spacing;
        else
            ts.layout_box.Min.y += pt_sz + ts.item_spacing;
    }
}

void Separator()
{
    auto& ts = get_toolbar_state();
    IM_ASSERT_USER_ERROR(
        ts.placement != placement_not_in_toolbar, "Calling Toolbar::Separator() outside toolbar");
    if (ts.placement == placement_nowhere)
        return;

    if (ts.placement == placement_overflow) {
        // showing overflow submenu
        ImGui::Separator();
        return;
    }
    else if (ts.placement == placement_pending_overflow) {
        // avoid separators at the top of overflow submenu
        return;
    }

    auto sz = std::round(ImGui::GetFontSize() * 0.5f);
    auto const horz = ts.item_stacking == Axis::Horz;

    if (horz) {
        if (sz + ts.overflow_size.x + ts.item_spacing > ts.layout_box.GetWidth()) {
            // trigger overflow, but do not yet insert anything into it to avoid
            // having a submenu that starts with a separator
            ts.placement = placement_pending_overflow;
            return;
        }
    }
    else {
        if (sz + ts.overflow_size.y + ts.item_spacing > ts.layout_box.GetHeight()) {
            // trigger overflow, but do not yet insert anything into it to avoid
            // having a submenu that starts with a separator
            ts.placement = placement_pending_overflow;
            return;
        }
    }

    if (ts.placement == placement_trail) {
        if (horz)
            ts.layout_box.Max.x -= sz + ts.item_spacing;
        else
            ts.layout_box.Max.y -= sz + ts.item_spacing;
    }
    else {
        if (horz)
            ts.layout_box.Min.x += sz + ts.item_spacing;
        else
            ts.layout_box.Min.y += sz + ts.item_spacing;
    }
}

auto BeginDropDown(ImPlus::ICD_view const& content, bool enabled, bool with_dropdown_arrow) -> bool
{
    auto& ts = get_toolbar_state();
    auto& g = *GImGui;
    auto* window = ImGui::GetCurrentWindow();

    auto save_flags = g.NextWindowData.Flags;
    g.NextWindowData.ClearFlags();
    if (window->SkipItems)
        return false;

    auto id = ts.window->GetID(ts.next_button_index++);

    auto const popup_id = ImHashStr("##DropDownPopup", 0, id);
    auto popup_open = ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None);

    auto pressed = make_button(content, popup_open, enabled,
        ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_NoSetKeyOwner, {}, true,
        with_dropdown_arrow);

    if (ts.placement == placement_overflow)
        return pressed;
    if (ts.placement == placement_nowhere)
        return false;

    if (pressed && !popup_open) {
        ImGui::OpenPopupEx(popup_id, ImGuiPopupFlags_None);
        popup_open = true;
    }

    if (!popup_open)
        return false;

    auto const& r = g.LastItemData.Rect;
    g.NextWindowData.Flags = save_flags;
    return ImPlus::BeginDropDownPopup(popup_id, r.Min, r.Max, ts.dropdown_placement);
}

} // namespace ImPlus::Toolbar