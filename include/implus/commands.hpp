#pragma once

#include <imgui.h>

#include "implus/accelerator.hpp"

#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ImPlus::Command {

template <typename T>
requires std::is_default_constructible_v<T>
struct property : public std::variant<std::function<T()>, T> {
    using value_type = T;
    using functor_type = typename std::function<T()>;
    using storage_type = typename std::variant<std::function<T()>, T>;
    using storage_type::storage_type;

    template <typename Callable>
    requires std::is_convertible_v<Callable, functor_type>
    property(Callable&& proc)
        : storage_type{std::forward<functor_type>(proc)}
    {
    }

    template <typename U>
    requires(std::is_convertible_v<U, value_type> && !std::is_convertible_v<U, functor_type>)
    property(U&& v)
        : storage_type{std::forward<U>(v)}
    {
    }

    auto value() const -> value_type
    {
        if (auto v = std::get_if<functor_type>(this)) {
            return v ? std::invoke(*v) : value_type{};
        }
        else if (auto v = std::get_if<value_type>(this)) {
            return *v;
        }
        else {
            return value_type{};
        }
    }
};

struct accel_list : public std::vector<ImGuiKeyChord> {
    using vector_type = std::vector<ImGuiKeyChord>;
    using vector_type::vector_type;
    using vector_type::operator=;
    using vector_type::push_back;

    accel_list(std::initializer_list<std::string_view> rhs) { push_back(rhs); }
    accel_list(std::string_view s) { push_back(s); }
    auto contains(ImGuiKeyChord accel) const -> bool
    {
        return std::find(begin(), end(), accel) != end();
    }
    auto operator=(std::initializer_list<std::string_view> rhs) -> accel_list
    {
        clear();
        reserve(rhs.size());
        for (auto&& s : rhs)
            push_back(s);
        return *this;
    }
    void push_back(std::string_view s)
    {
        auto kc = KeyChordFromStr(s);
        if (kc != ImGuiKey_None && !contains(kc))
            push_back(kc);
    }
    void push_back(std::initializer_list<std::string_view> rhs)
    {
        reserve(size() + rhs.size());
        for (auto&& s : rhs)
            push_back(s);
    }
};

struct Entry {
    property<std::string> Caption;
    accel_list Accelerators;
    std::function<void()> OnExecute;
    property<bool> Selected = {false};
    property<bool> Enabled = {true};

    auto MenuItem(char const* id_str) const -> bool;
};

inline auto Entry::MenuItem(char const* id_str) const -> bool
{
    auto label = Caption.value();
    label += "##";
    label += id_str;

    char buf[64] = {0};
    char const* shortcut = nullptr;
    if (!Accelerators.empty()) {
        auto [p, ec] = KeyChordToChars(buf, buf + 64, Accelerators[0], DefaultKeyNameFlags());
        if (ec == std::errc{}) {
            *p = '\0';
            shortcut = buf;
        }
    }

    auto hit = ImGui::MenuItem(label.c_str(), shortcut, Selected.value(), Enabled.value());
    if (hit && OnExecute)
        OnExecute();
    return hit;
}

struct List : public std::unordered_map<std::string, Entry> {
    auto FindAccel(ImGuiKeyChord accel) -> Entry*;
    auto FindAccel(ImGuiKeyChord accel) const -> Entry const*;
    auto MenuItem(char const* id_str) const -> bool;

    void ProcessAccelerators();
};

inline auto List::FindAccel(ImGuiKeyChord accel) -> Entry*
{
    auto it = std::find_if(
        begin(), end(), [accel](auto&& it) { return it.second.Accelerators.contains(accel); });
    return it == end() ? nullptr : &it->second;
}

inline auto List::FindAccel(ImGuiKeyChord accel) const -> Entry const*
{
    auto it = std::find_if(
        begin(), end(), [accel](auto&& it) { return it.second.Accelerators.contains(accel); });
    return it == end() ? nullptr : &it->second;
}

inline auto List::MenuItem(char const* id_str) const -> bool
{
    auto v = this->find(id_str);
    return v != end() && v->second.MenuItem(id_str);
}

} // namespace ImPlus::Command