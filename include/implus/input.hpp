#pragma once

#include "implus/arithmetics.hpp"
#include "implus/id.hpp"
#include <cassert>
#include <concepts>
#include <functional>
#include <imgui.h>
#include <string>

namespace ImPlus {

namespace internal {
void make_localized_decimal(ImGuiInputTextFlags& f);
void disable_mark_edited();
void mark_last_item_edited();
void reload_input_text_buffer();
auto user_decimal_char() -> char;

constexpr auto trim_whitespace(std::string_view s) -> std::string_view
{
    auto n = s.size();
    if (!n)
        return s;
    while (n > 0 && unsigned(s[n - 1]) <= 32u)
        --n;
    auto i = 0;
    while (i < n && unsigned(s[i]) < 32u)
        ++i;
    return s.substr(i, n - i);
}

} // namespace internal

template <typename T> using InputValidator = std::function<bool(T& value)>;

auto InputTextBuffered(
    ImID id, char const* hint, char* buf, std::size_t buf_size, ImGuiInputTextFlags flags) -> bool;

auto InputText(ImID id, std::string& str, ImGuiInputTextFlags flags = 0) -> bool;

auto InputTextMultiline(
    ImID id, std::string& str, ImVec2 const& size_arg, ImGuiInputTextFlags flags = 0) -> bool;

auto InputTextWithHint(ImID id, const char* hint, std::string& str, ImVec2 const& size_arg,
    ImGuiInputTextFlags flags = 0) -> bool;
auto InputTextWithHint(
    ImID id, const char* hint, std::string& str, ImGuiInputTextFlags flags = 0) -> bool;

template <typename T> struct sentinel {
    std::string key;
    T val;
};

template <typename T> auto same_value(T const& a, T const& b) -> bool
{
    if constexpr (std::is_floating_point_v<T>)
        return std::isnan(a) ? std::isnan(b) : a == b;
    else
        return a == b;
}

template <typename T>
auto find_sentinel_by_key(
    std::span<sentinel<T> const> const& sentinels, std::string_view key) -> sentinel<T> const*
{
    for (auto&& kv : sentinels)
        if (kv.key == key)
            return &kv;
    return nullptr;
}

template <typename T>
auto find_sentinel_by_val(
    std::span<sentinel<T> const> const& sentinels, T const& val) -> sentinel<T> const*
{
    if constexpr (std::is_floating_point_v<T>) {
        // special handler for NaNs
        if (std::isnan(val)) {
            for (auto&& kv : sentinels)
                if (std::isnan(kv.val))
                    return &kv;
            return nullptr;
        }
    }

    for (auto&& kv : sentinels)
        if (kv.val == val)
            return &kv;
    return nullptr;
}

template <maybe_optional_integral T>
auto InputIntegralEx(ImID id, char const* hint, T& v, int base = 10,
    InputValidator<maybe_optional_value_type<T>>&& on_validate = {},
    std::span<sentinel<T> const> const& sentinels = {}) -> bool
{
    using vtype = maybe_optional_value_type<T>;
    constexpr auto is_optional = is_optional_v<T>;

    auto prev_v = v;
    static constexpr std::size_t buf_capacity = 128; // large enough to cover any integer type
    char buf[buf_capacity];
    // char const* buf_end = buf + buf_capacity;

    auto const* sent = find_sentinel_by_val(sentinels, v);
    if (sent) {
        auto n = std::snprintf(buf, buf_capacity, "%s", sent->key.c_str());
        buf[n] = 0;
    }
    else {
        char* z = buf;
        if constexpr (!is_optional)
            z = std::to_chars(buf, buf + buf_capacity - 1, v, base).ptr;
        else if (v.has_value())
            z = std::to_chars(buf, buf + buf_capacity - 1, *v, base).ptr;
        *z = 0;
    }

    auto flags = ImGuiInputTextFlags(ImGuiInputTextFlags_AutoSelectAll);
    internal::disable_mark_edited();

    auto text_changed = InputTextBuffered(id, hint, buf, buf_capacity, flags);
    auto value_changed = false;
    if (text_changed) {
        auto s = internal::trim_whitespace(std::string_view{buf});
        if (sent && s.find(sent->key) != std::string_view::npos) {
            internal::reload_input_text_buffer();
        }
        else if (auto sent = find_sentinel_by_key(sentinels, s)) {
            v = sent->val;
        }
        else if (s.empty()) {
            if constexpr (is_optional)
                v.reset();
            else
                v = prev_v;
        }
        else {
            vtype parsed_v;
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), parsed_v, base);
            if (ec == std::errc{} && ptr == s.data() + s.size()) {
                if (!on_validate || on_validate(parsed_v))
                    v = parsed_v;
            }
        }

