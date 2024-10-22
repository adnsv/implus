#pragma once

#include "implus/font.hpp"
#include <cstdint>
#include <optional>

#if defined(IMPLUS_USE_FREETYPE)
#include <ft2build.h>
#include FT_FREETYPE_H
#include <imgui_freetype.h>
#else
#include <imstb_truetype.h>
#endif

namespace ImPlus::Font {

struct face {
    struct metrics {
        int ascender = 0;
        int descender = 0;
    };

    static constexpr auto nglyph = uint16_t(0);
    face(BlobInfo const& bi);
    ~face();

    auto loaded() const -> bool { return loaded_; }
    auto get_name() -> std::string;
    auto glyph_index_of(unsigned codepoint) const -> uint16_t;
    auto get_metrics() const -> std::optional<metrics>;
    auto codepoint_vmetrics(unsigned codepoint) const -> std::optional<metrics>;

private:
    bool loaded_ = false;
#if defined(IMPLUS_USE_FREETYPE)
    FT_Library ftlib = nullptr;
    FT_Face ftface = nullptr;
#else
    stbtt_fontinfo info;
#endif
};

} // namespace ImPlus::Font