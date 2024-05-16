#pragma once

#include <cmath>
#include <compare>
#include <functional>
#include <limits.h>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

#include <imgui.h>

namespace ImPlus {

enum length_units {
    pt_units,
    em_units,
};

using pt_length = float;
static constexpr auto screen_point = pt_length(1.0f);

struct em_length {
    explicit constexpr em_length(float v) noexcept
        : value_{v}
    {
    }
    constexpr em_length() noexcept = default;
    constexpr em_length(em_length const&) noexcept = default;

    constexpr auto numeric_value() -> float& { return value_; }
    constexpr auto numeric_value() const -> float const& { return value_; }

    friend constexpr auto operator<=>(em_length a, em_length b) = default;

    friend constexpr auto operator-(em_length v) { return em_length(-v.value_); }
    friend constexpr auto operator-(em_length a, em_length b)
    {
        return em_length(a.value_ - b.value_);
    };
    friend constexpr auto operator+(em_length a, em_length b)
    {
        return em_length(a.value_ + b.value_);
    }
    friend constexpr auto operator*(float f, em_length l) { return em_length(f * l.value_); }
    friend constexpr auto operator*(em_length l, float f) { return em_length(l.value_ * f); }
    friend constexpr auto operator/(em_length l, float f) { return em_length(l.value_ / f); }

private:
    float value_ = 0.0f;
};

namespace literals {
constexpr auto operator"" _pt(long double v) -> pt_length { return pt_length(v); }
constexpr auto operator"" _pt(unsigned long long v) -> pt_length { return pt_length(v); }
constexpr auto operator"" _em(long double v) -> em_length { return em_length(v); }
constexpr auto operator"" _em(unsigned long long v) -> em_length { return em_length(float(v)); }
} // namespace literals

using namespace literals;

static constexpr auto em = em_length(1.0f);

using length = std::variant<pt_length, em_length>;

auto GetFontHeight() -> pt_length;

constexpr auto to_pt(pt_length const v) -> pt_length { return v; }
inline auto to_pt(em_length const& v) -> pt_length
{
    return v.numeric_value() ? v.numeric_value() * GetFontHeight() : 0.0f;
}
inline auto to_pt(length const& v) -> pt_length
{
    if (std::holds_alternative<em_length>(v))
        return to_pt(std::get<em_length>(v));
    else
        return std::get<pt_length>(v);
}

inline auto to_pt(length const& x, length const& y) -> ImVec2 { return ImVec2{to_pt(x), to_pt(y)}; }

constexpr auto to_em(em_length const& v) -> em_length { return v; }
inline auto to_em(pt_length const& v) -> em_length { return em_length(v / GetFontHeight()); }
inline auto to_em(length const& v) -> em_length
{
    if (!std::holds_alternative<em_length>(v))
        return to_em(std::get<pt_length>(v));
    else
        return std::get<em_length>(v);
}

constexpr auto units_of(length const& v) -> length_units
{
    if (std::holds_alternative<em_length>(v))
        return em_units;
    else
        return pt_units;
}

constexpr auto numeric_value_of(length const& v) -> float
{
    if (std::holds_alternative<em_length>(v))
        return std::get<em_length>(v).numeric_value();
    else
        return std::get<pt_length>(v);
}

constexpr void assign_numeric_value(length& v, float nv)
{
    if (std::holds_alternative<em_length>(v))
        std::get<em_length>(v).numeric_value() = nv;
    else
        std::get<pt_length>(v) = nv;
}

template <typename T>
concept convertible_to_scalar_pt_length =
    std::is_same_v<T, pt_length> || std::is_same_v<T, em_length> || std::is_same_v<T, length>;

template <convertible_to_scalar_pt_length L>
auto to_pt(std::optional<L> const& v) -> std::optional<pt_length>
{
    if (!v)
        return {};
    return to_pt(*v);
}

enum numeric_round_style {
    not_rounded = std::round_indeterminate,
    rounded = std::round_to_nearest,
    rounded_up = std::round_toward_infinity,
    rounded_down = std::round_toward_neg_infinity,
};

template <numeric_round_style R, convertible_to_scalar_pt_length L>
auto to_pt(L const& v) -> pt_length
{
    auto f = to_pt(v);
    if constexpr (R == rounded_up)
        return std::ceil(f);
    else if constexpr (R == rounded_down)
        return std::floor(f);
    else if constexpr (R == rounded)
        return std::round(f);
    else
        return v;
}

template <numeric_round_style R, convertible_to_scalar_pt_length L>
auto to_pt(std::optional<L> const& v) -> std::optional<pt_length>
{
    if (!v)
        return {};
    return to_pt<R, L>(*v);
}

template <numeric_round_style R, convertible_to_scalar_pt_length X,
    convertible_to_scalar_pt_length Y>
auto to_pt(X const& x, Y const& y) -> ImVec2
{
    return {to_pt<R, X>(x), to_pt<R, Y>(y)};
}

inline void assign_from_pt(em_length& v, pt_length sp) { v.numeric_value() = sp / GetFontHeight(); }

inline void assign_from_pt(length& v, pt_length sp)
{
    if (std::holds_alternative<em_length>(v))
        assign_from_pt(std::get<em_length>(v), sp);
    else
        std::get<pt_length>(v) = sp;
}

struct length_vec {
    length x = 0_pt;
    length y = 0_pt;

    length_vec() noexcept = default;
    length_vec(length x, length y) noexcept
        : x{x}
        , y{y}
    {
    }
    length_vec(length_vec const&) noexcept = default;
    length_vec(ImVec2 const& v) noexcept
        : x{v.x}
        , y{v.y}
    {
    }
};

using pt_vec = ImVec2;

template <convertible_to_scalar_pt_length X, convertible_to_scalar_pt_length Y>
auto to_pt(X const& x, Y const& y) -> pt_vec
{
    return ImVec2{to_pt(x), to_pt(y)};
}

inline auto to_pt(length_vec const& v) -> pt_vec { return pt_vec{to_pt(v.x), to_pt(v.y)}; }

template <numeric_round_style R> auto to_pt(length_vec const& v) -> pt_vec
{
    return ImVec2{to_pt<R>(v.x), to_pt<R>(v.y)};
}

// deprecated: refactor the rest of the code to use 'screen_point' and 'em'
[[deprecated]] static constexpr auto Pixel = screen_point;
[[deprecated]] static constexpr auto FontH = em;
using Length = length;

struct spring {

    constexpr spring(spring const&) noexcept = default;
    constexpr explicit spring(float v = 0.0f) noexcept
        : value_{v}
    {
    }

    auto numeric_value() const -> float const& { return value_; }
    auto numeric_value() -> float& { return value_; }

protected:
    float value_;
};

using length_or_spring = std::variant<length, spring>;

inline auto spring_numeric_value(length_or_spring const& v) -> std::optional<float>
{
    if (std::holds_alternative<spring>(v))
        return std::get<spring>(v).numeric_value();
    else
        return {};
}

inline auto to_pt(length_or_spring const& v) -> std::optional<pt_length>
{
    if (std::holds_alternative<length>(v))
        return to_pt(std::get<length>(v));
    else
        return {};
}

auto CalcFrameRounding() -> pt_length;
auto CalcFrameRounding(std::optional<length> const& v) -> pt_length;
auto CalcFramePadding() -> pt_vec;
auto CalcFramePadding(std::optional<ImPlus::length_vec> const& v) -> pt_vec;

} // namespace ImPlus