        value_changed = v != prev_v;
    }

    if (value_changed)
        internal::mark_last_item_edited();

    return ImGui::IsItemDeactivatedAfterEdit();
}

template <maybe_optional_floating_point T>
auto InputFloatingPointEx(ImID id, char const* hint, T& v,
    InputValidator<maybe_optional_value_type<T>>&& on_validate = {},
    std::span<sentinel<T> const> const& sentinels = {}) -> bool
{
    using vtype = maybe_optional_value_type<T>;
    constexpr auto is_optional = is_optional_v<T>;

    auto const decimal = Style::Numeric::DecimalSeparator();

    auto prev_v = v;
    static constexpr std::size_t buf_capacity = 128; // large enough to cover any integer type
    char buf[buf_capacity];
    char const* buf_end = buf + buf_capacity;

    auto const spec = std::is_same_v<std::remove_cvref_t<vtype>, long double> ? "%.16Lg"
                      : std::is_same_v<std::remove_cvref_t<vtype>, double>    ? "%.16lg"
                                                                              : "%.7g";

    auto const* sent = find_sentinel_by_val(sentinels, v);
    if (sent) {
        auto n = std::snprintf(buf, buf_capacity, "%s", sent->key.c_str());
        buf[n] = 0;
    }
    else {
        int n = 0;
        if constexpr (!is_optional) {
            n = std::snprintf(buf, buf_capacity, spec, v);
            assert(n > 0 && n < buf_capacity);
        }
        else if (v.has_value()) {
            n = std::snprintf(buf, buf_capacity, spec, *v);
            assert(n > 0 && n < buf_capacity);
        }
        buf[n] = 0;

        if (decimal != '.')
            for (auto i = 0; i < n; ++i)
                if (buf[i] == '.') {
                    buf[i] = decimal;
                    break;
                }
    }

    auto flags =
        ImGuiInputTextFlags(ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CharsDecimal);
    internal::make_localized_decimal(flags);
    internal::disable_mark_edited();

    auto text_changed = InputTextBuffered(id, hint, buf, buf_capacity, flags);
    auto value_changed = false;
    if (text_changed) {
        auto s = internal::trim_whitespace(std::string_view{buf});
        if (sent && s.find(sent->key) != std::string_view::npos) {
            internal::reload_input_text_buffer();
        }
        else if (auto const* sent = find_sentinel_by_key(sentinels, s)) {
            v = sent->val;
        }
        else if (s.empty()) {
            if constexpr (is_optional)
                v.reset();
            else
                v = prev_v;
        }
        else {
            auto const p = buf + (s.data() - buf);
            auto const n = s.size();
            p[n] = 0;
            for (auto i = 0; i < n; ++i)
                if (p[i] == ',' || p[i] == decimal) {
                    p[i] = '.';
                    break;
                }

            vtype parsed_v;
            auto const spec = std::is_same_v<std::remove_cv_t<T>, long double> ? "%Lg"
                              : std::is_same_v<std::remove_cv_t<T>, double>    ? "%lg"
                                                                               : "%g";
            auto nargs_parsed = std::sscanf(p, spec, &parsed_v);
            if (nargs_parsed == 1) {
                if (!on_validate || on_validate(parsed_v))
                    v = parsed_v;
            }
        }

        value_changed = v != prev_v;
    }

    if (value_changed)
        internal::mark_last_item_edited();

    return ImGui::IsItemDeactivatedAfterEdit();
}

