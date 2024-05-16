#pragma once

#include <imgui.h>

#include "implus/buttonbar.hpp"
#include "implus/length.hpp"
#include "implus/overridable.hpp"

#include <any>
#include <variant>

namespace ImPlus::Style::Dialog {

inline auto FlatTitle = overridable<bool>(true);
inline auto ContentPadding = overridable<length>(1_em);

inline auto VerticalButtonWidth = overridable<length>(6_em);
inline auto HorizontalButtonHeight =
    overridable<std::optional<length>>{}; // defaults to ImGui::GetFrameHeight()

} // namespace ImPlus::Style::Dialog

namespace ImPlus::Dlg {

enum class Flags {
    NoButtonArea = 1 << 0,       // do not reserve area for button placement
    VerticalButtonArea = 1 << 1, // placeme buttons vertically on the right hand side
    NoScrollbar = 1 << 2,
    HorizontalScrollbar = 1 << 3,
    Resizable = 1 << 4,
};
constexpr auto operator|(Flags a, Flags b) -> Flags { return Flags(unsigned(a) | unsigned(b)); };
constexpr auto operator&(Flags a, Flags b) -> Flags { return Flags(unsigned(a) & unsigned(b)); };
constexpr auto operator&&(Flags a, Flags b) -> bool { return bool(a) && bool(b); };
constexpr auto operator&&(Flags a, bool b) -> bool { return bool(a) && b; };
constexpr auto operator&&(bool a, Flags b) -> bool { return a && bool(b); };

struct Options {
    std::string Title;
    Dlg::Flags Flags = {};
    length MinimumContentWidth = 4_em;
    length MinimumContentHeight = 2_em;
    length StretchContentWidth = 10_em;

    length InitialWidth = 0_pt;
    length InitialHeight = 0_pt;
};

auto Begin(ImGuiID id, Options const& opts) -> bool;
void End();
void KeepOpen(bool on = true);
inline void Close() { KeepOpen(false); }
auto IsClosing() -> bool;
auto IsAutosizing() -> bool;
auto InDialogWindow(bool popup_hierarchy = false) -> bool;

void AllowDefaultAction();
void ForbidDefaultAction();
auto DefaultActionAllowed() -> bool;

enum class InputFieldFlags {
    DefaultFocus = 1 << 0,
    DefaultAction = 1 << 1,
};
constexpr auto operator|(InputFieldFlags a, InputFieldFlags b) -> InputFieldFlags
{
    return InputFieldFlags(unsigned(a) | unsigned(b));
};
constexpr auto operator&(InputFieldFlags a, InputFieldFlags b) -> InputFieldFlags
{
    return InputFieldFlags(unsigned(a) & unsigned(b));
};
constexpr auto operator&&(InputFieldFlags a, InputFieldFlags b) -> bool
{
    return bool(a) && bool(b);
};
constexpr auto operator&&(InputFieldFlags a, bool b) -> bool { return bool(a) && b; };
constexpr auto operator&&(bool a, InputFieldFlags b) -> bool { return a && bool(b); };

void ConfigureNextInputField(InputFieldFlags flags);

// HandleInputField is a helper function that needs to be called after text-input-like fields
// to improve navigation experience.
// Note: HandleInputField is automatically called for ImPlus::** inputs, call it only if using stock
// ImGui input fields.
void HandleInputField();

namespace internal {
auto CurrentFlags() -> Flags;
auto GetButtonSize() -> ImVec2;
} // namespace internal

void DoneContentArea();

// Buttons renders dialog buttons, also captures default action and close/dismiss actions.
template <typename T>
requires std::is_convertible_v<T, std::string_view>
auto Buttons(std::initializer_list<T> items, float align, Buttonbar::Flags flags)
    -> std::optional<std::size_t>
{
    DoneContentArea();
    auto const vertical = (internal::CurrentFlags() & Flags::VerticalButtonArea) != Flags{};
    auto const align_xy = vertical ? ImVec2{0.0f, align} : ImVec2{align, 0.0f};
    auto const allow_highlight = DefaultActionAllowed();

    Style::Buttonbar::ItemStacking.push(vertical ? Axis::Vert : Axis::Horz);
    auto const sz = internal::GetButtonSize();
    Style::Buttonbar::ForceItemWidth.push(sz.x ? std::make_optional(sz.x) : std::nullopt);
    Style::Buttonbar::ForceItemHeight.push(sz.y ? std::make_optional(sz.y) : std::nullopt);
    Style::Buttonbar::HighlightDefault.push(allow_highlight);

    auto ret = Buttonbar::Display("##.DLG.BUTTONBAR.", items, align_xy, flags);

    Style::Buttonbar::ForceItemWidth.pop();
    Style::Buttonbar::ForceItemHeight.pop();
    Style::Buttonbar::ItemStacking.pop();
    Style::Buttonbar::HighlightDefault.pop();

    if (IsClosing())
        ret = Buttonbar::CancelSentinel;
    else if (ret && *ret == Buttonbar::CancelSentinel)
        KeepOpen(false);

    return ret;
}

template <typename T>
requires std::is_convertible_v<T, std::string_view>
auto Buttons(std::initializer_list<T> items, Buttonbar::Flags flags) -> std::optional<std::size_t>
{
    auto const vertical = (internal::CurrentFlags() & Flags::VerticalButtonArea) != Flags{};
    auto const align = vertical ? 0.0f : 0.5f;
    return Buttons(items, align, flags);
}

// Functions to assist with keyboard navigation.
auto IsRejectActionKeyPressed() -> bool;
auto IsAcceptActionKeyPressed() -> bool;

// DbgDisplayNavInfo displays information helpful for debugging navigation within dialogs.
void DbgDisplayNavInfo();

namespace internal {
void Initialize();
} // namespace internal

// HasModality indicates whether there is a modal box currently active.
auto HasModality() -> bool;

// OpenModality opens a new modal box. This is the primary way of dealing
// with modal boxes in ImPlus.
void OpenModality(ImGuiID id, Options&& opts, std::function<void()>&& on_display);

} // namespace ImPlus::Dlg