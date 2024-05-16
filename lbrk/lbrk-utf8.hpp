#pragma once

#include <string>

namespace lbrk {

using codepoint = char32_t;

constexpr auto replacement_character = codepoint{0xFFFD};

auto u8_decode(char const* first, char const* last, codepoint& output) -> char const*
{
    static constexpr auto error_bit = replacement_character;
    static constexpr auto insufficient = replacement_character;
    static constexpr auto incomplete = replacement_character;
    static constexpr auto overlong = replacement_character;
    static constexpr auto unexpected = replacement_character;

    using cu = unsigned char;

    output = insufficient;
    if (first == last)
        return first; // insufficient

    auto c0 = codepoint(cu(*first++));

    if (c0 < 0b10000000u) {
        // 1-byte sequence [U+0000..U+007F]
        output = c0;
        return first;
    }

    auto is_trail = [](cu c) { return (c & 0b11000000u) == 0b10000000u; };

    if (c0 < 0b11000000u) {
        output = unexpected;
        while (first != last && is_trail(cu(*first)))
            ++first;
        return first;
    }

    codepoint c2, c3, c4, c5, c6;
    constexpr codepoint trail_mask = 0b00111111;

    auto is_starter = [](codepoint c) { return (c & 0b11000000u) != 0b10000000u; };

    // byte #2
    if (first == last) // insufficient_input
        return first;
    c2 = codepoint(cu(*first));
    if (is_starter(c2)) {
        output = incomplete;
        return first;
    }
    ++first;
    if (c0 < 0b11100000u) {
        // 2-byte sequence [U+0080..U+07FF]
        output = codepoint(             //
            ((c0 & 0b00011111) << 06) | // #1
            (c2 & trail_mask)           // #2
        );
        if (output < codepoint(0x80))
            output = overlong;
        return first;
    }

    // byte #3
    if (first == last) // insufficient_input
        return first;
    c3 = codepoint(cu(*first));
    if (is_starter(c3)) {
        output = incomplete;
        return first;
    }
    ++first;
    if (c0 < 0b11110000u) {
        // 3-byte sequence [U+00800..U+FFFF]
        output = codepoint(             //
            ((c0 & 0b00001111) << 12) | // #1
            ((c2 & trail_mask) << 06) | // #2
            (c3 & trail_mask)           // #3
        );
        if (output < codepoint(0x800))
            output = overlong;
        return first;
    }

    // byte #4
    if (first == last) // insufficient_input
        return first;
    c4 = codepoint(cu(*first));
    if (is_starter(c4)) {
        output = incomplete;
        return first;
    }
    ++first;
    if (c0 < 0b11111000u) {
        // 4-byte sequence [U+00010000..U+001FFFFF]
        output = codepoint(             //
            ((c0 & 0b00000111) << 18) | // #1
            ((c2 & trail_mask) << 12) | // #2
            ((c3 & trail_mask) << 06) | // #3
            (c4 & trail_mask)           // #4
        );
        if (output < codepoint(0x00010000))
            output = overlong;
        return first;
    }

    // byte #5
    if (first == last) // insufficient_input
        return first;
    c5 = codepoint(cu(*first));
    if (is_starter(c5)) {
        output = incomplete;
        return first;
    }
    ++first;
    if (c0 < 0b11111100u) {
        // 5-byte sequence [U+00200000..U+03FFFFFF]
        output = codepoint(             //
            ((c0 & 0b00000011) << 24) | // #1
            ((c2 & trail_mask) << 18) | // #2
            ((c3 & trail_mask) << 12) | // #3
            ((c4 & trail_mask) << 06) | // #4
            (c5 & trail_mask)           // #5
        );
        if (output < codepoint(0x00200000))
            output = overlong;
        return first;
    }

    // byte #6
    if (first == last) // insufficient_input
        return first;
    c6 = codepoint(cu(*first));
    if (is_starter(c6)) {
        output = incomplete;
        return first;
    }
    ++first;
    // 6-byte sequence [U+04000000..U+7FFFFFFF]
    output = codepoint(             //
        ((c0 & 0b00000001) << 30) | // #1
        ((c2 & trail_mask) << 24) | // #2
        ((c3 & trail_mask) << 18) | // #3
        ((c4 & trail_mask) << 12) | // #4
        ((c5 & trail_mask) << 06) | // #5
        (c6 & trail_mask)           // #6
    );
    if (output < codepoint(0x04000000))
        output = overlong;
    return first;
}

auto u8_decode(std::string_view s8) -> std::u32string
{
    auto ret = std::u32string{};
    auto const n = s8.size();
    if (n > 0) {
        auto p = s8.data();
        auto const e = p + n;
        codepoint cp;
        while (p != e) {
            p = u8_decode(p, e, cp);
            ret += cp;
        }
    }
    return ret;
}

auto u8_decode(std::u8string_view s8) -> std::u32string
{
    auto ret = std::u32string{};
    auto const n = s8.size();
    if (n > 0) {
        auto p = reinterpret_cast<char const*>(s8.data());
        auto const e = p + n;
        codepoint cp;
        while (p != e) {
            p = u8_decode(p, e, cp);
            ret += cp;
        }
    }
    return ret;
}

void u8_encode_codepoint(codepoint c, std::string& dst)
{
    if (c < 0x80) {
        dst += char(c);
    }
    else if (c < 0x800) {
        dst += char((c >> 6) | 0xc0);
        dst += char((c & 0x3f) | 0x80);
    }
    else if (c < 0x10000) {
        dst += char((c >> 12) | 0xe0);
        dst += char(((c >> 6) & 0x3f) | 0x80);
        dst += char((c & 0x3f) | 0x80);
    }
    else {
        dst += char((c >> 18) | 0xF0);
        dst += char(((c >> 12) & 0x3F) | 0x80);
        dst += char(((c >> 6) & 0x3F) | 0x80);
        dst += char((c & 0x3F) | 0x80);
    }
}

auto u8_encode(std::u32string_view s32) -> std::string
{
    auto ret = std::string{};
    if (!s32.empty()) {
        ret.reserve(s32.size());
        for (auto cp : s32)
            u8_encode_codepoint(cp, ret);
    }
    return ret;
}

} // namespace lbrk