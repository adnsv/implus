#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <optional>

#include <implus/blocks.hpp>
#include <implus/button.hpp>
#include <implus/color.hpp>
#include <implus/dropdown.hpp>

namespace ImPlus {

// note: name is used only for test_engine
auto CustomButton(ImID id, char const* name, ImVec2 const& size, float baseline_offset,
    ImGuiButtonFlags flags, ButtonDrawCallback const& draw_callback) -> InteractState
{
    auto state = InteractState{};

    auto const& g = *GImGui;
    auto const* window = g.CurrentWindow;
    if (window->SkipItems)
        return state;

    auto pos = window->DC.CursorPos;
    if ((flags & ImGuiButtonFlags_AlignTextBaseLine) &&
        baseline_offset < window->DC.CurrLineTextBaseOffset)
        pos.y += window->DC.CurrLineTextBaseOffset - baseline_offset;

    ImGui::ItemSize(size, baseline_offset);
    auto const bb = ImRect{pos, pos + size};
    if (!ImGui::ItemAdd(bb, id))
        return state;

    state.Pressed = ImGui::ButtonBehavior(bb, id, &state.Hovered, &state.Held, flags);
    ImGui::RenderNavHighlight(bb, id);

    if (!(state.Pressed || state.Held) && g.NavId == id && g.NavCursorVisible) {
        if (ImGui::Shortcut(ImGuiKey_Enter, ImGuiInputFlags_None, id) ||
            ImGui::Shortcut(ImGuiKey_KeypadEnter, ImGuiInputFlags_None, id)) {
            state.Pressed = true;
        }
    }

    if (draw_callback)
        draw_callback(id, window->DrawList, bb.Min, bb.Max, state);

#ifdef IMGUI_ENABLE_TEST_ENGINE
    IMGUI_TEST_ENGINE_ITEM_INFO(id, name, window->DC.LastItemStatusFlags);
#endif

    return state;
}

static auto regular_button_regular_colors(InteractState const& st) -> ColorSet
{
    return ColorSet{
        .Content = Color::FromStyle(ImGuiCol_Text),
        .Background = st.Held || st.Hovered ? Color::FromStyle(ImGuiCol_ButtonActive)
                      : st.Hovered          ? Color::FromStyle(ImGuiCol_ButtonHovered)
                                            : Color::FromStyle(ImGuiCol_Button),
    };
}

static auto regular_button_selected_colors(InteractState const& st) -> ColorSet
{
    return ColorSet{
        .Content = Color::FromStyle(ImGuiCol_Text),
        .Background = Color::FromStyle(ImGuiCol_ButtonActive),
    };
}

static auto flat_button_regular_colors(InteractState const& st) -> ColorSet
{
    return ColorSet{
        .Content = Color::FromStyle(ImGuiCol_Text),
        .Background = st.Held && st.Hovered ? Color::FromStyle(ImGuiCol_ButtonActive)
                      : st.Hovered
                          ? Color::ModulateAlpha(Color::FromStyle(ImGuiCol_ButtonHovered), 0.5f)
                          : ImVec4{0, 0, 0, 0},
    };
}

static auto flat_button_selected_colors(InteractState const& st) -> ColorSet
{
    return ColorSet{
        .Content = Color::FromStyle(ImGuiCol_Text),
        .Background = Color::FromStyle(ImGuiCol_ButtonActive),
    };
}

static auto link_button_regular_colors(InteractState const& st) -> ColorSet
{
    return ColorSet{
        .Content = Style::LinkButton::Color::Content(),
        .Background = st.Held && st.Hovered ? Style::LinkButton::Color::Background::Active()
                      : st.Hovered          ? Style::LinkButton::Color::Background::Hovered()
                                            : Style::LinkButton::Color::Background::Regular(),
    };
}

static auto link_button_selected_colors(InteractState const& st) -> ColorSet
{
    return ColorSet{
        .Content = Style::LinkButton::Color::Content(),
        .Background = Style::LinkButton::Color::Background::Active(),
    };
}

static auto transparent_button_regular_colors(InteractState const& st) -> ColorSet
{
    return ColorSet{
        .Content = st.Held || st.Hovered
                       ? Color::FromStyle(ImGuiCol_Text)
                       : Color::ModulateAlpha(Color::FromStyle(ImGuiCol_Text), 0.7f),
        .Background = ImVec4{0, 0, 0, 0},
    };
}

static auto transparent_button_selected_colors(InteractState const& st) -> ColorSet
{
    return ColorSet{
        .Content = Color::FromStyle(ImGuiCol_Text),
        .Background = {0, 0, 0, 0},
    };
}

auto GetButtonColors(ButtonAppearance appearance, bool selected) -> InteractColorSetCallback
{
    switch (appearance) {
    case ButtonAppearance::Flat: {
        return selected ? flat_button_selected_colors : flat_button_regular_colors;
    } break;
    case ButtonAppearance::Link: {
        return selected ? link_button_selected_colors : link_button_regular_colors;
    } break;
    case ButtonAppearance::Transparent: {
        return selected ? transparent_button_selected_colors : transparent_button_regular_colors;
    } break;
    default: // ButtonAppearance::Regular
        return selected ? regular_button_selected_colors : regular_button_regular_colors;
    }
}

inline auto make_pointy_shaped_path(
    ImDrawList* dl, float l, float t, float r, float b, float rounding, ImGuiDir dir)
{
    auto w = std::max(0.0f, r - l);
    auto h = std::max(0.0f, b - t);
    rounding = std::min({rounding, w * 0.5f, h * 0.5f});

    auto const slope_adj = std::min(h * 0.5f * 0.577f, w * 0.33f);

    auto l_adj = (dir == ImGuiDir_Left) ? slope_adj * 0.5f + rounding * 0.577f : rounding;
    auto r_adj = (dir == ImGuiDir_Right) ? slope_adj * 0.5f + rounding * 0.577f : rounding;

    if (rounding < 0.5f) {
        dl->PathLineTo({l + l_adj, t});
        dl->PathLineTo({r - r_adj, t});
        if (dir == ImGuiDir_Right)
            dl->PathLineTo({r, (t + b) * 0.5f});
        dl->PathLineTo({r - r_adj, b});
        dl->PathLineTo({l + l_adj, b});
        if (dir == ImGuiDir_Left)
            dl->PathLineTo({l, (t + b) * 0.5f});
    }
    else {
        dl->PathArcToFast({l + l_adj, t + rounding}, rounding, (dir == ImGuiDir_Left) ? 7 : 6, 9);
        dl->PathArcToFast(
            {r - r_adj, t + rounding}, rounding, 9, (dir == ImGuiDir_Right) ? 11 : 12);
        if (dir == ImGuiDir_Right)
            dl->PathLineTo({r, (t + b) * 0.5f});
        dl->PathArcToFast({r - r_adj, b - rounding}, rounding, (dir == ImGuiDir_Right) ? 1 : 0, 3);
        dl->PathArcToFast({l + l_adj, b - rounding}, rounding, 3, (dir == ImGuiDir_Left) ? 5 : 6);
        if (dir == ImGuiDir_Left)
            dl->PathLineTo({l, (t + b) * 0.5f});
    }
}

inline auto make_shaped_path(
    ImDrawList* dl, ImVec2 bb_min, ImVec2 bb_max, ButtonShape shape, float rounding)
{
    switch (shape) {
    case ButtonShape::PointyLeft:
        make_pointy_shaped_path(
            dl, bb_min.x, bb_min.y, bb_max.x, bb_max.y, rounding, ImGuiDir_Left);
        break;
    case ButtonShape::PointyRight:
        make_pointy_shaped_path(
            dl, bb_min.x, bb_min.y, bb_max.x, bb_max.y, rounding, ImGuiDir_Right);
        break;
    default: dl->PathRect(bb_min, bb_max, rounding, 0);
    }
}

inline auto calc_sidebar_rect(ImRect bb, Side side) -> std::optional<ImRect>
{
    switch (side) {
    case Side::West: {
        auto const w = std::round(std::min((bb.Max.x - bb.Min.x) * 0.25f, to_pt(0.2_em)));
        bb.Max.x = bb.Min.x + w;
        return bb;
    } break;
    case Side::East: {
        auto const w = std::round(std::min((bb.Max.x - bb.Min.x) * 0.25f, to_pt(0.2_em)));
        bb.Min.x = bb.Max.x - w;
        return bb;
    } break;
    case Side::North: {
        auto const h = std::round(std::max((bb.Max.y - bb.Min.y) * 0.25f, to_pt(0.2_em)));
        bb.Max.y = bb.Min.y + h;
        return bb;
    } break;
    case Side::South: {
        auto const h = std::round(std::min((bb.Max.y - bb.Min.y) * 0.25f, to_pt(0.2_em)));
        bb.Min.y = bb.Max.y - h;
        return bb;
    } break;
    }
    return {};
}

inline auto add_shaped_outline(ImDrawList* dl, ImVec2 bb_min, ImVec2 bb_max, ButtonShape shape,
    float rounding, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    bb_min += ImVec2{0.5f, 0.5f};
    bb_max -= ImVec2{0.49f, 0.49f};

    make_shaped_path(dl, bb_min, bb_max, shape, rounding);
    dl->PathStroke(col, ImDrawFlags_Closed, thickness);
}

inline auto add_shaped_filled(
    ImDrawList* dl, ImVec2 bb_min, ImVec2 bb_max, ButtonShape shape, float rounding, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    bb_min += ImVec2{0.5f, 0.5f};
    bb_max -= ImVec2{0.49f, 0.49f};

    make_shaped_path(dl, bb_min, bb_max, shape, rounding);
    dl->PathFillConvex(col);
}

inline auto render_shaped_nav_highlight(
    ImRect const& bb, ButtonShape shape, float rounding, ImGuiID id, bool compact)
{
    auto& g = *GImGui;
    if (id != g.NavId)
        return;
    if (!g.NavCursorVisible)
        return;
    auto* window = g.CurrentWindow;
    if (window->DC.NavHideHighlightOneFrame)
        return;

    auto display_rect = bb;
    display_rect.ClipWith(window->ClipRect);
    const float thickness = 2.0f;

    auto const col = ImGui::GetColorU32(ImGuiCol_NavHighlight);
    auto* dl = window->DrawList;

    if (compact) {
        add_shaped_outline(dl, display_rect.Min, display_rect.Max, shape, rounding, col, thickness);
    }
    else {
        auto const distance = 3.0f + thickness * 0.5f;
        display_rect.Expand(ImVec2(distance, distance));
        auto const fully_visible = window->ClipRect.Contains(display_rect);
        if (!fully_visible)
            dl->PushClipRect(display_rect.Min, display_rect.Max);
        add_shaped_outline(dl, display_rect.Min, display_rect.Max, shape,
            rounding ? rounding + distance : 0.0f, col, thickness);
        if (!fully_visible)
            dl->PopClipRect();
    }
}

inline auto render_shaped_frame(
    ImDrawList* dl, ImVec2 bb_min, ImVec2 bb_max, ButtonShape shape, ImU32 col, float rounding)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    add_shaped_filled(dl, bb_min, bb_max, shape, rounding, col);
    const float border_size = g.Style.FrameBorderSize;
    if (border_size > 0.0f) {
        add_shaped_outline(dl, bb_min + ImVec2(1, 1), bb_max + ImVec2(1, 1), shape,
            ImGui::GetColorU32(ImGuiCol_BorderShadow), rounding, border_size);
        add_shaped_outline(
            dl, bb_min, bb_max, shape, ImGui::GetColorU32(ImGuiCol_Border), rounding, border_size);
    }
}

