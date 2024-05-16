#pragma once

#include <functional>
#include <string>
#include <variant>

namespace ImPlus {

using StringSpec = std::variant<std::function<std::string()>, std::string>;
inline auto GetString(StringSpec const& spec) -> std::string
{
    if (auto v = std::get_if<std::string>(&spec))
        return *v;
    else if (auto v = std::get_if<std::function<std::string()>>(&spec))
        if (*v)
            return (*v)();
    return {};
}

} // namespace ImPlus