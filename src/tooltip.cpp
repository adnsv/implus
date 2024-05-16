#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/blocks.hpp"
#include "implus/geometry.hpp"
#include "implus/tooltip.hpp"
#include "internal/draw-utils.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

namespace ImPlus {

inline auto clamp(float v, float lo, float hi) { return v < lo ? lo : hi < v ? hi : v; }

auto calc_tooltip_stem(ImPlus::Rect const& bb, ImPlus::Rect const& anchor, float corner_spacing,
    float& half_stem_width, ImVec2& pos, ImGuiDir& dir) -> bool
{
    ImVec2 distance = {0, 0};
    pos = bb.Center();

    auto clamp_vert = [&] {
        half_stem_width =
            std::max(3.0f, std::min(half_stem_width, 0.5f * bb.Height() - corner_spacing));

        auto const d = std::round(corner_spacing + half_stem_width + 1.0f);
        pos.y = clamp(anchor.Center().y, bb.Min.y + d, bb.Max.y - d);
        return pos.y >= anchor.Min.y && pos.y < anchor.Max.y;
    };
    auto clamp_horz = [&] {
        half_stem_width =
            std::max(3.0f, std::min(half_stem_width, 0.5f * bb.Width() - corner_spacing));
        auto const d = std::round(corner_spacing + half_stem_width + 1.0f);
        pos.x = clamp(anchor.Center().x, bb.Min.x + d, bb.Max.x - d);
        return pos.x >= anchor.Min.x && pos.x < anchor.Max.x;
    };

    auto const edge_tolerance = corner_spacing * 0.5f;

    if (anchor.Max.x < bb.Min.x + edge_tolerance) {
        dir = ImGuiDir_Left;
        pos.x = bb.Min.x;
        return clamp_vert();
    }
    else if (anchor.Min.x > bb.Max.x - edge_tolerance) {
        dir = ImGuiDir_Right;
        pos.x = bb.Max.x;
        return clamp_vert();
    }
    else if (anchor.Max.y < bb.Min.y + edge_tolerance) {
        dir = ImGuiDir_Up;
        pos.y = bb.Min.y;
        return clamp_horz();
    }
    else if (anchor.Min.y > bb.Max.y - edge_tolerance) {
        dir = ImGuiDir_Down;
        pos.y = bb.Max.y;
        return clamp_horz();
    }
    else {
        return false;
    }
}

void render_tooltip_background(ImDrawList* dl, ImPlus::Rect const& bb, ImPlus::Rect const& anchor,
    float stem_length, ImU32 c32, ImU32 f32, float stroke_thickness)
{
    auto const bb_size = bb.Size();
    auto cr = std::min(std::min(bb_size.x, bb_size.y) * 0.25f, ImGui::GetStyle().WindowRounding);
    dl->AddRectFilled(bb.Min, bb.Max, c32, cr);
    ImVec2 pos;
    ImGuiDir dir;

    float half_stem_width = stem_length * 0.75f;

    auto const do_stroke = stroke_thickness > 0 && f32 > 0x00ffffff;
    if (calc_tooltip_stem(bb, anchor, cr, half_stem_width, pos, dir)) {
        auto d = vec2_of(dir);
        auto n = ImVec2{-d.y, d.x};
        d *= stem_length;
        n *= half_stem_width;
        auto c = pos;
        dl->AddTriangleFilled(c - n, c + n, c + d, c32);
        dl->PathClear();

        if (do_stroke) {
            dl->PathLineTo(c - n);
            dl->PathLineTo(c + d);
            dl->PathLineTo(c + n);

            auto tl = [&] { dl->PathArcToFast(ImVec2(bb.Min.x + cr, bb.Min.y + cr), cr, 6, 9); };
            auto tr = [&] { dl->PathArcToFast(ImVec2(bb.Max.x - cr, bb.Min.y + cr), cr, 9, 12); };
            auto br = [&] { dl->PathArcToFast(ImVec2(bb.Max.x - cr, bb.Max.y - cr), cr, 0, 3); };
            auto bl = [&] { dl->PathArcToFast(ImVec2(bb.Min.x + cr, bb.Max.y - cr), cr, 3, 6); };
            switch (dir) {
            case ImGuiDir_Left:
                tl();
                tr();
                br();
                bl();
                break;
            case ImGuiDir_Up:
                tr();
                br();
                bl();
                tl();
                break;
            case ImGuiDir_Right:
                br();
                bl();
                tl();
                tr();
                break;
            case ImGuiDir_Down:
                bl();
                tl();
                tr();
                br();
                break;
            default:;
            }
            dl->PathStroke(f32, ImDrawFlags_Closed, stroke_thickness);
            dl->PathClear();
        }
    }
    else {
        if (do_stroke) {
            dl->AddRect(bb.Min, bb.Max, f32, cr, stroke_thickness);
            dl->PathClear();
        }
    }
}


void CustomTooltip(Rect const& anchor, Placement::Options const& placement, ColorSet const& clr,
    ImVec2 const& content_size, Content::DrawCallback&& on_content)
{
    auto const p = to_pt<rounded>(0.33_em);
    auto padding = ImVec2(p, p);
    auto window_size = content_size + padding * 2;
    auto [window_pos, dir] = Placement::PlaceAroundAnchor(anchor, window_size, placement);

    auto flags = ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoInputs |
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoSavedSettings;

    flags = flags | ImGuiWindowFlags_NoBackground;


    auto stem_length = to_pt<rounded>(0.33_em);

    auto extra_padding = stem_length + 1;
    padding.x += extra_padding;
    padding.y += extra_padding;

    ImGui::SetNextWindowPos(window_pos - ImVec2{extra_padding, extra_padding}, ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        window_size + ImVec2{extra_padding, extra_padding} * 2.0f, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    ImGui::PushStyleColor(ImGuiCol_PopupBg, clr.Background);
    ImGui::Begin("###implus-tooltip", nullptr, flags);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    auto dl = ImGui::GetWindowDrawList();
    auto f32 = ImGui::GetColorU32(ImGuiCol_Border);
    render_tooltip_background(dl, {window_pos, window_pos + window_size}, anchor, stem_length,
        ImGui::GetColorU32(clr.Background), f32, 2.0f);

    if (on_content) {
        auto const& r = ImGui::GetCurrentWindow()->WorkRect;
        on_content(ImGui::GetWindowDrawList(), r.Min + padding, r.Max - padding, clr);
    }
    ImGui::End();
}

void Tooltip(Rect const& anchor, Placement::Options placement, ColorSet const& clr,
    ICD_view const& content, Content::Layout layout, Text::CDOverflowPolicy const& op,
    std::optional<length> const& overflow_width)
{
    auto dir = placement.Direction;
    if (dir != ImGuiDir_Left && dir != ImGuiDir_Right && dir != ImGuiDir_Up)
        dir = ImGuiDir_Down;

    auto horz = (dir == ImGuiDir_Left) || (dir == ImGuiDir_Right);
    auto const bb = Placement::CalcEffectiveOuterLimits(placement);
    auto [avail, avail_other] = Placement::CalcAvailableArea(anchor, bb, dir);

    auto const padding = ImGui::GetStyle().WindowPadding;
    auto const padding_dx = padding.x * 2;

    auto dw = to_pt(overflow_width);
    if (!horz) {
        // vertical stacking
        // - avail.x is the whole width of the outer limits
        if (!dw)
            dw = avail.x - padding_dx;
        else if (*dw + padding_dx > avail.x)
            dw = avail.x - padding_dx;
    }
    auto block = ICDBlock{content, layout, {}, op, dw};

    if (horz) {
        if (block.Size.x + padding_dx > avail.x) {
            if (placement.Fitting == Placement::Fitting::AllowFlipToOtherSide &&
                avail_other.x > avail.x) {
                avail = avail_other;
                dir = Placement::OppositeOf(dir);
                placement.Direction = dir;
                placement.Fitting = Placement::Fitting::AsSpecified;
            }
            if (block.Size.x + padding_dx > avail.x) {
                dw = avail.x - padding_dx;
                block.Reflow(dw);
            }
        }
    }

    CustomTooltip(anchor, placement, clr, block.Size,
        [&](ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max, ColorSet const& clr) {
            block.Render(dl, bb_min, bb_max, clr.Content);
        });
}

} // namespace ImPlus