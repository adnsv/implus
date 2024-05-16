#pragma once

#include <implus/button.hpp>
#include <implus/color.hpp>
#include <implus/id.hpp>
#include <implus/length.hpp>
#include <implus/overridable.hpp>
#include <implus/tooltip.hpp>
#include <variant>

#include <imgui.h>

namespace ImPlus::Toolbar {
// default button appearance
enum ButtonAppearance {
    Regular,
    Flat,
    Transparent,
};
} // namespace ImPlus::Toolbar

namespace ImPlus::Style::Toolbar {

namespace Horz {
inline auto ItemLayout = overridable<Content::Layout>{Content::Layout::HorzCenter};
inline auto MinimumButtonWidth = overridable<length>{1.25_em};
inline auto ItemOverflowPolicy =
    overridable<Text::CDOverflowPolicy>({Text::OverflowNone, Text::OverflowNone});
inline auto Height = overridable<length>(0_em); // automatically calculated
} // namespace Horz

namespace Vert {
inline auto ItemLayout = overridable<Content::Layout>{Content::Layout::VertCenter};
inline auto MinimumButtonHeight = overridable<length>([] { return 1.25_em; });
inline auto ItemOverflowPolicy =
    overridable<Text::CDOverflowPolicy>({Text::OverflowNone, Text::OverflowNone});
inline auto Width = overridable<length>(3_em);
} // namespace Vert

inline auto BackgroundColor = overridable<ImVec4>(Color::FromStyle<ImGuiCol_ChildBg>);

namespace LineAfter {
// line that is drawn after the toolbar
inline auto Color = overridable<ImVec4>(Color::FromStyle<ImGuiCol_Separator>);
inline auto Thickness = overridable<length>(1_pt);
} // namespace LineAfter

namespace Button {

inline auto Appearance =
    overridable<ImPlus::Toolbar::ButtonAppearance>(ImPlus::Toolbar::ButtonAppearance::Flat);

} // namespace Button

} // namespace ImPlus::Style::Toolbar

namespace ImPlus::Toolbar {

enum class TextVisibility {
    Always,      // always show caption and description
    CaptionOnly, // show caption, use description for tooltip
    Auto,        // hide text if there is an icon, use both caption and description for tooltip
};

// Options can be used to configure toolbar appearance
//
// - UseWindowPadding: padding between outer bounding box and the inner content
//
// - RemoveSpacingAfterToolbar: removes ItemSpacing gap that would otherwise be
//   inserted after the toolbar (this can be helpful when a toolbar is
//   immediately followed by another scrollable child window)
//
// - IconStacking: layout of images and text for contained items, defaults to
//   Style::Toolbar::Horz/Vert::IconStacking
//
// - UseTransparentButtons: only show button background when hovered
//
// - ShowText: allows to control text visibility in items and labels
//
struct Options {
    std::optional<ImVec4> BackgroundColor = {};
    std::variant<bool, ImVec2> Padding = false;
    bool WantLineAfter = true; // draws line below the toolbar uses Style::Toolbar::LineAfter
    bool RemoveSpacingAfterToolbar = false;
    std::optional<Content::Layout> ItemLayout = {};
    TextVisibility ShowText = TextVisibility::CaptionOnly;
};

// Toolbar::Begin / Toolbar::End - renders a horizontal toolbar strip
//
// - Toolbar::Begin() Returns false if the toolbar is fully clipped or its
//   parent window is collapsed. You can skip the drawing of its content based
//   on this result.
//
// - Always call a matching Toolbar::End() regardles of what Toolbar::Begin()
//   returns.
//
// - Toolbar::End() has an optional remove_spacing parameter that removes
//   ItemSpacing gap that would otherwise be inserted after the toolbar. This
//   can be helpful when a toolbar is immediately followed by another scrollable
//   child window.
//
auto BeginEx(ImID id, ImVec2 const& size, Axis item_stacking, Options const& opts = {}) -> bool;
auto Begin(ImID id, Axis item_stacking, Options const& opts = {}) -> bool;
void End(bool remove_spacing = false);

// FrontPlacement / TrailPlacement / OverflowPlacement controls placement of
// items that follow.
//
// - Front placement (default) places items left-to-right
//
// - Trail placement places items right-to-left
//
// - Overflow placement indicates that all following items will be placed into
//   the overflow submenu.
//
// - When toolbar runs out of space for the next incoming item, an overflow
//   '...' drop down is shown.
//
// - Once overflow is triggered, all the following items will go into it, an
//   subsequent calls to FrontPlacement/TrailPlacement will be ignored.
//
void FrontPlacement();
void TrailPlacement();
void OverflowPlacement();

auto Button(ImPlus::ICD_view const& content, bool selected = false, bool enabled = true,
    ImGuiButtonFlags flags = ImGuiButtonFlags_None, ButtonOptions const& opts = {}) -> bool;

inline auto Button(ImPlus::ICD_view const& content, bool* p_selected, bool enabled = true,
    ImGuiButtonFlags flags = ImGuiButtonFlags_None, ButtonOptions const& opts = {}) -> bool
{
    auto clicked = Button(content, p_selected ? *p_selected : false, enabled, flags, opts);
    if (clicked && p_selected)
        *p_selected = !*p_selected;
    return clicked;
}

void Label(ImPlus::ICD_view const& content, float width = -1, bool enabled = true);
void Spacing(ImPlus::length sz = 0.5_em);
void Separator();

// BeginDropDown displays a button with dropdown menu
//
// - handle drop down content only if this returns true
// - populate subitems with ImGui::MenuItem() calls
// - call EndDropDown() at the end
//
auto BeginDropDown(
    ImPlus::ICD_view const& content, bool enabled = true, bool with_dropdown_arrow = true) -> bool;
inline auto EndDropDown() { ImGui::EndPopup(); }

} // namespace ImPlus::Toolbar