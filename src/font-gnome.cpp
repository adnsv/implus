#include "implus/font.hpp"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>

namespace ImPlus::Font {

std::string exec(char const* cmd)
{
    static constexpr std::size_t capacity = 256;
    char buffer[capacity];
    auto pipe =
        std::unique_ptr<FILE, decltype(&pclose)>{popen(cmd, "r"), pclose};
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    std::string result;
    while (fgets(buffer, int(capacity), pipe.get()) != nullptr) {
        result.assign(buffer);
    }
    return result;
}

auto trimmed(std::string_view sv) -> std::string_view
{
    auto pos = sv.find_last_not_of(" \r\n");
    if (pos != std::string_view::npos)
        return sv.substr(0, pos + 1);
    return sv;
}
auto unquoted(std::string_view sv) -> std::string_view
{
    if (sv.size() > 1 && sv.front() == '\'' && sv.back() == '\'')
        return sv.substr(1, sv.size() - 2);
    return sv;
}

auto GetDefaultInfo() -> NameInfo
{
    // default gnome desktop font
    // typically returns 'FontName SZ'
    auto str = exec("gsettings get org.gnome.desktop.interface font-name");

    auto name = unquoted(trimmed(str));
    if (name.empty())
        return {"Helvetica", 12.0f};

    if (auto pos = name.find_last_not_of("0123456789");
        pos != std::string_view::npos) {
        auto s = name.substr(pos + 1, name.size());
        auto sz = int{0};
        auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), sz);
        if (ec == std::errc{}) {
            name = name.substr(0, pos + 1);
            while (!name.empty() && name.back() == ' ')
                name.remove_suffix(1);
            return {std::string{name}, float(sz)};
        }
    }

    return NameInfo{std::string{name}, 12.0f};
}

} // namespace ImPlus::Font