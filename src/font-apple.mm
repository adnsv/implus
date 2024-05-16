#include "implus/font.hpp"

#import <Cocoa/Cocoa.h>

namespace ImPlus::Font {

auto GetDefaultInfo() -> NameInfo {
    NSFont *font = [NSFont systemFontOfSize: 0.0f];

    float point_size = [font pointSize];
    const char* fontname = [[font fontName] UTF8String];

    return NameInfo{fontname, point_size};
}

}