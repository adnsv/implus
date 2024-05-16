#pragma once

#include "implus/color.hpp"
#include "implus/content.hpp"
#include "implus/geometry.hpp"
#include "implus/icd.hpp"
#include "implus/id.hpp"
#include "implus/length.hpp"
#include "implus/overridable.hpp"
#include "implus/placement.hpp"
#include "implus/text.hpp"
#include <cmath>
#include <concepts>
#include <functional>
#include <imgui.h>
#include <optional>

namespace ImPlus {

namespace Style::Tooltip {

inline auto OverflowPolicy =
    overridable<Text::CDOverflowPolicy>(Text::CDOverflowPolicy{Text::OverflowWrap});
inline auto OverflowWidth = overridable<length>{20_em};

inline auto Layout = overridable<Content::Layout>{Content::Layout::HorzNear};
inline auto Colors = overridable<ColorSet>(GetStylePopupColors);

// see also (in blocks.hpp):
// - Style::ICD::DescrOffset
// - Style::ICD::DescrSpacing
// - Style::ICD::DescrOpacity

} // namespace Style::Tooltip

void CustomTooltip(Rect const& anchor, Placement::Options const& placement, ColorSet const& clr,
    ImVec2 const& content_size, Content::DrawCallback&& on_content);

// Tooltip with all parameters explicitly specified.
void Tooltip(Rect const& anchor, Placement::Options placement, ColorSet const& clr,
    ICD_view const& content, Content::Layout layout, Text::CDOverflowPolicy const&,
    std::optional<length> const& overflow_width);

// Tooltip with all parameters from Style::Tooltip.
inline void Tooltip(
    Rect const& anchor, Placement::Options const& placement, ICD_view const& content)
{
    Tooltip(anchor, placement, Style::Tooltip::Colors(), content, Style::Tooltip::Layout(),
        Style::Tooltip::OverflowPolicy(), Style::Tooltip::OverflowWidth());
}

} // namespace ImPlus