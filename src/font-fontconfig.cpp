#include "implus/font.hpp"

#include <cmath>
#include <filesystem>
#include <fontconfig/fontconfig.h>

namespace ImPlus::Font {

auto GetFileInfo(char const* facename) -> FileInfo
{
    auto fi = FileInfo{};

    auto fontconfig = FcInitLoadConfigAndFonts();
    auto pattern = FcNameParse((FcChar8*)facename);
    FcConfigSubstitute(fontconfig, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    auto fc_result = FcResultNoMatch;
    if (auto res = FcFontMatch(fontconfig, pattern, &fc_result)) {
        FcChar8* file = nullptr;

        if (FcPatternGetString(res, FC_FILE, 0, &file) == FcResultMatch)
            fi.Filename = reinterpret_cast<char const*>(file);

        FcPatternGetInteger(res, FC_INDEX, 0, &fi.FaceIndex);

        FcPatternDestroy(pattern);
        FcConfigDestroy(fontconfig);
    }
    FcPatternDestroy(pattern);
    return fi;
}

auto LoadDefault() -> Resource
{
    auto ni = GetDefaultInfo();
#ifdef IMPLUS_REPLACE_SEGOE_WITH_CALIBRI
    if (fi.name == "Segoe UI")
        fi.name = "Calibri";
#endif

#ifdef __APPLE__
    // NSFont.systemFont returns useless name, luckily, fontconfig seems to
    // recognize "System Font" resolving it to "/System/Library/Fonts/SFNS.ttf"
    ni.Name = "System Font";
    ni.PointSize = std::round(ni.PointSize * 96.0f / 72.0f);
#endif
#ifdef __linux__
    // This adjustment seems to produce font sizes that better match other apps, 
    // at least when running Gnome Desktop.
    ni.PointSize = std::round(ni.PointSize * 96.0f / 72.0f);
#endif

    auto fi = GetFileInfo(ni.Name.c_str());
    auto r = Load(fi, {}, ni.PointSize, Adjust[ni.Name]);
    if (r)
        Regular = r;

    return r;
}

} // namespace ImPlus::Font