#pragma once

#include "implus/id.hpp"
#include "implus/icd.hpp"
#include "implus/overridable.hpp"
#include "implus/tooltip.hpp"

namespace ImPlus {

using time_duration_seconds = float;

namespace Style::BalloonTip {
inline auto SolidDuration = overridable<time_duration_seconds>{3.0f};
inline auto FadeDuration = overridable<time_duration_seconds>{0.25f};
} // namespace Style

void OpenBalloonTip(ImID id, ICD_view const& content, Placement::Options const& placement = {});
void OpenLastItemBalloonTip(ICD_view const& content, Placement::Options const& placement = {});

void BalloonTip(ImID anchor_id, ImVec2 const& anchor_bb_min, ImVec2 const& anchor_bb_max);

void HandleLastItemBalloonTip();

} // namespace ImPlus