#include "implus/font.hpp"

#include <cmath>
#include <cwchar>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#define IMPLUS_DWRITE_FONT_FALLBACK

#ifdef IMPLUS_DWRITE_FONT_FALLBACK
#include "internal/font-win32-dwrite.hpp"
#endif

#include "internal/strconv-win32.hpp"

namespace ImPlus::Font {

struct enumerator {
    HDC dc;
    std::vector<std::byte>& dst;
    enumerator(std::vector<std::byte>& dst)
        : dc{::CreateCompatibleDC(0)}
        , dst{dst}
    {
    }
    ~enumerator() { ::DeleteDC(dc); }
    static int CALLBACK proc(
        ENUMLOGFONTEXW const* lf, ENUMTEXTMETRICW const* tm, DWORD font_type, enumerator* arg)
    {
        return arg->handle(lf, tm, font_type) ? 0 : 1;
    }
    auto handle(ENUMLOGFONTEXW const* lf, ENUMTEXTMETRICW const* tm, DWORD font_type) -> bool
    {
        auto f = ::CreateFontIndirectW(&lf->elfLogFont);
        if (!f)
            return false;

        auto save_f = ::SelectObject(dc, f);
        auto sz = ::GetFontData(dc, 0, 0, nullptr, 0);
        dst.resize(sz);
        ::GetFontData(dc, 0, 0, dst.data(), sz);
        ::SelectObject(dc, save_f);
        ::DeleteObject(f);
        return !dst.empty();
    }
};

using blob = std::vector<std::byte>;
static auto blobs = std::unordered_map<std::string, blob>{};

auto GetDataBlob(char const* facename) -> BlobInfo
{
    auto s = std::string{facename};
    auto& b = blobs[s];

    if (b.empty()) {
        auto wfacename = to_wstring(s);
        auto lf = LOGFONTW{.lfCharSet = DEFAULT_CHARSET};
        std::wcsncpy(lf.lfFaceName, wfacename.c_str(), wfacename.size());
        auto en = enumerator{b};
        ::EnumFontFamiliesExW(en.dc, &lf, (FONTENUMPROCW)(enumerator::proc), (LPARAM)(&en), 0);
    }

    return BlobInfo{b, 0};
}

auto GetDefaultInfo() -> NameInfo
{
    auto lf = LOGFONTW{};
    if (!::SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0))
        return {};

    auto ni = NameInfo{
        .Name = to_string(lf.lfFaceName),
        .PointSize = 12,
    };

    auto tm = TEXTMETRICW{};
    auto f = ::CreateFontIndirectW(&lf);
    auto dc = ::CreateCompatibleDC(0);
    auto save_f = ::SelectObject(dc, f);
    auto dpi = ::GetDeviceCaps(dc, LOGPIXELSY);
    ::GetTextMetricsW(dc, &tm);
    ni.PointSize = std::round(tm.tmHeight * 72.0f / dpi);
    ::SelectObject(dc, save_f);
    ::DeleteDC(dc);
    ::DeleteObject(f);

    return ni;
}

auto LoadDefault() -> Resource
{
    auto ni = GetDefaultInfo();
#ifdef IMPLUS_REPLACE_SEGOE_WITH_CALIBRI
    if (ni.name == "Segoe UI")
        ni.name = "Calibri";
#endif

    auto bi = GetDataBlob(ni.Name.c_str());
    auto r = Load(bi, {}, ni.PointSize, Adjust[ni.Name]);
    if (r)
        Regular = r;
    return r;
}

auto GetFontsForCharset(BYTE charset) -> std::vector<std::string>
{
    LOGFONTW lf = {0};
    lf.lfHeight = -11; // Set a reasonable size
    lf.lfCharSet = charset;

    auto names = std::vector<std::string>{};

    auto EnumFontsCallback = [](LOGFONT const* lpelfe, TEXTMETRIC const* lpntme, DWORD FontType,
                                 LPARAM lParam) -> int {
        auto names = reinterpret_cast<std::vector<std::string>*>(lParam);
        names->emplace_back(to_string(lpelfe->lfFaceName));
        return 1;
    };

    auto dc = ::CreateCompatibleDC(0);
    EnumFontFamiliesExW(dc, &lf, (FONTENUMPROC)EnumFontsCallback, (LPARAM)&names, 0);
    ::DeleteDC(dc);

    return names;
}

auto ChoosePreferredFont(std::vector<std::string> const& enumerated,
    std::vector<std::string> const& preferred) -> std::string
{
    for (const auto& n : preferred)
        if (auto it = std::find(enumerated.begin(), enumerated.end(), n); it != enumerated.end())
            return *it;

    if (!enumerated.empty())
        return enumerated.front();
    else
        return {};
}

inline auto GetFontFileSourceInfo(IDWriteFontFace* ff) -> std::optional<FileInfo>
{
    if (!ff)
        return {};
    auto file_path = GetFontFilePath(ff);
    if (file_path.empty())
        return {};

    return FileInfo{
        .Filename = std::move(file_path),
        .FaceIndex = int(ff->GetIndex()),
    };
}

