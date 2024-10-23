#pragma once

#include <dwrite_2.h>
#include <wrl.h>

#include <filesystem>
#include <stdexcept>

#include "strconv-win32.hpp"

struct ComInitializer {
public:
    explicit ComInitializer(DWORD coinit = COINIT_MULTITHREADED)
    {
        auto hr = CoInitializeEx(nullptr, coinit);
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to initialize COM");
        }
    }

    ~ComInitializer() { CoUninitialize(); }

    // Prevent copying
    ComInitializer(const ComInitializer&) = delete;
    ComInitializer& operator=(const ComInitializer&) = delete;
};

struct FontAttributes {
    DWRITE_FONT_WEIGHT Weight = DWRITE_FONT_WEIGHT_NORMAL;
    DWRITE_FONT_STRETCH Stretch = DWRITE_FONT_STRETCH_NORMAL;
    DWRITE_FONT_STYLE Style = DWRITE_FONT_STYLE_NORMAL;
};

inline auto GetSystemDefaultUIFontInfo(
    std::string& familyName, FontAttributes& attrs, int& pointSize) -> bool
{
    // Get system UI font metrics
    NONCLIENTMETRICSW metrics = {sizeof(NONCLIENTMETRICSW)};
    if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0))
        return false;

    auto const& lf = metrics.lfMessageFont;

    auto tm = TEXTMETRICW{};
    auto f = ::CreateFontIndirectW(&lf);
    auto dc = ::CreateCompatibleDC(0);
    auto save_f = ::SelectObject(dc, f);
    auto dpi = ::GetDeviceCaps(dc, LOGPIXELSY);
    ::GetTextMetricsW(dc, &tm);
    pointSize = std::round(tm.tmHeight * 72.0f / dpi);
    ::SelectObject(dc, save_f);
    ::DeleteDC(dc);
    ::DeleteObject(f);

    familyName = to_string(lf.lfFaceName);
    attrs.Weight = static_cast<DWRITE_FONT_WEIGHT>(lf.lfWeight);
    attrs.Stretch = DWRITE_FONT_STRETCH_NORMAL,
    attrs.Style = lf.lfItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
    return true;
}

inline auto FindFontFamily(
    IDWriteFontCollection* fc, std::wstring familyName) -> Microsoft::WRL::ComPtr<IDWriteFontFamily>
{
    if (!fc)
        return {};

    UINT32 index;
    BOOL exists;

    auto hr = fc->FindFamilyName(familyName.c_str(), &index, &exists);

    if (FAILED(hr))
        throw std::runtime_error("Failed to search for font family");

    if (!exists)
        return {};

    Microsoft::WRL::ComPtr<IDWriteFontFamily> fontFamily;
    hr = fc->GetFontFamily(index, &fontFamily);
    if (FAILED(hr))
        throw std::runtime_error("Failed to get font family");

    return fontFamily;
}

inline auto FindFontFamily(IDWriteFontCollection* fc,
    std::string_view familyName) -> Microsoft::WRL::ComPtr<IDWriteFontFamily>
{
    return FindFontFamily(fc, to_wstring(familyName));
}

inline auto FindFontFamily(IDWriteFontCollection* fc,
    std::initializer_list<std::string_view> names) -> Microsoft::WRL::ComPtr<IDWriteFontFamily>
{
    for (auto n : names)
        if (auto ff = FindFontFamily(fc, n); ff)
            return ff;
    return {};
}

inline auto GetFirstMatchingFont(IDWriteFontFamily* fontFamily,
    FontAttributes const& attrs) -> Microsoft::WRL::ComPtr<IDWriteFont>
{
    if (!fontFamily)
        return {};

    // First get the matching font
    Microsoft::WRL::ComPtr<IDWriteFont> font;
    auto hr = fontFamily->GetFirstMatchingFont(attrs.Weight, attrs.Stretch, attrs.Style, &font);
    if (FAILED(hr))
        return {};

    return font;
}

inline auto GetFontFace(IDWriteFont* font) -> Microsoft::WRL::ComPtr<IDWriteFontFace>
{
    if (!font)
        return {};
    Microsoft::WRL::ComPtr<IDWriteFontFace> face;
    auto hr = font->CreateFontFace(&face);
    if (FAILED(hr))
        return {};
    return face;
}

