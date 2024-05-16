#pragma once

#include <functional>
#include <implus/color.hpp>
#include <implus/content.hpp>
#include <implus/dropdown.hpp>
#include <implus/geometry.hpp>
#include <implus/icd.hpp>
#include <implus/id.hpp>
#include <implus/length.hpp>
#include <implus/overridable.hpp>
#include <implus/placement.hpp>
#include <implus/sizing.hpp>
#include <implus/text.hpp>
#include <optional>
#include <variant>

#include <imgui.h>

namespace ImPlus {

namespace Style {

namespace Button {
inline auto OverflowPolicy =
    overridable<Text::CDOverflowPolicy>({Text::OverflowNone, Text::OverflowNone});
inline auto OverflowWidth = overridable<std::optional<length>>{};
inline auto Layout = overridable<Content::Layout>{Content::Layout::HorzCenter};

// see also (in blocks.hpp):
// - Style::ICD::DescrOffset
// - Style::ICD::DescrSpacing
// - Style::ICD::DescrOpacity

}; // namespace Button

namespace LinkButton {
inline auto OverflowPolicy =
    overridable<Text::CDOverflowPolicy>({Text::OverflowWrap, Text::OverflowWrap});
inline auto OverflowWidth = overridable<std::optional<length>>{};
inline auto Layout = overridable<Content::Layout>{Content::Layout::HorzNear};

namespace Color {
inline auto Content = overridable<ImVec4>{[] {
    auto txt_clr = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    auto btn_clr = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
    return ImPlus::Color::Mix(btn_clr, 0.5f, txt_clr, 0.5f);
}};

// see also (in blocks.hpp):
// - Style::ICD::DescrOffset
// - Style::ICD::DescrSpacing
// - Style::ICD::DescrOpacity

} // namespace Color

namespace Color::Background {
inline auto Regular = overridable<ImVec4>{ImVec4{0, 0, 0, 0}}; // no regular background
inline auto Hovered = overridable<ImVec4>{ImVec4{0, 0, 0, 0}}; // no hovered background
inline auto Active = overridable<ImVec4>{[] {
    return ImPlus::Color::ModulateAlpha(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive), 0.25f);
}};
} // namespace Color::Background
} // namespace LinkButton

} // namespace Style

// ButtonDrawCallback draws everyting related to button visual representation
using ButtonDrawCallback = std::function<void(
    ImGuiID id, ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max, InteractState const&)>;

enum class ButtonShape {
    Regular,
    PointyLeft,
    PointyRight,
};

struct ButtonSidebar {
    ImPlus::Side Side;
    std::optional<ImVec4> Color;
};

struct ButtonOptions {
    bool DefaultAction = false;

    std::optional<length> Rounded = {};
    std::optional<length_vec> Padding = {};
    InteractColorSetCallback ColorSet = {};
    std::optional<ButtonShape> Shape = ButtonShape::Regular;
    std::optional<ButtonSidebar> Sidebar = {};
};

// MakeButtonDrawCallback
//
// - calls ImGui::RenderNavHighlight
// - grabs colors with the on_color() callback
// - draws background
//
auto MakeButtonDrawCallback(ButtonOptions const&, Content::DrawCallback&&) -> ButtonDrawCallback;

auto CalcFramePadding() -> ImVec2;
auto CalcPaddedSize(ImVec2 const& inner, ImVec2 const& padding) -> ImVec2;

// CustomButton provides a callback-based api for displaying buttons with fully-customized rendering
//
// - name is used for debugging
// - measured_size and baseline_offset is used for placing the button with ImGui::ItemSize()
// - actual_size is used to calculate the actual bounding box
//
auto CustomButton(ImID id, char const* name, ImVec2 const& measured_size, float baseline_offset,
    ImVec2 const& actual_size, ImGuiButtonFlags flags, ButtonDrawCallback const& draw_callback)
    -> InteractState;

auto Button(ImID id, ICD_view const& content, Sizing::XYArg const& sizing = {},
    ImGuiButtonFlags flags = ImGuiButtonFlags_None) -> bool;

auto BeginDropDownButton(ImID id, ICD_view const& content, Sizing::XYArg const& sizing = {},
    Placement::Options const& placement = DefaultDropdownPlacement) -> bool;
void EndDropDownButton();

auto LinkButton(ImID id, ICD_view const& content, Sizing::XYArg const& sizing = {},
    ImGuiButtonFlags flags = ImGuiButtonFlags_None) -> bool;

} // namespace ImPlus