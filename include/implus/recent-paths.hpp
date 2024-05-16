#pragma once

#include "settings.hpp"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace ImPlus {

struct recent_paths {
    using path = std::filesystem::path;

    recent_paths(std::size_t limit = 10)
        : limit_{limit}
    {
    }

    auto limit() const { return limit_; }

    void push_back(path const& p)
    {
        auto it = std::find(_entries.begin(), _entries.end(), p);
        if (it == _entries.end())
            _entries.insert(_entries.end(), p);
    }

    auto insert_or_promote(path const& p) -> bool
    {
        auto modified = false;
        auto it = std::find(_entries.begin(), _entries.end(), p);
        if (it == _entries.end()) {
            _entries.insert(_entries.begin(), p);
            modified = true;
        }
        else if (it != _entries.begin()) {
            std::rotate(_entries.begin(), it, it + 1);
            modified = true;
        }
        if (_entries.size() > limit()) {
            _entries.erase(_entries.begin() + limit(), _entries.end());
            modified = true;
        }
        return modified;
    }

    auto reset() -> bool
    {
        auto modified = !_entries.empty();
        _entries.clear();
        return modified;
    }

    auto empty() const { return _entries.empty(); }

    auto size() const { return _entries.size(); }

    auto contains(path const& p) const
    {
        return std::find(_entries.begin(), _entries.end(), p) != _entries.end();
    }

    auto remove(path const& p) -> bool
    {
        auto it = std::find(_entries.begin(), _entries.end(), p);
        if (it == _entries.end())
            return false;
        _entries.erase(it);
        return true;
    }

    auto remove_empty() -> bool
    {
        auto prev_sz = _entries.size();
        _entries.erase(std::remove(_entries.begin(), _entries.end(), path{}),
            _entries.end());
        return prev_sz != _entries.size();
    }

    auto remove_non_existing() -> bool
    {
        auto prev_sz = _entries.size();
        auto it = std::remove_if(_entries.begin(), _entries.end(),
            [](auto& p) { return !std::filesystem::exists(p); });
        _entries.erase(it, _entries.end());
        return prev_sz != _entries.size();
    }

    auto items() -> std::span<path const> { return _entries; }
    auto first(size_t n) -> std::span<path const>
    {
        auto all = items();
        n = std::min(n, all.size());
        return all.first(n);
    }

protected:
    std::size_t limit_;
    std::vector<path> _entries;
    friend struct Settings::binding_impl<recent_paths>;


};

namespace Settings {
template <> struct binding_impl<recent_paths> {
    void write_all_fields(Settings::writer const& w, recent_paths& vv)
    {
        auto idx = size_t{};
        for (auto& v : vv._entries)
            w.writef(std::to_string(++idx), "\"%s\"", v.string().c_str());
    }

    void read_field(std::string_view key, std::string_view s, recent_paths& vv)
    {
        auto idx = size_t{};
        auto [p, ec] =
            std::from_chars(key.data(), key.data() + key.size(), idx);
        if (ec == std::errc{} && s.starts_with('"') && s.ends_with('"'))
            vv._entries.push_back(s.substr(1, s.size() - 2));
    }
};
} // namespace Settings

} // namespace ImPlus