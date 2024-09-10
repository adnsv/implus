#pragma once

#include <implus/id.hpp>

#include <cstddef>
#include <optional>

namespace ImPlus::Pagination {

enum class Placement {
    Near,
    Center,
    Far,
    Stretch,
};

struct Options {
    Placement HorzPlacement = Placement::Near;
    std::size_t LimitBoxCount = 11; // avoid showing more boxes than this, odd numbers look best
};


auto Display(ImID, ImVec2 const& size_arg, std::size_t index, std::size_t count,
    Options const&) -> std::optional<std::size_t>;

} // namespace ImPlus::Pagination