#include "implus/length.hpp"

#include <imgui.h>
#include <imgui_internal.h>

namespace ImPlus {

auto GetFontHeight() -> pt_length
{
    IM_ASSERT(GImGui && GImGui->FontSize > 0.0f);
    return pt_length(GImGui->FontSize);
}

auto CalcFrameRounding() -> pt_length {
    return std::round(GImGui->Style.FrameRounding);
}

auto CalcFrameRounding(std::optional<ImPlus::length> const& v) -> pt_length {
    return std::round(v ? ImPlus::to_pt(*v) : GImGui->Style.FrameRounding);
}

auto CalcFramePadding() -> pt_vec {
    auto const& from_sty = GImGui->Style.FramePadding;
    return pt_vec{std::round(from_sty.x), std::round(from_sty.y)};
}

auto CalcFramePadding(std::optional<ImPlus::length_vec> const& v) -> pt_vec
{
    if (v)
        return ImPlus::to_pt<ImPlus::rounded>(*v);
    auto const& from_sty = GImGui->Style.FramePadding;
    return pt_vec{std::round(from_sty.x), std::round(from_sty.y)};
}


} // namespace ImPlus