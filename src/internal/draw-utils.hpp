#pragma once

#include <imgui.h>

namespace ImPlus {

constexpr auto vec2_of(ImGuiDir direction) -> ImVec2
{
    switch (direction) {
    case ImGuiDir_Left: return {-1, 0};
    case ImGuiDir_Right: return {+1, 0};
    case ImGuiDir_Up: return {0, -1};
    case ImGuiDir_Down: return {0, +1};
    default: return {0, 0};
    }
}

void DrawArrow(ImDrawList* dl, ImVec2 const& direction, ImVec2 const& pos, float size, ImU32 clr32);

inline void DrawArrow(ImDrawList* dl, ImGuiDir direction, ImVec2 const& pos, float size, ImU32 clr32)
{
    DrawArrow(dl, vec2_of(direction), pos, size, clr32);
}

} // namespace ImPlus