auto MakeButtonDrawCallback(ButtonOptions const& opts, InteractColorSetCallback color_set,
    Content::DrawCallback&& on_content) -> ButtonDrawCallback
{
    return [opts, color_set, on_content = std::move(on_content)](ImGuiID id, ImDrawList* dl,
               ImVec2 const& bb_min, ImVec2 const& bb_max, InteractState const& state) {
        auto const bb = ImRect{bb_min, bb_max};

        auto disp_state = state;

        if (!NeedsHoverHighlight() && !disp_state.Held)
            disp_state.Hovered = false;

        auto const cs = color_set ? color_set(disp_state) : ColorSets_RegularButton(disp_state);
        auto const bg_clr = ImGui::GetColorU32(cs.Background);

        auto const rounding = CalcFrameRounding(opts.Rounded);
        auto const padding = CalcFramePadding(opts.Padding);
        auto const shape = opts.Shape.value_or(ButtonShape::Regular);

        render_shaped_nav_highlight(bb, shape, rounding, id, false);

        if (opts.DefaultAction) {
            auto const d = ImVec2{3.0f, 3.0f};
            add_shaped_outline(dl, bb_min - d, bb_max + d, shape, rounding + d.y, bg_clr, 1.5f);
        }

        render_shaped_frame(dl, bb_min, bb_max, shape, bg_clr, rounding);

        if (opts.Sidebar) {
            if (auto r = calc_sidebar_rect({bb_min, bb_max}, opts.Sidebar->Side)) {
                auto const clr = opts.Sidebar->Color.value_or(cs.Content);
                dl->AddRectFilled(r->Min, r->Max, ImGui::GetColorU32(clr));
            }
        }

        if (on_content) {
            auto const tl = bb.Min + padding;
            auto const br = bb.Max - padding;
            ImGui::PushClipRect(tl, br, true);
            on_content(dl, tl, br, cs);
            ImGui::PopClipRect();
        }
    };
}

