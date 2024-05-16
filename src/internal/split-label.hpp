#pragma once

#include <imgui.h>
#include <string_view>

namespace ImPlus::internal {

struct split_label_result {
    ImGuiID id;
    std::string_view content;
};

auto split_label(std::string_view label) -> split_label_result;

} // namespace ImPlus::internal