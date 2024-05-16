#pragma once

#include <functional>
#include <imgui.h>
#include "host.hpp"

namespace ImPlus::Visuals {

// Zoom controls default font size
//
// - Specified as percentage, default is 100
//
// - All UI elements that use font size for calculations will get scaled
//   accordingly
//
auto Zoom() -> int;
void SetZoom(int z);
void ResetZoom();
void StepZoom(int step);
inline void ZoomIn() { StepZoom(+1); }
inline void ZoomOut() { StepZoom(-1); }

namespace Theme {
using StyleProc = std::function<void(ImGuiStyle&)>;

inline auto operator|(StyleProc const& a, StyleProc const& b) -> StyleProc
{
    return [a, b](ImGuiStyle& sty) {
        a(sty);
        b(sty);
    };
}

struct Style {
    Style(StyleProc p)
        : apply{p}
    {
        static std::size_t last_id = 0;
        id = ++last_id;
    }
    friend auto operator==(Style const& a, Style const& b)
    {
        return a.id == b.id;
    }
    StyleProc apply;
    std::size_t id;
};

// Setup is to be used for switching to a new theme style set
void Setup(Style const&);
auto Is(Style const&) -> bool;

extern StyleProc const LightColors;
extern StyleProc const DarkColors;
extern StyleProc const ClassicColors;
extern StyleProc const AdjustSizesAndRounding;

extern Style const Light;
extern Style const Dark;
extern Style const Classic;
} // namespace Theme

// SetupFrame configures or updates font scale and theme scale based on current
// DPI. This funtion is to be called at the beginning of the render loop before
// each frame. In needs the DPI parameter that normally should be obtained from
// host's main window.
//
//
// For example:
//
//     while (!window.ShouldClose) {
//         ImPlus::Visuals::SetupFrame(window.ContentScale)
//         window.NewFrame(true);
//
//         ... render content ...
//
//         window.RenderFrame();
//     }
//
auto SetupFrame(Host::Window::Scale const& scale) -> bool;

} // namespace ImPlus::Visuals