#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <implus/pagination.hpp>
#include <string>

namespace ImPlus::Pagination {

static auto measure_text(std::string_view s) -> ImVec2
{
    return ImGui::CalcTextSize(s.data(), s.data() + s.size());
}

static auto display_button(ImID id, ImVec2 const& pos, ImVec2 const& size,
    std::string_view const& s, bool active, bool disabled) -> bool
{
    ImGui::BeginDisabled(disabled);

    auto const bb = ImRect{pos, pos + size};
    ImGui::ItemAdd(bb, id);

    bool hovered, held;
    auto pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_None);

    ImGui::RenderNavHighlight(bb, id);
    auto bg_clr = uint32_t{0};
    auto fg_clr = ImGui::GetColorU32(ImGuiCol_Text);
    if (active || pressed || (held && hovered))
        bg_clr = ImGui::GetColorU32(ImGuiCol_ButtonActive);
    else if (hovered)
        bg_clr = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
    // else
    //     bg_clr = ImGui::GetColorU32(ImGuiCol_Button);

    if (bg_clr) {
        ImGui::RenderFrame(bb.Min, bb.Max, bg_clr, true, GImGui->Style.FrameRounding);
    }

    ImGui::EndDisabled();

    auto const text_sz = measure_text(s);

    auto window = ImGui::GetCurrentWindow();
    window->DrawList->AddText(
        {pos.x + 0.5f * (size.x - text_sz.x), pos.y + 0.5f * (size.y - text_sz.y)}, fg_clr,
        s.data(), s.data() + s.size());

    return pressed;
}

auto Display(ImID id, ImVec2 const& size_arg, std::size_t index, std::size_t count,
    Options const& opts) -> std::optional<std::size_t>
{
    auto const ellipsis = std::string_view{"..."};
    if (count < 1)
        count = 1;

    auto* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return {};

    auto& g = *GImGui;
    auto const& style = g.Style;

    auto const em = g.FontSize;
    auto const padding = style.FramePadding;
    auto const min_item_width = em * 0.75f + padding.x * 2.0f;
    auto const arrow_width = min_item_width;
    auto const ellipsis_width = measure_text(ellipsis).x * 1.5f;

    auto item_w = measure_text(std::to_string(8)).x;
    if (count >= 1000)
        item_w *= 3.5f;
    else if (count >= 100)
        item_w *= 2.75f;
    else
        item_w *= 2.0f;
    item_w += padding.x * 2.0f;
    item_w = std::max(min_item_width, item_w);

    auto const avail = ImGui::GetContentRegionAvail();

    auto i = index;
    if (i >= count)
        i = 0;

    auto range_lo = std::size_t{0};
    auto range_hi = std::size_t{0};

    if (count <= 1) {
        // noop
    }
    else if (count <= 5) {
        range_lo = 0;
        range_hi = count - 1;
    }
    else {
        range_lo = index;
        range_hi = index;
        if (index <= 1) {
            range_lo = 0;
            range_hi = 3;
        }
        else if (range_hi + 2 >= count) {
            range_lo = count - 4;
            range_hi = count - 1;
        }
        else {
            --range_lo;
            ++range_hi;
        }
    }

    auto size = size_arg;
    if (size_arg.y == 0)
        size.y = em + padding.y * 2.0f;
    else if (size_arg.y < 0)
        size.y = std::max(4.0f, avail.y + size_arg.y);

    if (size_arg.x < 0) {
        size.x = std::max(4.0f, avail.x + size_arg.x);
    }
    else if (size_arg.x == 0) {
        size.x = arrow_width * 2.0f;
        for (auto i = range_lo; i <= range_hi; ++i)
            size.x += item_w;

        if (range_lo > 0) {
            size.x += item_w;
        }

        if (range_hi + 1 < count) {
            size.x += item_w;
        }

        if (range_lo > 1 || range_hi + 2 < count)
            size.x += ellipsis_width * 2.0f;
    }

    auto pos = window->DC.CursorPos;
    ImGui::ItemSize(size, padding.y);
    auto const bb = ImRect{pos, pos + size};

    ImGui::PushID(id);

    auto x = pos.x;

    auto clicked = std::optional<std::size_t>{};
    auto make_item = [&](std::size_t page_index) {
        if (display_button(page_index + 1, {x, pos.y}, {item_w, size.y},
                std::to_string(page_index + 1), index == page_index, false))
            clicked = page_index;
        x += item_w;
    };

    auto make_ellipsis = [&](bool double_width) {
        window->DrawList->AddText(
            {x + (double_width ? ellipsis_width * 0.5f : 0.0f), pos.y + padding.y},
            ImGui::GetColorU32(ImGuiCol_Text), ellipsis.data(), ellipsis.data() + ellipsis.size());
        x += ellipsis_width;
        if (double_width)
            x += ellipsis_width;
    };

    if (display_button("-1", pos, {arrow_width, size.y}, "<", false, index <= 0) && index > 0)
        clicked = index - 1;
    x += arrow_width;

    auto const ellipsis_before = range_lo > 1;
    auto const ellipsis_after = range_hi + 2 < count;

    if (range_lo > 0)
        make_item(0);
    if (ellipsis_before)
        make_ellipsis(!ellipsis_after);
    for (auto i = range_lo; i <= range_hi; ++i) {
        make_item(i);
    }
    if (ellipsis_after)
        make_ellipsis(!ellipsis_before);
    if (range_hi + 1 < count)
        make_item(count - 1);

    if (display_button("+1", {x, pos.y}, {arrow_width, size.y}, ">", false, index + 1 >= count) &&
        (index + 1 < count)) {
        clicked = index + 1;
    }

    ImGui::PopID();

    return clicked;
}

} // namespace ImPlus::Pagination