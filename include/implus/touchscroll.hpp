#pragma once

namespace ImPlus {

enum ScrollDirection {
    ScrollNone,
    ScrollHorz,
    ScrollVert,
    ScrollBoth,
};

typedef int TouchScrollFlags;

enum TouchScrollFlags_ {
    TouchScrollFlags_None = 0, 
    TouchScrollFlags_EnableMouseScrolling = 1 << 0, // enable scrolling with mouse
    TouchScrollFlags_EnablePenScrolling = 1 << 1, // enable scrolling with pen
    TouchScrollFlags_DisableTouchScrolling = 1 << 2, // disable scrolling with touch panel

    TouchScrollFlags_DisableOnActiveChild = 1 << 3, // do not scroll when clicked on an active child
};

// HandleTouchScroll implements one-finger kinetic scrolling
//
// Place HandleTouchScroll() immediately before calling ImGui::EndChild() when implementing
// a scrollable child window.
//
void HandleTouchScroll(TouchScrollFlags flags = TouchScrollFlags_None);

} // namespace ImPlus