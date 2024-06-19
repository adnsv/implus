#pragma once

#include <imgui.h>

#include "implus/overridable.hpp"
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <system_error>
#include <type_traits>

namespace ImPlus {
// std::optional type helpers

namespace detail {
template <typename T> struct maybe_optional {
    using value_type = T;
};
template <typename T> struct maybe_optional<std::optional<T>> {
    using value_type = T;
};
} // namespace detail


template <typename T> using maybe_optional_value_type = typename detail::maybe_optional<T>::value_type;

template <typename T> constexpr auto is_optional_v = false;
template <typename T> constexpr auto is_optional_v<std::optional<T>> = true;

template <typename T>
concept optional = is_optional_v<T>;

template <typename T>
concept arithmetic = std::is_arithmetic_v<T>;

template <typename T>
concept maybe_optional_integral = std::is_integral_v<maybe_optional_value_type<T>>;

template <typename T>
concept maybe_optional_floating_point = std::is_floating_point_v<maybe_optional_value_type<T>>;

template <typename T>
concept maybe_optional_arithmetic = std::is_arithmetic_v<maybe_optional_value_type<T>>;

} // namespace ImPlus

namespace ImPlus::Arithmetics {
auto UserDecimalSeparator() -> char;
}

namespace ImPlus::Style::Numeric {
inline auto DecimalSeparator = overridable<char>(Arithmetics::UserDecimalSeparator);
inline auto NullOptHint = overridable<std::string>("");
} // namespace ImPlus::Style::Numeric

namespace ImPlus::Arithmetics {

template <arithmetic T> constexpr auto DataType() -> ImGuiDataType_
{
    using t = std::remove_cv_t<T>;
    if constexpr (std::is_same_v<t, uint8_t>)
        return ImGuiDataType_U8;
    if constexpr (std::is_same_v<t, uint16_t>)
        return ImGuiDataType_U16;
    if constexpr (std::is_same_v<t, uint32_t>)
        return ImGuiDataType_U32;
    if constexpr (std::is_same_v<t, uint64_t>)
        return ImGuiDataType_U64;
    if constexpr (std::is_same_v<t, int8_t>)
        return ImGuiDataType_S8;
    if constexpr (std::is_same_v<t, int16_t>)
        return ImGuiDataType_S16;
    if constexpr (std::is_same_v<t, int32_t>)
        return ImGuiDataType_S32;
    if constexpr (std::is_same_v<t, int64_t>)
        return ImGuiDataType_S64;
    if constexpr (std::is_floating_point_v<t> && sizeof(t) == 4)
        return ImGuiDataType_Float;
    if constexpr (std::is_floating_point_v<t> && sizeof(t) == 8)
        return ImGuiDataType_Double;
    return ImGuiDataType_COUNT;
    static_assert(std::is_arithmetic_v<t>, "unsupported type");
}

template <arithmetic T> constexpr auto DefaultFmtSpec() -> char const*
{
    using t = std::remove_cv_t<T>;
    if constexpr (std::is_floating_point_v<t>) {
        return "%.3f";
    }
    if constexpr (std::is_signed_v<t>) {
        return "%d";
    }
    if constexpr (std::is_unsigned_v<t>) {
        if constexpr (std::is_same_v<t, unsigned short>)
            return "%hu";
        if constexpr (std::is_same_v<t, unsigned long>)
            return "%lu";
        if constexpr (std::is_same_v<t, unsigned long long>)
            return "%llu";
        if constexpr (std::is_same_v<t, uintmax_t>)
            return "%ju";
        if constexpr (std::is_same_v<t, size_t>)
            return "%zu";
        return "%u";
    }
    return "";
}

} // namespace ImPlus::Arithmetics