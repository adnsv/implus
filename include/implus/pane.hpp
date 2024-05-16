#pragma once

#include <imgui.h>

#include <implus/id.hpp>
#include <implus/length.hpp>

namespace ImPlus {
void HorzSpacing(ImPlus::length dx);
void VertSpacing(ImPlus::length dy);
} // namespace ImPlus

namespace ImPlus::Pane {

struct Options {
    std::variant<bool, ImVec2> Padding = false;
    std::optional<ImVec4> Color = {};
};

auto Begin(ImPlus::ImID id, ImVec2 const& size_arg, ImGuiWindowFlags window_flags, Options const& opts = {}) -> bool;
void End(); // always call, regardless of what is returned in the Begin

} // namespace ImPlus::Pane