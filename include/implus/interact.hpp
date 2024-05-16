#pragma once

namespace ImPlus {

struct InteractState {
    bool Hovered = false;
    bool Held = false;
    bool Pressed = false;
};

enum class SelectionModifier {
    Regular,
    Toggle,  // e.g. CTRL pressed
    Range,   // e.g. SHIFT pressed
    Context, // no changes if an item is already selected, otherwise select this
             // and deselect others (this is helpful when handling item dragging
             // or context popups in multi-select lists)
};

auto GetSelectionModifier() -> SelectionModifier;

} // namespace ImPlus