inline auto extractName(IDWriteLocalizedStrings* strings) -> std::string
{
    if (!strings)
        return {};

    // Try to find the English name
    UINT32 index = 0;
    BOOL exists = FALSE;
    auto hr = strings->FindLocaleName(L"en-us", &index, &exists);
    if (!exists) {
        hr = strings->FindLocaleName(L"en", &index, &exists);
    }
    if (!exists) {
        index = 0; // Use the first available name
    }

    // Get the length of the font family name
    UINT32 length = 0;
    hr = strings->GetStringLength(index, &length);
    if (FAILED(hr))
        return {};

    // Retrieve the font family name
    auto s = std::wstring(length + 1, L'\0');
    hr = strings->GetString(index, &s[0], length + 1);
    if (FAILED(hr))
        return {};

    return to_string(s);
}

inline auto GetFontFamilyName(IDWriteFontFamily* ff) -> std::string
{
    using namespace Microsoft::WRL;

    if (!ff)
        return {};

    // Get the localized family names
    ComPtr<IDWriteLocalizedStrings> names;
    auto hr = ff->GetFamilyNames(&names);
    if (FAILED(hr))
        return {};

    return extractName(names.Get());
}

inline auto GetInformationalString(
    IDWriteFont* font, DWRITE_INFORMATIONAL_STRING_ID string_id) -> std::string
{
    using namespace Microsoft::WRL;

    if (!font)
        return {};

    // Get the localized family names
    ComPtr<IDWriteLocalizedStrings> names;
    BOOL exists;
    auto hr = font->GetInformationalStrings(string_id, &names, &exists);
    if (FAILED(hr) || !exists)
        return {};

    return extractName(names.Get());
}

struct DWriteFactory {
public:
    DWriteFactory()
    {
        auto hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory2),
            reinterpret_cast<IUnknown**>(m_factory.GetAddressOf()));

        if (FAILED(hr))
            throw std::runtime_error("Failed to create IDWriteFactory2");
    }

    auto GetSystemFontFallback() -> Microsoft::WRL::ComPtr<IDWriteFontFallback>
    {
        Microsoft::WRL::ComPtr<IDWriteFontFallback> fontFallback;
        auto hr = m_factory->GetSystemFontFallback(&fontFallback);

        if (FAILED(hr))
            throw std::runtime_error("Failed to get system font fallback");

        return fontFallback;
    }

    auto GetSystemFontCollection(
        BOOL checkForUpdates = FALSE) -> Microsoft::WRL::ComPtr<IDWriteFontCollection>
    {
        Microsoft::WRL::ComPtr<IDWriteFontCollection> fontCollection;
        auto hr = m_factory->GetSystemFontCollection(&fontCollection, checkForUpdates);

        if (FAILED(hr))
            throw std::runtime_error("Failed to get system font collection");

        return fontCollection;
    }

private:
    Microsoft::WRL::ComPtr<IDWriteFactory2> m_factory;
};

struct TextAnalysisSource : public IDWriteTextAnalysisSource {
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

inline auto GetFontFilePath(IDWriteFontFace* ff) -> std::filesystem::path
{
    using namespace Microsoft::WRL;

    if (!ff)
        return {};

    UINT32 fileCount = 0;
    auto hr = ff->GetFiles(&fileCount, nullptr);
    if (FAILED(hr) || fileCount != 1)
        return {};

    std::vector<ComPtr<IDWriteFontFile>> fontFiles(fileCount);
    hr = ff->GetFiles(&fileCount, reinterpret_cast<IDWriteFontFile**>(fontFiles.data()));
    if (FAILED(hr) || fileCount != 1)
        return {};

    auto const& fontFile = fontFiles.front();

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

    std::wstring path(pathLength + 1, L'\0');
    hr = localLoader->GetFilePathFromKey(
        fontFileReferenceKey, fontFileReferenceKeySize, &path[0], pathLength + 1);
    if (FAILED(hr))
        return {};

    return std::filesystem::path{path};
}

inline auto LookupFontForText(IDWriteFontFallback* pFontFallback, FontAttributes const& attrs,
    std::string_view sample, wchar_t const* localeName) -> Microsoft::WRL::ComPtr<IDWriteFont>
{
    auto const text = to_wstring(sample);
    auto source = TextAnalysisSource(
        text.data(), text.length(), localeName, DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);

    UINT32 mappedLength = 0;
    Microsoft::WRL::ComPtr<IDWriteFont> font;
    FLOAT scale = 1.0f;

    auto hr =
        pFontFallback->MapCharacters(&source, 0, 1, nullptr, nullptr, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, &mappedLength, &font, &scale);

    if (FAILED(hr) || !font)
        return {};

    return font;
}