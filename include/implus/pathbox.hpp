#pragma once

#include <implus/id.hpp>
#include <functional>
#include <imgui.h>
#include <optional>
#include <span>
#include <string>

namespace ImPlus::Pathbox {

struct Options {
    bool RemoveSpacingAfter = false;
};

enum ItemFlags {
    ItemFlags_Regular = 0,
    ItemFlags_DisableClick = 1 << 0,
};

struct Item {
    std::string text = "";
    ItemFlags flags = ItemFlags_Regular;
    Item(std::string const& t, ItemFlags flags_ = ItemFlags_Regular)
        : text{t}
        , flags{flags_}
    {
    }
    Item(std::string&& t, ItemFlags flags_ = ItemFlags_Regular)
        : text{t}
        , flags{flags_}
    {
    }

protected:
    friend auto Display(ImID id, ImVec2 const&, std::span<Item>, Options const&)
        -> std::optional<std::size_t>;
    float disp_w = 0;
};

auto DefaultHeightPixels() -> float;

auto Display(ImID id, ImVec2 const& size_arg, std::span<Item>, Options const& = {})
    -> std::optional<std::size_t>;

} // namespace ImPlus::Pathbox