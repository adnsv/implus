#pragma once

#include <imgui.h>

#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <type_traits>

#include "implus/blocks.hpp"
#include "implus/content.hpp"
#include "implus/interact.hpp"
#include "implus/selbox.hpp"

#ifndef IMPLUS_APPLE_CLANG_RANGES
#include <ranges>
#else
#include <__ranges/all.h>
#endif

namespace ImPlus::Listbox {

struct InteractResult {
    std::optional<std::size_t> HoveredIndex;
    std::optional<std::size_t> FocusedIndex;
    std::optional<std::size_t> PressedIndex;
};

namespace internal {
void ItemSize(const ImVec2& size, float text_baseline_y);
auto LastPushedID() -> ImGuiID;
} // namespace internal

struct BoxContent {
    ImVec2 Size = {32.0f, 32.0f};
    Content::DrawCallback DrawProc = {};
    std::function<void()> OnAfterItem = {};
};

namespace detail {

// detail::input is a generic list of items to be displayed in a list.
template <typename R>
concept input = std::ranges::input_range<R>;

// detail::string_like_input is a list of items that can be displayed as strings.
template <typename R>
concept string_like_input =
    (input<R> && std::is_convertible_v<std::ranges::range_value_t<R>, std::string>);

// ---------- selectors ---------------------------------------------------
// Concepts for the is_selected(item& [, index])->bool callback.

template <typename R, typename Callable>
concept selector_unindexed =
    std::is_invocable_r_v<bool, Callable, std::ranges::range_reference_t<R>>;

template <typename R, typename Callable>
concept selector_indexed =
    std::is_invocable_r_v<bool, Callable, std::ranges::range_reference_t<R>, std::size_t>;

template <typename R, typename Callable>
concept selector = selector_unindexed<R, Callable> || selector_indexed<R, Callable>;

// ----------- boxmakers --------------------------------------------------
// Concepts for on_content(item& [, index], BoxContent&) callback.

template <typename R, typename Callable>
concept boxmaker_unindexed =
    std::is_invocable_v<Callable, std::ranges::range_reference_t<R>, BoxContent&>;

template <typename R, typename Callable>
concept boxmaker_indexed =
    std::is_invocable_v<Callable, std::ranges::range_reference_t<R>, std::size_t, BoxContent&>;

template <typename R, typename Callable>
concept boxmaker = boxmaker_unindexed<R, Callable> || boxmaker_indexed<R, Callable>;

// ----------- stringers --------------------------------------------------
// Concepts for on_content(item& [, index])->std::string callback.

template <typename R, typename Callable>
concept stringer_unindexed =
    std::is_invocable_v<Callable, std::ranges::range_reference_t<R>> &&
    std::is_convertible_v<std::invoke_result_t<Callable, std::ranges::range_reference_t<R>>,
        std::string>;

template <typename R, typename Callable>
concept stringer_indexed =
    std::is_invocable_v<Callable, std::ranges::range_reference_t<R>, std::size_t> &&
    std::is_convertible_v<
        std::invoke_result_t<Callable, std::ranges::range_reference_t<R>, std::size_t>,
        std::string>;

template <typename R, typename Callable>
concept stringer = stringer_unindexed<R, Callable> || stringer_indexed<R, Callable>;

} // namespace detail

// Boxes shows items where Selector is used to highlight selected items and Content can be used for
// custom rendering.
template <typename R, typename Selector, typename Content>
requires(detail::input<R> && detail::selector<R, Selector> && detail::boxmaker<R, Content>)
auto Boxes(ImID id, R&& items, Selector&& is_selected, Content&& on_item) -> InteractResult
{
    using iterator_t = typename std::ranges::iterator_t<R>;

    auto item_flags = ImGuiSelectableFlags_(1 << 24); // ImGuiSelectableFlags_SpanAvailWidth;

    auto last_it = items.begin() + items.size();
    auto ret = InteractResult{};

    internal::ItemSize(ImVec2(0.0f, 0.0f), 0.0f);

    auto const& item_spacing = ImGui::GetStyle().ItemSpacing;
    auto curr_pos = ImGui::GetCursorPos();

    auto gen_id = ImIDMaker(id);
    auto item_index = 0;
    for (auto&& item : items) {

        ImGui::SetCursorPos(curr_pos);

        auto box = BoxContent{};
        if constexpr (detail::boxmaker_indexed<R, Content>)
            on_item(item, item_index, box);
        else
            on_item(item, box);

        auto show_selected = false;
        if constexpr (detail::selector_indexed<R, Selector>)
            show_selected = is_selected(item, item_index);
        else
            show_selected = is_selected(item);

        auto state = ImPlus::SelectableBox(
            gen_id(), nullptr, show_selected, item_flags, box.Size, {}, box.DrawProc);

        curr_pos.y = std::round(curr_pos.y + box.Size.y + item_spacing.y);

        if (state.Pressed)
            ret.PressedIndex = item_index;
        if (state.Hovered)
            ret.HoveredIndex = item_index;
        if (ImGui::IsItemFocused())
            ret.FocusedIndex = item_index;
        ++item_index;
    }

    return ret;
}

inline auto StringItems(ImID id, std::size_t count, std::size_t sel_index,
    std::function<std::string(std::size_t idx)> on_item) -> InteractResult
{
    auto item_flags = ImGuiSelectableFlags_(1 << 24); // ImGuiSelectableFlags_SpanAvailWidth;

    auto ret = InteractResult{};

    internal::ItemSize(ImVec2(0.0f, 0.0f), 0.0f);

    auto const& item_spacing = ImGui::GetStyle().ItemSpacing;
    auto curr_pos = ImGui::GetCursorPos();

    auto gen_id = ImIDMaker(id);
    auto item_index = 0;
    for (std::size_t item_idx = 0; item_idx < count; ++item_idx) {

        ImGui::SetCursorPos(curr_pos);

        auto const s = on_item(item_idx);
        auto textblock = TextBlock{s, {0, 0.5f}};
        auto const sz = textblock.Size;

        auto state = ImPlus::SelectableBox(gen_id(), nullptr, item_idx == sel_index, item_flags, sz,
            {}, MakeContentDrawCallback(std::move(textblock)));

        curr_pos.y = std::round(curr_pos.y + sz.y + item_spacing.y);

        if (state.Pressed)
            ret.PressedIndex = item_index;
        if (state.Hovered)
            ret.HoveredIndex = item_index;
        if (ImGui::IsItemFocused())
            ret.FocusedIndex = item_index;
        ++item_index;
    }

    return ret;
}

// -- Strings shows a list of string-like items -----------------------------

// string-like items, using Selector and Stringer callbacks.
template <typename R, typename Selector, typename Stringer>
// requires(detail::input<R> && detail::selector<R, Selector> && detail::stringer<R, Stringer>)
auto Strings(ImID id, R&& items, Selector&& is_selected, Stringer to_string) -> InteractResult
{
    std::string v;
    return Boxes(id, std::forward<R>(items), std::forward<Selector>(is_selected),
        [&v, str = std::forward<Stringer>(to_string)](
            std::ranges::range_reference_t<R> it, std::size_t item_index, BoxContent& box) {
            if constexpr (detail::stringer_indexed<R, Stringer>)
                v = str(it, item_index);
            else
                v = str(it);

            auto textblock = TextBlock{v, {0, 0.5f}};
            box.Size = textblock.Size;
            box.DrawProc = MakeContentDrawCallback(std::move(textblock));
        });

    return {};
}

// string-like items, using sel_index and Stringer callback.
template <typename R, typename Stringer>
requires(detail::input<R> && detail::stringer<R, Stringer>)
auto Strings(ImID id, R&& items, std::size_t& sel_index, Stringer&& to_string) -> bool
{
    auto r = Strings(
        id, std::forward<R>(items),
        [sel_index](auto&, std::size_t item_index) { return sel_index == item_index; },
        std::forward<Stringer>(to_string));

    if (r.PressedIndex) {
        sel_index = *r.PressedIndex;
        return true;
    }
    return false;
}

// string-like items, using sel_index and convertible-to-string items.
template <typename R>
requires detail::string_like_input<R>
auto Strings(ImID id, R&& items, std::size_t& sel_index) -> bool
{
    return Strings(
        id, std::forward<R>(items), sel_index, [](auto&& item) { return std::string{item}; });
}

template <typename R, typename Stringer>
requires(detail::input<R> && detail::stringer<R, Stringer>)
auto Enums(ImID id, R&& items, std::remove_cv_t<std::ranges::range_value_t<R>>& v,
    Stringer&& to_string) -> bool
{
    auto r = Strings(
        id, items, [&](auto&& it) { return it == v; }, std::forward<Stringer>(to_string));

    if (r.PressedIndex) {
        v = items[*r.PressedIndex];
        return true;
    }
    return false;
}

template <typename T, typename Stringer>
auto Enums(ImID id, std::initializer_list<T> items, T& v, Stringer&& to_string) -> bool
{
    return Enums(id, std::ranges::subrange(items.begin(), items.end()), v,
        std::forward<Stringer>(to_string));
}

#if 0
template <typename T>
requires itemizable<T>
void MultiSelectBehavior(T& items, InteractResult const& disp_result)
{
    if (!disp_result.PressedIndex || disp_result.PressedIndex >= items.size())
        return;

    auto const& key = items[*disp_result.PressedIndex].key;

    if (ImGui::GetIO().KeyCtrl)
        items.select(key, ImPlus::Itemizer::SelModifier::toggle);
    else if (ImGui::GetIO().KeyShift)
        items.select(key, ImPlus::Itemizer::SelModifier::range);
    else
        items.select(key);
}

template <typename T>
requires itemizable<T>
void SingleSelectBehavior(T& items, InteractResult const& disp_result)
{
    if (!disp_result.PressedIndex || disp_result.PressedIndex >= items.size())
        return;

    auto const& key = items[*disp_result.PressedIndex].key;

    items.select(key);
}
#endif

} // namespace ImPlus::Listbox
