#include <imgui_internal.h>

#include "implus/dropdown.hpp"
#include "implus/geometry.hpp"

namespace ImPlus {

auto BeginDropDownPopup(ImID id, ImVec2 const& bb_min, ImVec2 const& bb_max,
    Placement::Options const& placement) -> bool
{
    auto& g = *GImGui;
    if (!ImGui::IsPopupOpen(id, ImGuiPopupFlags_None)) {
        g.NextWindowData.ClearFlags();
        return false;
    }

    char name[16];
    ImFormatString(name, IM_ARRAYSIZE(name), "##IPDropdown_%02d",
        g.BeginPopupStack.Size); // Recycle windows based on depth

    if (auto* popup_window = ImGui::FindWindowByName(name))
        if (popup_window->WasActive) {
            auto size_expected = ImGui::CalcWindowNextAutoFitSize(popup_window);

            auto [pos, dir] =
                ImPlus::Placement::PlaceAroundAnchor({bb_min, bb_max}, size_expected, placement);

            popup_window->AutoPosLastDirection = dir;
            ImGui::SetNextWindowPos(pos);
        }

    auto window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup |
                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;

    auto ret = ImGui::Begin(name, nullptr, window_flags);
    if (!ret)
        ImGui::EndPopup();

    return ret;
}

auto BeginDropDownPopup(ImID id, Placement::Options const& placement) -> bool
{
    auto const bb = ImGui::GetCurrentContext()->LastItemData.Rect;
    return BeginDropDownPopup(id, bb.Min, bb.Max, placement);
}

} // namespace ImPlus