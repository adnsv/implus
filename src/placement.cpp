#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/placement.hpp"
#include <cmath>

namespace ImPlus::Placement {

auto CalcEffectiveOuterLimits(Options const& opts) -> ImPlus::Rect
{
    if (opts.OuterLimits)
        return *opts.OuterLimits;

    auto r = ImGui::GetPopupAllowedExtentRect(nullptr);
    return Rect{r};
}

auto OppositeOf(Direction dir) -> Direction
{
    switch (dir) {
    case ImGuiDir_Left: return ImGuiDir_Right;
    case ImGuiDir_Right: return ImGuiDir_Left;
    case ImGuiDir_Up: return ImGuiDir_Down;
    case ImGuiDir_Down: return ImGuiDir_Up;
    default: return ImGuiDir_None;
    }
}

auto CalcAvailableArea(Rect const& inner, Rect const& outer, Direction dir)
    -> std::pair<ImVec2, ImVec2>
{
    auto s1 = outer.Max - outer.Min;
    auto s2 = s1;
    switch (dir) {
    case ImGuiDir_Left:
        s1.x = inner.Min.x - outer.Min.x;
        s2.x = outer.Max.x - inner.Max.x;
        break;
    case ImGuiDir_Right:
        s1.x = outer.Max.x - inner.Max.x;
        s2.x = inner.Min.x - outer.Min.x;
        break;
    case ImGuiDir_Up:
        s1.y = inner.Min.y - outer.Min.y;
        s2.y = outer.Max.y - inner.Max.y;
        break;
    case ImGuiDir_Down:
        s1.y = outer.Max.y - inner.Max.y;
        s2.y = inner.Min.y - outer.Min.y;
        break;
    default:;
    }
    return {s1, s2};
}

auto align(float b1, float b2, float a1, float a2, float sz, float alignment) -> float
{
    b2 -= sz;
    auto p = a1 + (a2 - sz - a1) * alignment;
    return p <= b1 ? b1 : p >= b2 ? b2 : p;
}

auto PlaceAroundAnchor(Rect const& anchor, ImVec2 const& size, Options const& placement)
    -> PlaceResult
{
    auto dir = placement.Direction;
    if (dir != ImGuiDir_Left && dir != ImGuiDir_Right && dir != ImGuiDir_Up)
        dir = ImGuiDir_Down;

    auto horz = (dir == ImGuiDir_Left) || (dir == ImGuiDir_Right);
    auto const outer = CalcEffectiveOuterLimits(placement);
    auto [avail, avail_other] = CalcAvailableArea(anchor, outer, dir);

    if (placement.Fitting == Fitting::AllowFlipToOtherSide) {
        auto flip = false;
        if (horz)
            flip = size.x > avail.x && avail_other.x > avail.x;
        else
            flip = size.y > avail.y && avail_other.y > avail.y;
        if (flip) {
            dir = OppositeOf(dir);
            avail = avail_other;
        }
    }

    auto ret = PlaceResult{.Dir = dir};

    if (horz) {
        // place horizontally left or right
        ret.Pos.x = std::round((dir == ImGuiDir_Left) ? anchor.Min.x - size.x : anchor.Max.x);
        ret.Pos.y = std::round(align(
            outer.Min.y, outer.Max.y, anchor.Min.y, anchor.Max.y, size.y, placement.Alignment));
    }
    else {
        // place vertically above or below
        ret.Pos.x = std::round(align(
            outer.Min.x, outer.Max.x, anchor.Min.x, anchor.Max.x, size.x, placement.Alignment));
        ret.Pos.y = std::round((dir == ImGuiDir_Up) ? anchor.Min.y - size.y : anchor.Max.y);
    }
    return ret;
}

} // namespace ImPlus::Placement