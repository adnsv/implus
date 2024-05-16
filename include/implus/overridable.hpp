#pragma once

#include <concepts>
#include <functional>
#include <stack>
#include <type_traits>
#include <variant>

namespace ImPlus {

template <typename T> struct overridable {
    using value_type = T;
    using callback_type = std::function<T()>;
    using entry_type = std::variant<T, callback_type>;

    entry_type dflt;

    overridable(overridable const&) = delete;

    overridable(callback_type&& f)
        : dflt{std::move(f)}
    {
    }
    overridable(callback_type const& f)
        : dflt{f}
    {
    }

    overridable(value_type&& v)
        : dflt{std::move(v)}
    {
    }
    overridable(value_type const& v)
        : dflt{v}
    {
    }

    overridable()
    requires std::is_default_constructible_v<T>
        : dflt{}
    {
    }

    auto operator()() const -> value_type { return get(); }

    auto push(callback_type&& f) { stk.push(std::move(f)); }
    auto push(callback_type const& f) { stk.push(f); }
    auto push(value_type&& v) { stk.push(std::move(v)); }
    auto push(value_type const& v) { stk.push(v); }
    void pop() { stk.pop(); }

protected:
    std::stack<entry_type> stk;

    static auto from(entry_type const& en) -> value_type
    {
        if (auto v = std::get_if<value_type>(&en))
            return *v;
        else
            return std::get<callback_type>(en)();
    }

    auto get() const -> value_type { return stk.empty() ? from(dflt) : from(stk.top()); }
};

} // namespace ImPlus