#pragma once

#include "implus/id.hpp"
#include "implus/length.hpp"
#include "implus/listbox.hpp"
#include "implus/overridable.hpp"
#include <imgui.h>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>

namespace ImPlus::Style::Combo {

inline auto LimitHeight = overridable<length>(20_em);
inline auto LimitVisibleItems = overridable<std::size_t>(32);

} // namespace ImPlus::Style::Combo

namespace ImPlus::Combo {

namespace internal {
void SetNextWindowLimits(float item_height, float item_spacing);
} // namespace internal

inline auto StringItems(ImID id, std::size_t count, std::size_t& sel_index,
    std::function<std::string(std::size_t idx)> on_item) -> bool
{
    auto const item_height = ImGui::GetFontSize();
    auto const& item_spacing = ImGui::GetStyle().ItemSpacing;
    internal::SetNextWindowLimits(item_height, item_spacing.y);

    // todo: consider using ImGui::BeginComboPreview() once that API is stable
    auto preview = on_item(sel_index);

    auto pressed = false;
    ImGui::PushID(id);
    if (ImGui::BeginCombo("", preview.c_str(), ImGuiComboFlags_None)) {
        auto r = Listbox::StringItems("", count, sel_index, on_item);
        if (r.PressedIndex) {
            sel_index = *r.PressedIndex;
            pressed = true;
        }

        ImGui::EndCombo();
    }
    ImGui::PopID();

    return pressed;
}

template <typename R, typename Stringer>
requires(std::ranges::random_access_range<R> && Listbox::detail::stringer<R, Stringer>)
auto Strings(ImID id, R&& items, std::size_t& sel_index, Stringer&& to_string) -> bool
{
    auto const item_height = ImGui::GetFontSize();
    auto const& item_spacing = ImGui::GetStyle().ItemSpacing;
    internal::SetNextWindowLimits(item_height, item_spacing.y);

    // todo: consider using ImGui::BeginComboPreview() once that API is stable
    auto preview = std::string{};
    if (sel_index < items.size()) {
        if constexpr (Listbox::detail::stringer_indexed<R, Stringer>)
            preview = to_string(items[sel_index], sel_index);
        else // assuming stringer_for_item_reference
            preview = to_string(items[sel_index]);
    }

    auto pressed = false;
    ImGui::PushID(id);
    if (ImGui::BeginCombo("", preview.c_str(), ImGuiComboFlags_None)) {
        pressed = Listbox::Strings(
            "", std::forward<R>(items), sel_index, std::forward<Stringer>(to_string));
        ImGui::EndCombo();
    }
    ImGui::PopID();

    return pressed;
}

template <typename R>
requires(std::ranges::random_access_range<R> &&
         std::is_convertible_v<std::ranges::range_value_t<R>, std::string>)
auto Strings(ImID id, R&& items, std::size_t& sel_index) -> bool
{
    return Strings(id, std::forward<R>(items), sel_index,
        [](std::ranges::range_value_t<R> const& s) { return s; });
}

template <typename T>
requires std::is_convertible_v<T, std::string>
auto Strings(ImID id, std::initializer_list<T> items, std::size_t& sel_index) -> bool
{
    return Strings(id, std::ranges::subrange(items.begin(), items.end()), sel_index);
}

template <typename R, typename Stringer>
requires(Listbox::detail::input<R> && Listbox::detail::stringer_unindexed<R, Stringer>)
auto Enums(ImID id, R&& items, std::remove_cv_t<std::ranges::range_value_t<R>>& v,
    Stringer&& to_string) -> bool
{
    auto const item_height = ImGui::GetFontSize();
    auto const& item_spacing = ImGui::GetStyle().ItemSpacing;
    internal::SetNextWindowLimits(item_height, item_spacing.y);

    auto preview = std::string{};
    preview = to_string(v);

    auto pressed = false;
    ImGui::PushID(id);
    if (ImGui::BeginCombo("", preview.c_str(), ImGuiComboFlags_None)) {
        pressed = Listbox::Enums("", std::forward<R>(items), v, std::forward<Stringer>(to_string));
        ImGui::EndCombo();
    }
    ImGui::PopID();

    return pressed;
}

template <typename T, typename Stringer>
auto Enums(ImID id, std::initializer_list<T> items, T& v, Stringer&& to_string) -> bool
{
    return Enums(id, std::ranges::subrange(items.begin(), items.end()), v,
        std::forward<Stringer>(to_string));
}

} // namespace ImPlus::Combo