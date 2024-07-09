#pragma once

#include <implus/id.hpp>

namespace ImPlus {

// main menu bar that supports rhs item alignment and overflow
//
// MenuTrailPlacement() to start placing items to the right hand side
// MenuFrontPlacement() to return to the normal left-to-right placement
//
// Items that don't fit will be placed into the overflow popup. The items that
// are added last, will be the first candidates to go into the overflow menu.

auto BeginMainMenuBar() -> bool;
void EndMainMenuBar();

void MenuFrontPlacement();
void MenuTrailPlacement();

auto BeginMenu(char const* label, bool enabled = true) -> bool;
void EndMenu();

auto MenuItem(ImPlus::ImID const& id, ImPlus::Icon const& icon, std::string_view caption,
    std::string_view shortcut = {}, bool selected = false, bool enabled = true) -> bool;

} // namespace ImPlus