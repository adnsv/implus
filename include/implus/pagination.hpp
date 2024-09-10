#pragma once

#include <implus/id.hpp>

#include <cstddef>
#include <optional>

namespace ImPlus::Pagination {

struct Options {};

auto Display(ImID, ImVec2 const& size_arg, std::size_t index, std::size_t count,
    Options const&) -> std::optional<std::size_t>;

} // namespace ImPlus::Pagination