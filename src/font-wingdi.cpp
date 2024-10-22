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
#include <dwrite_2.h>
#include <wrl.h>
#endif

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

#ifdef IMPLUS_DWRITE_FONT_FALLBACK

class TextAnalysisSource : public IDWriteTextAnalysisSource {
public:
    // IUnknown interface
    IFACEMETHOD(QueryInterface)(IID const& iid, OUT void** ppObject)
    {
        if (iid == __uuidof(IDWriteTextAnalysisSource)) {
            *ppObject = static_cast<IDWriteTextAnalysisSource*>(this);
            return S_OK;
        }
        else if (iid == __uuidof(IUnknown)) {
            *ppObject = static_cast<IUnknown*>(static_cast<IDWriteTextAnalysisSource*>(this));
            return S_OK;
        }
        else {
            return E_NOINTERFACE;
        }
    }

    IFACEMETHOD_(ULONG, AddRef)() { return InterlockedIncrement(&mRefValue); }

    IFACEMETHOD_(ULONG, Release)()
    {
        ULONG newCount = InterlockedDecrement(&mRefValue);
        if (newCount == 0)
            delete this;

        return newCount;
    }

public:
    TextAnalysisSource(const wchar_t* text, UINT32 textLength, const wchar_t* localeName,
        DWRITE_READING_DIRECTION readingDirection)
        : mText(text)
        , mTextLength(textLength)
        , mLocaleName(localeName)
        , mReadingDirection(readingDirection)
        , mRefValue(0)
    {
    }

    ~TextAnalysisSource() {}

    // IDWriteTextAnalysisSource implementation
    IFACEMETHODIMP GetTextAtPosition(
        UINT32 textPosition, OUT WCHAR const** textString, OUT UINT32* textLength)
    {
        if (textPosition >= mTextLength) {
            *textString = NULL;
            *textLength = 0;
        }
        else {
            *textString = mText + textPosition;
            *textLength = mTextLength - textPosition;
        }
        return S_OK;
    }

    IFACEMETHODIMP GetTextBeforePosition(
        UINT32 textPosition, OUT WCHAR const** textString, OUT UINT32* textLength)
    {
        if (textPosition == 0 || textPosition > mTextLength) {
            *textString = NULL;
            *textLength = 0;
        }
        else {
            *textString = mText;
            *textLength = textPosition;
        }
        return S_OK;
    }

    IFACEMETHODIMP_(DWRITE_READING_DIRECTION)
    GetParagraphReadingDirection() { return mReadingDirection; }

    IFACEMETHODIMP GetLocaleName(
        UINT32 textPosition, OUT UINT32* textLength, OUT WCHAR const** localeName)
    {
        *localeName = mLocaleName;
        *textLength = mTextLength - textPosition;
        return S_OK;
    }

    IFACEMETHODIMP
    GetNumberSubstitution(UINT32 textPosition, OUT UINT32* textLength,
        OUT IDWriteNumberSubstitution** numberSubstitution)
    {
        *numberSubstitution = NULL;
        *textLength = mTextLength - textPosition;
        return S_OK;
    }

protected:
    UINT32 mTextLength;
    const wchar_t* mText;
    const wchar_t* mLocaleName;
    DWRITE_READING_DIRECTION mReadingDirection;
    ULONG mRefValue;

private:
    // No copy construction allowed.
    TextAnalysisSource(const TextAnalysisSource& b) = delete;
    TextAnalysisSource& operator=(TextAnalysisSource const&) = delete;
};

