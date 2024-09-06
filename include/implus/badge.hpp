#pragma once

#include <imgui.h>
#include <implus/color.hpp>
#include <implus/font.hpp>
#include <implus/icon.hpp>
#include <implus/length.hpp>
#include <implus/overridable.hpp>
#include <variant>

namespace ImPlus::Style::Badge {
inline auto MinWidth = overridable<length>{1_em};
inline auto Alignment = overridable<ImVec2>({1.0f, 1.0f});
inline auto Margin = overridable<length_vec>({0.2_em, -0.05_em});
inline auto Offset = overridable<length_vec>({0.1_em, 0.05_em});
} // namespace ImPlus::Style::Badge

namespace ImPlus::Badge {

struct Options {
    ImPlus::Font::Resource Font = {};
    std::optional<ImPlus::ColorSet> ColorSet = {};

    std::optional<ImVec2> Alignment = {}; // overrides Style::Badge::Alignment
    std::optional<ImVec2> Offset = {};    // overrides Style::Badge::Offset
};

auto Measure(std::string_view, Options const&) -> ImVec2;

void Render(ImDrawList* dl, ImVec2 const& pos, std::string_view, Options const&);

} // namespace ImPlus::Badge