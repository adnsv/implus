#include "doctest.h"

#include <lbrk-utf8.hpp>
#include <lbrk.hpp>
#include <string>
#include <string_view>
#include <stdexcept>

const auto lb = char32_t(0xf7); // line break marker '÷'

auto brk_line(std::u32string s)
{
    auto ctx = lbrk::context{};
    auto r = std::u32string{};
    for (auto codepoint : s) {
        auto lbc = lbrk::get_class(codepoint);
        auto lba = ctx.calc_action(lbc);
        if (lba != lbrk::lba::forbid)
            r += lb;
        r += codepoint;
    }
    return r;
}

void pattern(std::string_view pattern)
{
    auto pattern32 = lbrk::u8_decode(pattern);
    auto orig32 = pattern32;
    for (auto it = orig32.begin(); it != orig32.end();) 
        if (*it == lb) it = orig32.erase(it);
        else ++it;
    auto got = brk_line(orig32);
    if (got != pattern32)
        throw std::runtime_error("invalid linebreak");
}

TEST_CASE("LBRK")
{
    using namespace std::literals;
    auto ctx = lbrk::context{};

    CHECK(lbrk::get_class(' ') == lbrk::lbc::SP);
    CHECK(lbrk::get_class('A') == lbrk::lbc::AL);

    CHECK_NOTHROW(pattern("Hello, ÷World!"sv));
    CHECK_NOTHROW(pattern("#̈ ÷—"));
    CHECK_NOTHROW(pattern("# ÷%"));
    CHECK_NOTHROW(pattern("— ÷\u00a0"));
    CHECK_NOTHROW(pattern("—̈ ÷-"));
    CHECK_NOTHROW(pattern("ᅠ ÷⌚"));
    CHECK_NOTHROW(pattern("(ニュー・)÷ヨー÷ク"));
    CHECK_NOTHROW(pattern("ピュー÷タ÷で÷使÷用÷す÷る"));
    CHECK_NOTHROW(pattern("🇷🇺\u200b÷🇸🇪"));
}