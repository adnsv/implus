#pragma once

#include "implus/id.hpp"
#include "implus/placement.hpp"
#include <imgui.h>

namespace ImPlus {

constexpr auto DefaultDropdownPlacement = Placement::Options{
    .Direction = ImGuiDir_Down,
    .Alignment = 1.0f,
};

// BeginDropDownPopup is simplified version of ImGui::BeginComboPopup in imgui_widgets.cpp that is
// tailored for showing dropdown popups
//
auto BeginDropDownPopup(ImID id, ImVec2 const& bb_min, ImVec2 const& bb_max,
    Placement::Options const& placement = DefaultDropdownPlacement) -> bool;

} // namespace ImPlus