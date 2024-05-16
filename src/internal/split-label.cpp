#include <imgui_internal.h>

#include "split-label.hpp"

namespace ImPlus::internal {

auto split_label(std::string_view label) -> split_label_result
{
    auto const n = label.size();
    auto const p = label.find("##");
    auto* window = ImGui::GetCurrentWindow();
    auto const id = window ? window->GetID(label.data(), label.data() + label.size()) : 0;
    if (p < n)
        label.remove_suffix(n - p);
    return {id, label};
}

} // namespace ImPlus::internal