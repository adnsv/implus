#pragma once

#include <cmath>
#include <imgui.h>

namespace ImPlus {

enum class Axis {
    Horz,
    Vert,
};

constexpr auto Along(Axis axis, ImVec2 const& v) -> float { return axis == Axis::Vert ? v.y : v.x; }
constexpr auto Across(Axis axis, ImVec2 const& v) -> float
{
    return axis == Axis::Vert ? v.x : v.y;
}

enum class Side {
    West,
    East,
    North,
    South,
};

// Align fits a content of a certain size within a container defined
// by ref_lo and ref_hi boundaries:
//
//  |AlignNear             |
//  |      AlignCenter     |
//  |              AlignFar|
//
// This is also used to place content relative to a position:
//
//           |
//           |Near
//      AlignCenter
//   AlignFar|
//           |
//
enum Align {
    AlignNear,
    AlignCenter,
    AlignFar,
};

// Clamping defines how oversized content is placed in smaller containers:
//
//      ClampNone         ClampNear         ClampFar
//
//      |       |         |       |         |       |
//      |  XXX  |         |  XXX  |         |  xxx  |
//      |XXXXXXX|         |XXXXXXX|         |xxxxxxx|
//     XXXXXXXXXXX        |XXXXXXXXXXX   xxxxxxxxxxx|
//      |       |         |       |         |       |
//
enum Clamp {
    ClampNone,
    ClampNear,
    ClampFar,
};

// Aligned returns the starting position of a content of a given size
// placed relative to a position specified by ref.
inline auto Aligned(Align a, float ref, float size, bool rounded) -> float
{
    float v;
    switch (a) {
    case AlignFar: v = ref - size; break;
    case AlignCenter: v = ref - size * 0.5f; break;
    default: v = ref;
    }
    return rounded ? std::round(v) : v;
}

inline auto Aligned(float a, float ref, float size, bool rounded) -> float
{
    auto v = ref - size * a;
    return rounded ? std::round(v) : v;
}

inline auto XYAligned(Align xa, Align ya, ImVec2 const& ref, ImVec2 const& size, bool rounded)
    -> ImVec2
{
    return ImVec2{
        Aligned(xa, ref.x, size.x, rounded),
        Aligned(ya, ref.y, size.y, rounded),
    };
}

inline auto XYAligned(ImVec2 const& align_xy, ImVec2 const& ref, ImVec2 const& size, bool rounded)
    -> ImVec2
{
    return ImVec2{
        Aligned(align_xy.x, ref.x, size.x, rounded),
        Aligned(align_xy.y, ref.y, size.y, rounded),
    };
}

inline auto XAligned(Align xa, ImVec2 const& ref, ImVec2 const& size, bool rounded) -> ImVec2
{
    return ImVec2{
        Aligned(xa, ref.x, size.x, rounded),
        rounded ? std::round(ref.y) : ref.y,
    };
}

// Aligned returns the starting position of a content of a given size
// fitted in a container specified by ref_lo and ref_hi boundaries.
template <Clamp C = ClampNear>
auto Aligned(Align a, float ref_lo, float ref_hi, float size, bool rounded) -> float
{
    float v;
    switch (a) {
    case AlignFar: v = ref_hi - size; break;
    case AlignCenter: v = (ref_lo + ref_hi - size) * 0.5f; break;
    default: v = ref_lo; // assuming AlignNear
    }
    if constexpr (C == ClampNear) {
        if (v < ref_lo)
            v = ref_lo;
    }
    else if constexpr (C == ClampFar) {
        if (v + size > ref_hi)
            v = ref_hi - size;
    }
    return rounded ? std::round(v) : v;
}

template <Clamp C = ClampNear>
auto Aligned(float a, float ref_lo, float ref_hi, float size, bool rounded) -> float
{
    auto v = ref_lo + (ref_hi - ref_lo - size) * a;
    if constexpr (C == ClampNear) {
        if (v < ref_lo)
            v = ref_lo;
    }
    else if constexpr (C == ClampFar) {
        if (v + size > ref_hi)
            v = ref_hi - size;
    }
    return rounded ? std::round(v) : v;
}

template <Clamp C = ClampNear>
auto XYAligned(Align xa, Align ya, ImVec2 const& ref_lo, ImVec2 const& ref_hi, ImVec2 const& size,
    bool rounded) -> ImVec2
{
    return ImVec2{
        Aligned<C>(xa, ref_lo.x, ref_hi.x, size.x, rounded),
        Aligned<C>(ya, ref_lo.y, ref_hi.y, size.y, rounded),
    };
};

template <Clamp C = ClampNear>
auto XYAligned(ImVec2 const& align_xy, ImVec2 const& ref_lo, ImVec2 const& ref_hi,
    ImVec2 const& size, bool rounded) -> ImVec2
{
    return ImVec2{
        Aligned<C>(align_xy.x, ref_lo.x, ref_hi.x, size.x, rounded),
        Aligned<C>(align_xy.y, ref_lo.y, ref_hi.y, size.y, rounded),
    };
};

} // namespace ImPlus