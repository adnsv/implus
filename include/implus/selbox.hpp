#pragma once

#include "implus/content.hpp"
#include "implus/id.hpp"
#include "implus/interact.hpp"
#include <functional>
#include <imgui.h>
#include <string_view>

const auto ImGuiSelectableFlags_ExtendFrameHorz = ImGuiSelectableFlags(1 << 29);
const auto ImGuiSelectableFlags_ExtendFrameVert = ImGuiSelectableFlags(1 << 30);

namespace ImPlus {

// note: name is used only for test_engine
auto SelectableBox(ImID id, char const* name, bool selected, ImGuiSelectableFlags flags,
    ImVec2 size, InteractColorSetCallback on_color, Content::DrawCallback draw_callback)
    -> InteractState;

} // namespace ImPlus