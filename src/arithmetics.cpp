#include "implus/arithmetics.hpp"

#include <clocale>
#include <optional>

namespace ImPlus::Arithmetics {

inline auto GetUserDecimalSeparator() -> char
{
    auto ret = char('.');
    auto prev = std::setlocale(LC_NUMERIC, "");
    auto ld = std::localeconv();
    if (ld && ld->decimal_point && ld->decimal_point[0] == ',')
        ret = ',';
    std::setlocale(LC_NUMERIC, prev);
    return ret;
}

std::optional<char> cached_decimal_separator;

auto UserDecimalSeparator() -> char
{
    if (!cached_decimal_separator)
        cached_decimal_separator = GetUserDecimalSeparator();
    return *cached_decimal_separator;
}

} // namespace ImPlus::Arithmetics