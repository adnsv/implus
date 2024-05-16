#pragma once

#include <imgui.h>

#include "implus/color.hpp"
#include "implus/icd.hpp"
#include "implus/id.hpp"
#include "implus/length.hpp"
#include "implus/overridable.hpp"
#include "implus/text.hpp"

namespace ImPlus {

namespace Style::Banner {

// Style::Banner::Banner::BackgroundColor is a fill color for the banner
inline auto BackgroundColor = overridable<ImVec4>(Color::FromStyle<ImGuiCol_ChildBg>);

inline auto OverflowPolicy =
    overridable<Text::CDOverflowPolicy>({Text::OverflowWrap, Text::OverflowWrap});

inline auto MaxHeight = overridable<length>(6_em);

// see also (in blocks.hpp):
// - Style::ICD::DescrOffset
// - Style::ICD::DescrSpacing
// - Style::ICD::DescrOpacity

} // namespace Style::Banner

namespace Banner {

enum Flags {
    None = 0,
    ShowCloseButton = 0x01,
};

struct Options {
    bool ShowCloseButton = false;
    std::optional<length> MaxHeight = {}; // show scroller if overflows vertically
};

auto Display(ImID id, ICD_view const& id_content, Flags flags = None) -> bool;

} // namespace Banner

} // namespace ImPlus