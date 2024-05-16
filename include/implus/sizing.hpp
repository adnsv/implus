#pragma once

#include "implus/length.hpp"
#include <imgui.h>
#include <optional>
#include <variant>

namespace ImPlus::Sizing {

struct Advanced {
    std::optional<length> Desired;
    std::optional<length> Stretch; // todo: consider handling infinity here
};

using AxisArg = std::variant<std::monostate, length, Advanced>;

// extended version of size_arg
struct XYArg {
    AxisArg Horz;
    AxisArg Vert;
};

auto CalcOverflowWidth(AxisArg const& horz_arg, std::optional<length> default_overflow_width,
    pt_length const& horz_padding, pt_length const& avail_width) -> std::optional<pt_length>;

auto GetDesired(AxisArg const&) -> std::optional<pt_length>;

auto CalcActual(AxisArg const&, pt_length const& measured, pt_length const& available) -> pt_length;
auto CalcActual(XYArg const&, ImVec2 const& measured, ImVec2 const& available) -> ImVec2;

} // namespace ImPlus::Sizing