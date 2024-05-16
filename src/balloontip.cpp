#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/balloontip.hpp"
#include "implus/geometry.hpp"
#include "implus/length.hpp"
#include "implus/text.hpp"
#include "implus/tooltip.hpp"
#include <optional>

namespace ImPlus {

using timepoint_type = decltype(ImGui::GetTime()); // assume this is in seconds

ImGuiID current_anchor_id = 0;
Placement::Options current_placement = {};
ICD current_content = {};
std::optional<timepoint_type> show_time = {};
time_duration_seconds solid_duration = 3.0f;
time_duration_seconds fade_duration = 0.25f;
Text::CDOverflowPolicy overflow_policy = Text::OverflowWrap;
std::optional<length> overflow_width;
Content::Layout layout = Content::Layout::HorzNear;
ColorSet colors = {};

void OpenBalloonTip(ImID id, ICD_view const& content, Placement::Options const& placement)
{
    current_anchor_id = id;
    current_content = content;
    current_placement = placement;
    show_time = ImGui::GetTime();
    solid_duration = std::max(0.001f, Style::BalloonTip::SolidDuration());
    fade_duration = std::max(0.001f, Style::BalloonTip::FadeDuration());
    overflow_policy = Style::Tooltip::OverflowPolicy();
    overflow_width = Style::Tooltip::OverflowWidth();
    layout = Style::Tooltip::Layout();
    colors = Style::Tooltip::Colors();
}

void OpenLastItemBalloonTip(ICD_view const& content, Placement::Options const& placement)
{
    auto& g = *GImGui;
    if (g.LastItemData.ID)
        OpenBalloonTip(g.LastItemData.ID, content, placement);
}

void BalloonTip(ImID anchor_id, ImVec2 const& anchor_bb_min, ImVec2 const& anchor_bb_max)
{
    if (!show_time || anchor_id != current_anchor_id)
        return;

    auto& g = *GImGui;

    auto const now = g.Time;
    auto d = now - *show_time;
    if (d < 0 || d > solid_duration + fade_duration) {
        show_time.reset();
        return;
    }

    auto opacity = 1.0f;
    if (d > solid_duration) {
        d -= solid_duration;
        opacity *= 1.0f - (1.0f * d / fade_duration);
    }

    auto padding = to_pt(0.25_em);

    auto r = ImPlus::Rect{anchor_bb_min, anchor_bb_max};
    r.Expand(padding, padding);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, opacity);
    ImPlus::Tooltip(
        r, current_placement, colors, current_content, layout, overflow_policy, overflow_width);
    ImGui::PopStyleVar();
}

void HandleLastItemBalloonTip()
{
    auto& g = *GImGui;
    if (g.LastItemData.ID)
        BalloonTip(g.LastItemData.ID, g.LastItemData.Rect.Min, g.LastItemData.Rect.Max);
}

} // namespace ImPlus