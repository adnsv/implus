#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/sizing.hpp"

namespace ImPlus::Sizing {

auto CalcOverflowWidth(AxisArg const& horz_arg, std::optional<length> default_overflow_width,
    pt_length const& horz_padding, pt_length const& avail_width) -> std::optional<pt_length>
{
    if (std::holds_alternative<std::monostate>(horz_arg))
        return to_pt(default_overflow_width);
    if (auto forced = std::get_if<length>(&horz_arg))
        return std::max(1.0f, to_pt(*forced) - horz_padding);
    if (auto adv = std::get_if<Advanced>(&horz_arg)) {
        if (!adv->Desired.has_value())
            return to_pt(default_overflow_width);
        else
            return std::max(1.0f, std::min(to_pt(*adv->Desired), avail_width));
    }
    return to_pt(default_overflow_width);
}

auto GetDesired(AxisArg const& arg) -> std::optional<pt_length>
{
    if (auto adv = std::get_if<Advanced>(&arg); adv && adv->Desired)
        return to_pt(*adv->Desired);
    else
        return {};
}

auto CalcActual(AxisArg const& arg, pt_length const& measured, pt_length const& available)
    -> pt_length
{
    auto w = measured;
    if (auto forced = std::get_if<length>(&arg))
        w = to_pt(*forced);
    if (auto adv = std::get_if<Advanced>(&arg)) {
        if (adv->Desired.has_value())
            w = std::max(w, to_pt(*adv->Desired));
        if (adv->Stretch.has_value())
            w = std::max(w, to_pt(*adv->Stretch));
        w = std::max(measured, std::min(w, available));
    }
    return std::ceil(w);
}

auto CalcActual(XYArg const& arg, ImVec2 const& measured, ImVec2 const& available) -> ImVec2
{
    return ImVec2{
        CalcActual(arg.Horz, measured.x, available.x),
        CalcActual(arg.Vert, measured.y, available.y),
    };
}

} // namespace ImPlus::Sizing