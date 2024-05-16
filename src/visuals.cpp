#include <implus/visuals.hpp>
#include <implus/font.hpp>
#include <implus/host.hpp>
#include <implus/icon.hpp>

#include <imgui_internal.h>

#include <algorithm>
#include <cmath>

namespace ImPlus::Visuals {

auto zoom_ = 100; // screen scale percentage
auto zoom_min_ = 50;
auto zoom_max_ = 300;

auto Zoom() -> int { return zoom_; }
void SetZoom(int z) { zoom_ = std::clamp(z, zoom_min_, zoom_max_); }
void ResetZoom() { zoom_ = 100; }
void StepZoom(int step)
{
    auto z = std::round(20.0f * std::log10(Zoom() * 1e-2f) + step);
    SetZoom(int(1e2f * std::pow(10.0f, z * 0.05f)));
}

namespace Theme {

StyleProc const AdjustSizesAndRounding = [](ImGuiStyle& style) {
    style.ScrollbarSize = 12.0f;
    style.WindowRounding = 3.0f;
    style.FrameRounding = 2.0f;
};

StyleProc const LightColors = [](ImGuiStyle& style) {
    ImGui::StyleColorsLight(&style);
    style.Colors[ImGuiCol_Text] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
};

StyleProc const DarkColors = [](ImGuiStyle& style) {
    ImGui::StyleColorsDark(&style);
    style.Colors[ImGuiCol_Text] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.60f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.60f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.60f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.8f);
};

StyleProc const ClassicColors = [](ImGuiStyle& style) {
    ImGui::StyleColorsClassic(&style);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.14f, 1.0f);
};

Style const Light = Style{LightColors | AdjustSizesAndRounding};
Style const Dark = Style{DarkColors | AdjustSizesAndRounding};
Style const Classic = Style{ClassicColors | AdjustSizesAndRounding};

Style current = Light;
auto modified = false;

auto Is(Style const& p) -> bool { return current == p; }

void Setup(Style const& p)
{
    if (current != p) {
        current = p;
        modified = true;
    }
}

} // namespace Theme

auto SetupFrame(Host::Window::Scale const& scale) -> bool
{
    static auto first = true;

    auto& io = ImGui::GetIO();
    auto zoom_factor = zoom_ * 0.01f;
    auto rebuild = ImPlus::Font::Setup(zoom_factor * scale.dpi, scale.fb_scale);
    if (first || rebuild || Theme::modified) {
        first = false;
        Theme::modified = false;
        auto& sty = ImGui::GetCurrentContext()->Style;
        sty = ImGuiStyle{};
        Theme::current.apply(sty);
        sty.ScaleAllSizes(zoom_factor * scale.dpi * scale.fb_scale / 96.0f);
        ImPlus::ResettableResource::ResetAll();
        ImPlus::Host::InvalidateDeviceObjects();
    }
    return rebuild;
}

} // namespace ImPlus::Visuals