template <maybe_optional_arithmetic T>
auto InputNumeric(ImID id, T& v, std::string_view units,
    InputValidator<maybe_optional_value_type<T>>&& on_validate = {},
    std::span<sentinel<T> const> const& sentinels = {}) -> bool
{
    using vtype = maybe_optional_value_type<T>;

    auto hint = std::string{};
    if constexpr (is_optional_v<T>)
        hint = Style::Numeric::NullOptHint();

    bool r;

    if constexpr (std::is_integral_v<vtype>)
        r = InputIntegralEx(
            id, hint.c_str(), v, 10, std::forward<InputValidator<vtype>>(on_validate), sentinels);
    else
        r = InputFloatingPointEx(
            id, hint.c_str(), v, std::forward<InputValidator<vtype>>(on_validate), sentinels);

    if (!units.empty()) {
        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::TextUnformatted(units.data(), units.data() + units.size());
    }

    return r;
}

template <maybe_optional_arithmetic T>
auto InputNumeric(ImID id, T& v, InputValidator<maybe_optional_value_type<T>>&& on_validate = {},
    std::span<sentinel<T> const> const& sentinels = {}) -> bool
{
    return InputNumeric(id, v, std::string_view{},
        std::forward<InputValidator<maybe_optional_value_type<T>>>(on_validate), sentinels);
}

template <typename T>
requires std::is_arithmetic_v<T>
auto Slider(ImID id, T& v, T const& v_min, T const& v_max, std::string_view units,
    char const* fmt = Arithmetics::DefaultFmtSpec<T>(), ImGuiSliderFlags flags = 0) -> bool
{
    ImGui::PushID(id);
    auto r = ImGui::SliderScalar("", Arithmetics::DataType<T>(), &v, &v_min, &v_max, fmt, flags);
    ImGui::PopID();
    if (!units.empty()) {
        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::TextUnformatted(units.data(), units.data() + units.size());
    }
    return r;
}

template <typename T>
requires std::is_arithmetic_v<T>
auto Slider(ImID id, T& v, T const& v_min, T const& v_max,
    char const* fmt = Arithmetics::DefaultFmtSpec<T>(), ImGuiSliderFlags flags = 0) -> bool
{
    ImGui::PushID(id);
    auto r = ImGui::SliderScalar("", Arithmetics::DataType<T>(), &v, &v_min, &v_max, fmt, flags);
    ImGui::PopID();
    return r;
}

template <typename T>
requires std::is_arithmetic_v<T>
auto DragArithmetic(ImID id, T& v, float v_speed, T const& v_min, T const& v_max,
    char const* fmt = Arithmetics::DefaultFmtSpec<T>(), float power = 1.0f) -> bool
{
    static_assert(std::is_arithmetic_v<T>, "unsupported type");

    ImGui::PushID(id);
    auto r =
        ImGui::DragScalar("", Arithmetics::DataType<T>(), &v, v_speed, &v_min, &v_max, fmt, power);
    ImGui::PopID();
    return r;
}

template <typename T, size_t N>
requires(std::is_arithmetic_v<T> && N >= 1)
auto DragArithmeticN(ImID id, T v[N], float v_speed, T const& v_min, T const& v_max,
    char const* fmt = Arithmetics::DefaultFmtSpec<T>(), float power = 1.0f) -> bool
{
    ImGui::PushID(id);
    auto r = ImGui::DragScalarN(
        "", Arithmetics::DataType<T>(), v, N, v_speed, &v_min, &v_max, fmt, power);
    ImGui::PopID();
    return r;
}

} // namespace ImPlus