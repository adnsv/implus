#include "font-engine.hpp"

#if !defined(IMPLUS_USE_FREETYPE)
#define STB_TRUETYPE_IMPLEMENTATION
#include <imstb_truetype.h>
#endif

namespace ImPlus::Font {

#if defined(IMPLUS_USE_FREETYPE)

face::face(BlobInfo const& bi)
{
    auto ec = FT_Init_FreeType(&ftlib);
    if (ec)
        return;
    ec = FT_New_Memory_Face(
        ftlib, (const FT_Byte*)bi.Blob.data(), (FT_Long)bi.Blob.size(), bi.FaceIndex, &ftface);
    if (ec)
        return;

    loaded = true;
}

face::~face()
{
    if (ftface)
        FT_Done_Face(ftface);
    if (ftlib)
        FT_Done_FreeType(ftlib);
}

auto face::get_name() -> std::string { return FT_Get_Postscript_Name(ftface); }

auto face::glyph_index_of(unsigned codepoint) const -> std::uint16_t
{
    if (!loaded || !ftface)
        return nglyph;
    auto g = FT_Get_Char_Index(ftface, codepoint);
    return g <= 0xffff ? uint16_t(g) : nglyph;
}

auto face::get_metrics() const -> std::optional<metrics>
{
    if (!loaded || !ftface)
        return {};
    return metrics{
        .ascender = ftface->ascender,
        .descender = ftface->descender,
    };
}

auto face::codepoint_vmetrics(unsigned codepoint) const -> std::optional<metrics>
{
    auto g = glyph_index_of(codepoint);
    if (g == nglyph)
        return {};

    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;

    FT_Load_Glyph(ftface, g, FT_LOAD_DEFAULT);
    auto const& m = ftface->glyph->metrics;

    auto ret = metrics{};
    ret.ascender = m.horiBearingY;
    ret.descender = m.horiBearingY - m.height;

    return ret;
}

#else

face::face(BlobInfo const& bi)
{
    auto data = reinterpret_cast<unsigned char const*>(bi.Blob.data());
    auto face_offset = stbtt_GetFontOffsetForIndex(data, bi.FaceIndex);
    if (face_offset < 0)
        return;

    if (!stbtt_InitFont(&info, data, face_offset))
        return;

    loaded = true;
}

face::~face() {}

auto face::get_name() -> std::string
{
    auto n16 = 0;
    auto s16 = reinterpret_cast<uint8_t const*>(stbtt_GetFontNameString(&info, &n16,
        STBTT_PLATFORM_ID_MICROSOFT, STBTT_MS_EID_UNICODE_BMP, STBTT_MS_LANG_ENGLISH, 4));

    if (!s16 || !n16)
        return {};

    auto s = std::string{};

    for (auto i = 0; i < n16; i += 2) {
        uint16_t c = (s16[i] << 8) | s16[i + 1];
        if (c < 0x7f)
            s += char(c);
        else if (c < 0x800) {
            s += char((c >> 6) | 0xc0);
            s += char((c & 0x3f) | 0x80);
        } else {
            s += char((c >> 12) | 0xe0);
            s += char(((c >> 6) & 0x3f) | 0x80);
            s += char((c & 0x3f) | 0x80);
        }
    }

    return s;
}

auto face::glyph_index_of(unsigned codepoint) const -> std::uint16_t
{
    if (!loaded)
        return nglyph;
    auto g = stbtt_FindGlyphIndex(&info, codepoint);
    return g <= 0xffff ? uint16_t(g) : nglyph;
}

auto face::get_metrics() const -> std::optional<metrics>
{
    if (!loaded)
        return {};
    auto scale = stbtt_ScaleForMappingEmToPixels(&info, 1.0f);
    int ascent, descent, linegap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &linegap);
    return metrics{
        .ascender = ascent,
        .descender = descent,
    };
}

auto face::codepoint_vmetrics(unsigned codepoint) const -> std::optional<metrics>
{
    auto g = glyph_index_of(codepoint);
    if (g == nglyph)
        return {};

    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    stbtt_GetGlyphBox(&info, g, &x0, &y0, &x1, &y1);
    return metrics{
        .ascender = y1,
        .descender = y0,
    };
}

#endif

} // namespace ImPlus::Font