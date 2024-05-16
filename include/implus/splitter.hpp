#pragma once

#include <imgui.h>

#include "alignment.hpp"
#include "color.hpp"
#include "length.hpp"
#include "overridable.hpp"

#include <algorithm>
#include <functional>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace ImPlus {

class Splitter;

namespace detail {

using desired_sz_t = length_or_spring;

struct BandSize {
    desired_sz_t Desired = 10_em;
    length Minimum = 2_em;
};

struct BandFlags {
    ImGuiWindowFlags WindowFlags;
    ImGuiChildFlags ChildFlags;
    BandFlags(
        ImGuiWindowFlags wf = ImGuiWindowFlags_None, ImGuiChildFlags cf = ImGuiChildFlags_None)
        : WindowFlags{wf}
        , ChildFlags{cf}
    {
    }
    BandFlags(BandFlags const&) = default;
};

struct Band {
    BandSize Size = {};
    BandFlags Flags = {};
    ColorSpec BackgroundColor = {};

    Band() noexcept = default;

    Band(BandSize const& size, BandFlags flags = {})
        : Size{size}
        , Flags{flags}
    {
    }

    Band(BandSize const& size, BandFlags flags, ColorSpec&& bkgnd)
        : Size{size}
        , Flags{flags}
        , BackgroundColor{std::move(bkgnd)}
    {
    }

    Band(BandSize const& size, BandFlags flags, ColorSpec const& bkgnd)
        : Size{size}
        , Flags{flags}
        , BackgroundColor{bkgnd}
    {
    }

    auto GetSize() const -> pt_length { return size_; }

protected:
    pt_length size_ = 0.0f;
    pt_length minimum_size_ = 0.0f;

    friend class ImPlus::Splitter;
};

} // namespace detail

namespace Style::Splitter::Line {

inline auto Thickness = overridable<length>{screen_point};
inline auto HitExtend = overridable<length>{0.2_em};

namespace Color {
inline auto Regular = overridable<ImVec4>{ImPlus::Color::FromStyle<ImGuiCol_Separator>};
inline auto Hovered = overridable<ImVec4>{ImPlus::Color::FromStyle<ImGuiCol_SeparatorHovered>};
inline auto Active = overridable<ImVec4>{ImPlus::Color::FromStyle<ImGuiCol_SeparatorActive>};
} // namespace Color

} // namespace Style::Splitter::Line

class Splitter : public std::vector<detail::Band> {
public:
    using Band = detail::Band;
    Axis StackingDirection = Axis::Horz;
    void Display(std::initializer_list<std::function<void()>> display_procs);

protected:
    auto CalcSizes(pt_length target_sum) -> float;
};

} // namespace ImPlus