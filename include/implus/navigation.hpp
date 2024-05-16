#pragma once

namespace ImPlus::Navigation {

// navigation helpers

auto Active() -> bool;
auto Visible() -> bool;

// ActivatePressed returns true if the navigation input is trying to activate
// something.
//
// If extended is false, this is a simple wrapper testing for "Space" key click
// if keyboard navigation is enabled and also handles gamepad input (Activate /
// Open / Toggle / Tweak)
// - A (Xbox)
// - B (Switch)
// - Cross (PS)
//
// If extended is true, this function also tests for Key_Enter and
// Key_KeyPadEnter.
//
auto ActivatePressed(bool extended) -> bool;

// CancelPressed detects ImGuiKey_Escape presses if keyboard navigation is
// enabled. If gamepad navigation is enabled, it responds to (Cancel / Close /
// Exit):
// - B (Xbox)
// - A (Switch)
// - Circle (PS)
//
auto CancelPressed() -> bool;

} // namespace ImPlus::Navigation
