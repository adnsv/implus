#pragma once

#include <charconv>
#include <concepts>
#include <initializer_list>
#include <string_view>

namespace ImPlus::Font {

struct range {
    using codepoint = uint16_t;
    codepoint lo; // inclusive
    codepoint hi; // inclusive
    constexpr range(codepoint lo, codepoint hi)
        : lo{lo}
        , hi{hi}
    {
    }
    constexpr range(codepoint cp)
        : lo{cp}
        , hi{cp}
    {
    }
    constexpr range(range const&) = default;
};

template <typename Proc>
auto decode_ranges(
    std::string_view s, Proc&& proc) -> bool
{
    unsigned lo, hi;

    auto skip_white = [&]() {
        while (s.starts_with(' ') || s.starts_with('\t'))
            s.remove_prefix(1);
    };

    auto read_cp = [&](unsigned& cp) -> bool {
        if (auto [p, ec] =
                std::from_chars(s.data(), s.data() + s.size(), cp, 16);
            ec == std::errc{}) {
            s.remove_prefix(p - s.data());
            return true;
        }
        return false;
    };

    skip_white();
    while (!s.empty()) {
        if (!read_cp(lo))
            return false;
        skip_white();
        hi = lo;
        if (s.starts_with('-')) {
            s.remove_prefix(1);
            skip_white();
            if (!read_cp(hi))
                return false;
            skip_white();
        }

        if (hi >= lo && lo <= 0xffff) {
            if (hi > 0xffff)
                hi = 0xffff;
            proc(lo, hi);
        }

        if (s.starts_with(',')) {
            s.remove_prefix(1);
            skip_white();
            continue;
        }
        break;
    }
    return true;
}

namespace Ranges {
constexpr range BasicLatin = {0x20, 0x7f};
constexpr range Latin1Supplement = {0x80, 0xff};
constexpr range LatinExtendedA = {0x100, 0x17f};
constexpr range LatinExtendedB = {0x180, 0x24f};
constexpr range IPAExtensions = {0x250, 0x2af};
constexpr range SpacingModifierLetters = {0x2b0, 0x2ff};
constexpr range CombiningDiacriticalMarks = {0x300, 0x36f};
constexpr range GreekandCoptic = {0x370, 0x3ff};
constexpr range Cyrillic = {0x400, 0x4ff};
constexpr range CyrillicSupplement = {0x500, 0x52f};
constexpr range Armenian = {0x530, 0x58f};
constexpr range Hebrew = {0x590, 0x5ff};
constexpr range Arabic = {0x600, 0x6ff};
constexpr range Syriac = {0x700, 0x74f};
constexpr range ArabicSupplement = {0x750, 0x77f};
constexpr range Thaana = {0x780, 0x7bf};
constexpr range NKo = {0x7c0, 0x7ff};
constexpr range Samaritan = {0x800, 0x83f};
constexpr range Mandaic = {0x840, 0x85f};
constexpr range SyriacSupplement = {0x860, 0x86f};
constexpr range ArabicExtendedB = {0x870, 0x89f};
constexpr range ArabicExtendedA = {0x8a0, 0x8ff};
constexpr range Devanagari = {0x900, 0x97f};
constexpr range Bengali = {0x980, 0x9ff};
constexpr range Gurmukhi = {0xa00, 0xa7f};
constexpr range Gujarati = {0xa80, 0xaff};
constexpr range Oriya = {0xb00, 0xb7f};
constexpr range Tamil = {0xb80, 0xbff};
constexpr range Telugu = {0xc00, 0xc7f};
constexpr range Kannada = {0xc80, 0xcff};
constexpr range Malayalam = {0xd00, 0xd7f};
constexpr range Sinhala = {0xd80, 0xdff};
constexpr range Thai = {0xe00, 0xe7f};
constexpr range Lao = {0xe80, 0xeff};
constexpr range Tibetan = {0xf00, 0xfff};
constexpr range Myanmar = {0x1000, 0x109f};
constexpr range Georgian = {0x10a0, 0x10ff};
constexpr range HangulJamo = {0x1100, 0x11ff};
constexpr range Ethiopic = {0x1200, 0x137f};
constexpr range EthiopicSupplement = {0x1380, 0x139f};
constexpr range Cherokee = {0x13a0, 0x13ff};
constexpr range UnifiedCanadianAboriginalSyllabics = {0x1400, 0x167f};
constexpr range Ogham = {0x1680, 0x169f};
constexpr range Runic = {0x16a0, 0x16ff};
constexpr range Tagalog = {0x1700, 0x171f};
constexpr range Hanunoo = {0x1720, 0x173f};
constexpr range Buhid = {0x1740, 0x175f};
constexpr range Tagbanwa = {0x1760, 0x177f};
constexpr range Khmer = {0x1780, 0x17ff};
constexpr range Mongolian = {0x1800, 0x18af};
constexpr range UnifiedCanadianAboriginalSyllabicsExtended = {0x18b0, 0x18ff};
constexpr range Limbu = {0x1900, 0x194f};
constexpr range TaiLe = {0x1950, 0x197f};
constexpr range NewTaiLue = {0x1980, 0x19df};
constexpr range KhmerSymbols = {0x19e0, 0x19ff};
constexpr range Buginese = {0x1a00, 0x1a1f};
constexpr range TaiTham = {0x1a20, 0x1aaf};
constexpr range CombiningDiacriticalMarksExtended = {0x1ab0, 0x1aff};
constexpr range Balinese = {0x1b00, 0x1b7f};
constexpr range Sundanese = {0x1b80, 0x1bbf};
constexpr range Batak = {0x1bc0, 0x1bff};
constexpr range Lepcha = {0x1c00, 0x1c4f};
constexpr range OlChiki = {0x1c50, 0x1c7f};
constexpr range CyrillicExtendedC = {0x1c80, 0x1c8f};
constexpr range GeorgianExtended = {0x1c90, 0x1cbf};
constexpr range SundaneseSupplement = {0x1cc0, 0x1ccf};
constexpr range VedicExtensions = {0x1cd0, 0x1cff};
constexpr range PhoneticExtensions = {0x1d00, 0x1d7f};
constexpr range PhoneticExtensionsSupplement = {0x1d80, 0x1dbf};
constexpr range CombiningDiacriticalMarksSupplement = {0x1dc0, 0x1dff};
constexpr range LatinExtendedAdditional = {0x1e00, 0x1eff};
constexpr range GreekExtended = {0x1f00, 0x1fff};
constexpr range GeneralPunctuation = {0x2000, 0x206f};
constexpr range SuperscriptsandSubscripts = {0x2070, 0x209f};
constexpr range CurrencySymbols = {0x20a0, 0x20cf};
constexpr range CombiningDiacriticalMarksforSymbols = {0x20d0, 0x20ff};
constexpr range LetterlikeSymbols = {0x2100, 0x214f};
constexpr range NumberForms = {0x2150, 0x218f};
constexpr range Arrows = {0x2190, 0x21ff};
constexpr range MathematicalOperators = {0x2200, 0x22ff};
constexpr range MiscellaneousTechnical = {0x2300, 0x23ff};
constexpr range ControlPictures = {0x2400, 0x243f};
constexpr range OpticalCharacterRecognition = {0x2440, 0x245f};
constexpr range EnclosedAlphanumerics = {0x2460, 0x24ff};
constexpr range BoxDrawing = {0x2500, 0x257f};
constexpr range BlockElements = {0x2580, 0x259f};
constexpr range GeometricShapes = {0x25a0, 0x25ff};
constexpr range MiscellaneousSymbols = {0x2600, 0x26ff};
constexpr range Dingbats = {0x2700, 0x27bf};
constexpr range MiscellaneousMathematicalSymbolsA = {0x27c0, 0x27ef};
constexpr range SupplementalArrowsA = {0x27f0, 0x27ff};
constexpr range BraillePatterns = {0x2800, 0x28ff};
constexpr range SupplementalArrowsB = {0x2900, 0x297f};
constexpr range MiscellaneousMathematicalSymbolsB = {0x2980, 0x29ff};
constexpr range SupplementalMathematicalOperators = {0x2a00, 0x2aff};
constexpr range MiscellaneousSymbolsandArrows = {0x2b00, 0x2bff};
constexpr range Glagolitic = {0x2c00, 0x2c5f};
constexpr range LatinExtendedC = {0x2c60, 0x2c7f};
constexpr range Coptic = {0x2c80, 0x2cff};
constexpr range GeorgianSupplement = {0x2d00, 0x2d2f};
constexpr range Tifinagh = {0x2d30, 0x2d7f};
constexpr range EthiopicExtended = {0x2d80, 0x2ddf};
constexpr range CyrillicExtendedA = {0x2de0, 0x2dff};
constexpr range SupplementalPunctuation = {0x2e00, 0x2e7f};
constexpr range CJKRadicalsSupplement = {0x2e80, 0x2eff};
constexpr range KangxiRadicals = {0x2f00, 0x2fdf};
constexpr range IdeographicDescriptionCharacters = {0x2ff0, 0x2fff};
constexpr range CJKSymbolsandPunctuation = {0x3000, 0x303f};
constexpr range Hiragana = {0x3040, 0x309f};
constexpr range Katakana = {0x30a0, 0x30ff};
constexpr range Bopomofo = {0x3100, 0x312f};
constexpr range HangulCompatibilityJamo = {0x3130, 0x318f};
constexpr range Kanbun = {0x3190, 0x319f};
constexpr range BopomofoExtended = {0x31a0, 0x31bf};
constexpr range CJKStrokes = {0x31c0, 0x31ef};
constexpr range KatakanaPhoneticExtensions = {0x31f0, 0x31ff};
constexpr range EnclosedCJKLettersandMonths = {0x3200, 0x32ff};
constexpr range CJKCompatibility = {0x3300, 0x33ff};
constexpr range CJKUnifiedIdeographsExtensionA = {0x3400, 0x4dbf};
constexpr range YijingHexagramSymbols = {0x4dc0, 0x4dff};
constexpr range CJKUnifiedIdeographs = {0x4e00, 0x9fff};
constexpr range YiSyllables = {0xa000, 0xa48f};
constexpr range YiRadicals = {0xa490, 0xa4cf};
constexpr range Lisu = {0xa4d0, 0xa4ff};
constexpr range Vai = {0xa500, 0xa63f};
constexpr range CyrillicExtendedB = {0xa640, 0xa69f};
constexpr range Bamum = {0xa6a0, 0xa6ff};
constexpr range ModifierToneLetters = {0xa700, 0xa71f};
constexpr range LatinExtendedD = {0xa720, 0xa7ff};
constexpr range SylotiNagri = {0xa800, 0xa82f};
constexpr range CommonIndicNumberForms = {0xa830, 0xa83f};
constexpr range Phagspa = {0xa840, 0xa87f};
constexpr range Saurashtra = {0xa880, 0xa8df};
constexpr range DevanagariExtended = {0xa8e0, 0xa8ff};
constexpr range KayahLi = {0xa900, 0xa92f};
constexpr range Rejang = {0xa930, 0xa95f};
constexpr range HangulJamoExtendedA = {0xa960, 0xa97f};
constexpr range Javanese = {0xa980, 0xa9df};
constexpr range MyanmarExtendedB = {0xa9e0, 0xa9ff};
constexpr range Cham = {0xaa00, 0xaa5f};
constexpr range MyanmarExtendedA = {0xaa60, 0xaa7f};
constexpr range TaiViet = {0xaa80, 0xaadf};
constexpr range MeeteiMayekExtensions = {0xaae0, 0xaaff};
constexpr range EthiopicExtendedA = {0xab00, 0xab2f};
constexpr range LatinExtendedE = {0xab30, 0xab6f};
constexpr range CherokeeSupplement = {0xab70, 0xabbf};
constexpr range MeeteiMayek = {0xabc0, 0xabff};
constexpr range HangulSyllables = {0xac00, 0xd7af};
constexpr range HangulJamoExtendedB = {0xd7b0, 0xd7ff};
constexpr range HighSurrogates = {0xd800, 0xdb7f};
constexpr range HighPrivateUseSurrogates = {0xdb80, 0xdbff};
constexpr range LowSurrogates = {0xdc00, 0xdfff};
constexpr range PrivateUseArea = {0xe000, 0xf8ff};
constexpr range CJKCompatibilityIdeographs = {0xf900, 0xfaff};
constexpr range AlphabeticPresentationForms = {0xfb00, 0xfb4f};
constexpr range ArabicPresentationFormsA = {0xfb50, 0xfdff};
constexpr range VariationSelectors = {0xfe00, 0xfe0f};
constexpr range VerticalForms = {0xfe10, 0xfe1f};
constexpr range CombiningHalfMarks = {0xfe20, 0xfe2f};
constexpr range CJKCompatibilityForms = {0xfe30, 0xfe4f};
constexpr range SmallFormVariants = {0xfe50, 0xfe6f};
constexpr range ArabicPresentationFormsB = {0xfe70, 0xfeff};
constexpr range HalfwidthandFullwidthForms = {0xff00, 0xffef};
constexpr range Specials = {0xfff0, 0xffff};
} // namespace Ranges

} // namespace ImPlus::Fontman