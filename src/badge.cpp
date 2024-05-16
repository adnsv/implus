#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <implus/badge.hpp>
#include <implus/blocks.hpp>

namespace ImPlus::Badge {

void Render(ImDrawList* dl, ImVec2 const& pos, std::string_view content, Options const& opts)
{
    auto blk = ImPlus::TextBlock{content, {0.5f, 0.5f}, {}, {}, opts.Font};

    auto const margin = ImPlus::to_pt<ImPlus::rounded>(Style::Badge::Margin());
    auto const sz = ImVec2{
        std::round(std::max(ImPlus::to_pt(Style::Badge::MinWidth()), blk.Size.x + margin.x * 2.0f)),
        std::round(blk.Size.y + margin.y * 2.0f)};
    auto const rounding = std::round(std::min(sz.x * 0.5f, sz.y * 0.5f));

    auto colors = opts.ColorSet.value_or(ImPlus::ColorSet{
        .Content = ImPlus::Color::FromStyle(ImGuiCol_Text),
        .Background = ImPlus::Color::FromStyle(ImGuiCol_TabActive),
    });

    auto align = Style::Badge::Alignment();
    auto offset = ImPlus::to_pt(Style::Badge::Offset());
    auto p_min = ImVec2{
        std::round(pos.x - sz.x * align.x + offset.x),
        std::round(pos.y - sz.y * align.y + offset.y),
    };

    auto p_max = p_min + sz;

    // auto fr_32 = ImGui::GetColorU32(ImPlus::Color::ModulateAlpha(colors.Content, 0.3f));
    auto const fr_32 = ImGui::GetColorU32({0, 0, 0, 0.4});
    auto const bg_32 = ImGui::GetColorU32(colors.Background);
    auto fr_adj = ImVec2{2, 2};

    dl->AddRectFilled(p_min - fr_adj, p_max + fr_adj, fr_32, rounding + 2.0f, 0);
    dl->AddRectFilled(p_min, p_max, bg_32, rounding);
    blk.Render(dl, p_min + margin, p_max - margin, colors.Content);
}

} // namespace ImPlus::Badge