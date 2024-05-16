#pragma once

#include <imgui.h>

#include "color.hpp"
#include "interact.hpp"

#include <functional>

namespace ImPlus::Content {

enum class Layout {
    // HorzNear:   
    //    -----------------------------
    //   |                             |
    //   | [ICO] CAPTION               |
    //   |       DESCRIPTION           |
    //   |                             |
    //    -----------------------------
    HorzNear,

    // HorzCentered:
    //    -----------------------------
    //   |                             |
    //   |      [ICO] CAPTION          |
    //   |            DESCRIPTION      |
    //   |                             |
    //    -----------------------------
    //
    HorzCenter,

    // VertNear:
    //    --------------------------
    //   |          [ICO]           |
    //   |         CAPTION          |
    //   |        DESCRIPTION       |
    //   |                          |
    //   |                          |
    //    --------------------------
    VertNear,

    // VertCentered
    //    --------------------------
    //   |                          |
    //   |          [ICO]           |
    //   |         CAPTION          |
    //   |       DESCRIPTION        |
    //   |                          |
    //    --------------------------
    VertCenter,
};

using DrawCallback = std::function<void(
    ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max, ColorSet const&)>;

} // namespace ImPlus::Content