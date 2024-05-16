#include "implus/font.hpp"

#include <cwchar>
#include <cmath>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

namespace ImPlus::Font {

auto to_wstring(std::string_view s) -> std::wstring
{
    if (s.empty())
        return {};
    auto n = ::MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0);
    if (!n)
        return {};
    auto ret = std::wstring{};
    ret.resize(n);
    ::MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), ret.data(), n);
    return ret;
}
auto to_string(std::wstring_view s) -> std::string
{
    if (s.empty())
        return {};
    auto n = ::WideCharToMultiByte(
        CP_THREAD_ACP, 0, s.data(), int(s.size()), nullptr, 0, nullptr, nullptr);
    if (!n)
        return {};
    auto ret = std::string{};
    ret.resize(n);
    ::WideCharToMultiByte(
        CP_THREAD_ACP, 0, s.data(), int(s.size()), ret.data(), n, nullptr, nullptr);
    return ret;
}

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

} // namespace ImPlus::Font