#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#ifdef IMPLUS_ENABLE_WIN32_OSK

namespace osk {

auto GetInputPaneRect(RECT& r) -> bool;
auto IsInputPaneVisible() -> bool;
auto ToggleInputPaneVisibility() -> HRESULT;

} // namespace osk

#endif