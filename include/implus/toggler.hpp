#pragma once

#include "implus/id.hpp"
#include <cstddef>
#include <imgui.h>
#include <optional>
#include <string_view>
#include <type_traits>

namespace ImPlus {

struct TogglerOptions {
    std::string_view InactiveCaption;
    std::string_view ActiveCaption;
};

auto Toggler(ImID id, bool& active, TogglerOptions const& opts = {}) -> bool;
auto MultiToggler(ImID id, std::initializer_list<std::string_view> items, std::size_t& sel_index)
    -> bool;

template <typename T>
requires(std::is_convertible_v<T, std::size_t> || std::is_enum_v<T>)
auto MultiToggler(ImID id, std::initializer_list<std::string_view> items, T& sel) -> bool
{
    auto sel_index = std::size_t(sel);
    auto r = MultiToggler(id, items, sel_index);
    if (r)
        sel = T(sel_index);
    return r;
}

namespace Measure {
auto Toggler(TogglerOptions const& opts) -> ImVec2;
auto MultiToggler(std::initializer_list<std::string_view> items) -> ImVec2;
} // namespace Measure

} // namespace ImPlus