static auto name_for_test_engine(ICD_view const& content)
{
#ifdef IMGUI_ENABLE_TEST_ENGINE
    return std::string{content.Caption};
#else
    return std::string{};
#endif
}

auto CalcPaddedSize(ImVec2 const& inner, ImVec2 const& padding) -> ImVec2
{
    return {std::round(inner.x + padding.x * 2.0f), std::round(inner.y + padding.y * 2.0f)};
}

auto ICDCustomButton(ImID id, ICD_view const& content, Content::Layout layout,
    ICDOptions const& ic_opts, Sizing::XYArg const& sizing,
    std::optional<length> const& default_overflow_width,
    Text::CDOverflowPolicy const& overflow_policy, ImGuiButtonFlags flags,
    ButtonOptions const& btn_opts, InteractColorSetCallback color_set) -> InteractState
{
    auto& g = *GImGui;
    auto window = g.CurrentWindow;
    if (window->SkipItems)
        return {};
    auto& style = g.Style;

    auto const padding = CalcFramePadding();
    auto const region_avail = ImGui::GetContentRegionAvail();
    auto ow = Sizing::CalcOverflowWidth(
        sizing.Horz, default_overflow_width, padding.x * 2.0f, region_avail.x);

    auto const block = ICDBlock{content, layout, ic_opts, overflow_policy, ow};
    auto measured_size = CalcPaddedSize(block.Size, padding);

    if (auto w = Sizing::GetDesired(sizing.Horz))
        if (*w > measured_size.x)
            measured_size.x = *w;
    if (auto h = Sizing::GetDesired(sizing.Vert))
        if (*h > measured_size.y)
            measured_size.y = *h;

    auto const actual_size = Sizing::CalcActual(sizing, measured_size, region_avail);

    auto draw_callback =
        MakeButtonDrawCallback(btn_opts, color_set, MakeContentDrawCallback(block));

    auto dr = CustomButton(
        id, name_for_test_engine(content).c_str(), actual_size, padding.y, flags, draw_callback);

    if (dr.Hovered)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    return dr;
}

