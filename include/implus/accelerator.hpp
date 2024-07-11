#pragma once

#include <imgui.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace ImPlus {

namespace lookup {
constexpr std::array<std::string_view, ImGuiKey_NamedKey_COUNT> keynames = {{"Tab", "LeftArrow",
    "RightArrow", "UpArrow", "DownArrow", "PageUp", "PageDown", "Home", "End", "Insert", "Delete",
    "Backspace", "Space", "Enter", "Escape", "LeftCtrl", "LeftShift", "LeftAlt", "LeftSuper",
    "RightCtrl", "RightShift", "RightAlt", "RightSuper", "Menu", "0", "1", "2", "3", "4", "5", "6",
    "7", "8", "9", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
    "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "F1", "F2", "F3", "F4", "F5", "F6", "F7",
    "F8", "F9", "F10", "F11", "F12", "Apostrophe", "Comma", "Minus", "Period", "Slash", "Semicolon",
    "Equal", "LeftBracket", "Backslash", "RightBracket", "GraveAccent", "CapsLock", "ScrollLock",
    "NumLock", "PrintScreen", "Pause", "Keypad0", "Keypad1", "Keypad2", "Keypad3", "Keypad4",
    "Keypad5", "Keypad6", "Keypad7", "Keypad8", "Keypad9", "KeypadDecimal", "KeypadDivide",
    "KeypadMultiply", "KeypadSubtract", "KeypadAdd", "KeypadEnter", "KeypadEqual", "GamepadStart",
    "GamepadBack", "GamepadFaceLeft", "GamepadFaceRight", "GamepadFaceUp", "GamepadFaceDown",
    "GamepadDpadLeft", "GamepadDpadRight", "GamepadDpadUp", "GamepadDpadDown", "GamepadL1",
    "GamepadR1", "GamepadL2", "GamepadR2", "GamepadL3", "GamepadR3", "GamepadLStickLeft",
    "GamepadLStickRight", "GamepadLStickUp", "GamepadLStickDown", "GamepadRStickLeft",
    "GamepadRStickRight", "GamepadRStickUp", "GamepadRStickDown", "MouseLeft", "MouseRight",
    "MouseMiddle", "MouseX1", "MouseX2", "MouseWheelX", "MouseWheelY", "ModCtrl", "ModShift",
    "ModAlt", "ModSuper"}};
}

constexpr auto KeyToStrSpecial(ImGuiKey k) -> std::string_view
{
    switch (k) {
    case ImGuiKey_Apostrophe: return "\'";
    case ImGuiKey_Comma: return ",";
    case ImGuiKey_Minus: return "-";
    case ImGuiKey_Slash: return "/";
    case ImGuiKey_Period: return ".";
    case ImGuiKey_Semicolon: return ";";
    case ImGuiKey_Equal: return "=";
    case ImGuiKey_LeftBracket: return "[";
    case ImGuiKey_Backslash: return "\\";
    case ImGuiKey_RightBracket: return "]";
    case ImGuiKey_KeypadDecimal: return "Keypad_Decimal";
    case ImGuiKey_KeypadDivide: return "Keypad_Divide";
    case ImGuiKey_KeypadMultiply: return "Keypad_Multiply";
    case ImGuiKey_KeypadSubtract: return "Keypad_Subtract";
    case ImGuiKey_KeypadAdd: return "Keypad_Add";
    default: return "";
    }
}

constexpr auto KeyFromStrSpecial(std::string_view s) -> ImGuiKey
{
    if (s.size() == 1)
        switch (s[0]) {
        case '\'': return ImGuiKey_Apostrophe;
        case ',': return ImGuiKey_Comma;
        case '-': return ImGuiKey_Minus;
        case '/': return ImGuiKey_Slash;
        case '.': return ImGuiKey_Period;
        case ';': return ImGuiKey_Semicolon;
        case '=': return ImGuiKey_Equal;
        case '[': return ImGuiKey_LeftBracket;
        case '\\': return ImGuiKey_Backslash;
        case ']': return ImGuiKey_RightBracket;
        }
    if (s == "PageDn")
        return ImGuiKey_PageDown;
    else if (s == "Return")
        return ImGuiKey_Enter;
    else if (s == "Keypad_Decimal")
        return ImGuiKey_KeypadDecimal;
    else if (s == "Keypad_Divide")
        return ImGuiKey_KeypadDivide;
    else if (s == "Keypad_Multiply")
        return ImGuiKey_KeypadMultiply;
    else if (s == "Keypad_Subtract")
        return ImGuiKey_KeypadSubtract;
    else if (s == "Keypad_Add")
        return ImGuiKey_KeypadAdd;

    return ImGuiKey_None;
}

