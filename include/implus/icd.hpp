#pragma once

#include "implus/icon.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace ImPlus {

struct CD_view {
    std::string_view Caption = {};
    std::string_view Descr = {};
    constexpr CD_view() noexcept {}
    constexpr CD_view(CD_view const&) = default;

    template <typename C>
    requires std::is_convertible_v<C, std::string_view>
    constexpr CD_view(C const& caption) noexcept
        : Caption{caption}
    {
    }
    template <typename C, typename D>
    requires(std::is_convertible_v<C, std::string_view> &&
                std::is_convertible_v<D, std::string_view>)
    constexpr CD_view(C const& caption, D const& descr) noexcept
        : Caption{caption}
        , Descr{descr}
    {
    }
    constexpr auto Empty() const { return Caption.empty() && Descr.empty(); }
};

// ICD specifies a view into a content that has an icon, a caption, and a description
struct ICD_view {
    ImPlus::Icon Icon = {};
    std::string_view Caption = {};
    std::string_view Descr = {};

    ICD_view() noexcept {}

    ICD_view(ICD_view const&) noexcept = default;
    ICD_view(ICD_view&&) noexcept = default;

    template <typename C>
    requires std::is_convertible_v<C, std::string_view>
    ICD_view(C const& label) noexcept
        : Caption{label}
    {
    }

    template <typename C>
    requires std::is_convertible_v<C, std::string_view>
    ICD_view(ImPlus::Icon const& icon, C const& caption) noexcept
        : Icon{icon}
        , Caption{caption}
    {
    }
    template <typename C, typename D>
    requires(std::is_convertible_v<C, std::string_view> &&
                std::is_convertible_v<D, std::string_view>)
    ICD_view(ImPlus::Icon const& icon, C const& caption, D const& descr) noexcept
        : Icon{icon}
        , Caption{caption}
        , Descr{descr}
    {
    }

    auto operator=(ICD_view const&) -> ICD_view& = default;
    auto operator=(ICD_view&&) -> ICD_view& = default;

    auto Empty() const { return Icon.Empty() && Caption.empty() && Descr.empty(); }
};

struct ICD {
    ImPlus::Icon Icon = {};
    std::string Caption = {};
    std::string Descr = {};

    constexpr ICD() = default;
    ICD(ICD const&) = default;

    ICD(ICD_view const& v)
        : Icon{v.Icon}
        , Caption{v.Caption}
        , Descr{v.Descr}
    {
    }

    template <typename C>
    requires std::is_convertible_v<C, std::string>
    ICD(C&& caption) noexcept
        : Caption{std::forward<std::string>(caption)}
    {
    }

    ICD(ImPlus::Icon const& icon, std::string const& caption) noexcept
        : Icon{icon}
        , Caption{caption}
    {
    }

    ICD(ImPlus::Icon const& icon, std::string const& caption, std::string const& descr) noexcept
        : Icon{icon}
        , Caption{caption}
        , Descr{descr}
    {
    }

    auto Empty() const { return Icon.Empty() && Caption.empty() && Descr.empty(); }
    auto view() const -> ICD_view { return ICD_view{Icon, Caption, Descr}; }
    operator ICD_view() const { return view(); }
};

struct CDOptions {
    Font::Resource CaptionFont = {};
    Font::Resource DescrFont = {};
};

struct ICDOptions {
    bool WithDropdownArrow = false;
    Font::Resource CaptionFont = {};
    Font::Resource DescrFont = {};
};

} // namespace ImPlus