auto Button(ImID id, ICD_view const& content, Sizing::XYArg const& sizing, ImGuiButtonFlags flags,
    ButtonOptions const& opts) -> bool
{
    auto lt = Style::Button::Layout();
    auto op = Style::Button::OverflowPolicy();
    auto ow = Style::Button::OverflowWidth();

    auto cs = GetButtonColors(Style::Button::Appearance(), false);
    auto dr = ICDCustomButton(id, content, lt, ICDOptions{}, sizing, ow, op, flags, opts, cs);
    return dr.Pressed;
}

auto BeginDropDownButton(ImID id, ICD_view const& content, Sizing::XYArg const& sizing,
    Placement::Options const& placement) -> bool
{
    auto& g = *GImGui;
    auto* window = ImGui::GetCurrentWindow();

    auto save_flags = g.NextWindowData.Flags;
    g.NextWindowData.ClearFlags();
    if (window->SkipItems)
        return false;

    ImGui::PushID(id);

    auto const popup_id = ImHashStr("##DropDownPopup", 0, id);
    auto popup_open = ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None);

    auto lt = Style::Button::Layout();
    auto op = Style::Button::OverflowPolicy();
    auto ow = Style::Button::OverflowWidth();
    auto cs = GetButtonColors(Style::Button::Appearance(), popup_open);

    auto dr = ICDCustomButton("##", content, lt, ICDOptions{.WithDropdownArrow = true}, sizing, ow,
        op, ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_NoSetKeyOwner, {}, cs);

    ImGui::PopID();

    if (dr.Pressed && !popup_open) {
        ImGui::OpenPopupEx(popup_id, ImGuiPopupFlags_None);
        popup_open = true;
    }

    if (!popup_open)
        return false;

    auto last_item_in_parent = g.LastItemData;
    g.NextWindowData.Flags = save_flags;
    auto menu_is_open = ImPlus::BeginDropDownPopup(
        popup_id, last_item_in_parent.Rect.Min, last_item_in_parent.Rect.Max, placement);
    if (menu_is_open) {
        g.LastItemData = last_item_in_parent;
        if (g.HoveredWindow == window)
            g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredWindow;
    }
    else {
        g.NextWindowData.ClearFlags();
    }
    return menu_is_open;
}

void EndDropDownButton() { ImGui::EndPopup(); }

auto LinkButton(
    ImID id, ICD_view const& content, Sizing::XYArg const& sizing, ImGuiButtonFlags flags) -> bool
{
    auto lt = Style::LinkButton::Layout();
    auto op = Style::LinkButton::OverflowPolicy();
    auto ow = Style::LinkButton::OverflowWidth();
    auto dr = ICDCustomButton(id, content, lt, ICDOptions{}, sizing, ow, op, flags, {},
        GetButtonColors(ButtonAppearance::Link, false));
    if (dr.Hovered)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    return dr.Pressed;
}

} // namespace ImPlus