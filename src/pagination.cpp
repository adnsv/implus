#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <cmath>
#include <implus/pagination.hpp>
#include <string>

namespace ImPlus::Pagination {

static auto measure_text(std::string_view s) -> ImVec2
{
    return ImGui::CalcTextSize(s.data(), s.data() + s.size());
}

static auto display_button(ImID id, ImVec2 const& pos, ImVec2 const& size,
    std::string_view const& s, bool active, bool disabled, bool arrow) -> bool
{
    ImGui::BeginDisabled(disabled);

    auto const bb = ImRect{pos, pos + size};
    ImGui::ItemAdd(bb, id);

    bool hovered, held;
    auto pressed = ImGui::ButtonBehavior(
        bb, id, &hovered, &held, arrow ? ImGuiButtonFlags_Repeat : ImGuiButtonFlags_None);

    ImGui::RenderNavHighlight(bb, id);
    auto bg_clr = uint32_t{0};
    auto fg_clr = ImGui::GetColorU32(ImGuiCol_Text);
    if (pressed || (held && hovered))
        bg_clr = ImGui::GetColorU32(ImGuiCol_ButtonActive);
    else if (hovered)
        bg_clr = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
    else if (active)
        bg_clr = ImGui::GetColorU32(ImGuiCol_Button);

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

auto Display(ImID id, ImVec2 const& size_arg, std::size_t current_page, std::size_t total_pages,
    Options const& opts) -> std::optional<std::size_t>
{
    auto const ellipsis = std::string_view{"..."};
    if (total_pages < 1)
        total_pages = 1;
    if (current_page >= total_pages)
        current_page = total_pages - 1;

    auto* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return {};

    auto& g = *GImGui;
    auto const& style = g.Style;

    auto const avail = ImGui::GetContentRegionAvail();

    auto const em = g.FontSize;
    auto const padding = style.FramePadding;
    auto const min_item_width = em * 0.9f + padding.x * 2.0f;
    auto const arrow_width = min_item_width;

    auto box_w = measure_text("8").x;
    if (total_pages >= 10000)
        box_w *= 4.5f;
    else if (total_pages >= 1000)
        box_w *= 3.5f;
    else if (total_pages >= 100)
        box_w *= 2.75f;
    else
        box_w *= 2.0f;
    box_w += padding.x * 2.0f;
    box_w = std::max(min_item_width, box_w);

    auto h = size_arg.y;
    if (size_arg.y == 0)
        h = em + padding.y * 2.0f;
    else if (size_arg.y < 0)
        h = std::max(4.0f, avail.y + size_arg.y);

    auto w = size_arg.x;
    if (size_arg.x < 0) {
        w = std::max(arrow_width * 2.0f + min_item_width + 1.0f, avail.x + size_arg.x);
    }
    else if (size_arg.x > 0) {
        w = std::max(arrow_width * 2.0f + min_item_width + 1.0f, size_arg.x);
    }
    else {
        // auto-size
        w = arrow_width * 2.0f + std::min(total_pages, std::size_t(11)) * box_w + 1.0f;
    }

    auto pos = window->DC.CursorPos;
    ImGui::ItemSize({w, h}, padding.y);
    auto const bb = ImRect{pos, pos + ImVec2{w, h}};

    ImGui::PushID(id);

    auto x = pos.x;

    //window->DrawList->AddRect(pos, pos + ImVec2{w + 1, h + 1}, 0xff00ffff);

    auto clicked = std::optional<std::size_t>{};
    auto make_item = [&](std::size_t page_index) {
        if (display_button(page_index + 1, {x, pos.y}, {box_w, h}, std::to_string(page_index + 1),
                current_page == page_index, false, false))
            clicked = page_index;
        x += box_w;
    };

    auto make_text = [&](std::string_view s) {
        auto text_w = measure_text(s).x;
        window->DrawList->AddText({x + 0.5f * (box_w - text_w), pos.y + padding.y},
            ImGui::GetColorU32(ImGuiCol_Text), s.data(), s.data() + s.size());
        x += box_w;
    };

    if (display_button("-1", pos, {arrow_width, h}, "<", false, current_page <= 0, true) &&
        current_page > 0)
        clicked = current_page - 1;
    x += arrow_width;

    w = std::max(box_w, w - arrow_width * 2.0f);

    auto avail_boxes = static_cast<std::size_t>(w / box_w);
    if (avail_boxes >= total_pages) {
        // If everything fits, just display all pages
        box_w = w / total_pages;
        for (std::size_t i = 0; i < total_pages; ++i)
            make_item(i);
    }
    else if (avail_boxes < 5) {
        box_w = w;
        make_text(std::to_string(current_page + 1));
    }
    else {
        // Fit with ellipsis
        if ((avail_boxes & 0x01) == 0x00)
            --avail_boxes;

        box_w = w / avail_boxes;

        auto side_pages = avail_boxes > 4 ? (avail_boxes - 4) / 2 : 0;

        auto start_page = std::size_t(1);
        if (current_page > side_pages) {
            start_page = current_page - side_pages;
            if (start_page == 2)
                start_page = 1;
        }

        auto end_page = start_page + avail_boxes - 3;
        if (end_page + 1 >= total_pages) {
            end_page = total_pages - 2;
            start_page = end_page - avail_boxes + 3;
            if (start_page > 1)
                ++start_page;
        }
        else {
            if (start_page > 1)
                --end_page;
            if (end_page + 1 < total_pages)
                --end_page;
        }

        make_item(0);
        if (start_page > 1)
            make_text(ellipsis);
        for (auto i = start_page; i <= end_page; ++i)
            make_item(i);
        if (end_page < total_pages - 2)
            make_text(ellipsis);
        make_item(total_pages - 1);
    }

    if (display_button("+1", {x, pos.y}, {arrow_width, h}, ">", false,
            current_page + 1 >= total_pages, true) &&
        (current_page + 1 < total_pages)) {
        clicked = current_page + 1;
    }

    ImGui::PopID();

    return clicked;
}

} // namespace ImPlus::Pagination