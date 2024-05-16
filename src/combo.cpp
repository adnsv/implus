#include "implus/combo.hpp"

#include <imgui_internal.h>

namespace ImPlus::Combo {

void internal::SetNextWindowLimits(float item_height, float item_spacing)
{
    ImGuiContext& g = *GImGui;
    if (g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint)
        return;

    auto const n = Style::Combo::LimitVisibleItems();
    auto h = (item_height + item_spacing) * n - item_spacing + (g.Style.WindowPadding.y * 2);

    h = std::min(h, to_pt(Style::Combo::LimitHeight()));

    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, h));
}

} // namespace ImPlus::Combo