#include <imgui_internal.h>

#include <implus/pane.hpp>

namespace ImPlus {

void HorzSpacing(ImPlus::length dx) { ImGui::SameLine(0, ImPlus::to_pt<ImPlus::rounded>(dx)); }

void VertSpacing(ImPlus::length dy)
{
    ImGui::GetCurrentWindow()->DC.CursorPos.y +=
        ImPlus::to_pt<ImPlus::rounded>(dy) - ImGui::GetStyle().ItemSpacing.y;
}

} // namespace ImPlus

namespace ImPlus::Pane {

auto Begin(ImPlus::ImID id, ImVec2 const& size_arg, ImGuiWindowFlags window_flags, Options const& opts) -> bool
{
    if (opts.Color)
        ImGui::PushStyleColor(ImGuiCol_ChildBg, *opts.Color);

    auto child_flags = ImGuiChildFlags{ImGuiChildFlags_None};
    auto n_style_var = 0;

    if (auto b = std::get_if<bool>(&opts.Padding)) {
        if (*b)
            child_flags |= ImGuiChildFlags_AlwaysUseWindowPadding;
    }
    else if (auto v = std::get_if<ImVec2>(&opts.Padding)) {
        child_flags |= ImGuiChildFlags_AlwaysUseWindowPadding;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, *v);
        ++n_style_var;
    }

    auto ret = ImGui::BeginChild(id, size_arg, child_flags, window_flags);
    if (n_style_var)
        ImGui::PopStyleVar(n_style_var);
    if (opts.Color)
        ImGui::PopStyleColor();
    return ret;
}

void End() { ImGui::EndChild(); }

} // namespace ImPlus::Pane
