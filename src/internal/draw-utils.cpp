#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "draw-utils.hpp"
#include <cmath>

namespace ImPlus {

void DrawArrow(ImDrawList* dl, ImVec2 const& direction, ImVec2 const& pos, float size, ImU32 clr32)
{
    auto d = direction * size;
    auto n = ImVec2{-d.y, d.x}; // normal

    d *= 0.3f;
    n *= 0.3f;

    auto c = pos - d * 0.35f;
    c.x = std::round(c.x - 0.5f) + 0.5f;
    c.y = std::round(c.y - 0.5f) + 0.5f;
    dl->AddTriangleFilled(c - n, c + n, c + d, clr32);
}

} // namespace ImPlus