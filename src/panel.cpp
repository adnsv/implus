#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/panel.hpp"

#include <algorithm>

namespace ImPlus::Panel {

int stagger_ix = 0;
int stagger_iy = 0;

auto Begin(char const* name, Options const& options) -> bool
{
    int flags = ImGuiWindowFlags_None;

    ImGuiContext& g = *GImGui;

    auto viewport = ImGui::GetMainViewport();
    auto const box_pos = options.UseWorkArea ? viewport->WorkPos : viewport->Pos;
    auto const box_size = options.UseWorkArea ? viewport->WorkSize : viewport->Size;

    //    auto host_box = ImRect{{0, 0}, g.IO.DisplaySize};

    //    if (options.AdjustForMainMenu) {
    //        auto menu_bottom =
    //            g.NextWindowData.MenuBarOffsetMinVal.y + g.FontBaseSize + g.Style.FramePadding.y
    //            * 2.0f;
    //        host_box.Min.y = menu_bottom;
    //    }

    auto style_var_pops = 0;

    if (options.Floating) {
        auto const& uscale = g.FontSize;
        auto const stagger_origin = ImVec2{viewport->WorkPos.x + uscale, viewport->WorkPos.y + ImGui::GetFrameHeight() + uscale};
        auto const stagger_step = ImVec2{uscale * 2.0f, uscale * 2.0f};
        auto limsz = viewport->WorkSize - stagger_origin;

        limsz.x = std::max(limsz.x, stagger_step.x * 3.0f);
        limsz.y = std::max(limsz.y, stagger_step.x * 3.0f);

        if (auto wnd = ImGui::FindWindowByName(name); !wnd) {
            // first time show
            auto sz = to_pt<rounded>(options.InitWidth, options.InitHeight);
            sz.x = std::round(std::min(sz.x, limsz.x));
            sz.y = std::round(std::min(sz.y, limsz.y));
            if (stagger_origin.x + stagger_ix * stagger_step.x + sz.x > limsz.x)
                stagger_ix = 0;
            if (stagger_origin.y + stagger_iy * stagger_step.y + sz.y > limsz.y)
                stagger_iy = 0;

            auto pos = ImVec2{//
                std::round(stagger_origin.x + stagger_ix * stagger_step.x),
                std::round(stagger_origin.y + stagger_iy * stagger_step.y)};
            ++stagger_ix;
            ++stagger_iy;

            ImGui::SetNextWindowPos(pos, ImGuiCond_Once);
            ImGui::SetNextWindowSize(sz, ImGuiCond_Once);
        }

        flags = ImGuiWindowFlags_NoSavedSettings;
        if (options.Padding == Condition::WhenFloating || options.Padding == Condition::Always) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
            ++style_var_pops;
        }
    }
    else {
        flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::SetNextWindowPos(box_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(box_size, ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ++style_var_pops;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ++style_var_pops;

        if (options.Padding == Condition::WhenFloating || options.Padding == Condition::Never) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
            ++style_var_pops;
        }

        
    }

    if (!options.VertScrollbar)
        flags |= ImGuiWindowFlags_NoScrollbar;
    if (options.HorzScrollbar)
        flags |= ImGuiWindowFlags_HorizontalScrollbar;

    auto is_open = ImGui::Begin(name, nullptr, flags);

    ImGui::PopStyleVar(style_var_pops);

    return is_open;
}

void End() { ImGui::End(); }

} // namespace ImPlus::Panel