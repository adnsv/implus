#pragma once

#include "length.hpp"

namespace ImPlus::Panel {

enum class Condition { Never, WhenFull, WhenFloating, Always };

struct Options {
    bool Floating = true; // set to false to make a full-screen panel
    bool UseWorkArea = true; // avoid covering menu-bars, task-bars, etc
    bool VertScrollbar = false;
    bool HorzScrollbar = false;
    Condition Padding = Condition::Always;
    length InitWidth =
        20 * em; // desired initial width when floating
    length InitHeight =
        20 * em; // desired initial height when floating
};

auto Begin(char const* name, Options const& options) -> bool;
void End();

} // namespace ImPlus::Panel