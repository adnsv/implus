#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/dropdown.hpp"
#include "implus/pathbox.hpp"
#include "internal/draw-utils.hpp"

#include <cstdint>
#include <cmath>
#include <cstdint>
#include <vector>

namespace ImPlus::Pathbox {

inline auto measure_width(std::string_view s) -> float
{
    if (s.empty())
        return 0.0f;
    return ImGui::CalcTextSize(s.data(), s.data() + s.size(), false, 0.0f).x;
}

auto DefaultHeightPixels() -> float
{
    auto const& g = *GImGui;
    return std::round(g.FontSize + g.Style.FramePadding.y * 2.0f);
}

auto Display(ImID id, ImVec2 const& size_arg, std::span<Item> items, Options const& opts)
    -> std::optional<std::size_t>
{
    auto* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return {};

    auto& g = *GImGui;
    auto& style = g.Style;
    auto const fontsz = g.FontSize;
    auto const padding = std::round(style.FramePadding.x);
    auto const sep_w = std::round(fontsz * 0.5f);
    auto const prev_w = std::round(fontsz * 0.5f + style.FramePadding.x);

    auto const n = items.size();
    auto sum_w = 0.0f;
    for (auto&& it : items) {
        auto text_w = measure_width(it.text);
        it.disp_w = std::round(text_w + padding * 2.0f);
        sum_w += it.disp_w;
    }
    sum_w += n > 0 ? (n - 1) * sep_w : 0.0f;

    auto avail_w = ImGui::GetContentRegionAvail().x;

    auto f = std::size_t{0};
    if (n > 2 && sum_w > avail_w) {
        avail_w -= prev_w;
        while (f + 2 < n && sum_w > avail_w) {
            sum_w -= items[f].disp_w;
            sum_w -= sep_w;
            ++f;
        }
    }

    auto const shrink = sum_w > avail_w;
    if (shrink) {
        auto m = n - f;
        auto g = m > 1 ? sep_w * (m - 1) : 0.0f;
        sum_w -= g;
        avail_w -= g;
        for (auto& it : items.subspan(f))
            it.disp_w *= avail_w / sum_w;
    }

    auto pos = window->DC.CursorPos;
    auto size =
        ImGui::CalcItemSize(size_arg, sum_w, std::round(fontsz + style.FramePadding.y * 2.0f));
    ImGui::ItemSize(size, style.FramePadding.y);

    auto const bb = ImRect{pos, pos + size};

    auto const h = size.y;

    ImGui::PushID(id);

    auto display_as_button = [&](int id, float x, float w, bool dropdown, bool clickable,
                                 bool active, ImRect& bb) -> bool {
        bb = ImRect{x, pos.y, x + w, pos.y + h};
        if (!clickable)
            return false;

        auto pressed = false;

        if (ImGui::ItemAdd(bb, id)) {
            auto flags = dropdown ? ImGuiButtonFlags_(ImGuiButtonFlags_PressedOnClick)
                                  : ImGuiButtonFlags_None;
            bool hovered, held;
            pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

            ImGui::RenderNavHighlight(bb, id);
            auto clr = uint32_t{0};
            if (active || pressed || (held && hovered))
                clr = ImGui::GetColorU32(ImGuiCol_Button);
            else if (hovered)
                clr = ImGui::GetColorU32(ImGuiCol_ButtonHovered, 0.5f);

            if (clr)
                ImGui::RenderFrame(bb.Min, bb.Max, clr, true, style.FrameRounding);
        }

        return pressed;
    };

    auto click_index = std::optional<std::size_t>{};
    auto dl = window->DrawList;

    auto x = pos.x;
    if (f) {
        // show overflow button with dropdown for items that didn't fit
        auto id = window->GetID(-1);

        auto popup_id = window->GetID(-2);
        auto expanded = ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None);

        auto save_flags = g.NextWindowData.Flags;
        g.NextWindowData.ClearFlags();

        ImRect r;
        auto pressed = display_as_button(id, x, prev_w, true, true, expanded, r);
        if (pressed && !expanded) {
            ImGui::OpenPopup(popup_id, ImGuiPopupFlags_None);
            expanded = true;
        }
        DrawArrow(dl, expanded ? ImGuiDir_Down : ImGuiDir_Left, r.GetCenter(), fontsz * 0.5f,
            ImGui::GetColorU32(ImGuiCol_Text));
        x += prev_w;

        if (expanded) {
            g.NextWindowData.Flags = save_flags;
            if (ImPlus::BeginDropDownPopup(popup_id, r.Min, r.Max)) {
                for (auto i = f; i > 0; --i) {
                    auto& it = items[i - 1];
                    ImGui::PushID(id);
                    if (ImGui::MenuItem(it.text.c_str()))
                        click_index = i - 1;
                    ImGui::PopID();
                }
                ImGui::EndPopup();
            }
        }
    }

    auto idx = f;
    for (auto&& it : items.subspan(f)) {
        auto id = window->GetID(idx);
        auto is_last_item = &it == &items.back();

        auto clickable = (it.flags & ItemFlags_DisableClick) == 0;

        ImRect r;
        if (display_as_button(id, x, it.disp_w, false, clickable, false, r))
            click_index = idx;
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        r.Min += {padding, style.FramePadding.y};
        r.Max -= {padding, style.FramePadding.y};
        if (!it.text.empty()) {
            if (!shrink)
                ImGui::RenderTextClipped(r.Min, r.Max, it.text.data(),
                    it.text.data() + it.text.size(), nullptr, {0, 0.5f}, &r);
            else
                ImGui::RenderTextEllipsis(window->DrawList, r.Min, r.Max, r.Max.x, r.Max.x,
                    it.text.data(), it.text.data() + it.text.size(), nullptr);
        }
        x += it.disp_w;

        if (!is_last_item) {
            auto xy = ImVec2{x + 0.5f * sep_w, pos.y + h * 0.5f};
            DrawArrow(
                dl, ImGuiDir_Right, xy, fontsz * 0.5f, ImGui::GetColorU32(ImGuiCol_Text, 0.3f));
        }

        x += sep_w;
        ++idx;
    }

    ImGui::PopID();

    if (opts.RemoveSpacingAfter)
        window->DC.CursorPos.y -= style.ItemSpacing.y;

    return click_index;
}

} // namespace ImPlus::Pathbox