enum KeyNameFlags {
    // use default names, Shortcut->"Super"
    KeyNameFlags_Default = 0,

    // Use '/,-/.;=[\ instead of Names
    KeyNameFlags_AllowKeySymbols = 1 << 0,

    // allow unicode symbols depending on host platform (see below)
    KeyNameFlags_AllowModSymbols = 1 << 1,

    // Control -> "Ctrl" / "^"
    // Shift -> "Shift" / "⇧"
    // Alt -> "Option" / "⌥"
    // Super -> "Cmd" / "⌘"
    // Shortcut -> "Cmd" / "⌘" (if ResolveShortcut)
    KeyNameFlags_MacOS = 1 << 2,

    // Control -> "Ctrl"
    // Shift -> "Shift"
    // Alt -> "Option"
    // Super -> "Win" / "⊞"
    // Shortcut -> "Ctrl" (if ResolveShortcut)
    KeyNameFlags_Windows = 1 << 3,
};

constexpr auto ConvertSingleModFlagToKey(
    ImGuiKeyChord key, KeyNameFlags flags = KeyNameFlags{}) -> ImGuiKey
{
    switch (key) {
    case ImGuiMod_Ctrl: return ImGuiKey_ReservedForModCtrl;
    case ImGuiMod_Shift: return ImGuiKey_ReservedForModShift;
    case ImGuiMod_Alt: return ImGuiKey_ReservedForModAlt;
    case ImGuiMod_Super: return ImGuiKey_ReservedForModSuper;
    default: return ImGuiKey_None;
    }
}

constexpr auto IsNamedKey(ImGuiKey key) -> bool
{
    return key >= ImGuiKey_NamedKey_BEGIN && key < ImGuiKey_NamedKey_END;
}

constexpr auto IsKeyNameChar(char c) -> bool { return c >= ' ' && c <= '~' && c != '+'; }

constexpr auto KeyChordToChars(
    char* first, char* last, ImGuiKeyChord kc, KeyNameFlags flags) -> std::to_chars_result
{
    auto ret = std::to_chars_result{first, std::errc{}};

    auto write = [&](std::string_view sv) {
        auto n = sv.size();
        if (ret.ptr + n > last) {
            ret.ec = std::errc::value_too_large;
            return false;
        }
        ret.ptr = std::copy_n(sv.begin(), n, ret.ptr);
        return true;
    };

    if (kc == ImGuiKey_None) {
        write("None");
        return ret;
    }

    auto k = ImGuiKey(kc & ~ImGuiMod_Mask_);
    auto m = ImGuiKeyChord(kc & ImGuiMod_Mask_);

    if (!k && m) {
        k = ConvertSingleModFlagToKey(k, flags);
        m = ImGuiKeyChord(0);
    }

    auto const macos = bool(flags & KeyNameFlags_MacOS);
    auto const windows = bool(flags & KeyNameFlags_Windows);
    auto const keysymbols = bool(flags & KeyNameFlags_AllowKeySymbols);
    auto const modsymbols = bool(flags & KeyNameFlags_AllowModSymbols);

    if (m) {
        if (m & ImGuiMod_Ctrl)
            write((macos & modsymbols) ? "^ " : "Ctrl+");

        if (m & ImGuiMod_Shift)
            write((macos & modsymbols) ? "⇧ " : "Shift+");

        if (m & ImGuiMod_Alt)
            write(macos ? (modsymbols ? "⌥ " : "Option+") : "Alt+");

        if (m & ImGuiMod_Super)
            write(macos     ? (modsymbols ? "⌘ " : "Cmd+")
                  : windows ? (modsymbols ? "⊞+" : "Win+")
                            : "Super+");
    }

    if (!IsNamedKey(k)) {
        write("Unknown");
        return ret;
    }

    if (keysymbols) {
        auto special = KeyToStrSpecial(k);
        if (!special.empty()) {
            write(special);
            return ret;
        }
    }

    write(lookup::keynames[k - ImGuiKey_NamedKey_BEGIN]);
    return ret;
}

inline auto KeyChordToString(ImGuiKeyChord kc, KeyNameFlags flags) -> std::string
{
    char buf[64];
    auto r = KeyChordToChars(buf, buf + 64, kc, flags);
    return std::string{buf, r.ptr};
}

