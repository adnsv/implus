#pragma once

#include <optional>

namespace kbd {

// FullKeyboardAttached indicates whether the system has a full keyboard attached.
// It ignores keypads, remotes, and other non-standard input devices.
auto FullKeyboardAttached() -> std::optional<bool>;

} // namespace kbd