inline auto GetFontFileSourceInfo(IDWriteFont* font) -> std::optional<FileInfo>
{
    return GetFontFileSourceInfo(GetFontFace(font).Get());
}

void Test()
{
    auto _ = ComInitializer();
    auto factory = DWriteFactory{};

    auto dflt_name = std::string{};
    auto dflt_attrs = FontAttributes{};
    auto dflt_size = 12;

    GetSystemDefaultUIFontInfo(dflt_name, dflt_attrs, dflt_size);

    auto fonts = factory.GetSystemFontCollection();
    auto dflt_family = FindFontFamily(fonts.Get(), dflt_name);
    if (!dflt_family)
        dflt_family = FindFontFamily(fonts.Get(), "Segoe UI");
    if (!dflt_family)
        dflt_family = FindFontFamily(fonts.Get(), "Tahoma");
    if (!dflt_family)
        dflt_family = FindFontFamily(fonts.Get(), "Arial");

    auto dflt_family_name = GetFontFamilyName(dflt_family.Get());

    auto default_font = GetFirstMatchingFont(dflt_family.Get(), dflt_attrs);
    auto default_full_name =
        GetInformationalString(default_font.Get(), DWRITE_INFORMATIONAL_STRING_FULL_NAME);

    auto default_src = GetFontFileSourceInfo(default_font.Get());
    if (!default_src)
        return;

    auto fallback = factory.GetSystemFontFallback();
    if (!fallback)
        return;

    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    if (!GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH))
        GetSystemDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH);

    auto chinese_font = LookupFontForText(fallback.Get(), dflt_attrs, "汉", localeName);
    if (!chinese_font)
        chinese_font = GetFirstMatchingFont(
            FindFontFamily(fonts.Get(), {"Microsoft YaHei", "SimSun", "Microsoft JhengHei"}).Get(),
            dflt_attrs);
    auto chinese_full_name =
        GetInformationalString(chinese_font.Get(), DWRITE_INFORMATIONAL_STRING_FULL_NAME);
    auto chinese_src = GetFontFileSourceInfo(chinese_font.Get());

    auto japanese_font = LookupFontForText(fallback.Get(), dflt_attrs, "漢", localeName);
    if (!japanese_font)
        japanese_font = GetFirstMatchingFont(
            FindFontFamily(fonts.Get(), {"Yu Gothic", "Meiryo", "MS UI Gothic", "MS Gothic"}).Get(),
            dflt_attrs);
    auto japanese_full_name =
        GetInformationalString(japanese_font.Get(), DWRITE_INFORMATIONAL_STRING_FULL_NAME);
    auto japanese_src = GetFontFileSourceInfo(japanese_font.Get());

    auto korean_font = LookupFontForText(fallback.Get(), dflt_attrs, "한", localeName);
    if (!korean_font)
        korean_font = GetFirstMatchingFont(
            FindFontFamily(fonts.Get(), {"Malgun Gothic", "Gulim", "Batang"}).Get(), dflt_attrs);
    auto korean_full_name =
        GetInformationalString(korean_font.Get(), DWRITE_INFORMATIONAL_STRING_FULL_NAME);
    auto korean_src = GetFontFileSourceInfo(korean_font.Get());

    auto r = Load(*default_src, {}, dflt_size, Adjust[default_full_name]);
    auto c = Resource{};
    if (chinese_src && *chinese_src != *default_src) {
        c = Load(*chinese_src,
            {
                Ranges::CJKSymbolsandPunctuation,
                Ranges::CJKUnifiedIdeographsExtensionA,
                Ranges::CJKUnifiedIdeographs,
                Ranges::CJKCompatibilityIdeographs,
            },
            dflt_size, Adjust[chinese_full_name]);
        SetMergeMode(c);
    }
    auto j = Resource{};
    if (japanese_src && *japanese_src != *default_src) {
        j = Load(*japanese_src,
            {
                Ranges::Hiragana,
                Ranges::Katakana,
                Ranges::KatakanaPhoneticExtensions,
                Ranges::Kanbun,
                Ranges::CJKSymbolsandPunctuation,
                Ranges::CJKUnifiedIdeographs,
                Ranges::CJKCompatibilityIdeographs,
                Ranges::HalfwidthandFullwidthForms,
            },
            dflt_size, Adjust[japanese_full_name]);
        SetMergeMode(j);
    }
    auto k = Resource{};
    if (korean_src && *korean_src != *default_src) {
        k = Load(*korean_src, {
            Ranges::HangulJamo,
            Ranges::HangulSyllables,
            Ranges::HangulCompatibilityJamo,
            Ranges::HangulJamoExtendedA,
            Ranges::HangulJamoExtendedB,
            Ranges::CJKUnifiedIdeographs,
            Ranges::CJKCompatibilityIdeographs,
            Ranges::HalfwidthandFullwidthForms,
        }, dflt_size, Adjust[korean_full_name]);
        SetMergeMode(k);
    }
}

} // namespace ImPlus::Font