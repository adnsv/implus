#include <imgui.h>

#include "implus/interact.hpp"

namespace ImPlus {

auto GetSelectionModifier() -> SelectionModifier
{
    if (ImGui::IsKeyDown(ImGuiMod_Ctrl))
        return SelectionModifier::Toggle;
    else if (ImGui::IsKeyDown(ImGuiMod_Shift))
        return SelectionModifier::Range;
    else
        return SelectionModifier::Regular;
}

auto MouseSourceIsTouchScreen() -> bool
{
    return ImGui::GetIO().MouseSource == ImGuiMouseSource_TouchScreen;
}

auto NeedsHoverHighlight() -> bool { return !MouseSourceIsTouchScreen(); }

} // namespace ImPlus