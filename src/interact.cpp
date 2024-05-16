#include <imgui.h>

#include "implus/interact.hpp"

namespace ImPlus {

auto GetSelectionModifier() -> SelectionModifier
{
    if (ImGui::IsKeyDown(ImGuiKey_ModCtrl))
        return SelectionModifier::Toggle;
    else if (ImGui::IsKeyDown(ImGuiKey_ModShift))
        return SelectionModifier::Range;
    else
        return SelectionModifier::Regular;
}

} // namespace ImPlus