#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/blocks.hpp"
#include "implus/checkbox.hpp"

namespace ImPlus {

auto Checkbox(ImID id, CD_view const& content, bool& checked,
    Text::CDOverflowPolicy const& overflow_policty, std::optional<length> const& overflow_width,
    CheckboxFlags flags) -> bool
{
    auto* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    auto const& g = *GImGui;
    auto const& style = g.Style;

    auto const show_texts = !content.Empty();
    auto const small_check = (flags & CheckboxFlags::SmallCheck) == CheckboxFlags::SmallCheck;

    auto const square_sz = small_check ? g.FontSize : g.FontSize + style.FramePadding.y * 2.0f;
    auto const check_gap = show_texts ? style.ItemInnerSpacing.x : 0.0f;

    auto ow = ImPlus::to_pt(overflow_width);
    if (ow)
        *ow = std::max(1.0f, *ow - square_sz + check_gap);

    auto const blk =
        ImPlus::CDBlock{content, ImVec2{0, 0}, CDOptions{}, overflow_policty, ow};
    auto const baseline_padding = std::max(0.0f, (square_sz - g.FontSize) * 0.5f);

    auto const content_size = ImVec2{
        blk.Size.x + square_sz + check_gap, std::max(square_sz, blk.Size.y + baseline_padding)};

    auto pos = window->DC.CursorPos;
    pos.y += window->DC.CurrLineTextBaseOffset - baseline_padding;

    auto const total_bb = ImRect{pos, pos + content_size};
    ImGui::ItemSize(content_size, baseline_padding);

    auto pressed = false;

    if (!ImGui::ItemAdd(total_bb, id)) {
        IMGUI_TEST_ENGINE_ITEM_INFO(id, std::string(label).c_str(),
            g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable |
                (checked ? ImGuiItemStatusFlags_Checked : 0));
        return false;
    }

    bool hovered, held;
    pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed) {
        checked = !checked;
        ImGui::MarkItemEdited(id);
    }

    const ImRect check_bb(pos, pos + ImVec2{square_sz, square_sz});
    ImGui::RenderNavHighlight(total_bb, id);

    ImGui::PushClipRect(total_bb.Min, total_bb.Max, true);

    ImGui::RenderFrame(check_bb.Min, check_bb.Max,
        ImGui::GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive
                           : hovered         ? ImGuiCol_FrameBgHovered
                                             : ImGuiCol_FrameBg),
        true, style.FrameRounding);
    ImU32 check_col = ImGui::GetColorU32(ImGuiCol_CheckMark);
    bool mixed_value = (g.LastItemData.ItemFlags & ImGuiItemFlags_MixedValue) != 0;
    if (mixed_value) {
        // Undocumented tristate/mixed/indeterminate checkbox (#2644)
        // This may seem awkwardly designed because the aim is to make ImGuiItemFlags_MixedValue
        // supported by all widgets (not just checkbox)
        ImVec2 pad(std::max(1.0f, std::floor(square_sz / 3.6f)),
            std::max(1.0f, std::floor(square_sz / 3.6f)));
        window->DrawList->AddRectFilled(
            check_bb.Min + pad, check_bb.Max - pad, check_col, style.FrameRounding);
    }
    else if (checked) {
        const float pad = std::max(1.0f, std::floor(square_sz / 6.0f));
        ImGui::RenderCheckMark(
            window->DrawList, check_bb.Min + ImVec2(pad, pad), check_col, square_sz - pad * 2.0f);
    }

    if (show_texts) {
        auto clr = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        auto tl = ImVec2{total_bb.Min.x + square_sz + check_gap, total_bb.Min.y + baseline_padding};
        blk.Render(window->DrawList, tl, total_bb.Max, clr);
        if (g.LogEnabled)
            ImGui::LogRenderedText(&tl, mixed_value ? "[~]" : checked ? "[x]" : "[ ]");
    }

    ImGui::PopClipRect();

    IMGUI_TEST_ENGINE_ITEM_INFO(id, std::string(label).c_str(),
        g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable |
            (checked ? ImGuiItemStatusFlags_Checked : 0));

    return pressed;
}

auto Checkbox(ImID id, CD_view const& content, bool& checked, CheckboxFlags flags) -> bool
{
    return Checkbox(id, content, checked, Style::Checkbox::OverflowPolicy(),
        Style::Checkbox::OverflowWidth(), flags);
}

} // namespace ImPlus