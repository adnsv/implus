#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/banner.hpp"
#include "implus/blocks.hpp"
#include "implus/color.hpp"

namespace ImPlus::Banner {

struct colors {
    ImVec4 content = {};
    ImVec4 background = {};
};

auto DisplayEx(ImID id, ICD_view const& content, colors const& clr, Flags flags) -> bool
{
    auto close_clicked = false;

    auto const& style = ImGui::GetStyle();
    auto const fontsz = GImGui->FontSize;
    auto const button_sz = (flags & ShowCloseButton) ? fontsz : 0.0f;
    auto const padding = style.WindowPadding;
    auto const button_extra = button_sz ? button_sz : 0.0f;
    auto const scroller_extra = style.ScrollbarSize + padding.x;

    auto const avail = ImGui::GetContentRegionAvail();
    auto const overflow_w = std::max(1.0f, avail.x - padding.x * 2 - button_extra - scroller_extra);

    auto const block =
        ICDBlock{content, Content::Layout::HorzNear, {}, Style::Banner::OverflowPolicy(), overflow_w};

    auto window_flags = ImGuiWindowFlags_NoScrollbar;
    auto h = std::ceil(block.Size.y + padding.y * 2.0f);

    auto max_h = std::min(avail.y, to_pt<rounded>(Style::Banner::MaxHeight()));
    max_h = std::max(ImGui::GetFrameHeight(), max_h);

    auto const want_scroller = max_h && h > max_h;
    if (want_scroller)
        h = std::max(4.0f, max_h);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, clr.background);
    auto showing = ImGui::BeginChild(id, {avail.x, h},
        ImGuiChildFlags_Border | ImGuiChildFlags_AlwaysUseWindowPadding, window_flags);
    ImGui::PopStyleColor();

    if (showing) {
        if (want_scroller) {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, {0, 0, 0, 0});
            ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, {0, 0, 0, 0});
            ImGui::BeginChild(
                "##SCROLLABLE", {avail.x - padding.x * 2 - button_extra, h - padding.y * 2});
            ImGui::PopStyleColor(2);
        }

        {
            auto window = ImGui::GetCurrentWindow();
            auto dl = window->DrawList;
            auto pos = window->DC.CursorPos;
            auto bb = ImRect{pos, pos + block.Size};
            ImGui::ItemSize(block.Size, 0.0f);
            if (ImGui::ItemAdd(bb, 0))
                block.Render(dl, bb.Min, bb.Max, clr.content);
        }

        if (want_scroller)
            ImGui::EndChild();

        if (flags & ShowCloseButton) {
            auto window = ImGui::GetCurrentWindow();
            auto dl = window->DrawList;
            auto tl = ImVec2{
                window->Pos.x + window->Size.x - padding.x - button_sz, window->Pos.y + padding.y};
            tl -= style.FramePadding;
            ImGui::PushStyleColor(ImGuiCol_Text, clr.content);
            close_clicked = ImGui::CloseButton(window->GetID("##CLOSE"), tl);
            ImGui::PopStyleColor();
        }
    }
    ImGui::EndChild();

    return close_clicked;
}

auto Display(ImID id, ICD_view const& content, Flags flags) -> bool
{
    colors clr;
    clr.background = Style::Banner::BackgroundColor();
    clr.content = Color::ContrastTo(clr.background);
    return DisplayEx(id, content, clr, flags);
}

} // namespace ImPlus::Banner