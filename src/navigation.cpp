#include "implus/navigation.hpp"

#include <imgui.h>
#include <imgui_internal.h>

namespace ImPlus::Navigation {

auto Active() -> bool { return GImGui && GImGui->IO.NavActive; }
auto Visible() -> bool { return GImGui && GImGui->IO.NavVisible; }

auto ActivatePressed(bool extended) -> bool
{
    auto const* g = ImGui::GetCurrentContext();

    if (g->IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) {
        if (ImGui::IsKeyPressed(ImGuiKey_Space))
            return true;
        if (extended &&
            (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)))
            return true;
    }

    if (g->IO.ConfigFlags && ImGuiConfigFlags_NavEnableGamepad) {
        // Activate / Open / Toggle / Tweak
        // - A (Xbox)
        // - B (Switch)
        // - Cross (PS)
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown))
            return true;
    }

    return false;
}

auto CancelPressed() -> bool
{
    auto const* g = ImGui::GetCurrentContext();
    if (g->IO.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            return true;
    }

    if (g->IO.ConfigFlags && ImGuiConfigFlags_NavEnableGamepad) {
        // Cancel / Close / Exit
        // - B (Xbox)
        // - A (Switch)
        // - Circle (PS)
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight))
            return true;
    }

    return false;
};

} // namespace ImPlus::Navigation