auto analyze(IDWriteFontFallback* pFontFallback, std::string_view sample,
    const wchar_t* localeName) -> std::string
{
    using namespace Microsoft::WRL;

    auto text = to_wstring(sample);
    auto source = TextAnalysisSource(
        text.data(), text.length(), localeName, DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);

    // Map the Chinese character to a specific font
    UINT32 mappedLength = 0;
    ComPtr<IDWriteFont> pMappedFont;
    FLOAT scale = 1.0f;

    auto hr = pFontFallback->MapCharacters(&source, 0, 1, nullptr, nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        &mappedLength, &pMappedFont, &scale);

    if (FAILED(hr) || !pMappedFont) {
        return {};
    }

    auto result = std::string{};

    ComPtr<IDWriteFontFamily> pFontFamily;
    hr = pMappedFont->GetFontFamily(&pFontFamily);
    if (FAILED(hr))
        return {};

    ComPtr<IDWriteFontFace> pFontFace;
    hr = pMappedFont->CreateFontFace(&pFontFace);
    if (FAILED(hr))
        return {};

    auto face_index = pFontFace->GetIndex();

    UINT32 fileCount = 0;
    hr = pFontFace->GetFiles(&fileCount, nullptr);
    if (FAILED(hr) || fileCount != 1)
        return {};

    std::vector<ComPtr<IDWriteFontFile>> fontFiles(fileCount);
    hr = pFontFace->GetFiles(&fileCount, reinterpret_cast<IDWriteFontFile**>(fontFiles.data()));
    if (FAILED(hr) || fileCount != 1)
        return {};

    auto filepath = std::filesystem::path{};

    for (auto const& fontFile : fontFiles) {
        const void* fontFileReferenceKey;
        UINT32 fontFileReferenceKeySize;
        hr = fontFile->GetReferenceKey(&fontFileReferenceKey, &fontFileReferenceKeySize);
        if (FAILED(hr))
            return {};

        ComPtr<IDWriteFontFileLoader> loader;
        hr = fontFile->GetLoader(&loader);
        if (FAILED(hr))
            return {};

        ComPtr<IDWriteLocalFontFileLoader> localLoader;
        hr = loader.As(&localLoader);
        if (FAILED(hr))
            return {};

        UINT32 pathLength = 0;
        hr = localLoader->GetFilePathLengthFromKey(
            fontFileReferenceKey, fontFileReferenceKeySize, &pathLength);
        if (FAILED(hr))
            return {};

        std::wstring filePath(pathLength + 1, L'\0');
        localLoader->GetFilePathFromKey(
            fontFileReferenceKey, fontFileReferenceKeySize, &filePath[0], pathLength + 1);

        filepath = filePath;
    }

    // Get the localized family names
    ComPtr<IDWriteLocalizedStrings> pFamilyNames;
    hr = pFontFamily->GetFamilyNames(&pFamilyNames);
    if (FAILED(hr))
        return {};

    // Try to find the English name
    UINT32 index = 0;
    BOOL exists = FALSE;
    hr = pFamilyNames->FindLocaleName(L"en-us", &index, &exists);
    if (!exists) {
        hr = pFamilyNames->FindLocaleName(L"en", &index, &exists);
    }
    if (!exists) {
        index = 0; // Use the first available name
    }

    // Get the length of the font family name
    UINT32 length = 0;
    hr = pFamilyNames->GetStringLength(index, &length);
    if (SUCCEEDED(hr)) {
        // Retrieve the font family name
        std::wstring familyName(length + 1, L'\0');
        hr = pFamilyNames->GetString(index, &familyName[0], length + 1);
        if (SUCCEEDED(hr)) {
            result = to_string(familyName);
        }
    }

    return result;
}

auto find_substitution(std::string const& sample) -> std::string
{
    using namespace Microsoft::WRL;
    auto hr = CoInitialize(nullptr);
    if (FAILED(hr))
        return {};

    ComPtr<IDWriteFactory2> pDWriteFactory;
    hr =
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory2), &pDWriteFactory);
    if (FAILED(hr)) {
        CoUninitialize();
        return {};
    }

    // Get the system font fallback mechanism
    ComPtr<IDWriteFontFallback> pFontFallback;
    hr = pDWriteFactory->GetSystemFontFallback(&pFontFallback);
    if (FAILED(hr)) {
        CoUninitialize();
        return {};
    }

    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    if (!GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH)) {
        CoUninitialize();
        return {};
    }

    auto result = analyze(pFontFallback.Get(), sample, localeName);

    CoUninitialize();
    return result;
}

#endif

auto FindSubstitutionFontName(Substitution fs) -> std::string
{
    auto name = std::string{};
    switch (fs) {
    case Substitution::Chinese:
#ifdef IMPLUS_DWRITE_FONT_FALLBACK
        name = find_substitution("汉");
#endif
        if (name.empty())
            name = ChoosePreferredFont(GetFontsForCharset(GB2312_CHARSET),
                {"Microsoft YaHei", "SimSun", "Microsoft JhengHei"});
        break;

    case Substitution::Japanese:
#ifdef IMPLUS_DWRITE_FONT_FALLBACK
        name = find_substitution("漢");
#endif
        if (name.empty())
            name = ChoosePreferredFont(
                GetFontsForCharset(SHIFTJIS_CHARSET), {"Meiryo", "MS Gothic", "Yu Gothic"});
        break;

    case Substitution::Korean:
#ifdef IMPLUS_DWRITE_FONT_FALLBACK
        name = find_substitution("한");
#endif
        if (name.empty())
            name = ChoosePreferredFont(
                GetFontsForCharset(HANGEUL_CHARSET), {"Malgun Gothic", "Gulim", "Batang"});
        break;
    }

    return name;
}

} // namespace ImPlus::Font