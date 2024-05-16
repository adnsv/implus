#pragma once

#include <imgui.h>

#include "implus/blocks.hpp"
#include "implus/color.hpp"
#include "implus/id.hpp"
#include "implus/length.hpp"
#include "implus/overridable.hpp"
#include "implus/sizing.hpp"
#include "implus/text.hpp"

#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <span>

#ifndef IMPLUS_APPLE_CLANG_RANGES
#include <ranges>
#else
#include <__ranges/all.h>
#endif

namespace ImPlus::Style::Buttonbar {

// placement of button bar items
inline auto ItemStacking = overridable<Axis>(ImPlus::Axis::Horz);

// layout of content within each item
inline auto ItemLayout = overridable<Content::Layout>(Content::Layout::HorzCenter);

// ForceItemWidth allows to enforce all items to have a certain width.
inline auto ForceItemWidth = overridable<std::optional<length>>{};

// ForceItemHeight allows to enforce all items to have a certain height.
inline auto ForceItemHeight = overridable<std::optional<length>>{};

inline auto OverflowPolicy =
    overridable<Text::CDOverflowPolicy>(Text::CDOverflowPolicy{Text::OverflowEllipsify});

inline auto HighlightDefault = overridable<bool>{false};

// properties for horizontal button bars (ItemStacking = ImPlus::Axis::Horz).
namespace Horizontal {

// StretchItemWidth stretches small buttons up to this width, if space permits.
// If an item measures wider than StretchItemWidth, it remains unchanged.
// This property is ignored when ForceItemWidth is specified.
inline auto StretchItemWidth = overridable<std::optional<length>>(5_em);

// OverflowWidth can be used to limit the width of items to a certain value.
// This property is ignored when ForceItemWidth is specified.
inline auto OverflowWidth = overridable<std::optional<length>>(20_em);

} // namespace Horizontal

// properties for vertical button bars (ItemStacking = ImPlus::Axis::Vert).
namespace Vertical {

// StretchItemHeight stretches small buttons up to this height, if space permits.
// If an item measures taller than StretchItemHeight, it remains unchanged.
// This property is ignored when ForceItemHeight is specified.
inline auto StretchItemHeight = overridable<std::optional<length>>();
} // namespace Vertical

// see also (in blocks.hpp):
// - Style::ICD::DescrOffset
// - Style::ICD::DescrSpacing
// - Style::ICD::DescrOpacity

} // namespace ImPlus::Style::Buttonbar

namespace ImPlus::Buttonbar {

struct Button {
    ImPlus::ICD_view Content;
    bool Enabled = true;
    bool Default = false;
    bool Cancel = false; // may be used in modal boxes to indicate exit from
                         // modality (see CancelSentinel below)

    std::optional<length> SpacingBefore;
    std::optional<length> SpacingAfter;
    ImPlus::InteractColorSetCallback Colors = nullptr;
};

using SourceCallback = std::function<auto(Button&)->bool>;

// Display handles displaying of the bar buttons
//
// returns:
//   - nullopt if nothing was clicked
//   - clicked item index, if the button was clicked
//   - CancelSentinel, if the clicked button had Cancel=true
//
auto Display(ImID id, SourceCallback&& buttons, ImVec2 const& align_all = {})
    -> std::optional<std::size_t>;

template <typename R>
requires(std::ranges::input_range<R> && std::is_same_v<std::ranges::range_value_t<R>, Button>)
auto Display(ImID id, R&& items, ImVec2 const& align_all = {}) -> std::optional<std::size_t>
{
    if (items.empty())
        return {};

    auto it = items.begin();

    return Display(
        id,
        [&](Button& btn) {
            if (it == items.end())
                return false;
            btn = *it++;
            return true;
        },
        align_all);
}

inline auto Display(ImID id, std::initializer_list<Button> items, ImVec2 const& align_all = {})
    -> std::optional<std::size_t>
{
    return Display(id, std::ranges::subrange(items.begin(), items.end()), align_all);
}

// using strings as inputs

enum class Flags {
    LastIsCancel = 1 << 0,
    FirstIsDefault = 1 << 1,
    DisableAllButLast = 1 << 2,
    DisableLast = 1 << 3,

    DisableAll = (1 << 2) | (1 << 3),
};

constexpr auto operator|(Flags a, Flags b) -> Flags { return Flags(unsigned(a) | unsigned(b)); };
constexpr auto operator&(Flags a, Flags b) -> Flags { return Flags(unsigned(a) & unsigned(b)); };
constexpr auto operator&&(Flags a, Flags b) -> bool { return bool(a) && bool(b); };
constexpr auto operator&&(Flags a, bool b) -> bool { return bool(a) && b; };
constexpr auto operator&&(bool a, Flags b) -> bool { return a && bool(b); };

template <typename R>
requires(std::ranges::input_range<R> &&
         std::is_convertible_v<std::ranges::range_value_t<R>, std::string_view>)
auto Display(ImID id, R&& items, ImVec2 const& align_all, Flags flags = {})
    -> std::optional<std::size_t>
{
    auto it = items.begin();
    return Display(
        id,
        [&](Button& btn) {
            if (it == items.end())
                return false;

            btn.Content.Caption = *it;
            auto is_first = it == items.begin();
            ++it;
            auto is_last = it == items.end();

            btn.Default = is_first && (flags & Flags::FirstIsDefault);
            btn.Cancel = is_last && (flags & Flags::LastIsCancel);
            if (is_last && (flags & Flags::DisableLast))
                btn.Enabled = false;
            if (!is_last && (flags & Flags::DisableAllButLast))
                btn.Enabled = false;

            return true;
        },
        align_all);
}

template <typename T>
requires std::is_convertible_v<T, std::string_view>
auto Display(ImID id, std::initializer_list<T> items, ImVec2 const& align_all, Flags flags = {})
    -> std::optional<std::size_t>
{
    return Display(id, std::ranges::subrange(items.begin(), items.end()), align_all, flags);
}

template <typename T>
requires std::is_convertible_v<T, std::string_view>
auto Display(ImID id, std::initializer_list<T> items, Flags flags = {})
    -> std::optional<std::size_t>
{
    return Display(items, {0, 0}, flags);
}

constexpr auto CancelSentinel = std::size_t(-1);

} // namespace ImPlus::Buttonbar