// DefaultKeyNameFlags returns host's default formating for displaying key chords.
//
//   - MacOS:   KeyNameFlags_ResolveShortcut | KeyNameFlags_AllowSymbols | KeyNameFlags_MacOS
//   - Windows: KeyNameFlags_ResolveShortcut | KeyNameFlags_Windows
//   - Others:  KeyNameFlags_ResolveShortcut
//
constexpr auto DefaultKeyNameFlags() -> KeyNameFlags
{
#if defined(IMPLUS_KEYNAMES_MACOS)
    return KeyNameFlags(
        KeyNameFlags_AllowKeySymbols | KeyNameFlags_AllowModSymbols | KeyNameFlags_MacOS);
#elif defined(IMPLUS_KEYNAMES_WINDOWS)
    return KeyNameFlags(KeyNameFlags_AllowKeySymbols | KeyNameFlags_Windows);
#elif defined(IMPLUS_KEYNAMES_LINUX)
    return KeyNameFlags(KeyNameFlags_AllowKeySymbols);
#elif defined(__APPLE__)
    return KeyNameFlags(
        KeyNameFlags_AllowKeySymbols | KeyNameFlags_AllowModSymbols | KeyNameFlags_MacOS);
#elif defined(_WIN32)
    return KeyNameFlags(KeyNameFlags_AllowKeySymbols | KeyNameFlags_Windows);
#else
    return KeyNameFlags(KeyNameFlags_AllowKeySymbols);
#endif
}

inline auto KeyChordToString(ImGuiKeyChord kc) -> std::string
{
    return KeyChordToString(kc, DefaultKeyNameFlags());
}

constexpr auto KeyFromStr(std::string_view s, ImGuiKey& k) -> bool
{
    for (auto i = 0; i < lookup::keynames.size(); ++i)
        if (s == lookup::keynames[i]) {
            k = ImGuiKey(ImGuiKey_NamedKey_BEGIN + i);
            return true;
        }

    if (auto special = KeyFromStrSpecial(s); special != ImGuiKey_None) {
        k = special;
        return true;
    }

    return false;
}

constexpr auto KeyModFromStr(std::string_view s, ImGuiKeyChord& m) -> bool
{
    if (s == "Ctrl" || s == "Control" || s == "^")
        m = ImGuiMod_Ctrl;
    else if (s == "Shift" || s == "⇧")
        m = ImGuiMod_Shift;
    else if (s == "Alt" || s == "Option" || s == "⌥")
        m = ImGuiMod_Alt;
    else if (s == "Super" || s == "Cmd" || s == "Command" || s == "Win" || s == "Windows" ||
             s == "⊞")
        m = ImGuiMod_Super;
    else
        return false;

    return true;
}

constexpr auto KeyModFromChars(
    const char* first, const char* last, ImGuiKeyChord& m) -> std::from_chars_result
{
    auto p = first;
    while (p != last && IsKeyNameChar(*p))
        ++p;

    if (p != last && KeyModFromStr(std::string_view{p, std::size_t(p - first)}, m))
        return std::from_chars_result{p, std::errc{}};
    else
        return std::from_chars_result{first, std::errc::invalid_argument};
}

constexpr auto KeyFromChars(
    const char* first, const char* last, ImGuiKey& k) -> std::from_chars_result
{
    auto p = first;
    while (p != last && IsKeyNameChar(*p))
        ++p;

    if (p != last && KeyFromStr(std::string_view{p, std::size_t(p - first)}, k))
        return std::from_chars_result{p, std::errc{}};
    else
        return std::from_chars_result{first, std::errc::invalid_argument};
}

constexpr auto KeyChordFromChars(
    const char* first, const char* last, ImGuiKeyChord& kc) -> std::from_chars_result
{
    auto curr = first;
    kc = ImGuiKeyChord(0);

    while (true) {
        auto p = curr;
        while (p != last && IsKeyNameChar(*p))
            ++p;

        if (p == curr)
            return std::from_chars_result{first, std::errc::invalid_argument};

        auto s = std::string_view{curr, std::size_t(p - curr)};
        if (p != last) {
            ImGuiKeyChord m;
            if (KeyModFromStr(s, m)) {
                kc = ImGuiKeyChord(kc | m);
                if (*p == '+' || *p == ' ') {
                    curr = p + 1;
                    continue;
                }
                else {
                    return std::from_chars_result{first, std::errc::invalid_argument};
                }
            }
        }

        ImGuiKey k;
        if (KeyFromStr(s, k)) {
            kc = ImGuiKeyChord(kc | k);
            return std::from_chars_result{p, std::errc{}};
        }
        else
            break;
    }
    return std::from_chars_result{first, std::errc::invalid_argument};
}

constexpr auto KeyChordFromStr(std::string_view sv, ImGuiKeyChord& kc) -> bool
{
    auto const first = sv.data();
    auto const last = first + sv.size();
    auto r = KeyChordFromChars(first, last, kc);
    return r.ec == std::errc{} && r.ptr == last;
}

constexpr auto KeyChordFromStr(std::string_view sv) -> ImGuiKeyChord
{
    ImGuiKeyChord k;
    if (!KeyChordFromStr(sv, k))
        return ImGuiKey_None;
    return k;
}

} // namespace ImPlus