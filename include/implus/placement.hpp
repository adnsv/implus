#pragma once

#include "implus/geometry.hpp"
#include <imgui.h>
#include <optional>
#include <utility>

namespace ImPlus::Placement {

using Direction = ImGuiDir;

auto OppositeOf(Direction dir) -> Direction;

enum class Fitting {
    AsSpecified,          // use specified Direction
    AllowFlipToOtherSide, // allow flipping to another side (up<->down, left<->right)
};

struct Options {
    Placement::Direction Direction = ImGuiDir_Up;
    Placement::Fitting Fitting = Fitting::AllowFlipToOtherSide;
    float Alignment = 0.5f; // alignment across another axis (0.0 - align tops
                            // or lefts, 1.0 - align bottoms or rights)

    std::optional<ImPlus::Rect> OuterLimits = {};
};

auto CalcEffectiveOuterLimits(Options const& opts) -> ImPlus::Rect;

auto CalcAvailableArea(Rect const& inner, Rect const& outer, Direction)
    -> std::pair<ImVec2, ImVec2>;

// PlaceAroundAnchor calculates location of a tooltip, hint or sumbenu
// of a given size that needs to placed around the specified anchor rectangle.
// Uses the specified direction and alignment for the new location. The new
// location will not overlap the anchor. Allows to specify outer
// limits to make sure it does not go outside the screen. Allows flipping the
// side left<->right and above<->below to avoid overlapping.
//
struct PlaceResult {
    ImVec2 Pos = {0, 0};
    Direction Dir = ImGuiDir_None;
};

auto PlaceAroundAnchor(Rect const& anchor, ImVec2 const& size, Options const&) -> PlaceResult;

} // namespace ImPlus::Placement