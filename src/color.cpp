#include "implus/color.hpp"
#include <imgui_internal.h>

namespace ImPlus::Color {

auto FromBackgroundStyle(ImGuiWindow const* window) -> ImVec4
{
    if (!window)
        return {};

    if (window->Flags & (ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup))
        return ImGui::GetStyleColorVec4(ImGuiCol_PopupBg);

    if (window->Flags & ImGuiWindowFlags_ChildWindow) {
        auto c = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
        if (c.w)
            return c;
        else
            return ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    }
    else
        return ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
}

auto FromBackgroundStyle() -> ImVec4 { return FromBackgroundStyle(ImGui::GetCurrentWindowRead()); }

auto FromStyle(ImGuiCol style_color_idx) -> ImVec4
{
    return ImGui::GetStyleColorVec4(style_color_idx);
}

} // namespace ImPlus::Color

namespace ImPlus {

template <ImGuiCol_ RegularID, ImGuiCol_ HoveredID, ImGuiCol_ ActiveID>
constexpr auto pick_interact_color_id(InteractState const& state) -> ImGuiCol_
{
    return (state.Held && state.Hovered) ? ActiveID : state.Hovered ? HoveredID : RegularID;
}

// InteractColor

template <ImGuiCol_ RegularID, ImGuiCol_ HoveredID, ImGuiCol_ ActiveID>
inline auto ic_(InteractState const& state) -> ImVec4
{
    return ImGui::GetStyleColorVec4(pick_interact_color_id<RegularID, ActiveID, HoveredID>(state));
}

InteractColorCallback Colors_Separator =
    ic_<ImGuiCol_Separator, ImGuiCol_SeparatorHovered, ImGuiCol_SeparatorActive>;

InteractColorCallback Colors_RegularButton_Background =
    ic_<ImGuiCol_Button, ImGuiCol_ButtonHovered, ImGuiCol_ButtonActive>;

InteractColorCallback Colors_Frame_Background =
    ic_<ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive>;

InteractColorCallback Colors_Tab_Background =
    ic_<ImGuiCol_Tab, ImGuiCol_TabHovered, ImGuiCol_TabSelected>;

InteractColorCallback Colors_Header_Background =
    ic_<ImGuiCol_Header, ImGuiCol_HeaderHovered, ImGuiCol_HeaderActive>;

// InteractColorSet

template <ImGuiCol_ ContentID, ImGuiCol_ RegularID, ImGuiCol_ HoveredID, ImGuiCol_ ActiveID>
constexpr auto ics_(InteractState const& state) -> ColorSet
{
    return ColorSet{
        .Content = Color::FromStyle(ContentID),
        .Background =
            Color::FromStyle(pick_interact_color_id<RegularID, HoveredID, ActiveID>(state)),
    };
}

InteractColorSetCallback ColorSets_RegularButton =
    ics_<ImGuiCol_Text, ImGuiCol_Button, ImGuiCol_ButtonHovered, ImGuiCol_ButtonActive>;

InteractColorSetCallback ColorSets_Frame =
    ics_<ImGuiCol_Text, ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive>;

InteractColorSetCallback ColorSets_Tab =
    ics_<ImGuiCol_Text, ImGuiCol_Tab, ImGuiCol_TabHovered, ImGuiCol_TabSelected>;

InteractColorSetCallback ColorSets_MenuItem = [](InteractState const& state) -> ColorSet {
    return ColorSet{
        .Content = Color::FromStyle(ImGuiCol_Text),
        .Background = state.Hovered && state.Held ? Color::FromStyle(ImGuiCol_HeaderActive)
                      : state.Hovered             ? Color::FromStyle(ImGuiCol_HeaderHovered)
                                                  : ImVec4{0, 0, 0, 0},
    };
};

InteractColorSetCallback ColorSets_RegularSelectable = [](InteractState const& state) -> ColorSet {
    return ColorSet{
        .Content = Color::FromStyle(ImGuiCol_Text),
        .Background = state.Hovered && state.Held
                          ? Color::ModulateAlpha(Color::FromStyle(ImGuiCol_Text), 0.15f)
                      : state.Hovered ? Color::ModulateAlpha(Color::FromStyle(ImGuiCol_Text), 0.1f)
                                      : ImVec4{0, 0, 0, 0},
    };
};

InteractColorSetCallback ColorSets_SelectedSelectable = [](InteractState const& state) -> ColorSet {
    return ColorSet{
        .Content = Color::FromStyle(ImGuiCol_Text),
        .Background = state.Hovered && state.Held ? Color::FromStyle(ImGuiCol_HeaderActive)
                      : state.Hovered             ? Color::FromStyle(ImGuiCol_HeaderHovered)
                                                  : Color::FromStyle(ImGuiCol_Header),
    };
};

} // namespace ImPlus