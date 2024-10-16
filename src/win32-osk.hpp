#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace osk {

auto GetInputPaneRect(RECT& r) -> bool;
auto IsInputPaneVisible() -> bool;
auto ToggleInputPaneVisibility() -> HRESULT;

} // namespace osk