#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/listbox.hpp"

namespace ImPlus::Listbox {

void internal::ItemSize(const ImVec2& size, float text_baseline_y) {
    ImGui::ItemSize(size, text_baseline_y);
}

auto internal::LastPushedID() -> ImGuiID {
    return GImGui->CurrentWindow->IDStack.back();
}

}