#pragma once

#include "interact.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <imgui.h>
#include <optional>
#include <variant>

namespace ImPlus {

// ColorSet
struct ColorSet {
    ImVec4 Content = {};
    ImVec4 Background = {};
};

namespace internal {

template <ImGuiCol ContentID, ImGuiCol BackgroundID> constexpr auto GetStyleColorSet() -> ColorSet
{
    return ColorSet{
        .Content = ImGui::GetStyleColorVec4(ContentID),
        .Background = ImGui::GetStyleColorVec4(BackgroundID),
    };
}

} // namespace internal

inline auto GetStyleWindowColors = internal::GetStyleColorSet<ImGuiCol_Text, ImGuiCol_WindowBg>;
inline auto GetStylePopupColors = internal::GetStyleColorSet<ImGuiCol_Text, ImGuiCol_PopupBg>;
inline auto GetStyleChildColors = internal::GetStyleColorSet<ImGuiCol_Text, ImGuiCol_ChildBg>;
inline auto GetStyleMenuBarColors = internal::GetStyleColorSet<ImGuiCol_Text, ImGuiCol_MenuBarBg>;
inline auto GetStyleScrollbarColors =
    internal::GetStyleColorSet<ImGuiCol_Text, ImGuiCol_ScrollbarBg>;

// InteractColorSet allows to pick a color from InteractState
using InteractColorCallback = std::function<auto(InteractState const&)->ImVec4>;
extern InteractColorCallback Colors_Separator;
extern InteractColorCallback Colors_RegularButton_Background;


// InteractColorSet allows to pick Fg/Bg colorset from InteractState
using InteractColorSetCallback = std::function<auto(InteractState const&)->ColorSet>;

extern InteractColorSetCallback ColorSets_RegularButton;
extern InteractColorSetCallback ColorSets_Frame;
extern InteractColorSetCallback ColorSets_Tab;

extern InteractColorSetCallback ColorSets_RegularSelectable;
extern InteractColorSetCallback ColorSets_SelectedSelectable;

namespace Color {

// FromBackgroundStyle - tries to get the current window background
// depending on whether the current window is floating, child, or popup, it will
// pull value from ImGuiCol_WindowBg, ImGuiCol_ChildBg, ImGuiCol_PopupBg styles.
//
// Note: if ImGuiCol_ChildBg is completely transparent, it return the value
// of ImGuiCol_WindowBg.
//
auto FromBackgroundStyle() -> ImVec4; // pulls window background

// FromStyle - an alias for ImGui::GetStyleColorVec4
auto FromStyle(ImGuiCol style_color_idx) -> ImVec4;

template <ImGuiCol Sty> auto FromStyle() -> ImVec4 { return ImGui::GetStyleColorVec4(Sty); }

inline auto Intensity(ImVec4 const& c) -> float
{
    return 0.2989f * c.x + 0.587f * c.y + 0.114f * c.z;
}

inline auto IsDark(float intensity) -> bool { return intensity < 0.6f; }
inline auto IsDark(ImVec4 const& c) -> bool { return IsDark(Intensity(c)); }

inline auto IsOpaque(ImVec4 const& c, float tolerance = 0.0f) -> bool
{
    return c.w >= 1.0f - tolerance;
}
inline auto IsEmpty(ImVec4 const& c, float tolerance = 0.0f) -> bool { return c.w <= tolerance; }

[[nodiscard]] inline auto Desaturate(ImVec4 const& c) -> ImVec4
{
    auto i = Intensity(c);
    return {i, i, i, c.w};
}
[[nodiscard]] inline auto Desaturate(ImVec4 const& c, float amount) -> ImVec4
{
    auto i = Intensity(c) * amount;
    auto ramount = 1.0f - amount;
    return {i + c.x * ramount, i + c.y * ramount, i + c.z * ramount, c.w};
}

[[nodiscard]] inline auto ContrastTo(ImVec4 const& c) -> ImVec4
{
    return IsDark(c) ? ImVec4{1, 1, 1, 1} : ImVec4{0, 0, 0, 1};
}

[[nodiscard]] inline auto Mix(ImVec4 c1, float w1, ImVec4 c2, float w2) -> ImVec4
{
    c1.w *= w1;
    c2.w *= w2;
    if (c1.w <= 0)
        return c2;
    if (c2.w <= 0)
        return c1;
    auto w = w1 + w2;
    auto wx = c1.x * c1.w + c2.x * c2.w;
    auto wy = c1.y * c1.w + c2.y * c2.w;
    auto wz = c1.z * c1.w + c2.z * c2.w;
    return {std::clamp(wx / w, 0.0f, 1.0f), std::clamp(wy / w, 0.0f, 1.0f),
        std::clamp(wz / w, 0.0f, 1.0f), w};
}

// Modulate makes color brighter (for positive amount values) or darker (for
// negative amount values)
[[nodiscard]] inline auto Modulate(ImVec4 const& c, float amount) -> ImVec4
{
    if (!amount)
        return c;
    amount = std::clamp(amount, -1.0f, 1.0f);
    auto w2 = std::min(std::abs(amount), 1.0f);
    auto w1 = 1.0f - w2;
    return {std::clamp(amount + w1 * c.x + w2, 0.0f, 1.0f),
        std::clamp(amount + w1 * c.y + w2, 0.0f, 1.0f),
        std::clamp(amount + w1 * c.z + w2, 0.0f, 1.0f), c.w};
}

// Enhance makes bright colors brighter and dark colors darker. When amount is
// negative, its effect is refersed.
//
// When input color is completely or partially transparent, this function
// mixes in white or black at certain proportion to keep the effect more
// prominent.
//
[[nodiscard]] inline auto Enhance(ImVec4 c, float amount) -> ImVec4
{
    auto dark = IsDark(c);
    auto effective_amount = dark ? -amount : +amount;
    c = Modulate(c, effective_amount);
    auto op = c.w;
    if (op >= 1.0f)
        return c;
    auto s = amount >= 0 ? ImVec4{1, 1, 1, effective_amount} : ImVec4{0, 0, 0, -effective_amount};
    if (op <= 0)
        return s;
    return Mix(c, op, s, 1 - op);
}

[[nodiscard]] inline auto ModulateAlpha(ImVec4 const& c, float amount) -> ImVec4
{
    return {c.x, c.y, c.z, std::clamp(c.w * amount, 0.0f, 1.0f)};
}

[[nodiscard]] inline auto EnhanceBackground(float amount) -> ImVec4
{
    return Color::Enhance(Color::FromBackgroundStyle(), amount);
}

namespace Func {

using callback = std::function<ImVec4()>;

inline auto FromBackgroundStyle() -> callback
{
    return [] { return ImPlus::Color::FromBackgroundStyle(); };
}

inline auto FromStyle(ImGuiCol style_color_idx) -> callback
{
    return [style_color_idx] { return Color::FromStyle(style_color_idx); };
}

inline auto DesaturateBackground(float amount) -> callback
{
    return [amount] { return Color::Desaturate(Color::FromBackgroundStyle(), amount); };
}

inline auto Desaturate(ImGuiCol style_color_idx, float amount) -> callback
{
    return [style_color_idx, amount] {
        return Color::Desaturate(Color::FromStyle(style_color_idx), amount);
    };
}

inline auto ModulateBackground(float amount) -> callback
{
    return [amount] { return Color::Modulate(Color::FromBackgroundStyle(), amount); };
}

inline auto Modulate(ImGuiCol style_color_idx, float amount) -> callback
{
    return [style_color_idx, amount] {
        return Color::Modulate(Color::FromStyle(style_color_idx), amount);
    };
}

inline auto EnhanceBackground(float amount) -> callback
{
    return [amount] { return Color::EnhanceBackground(amount); };
}

inline auto Enhance(ImGuiCol style_color_idx, float amount) -> callback
{
    return [style_color_idx, amount] {
        return Color::Enhance(Color::FromStyle(style_color_idx), amount);
    };
}

} // namespace Func

} // namespace Color

using ColorSpec = std::variant<Color::Func::callback, ImVec4, ImGuiCol>;
inline auto GetColor(ColorSpec const& spec) -> std::optional<ImVec4>
{
    if (auto v = std::get_if<ImVec4>(&spec))
        return *v;
    else if (auto v = std::get_if<ImGuiCol>(&spec))
        return ImGui::GetStyleColorVec4(*v);
    else if (auto v = std::get_if<Color::Func::callback>(&spec)) {
        auto& functor = *v;
        if (functor)
            return functor();
    }
    return {};
}

} // namespace ImPlus