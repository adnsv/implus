#pragma once

#include <functional>
#include <imgui.h>
#include <implus/blocks.hpp>
#include <implus/content.hpp>
#include <implus/icd.hpp>
#include <implus/id.hpp>
#include <implus/interact.hpp>
#include <string_view>

const auto ImGuiSelectableFlags_ExtendFrameHorz = ImGuiSelectableFlags(1 << 29);
const auto ImGuiSelectableFlags_ExtendFrameVert = ImGuiSelectableFlags(1 << 30);

namespace ImPlus {

// note: name is used only for test_engine
auto SelectableBox(ImID id, char const* name, bool selected, ImGuiSelectableFlags flags,
    ImVec2 size, InteractColorSetCallback on_color,
    Content::DrawCallback draw_callback) -> InteractState;

inline auto Selectable(ImID id, char const* name, bool selected, ImGuiSelectableFlags flags,
    ImPlus::IconBlock const& v) -> InteractState
{
    return SelectableBox(id, name, selected, flags, v.Size, {},
        [&](ImDrawList* dl, ImVec2 const& tl, ImVec2 const& br, ImPlus::ColorSet const& cs) {
            v.RenderXY(dl, tl, cs.Content);
        });
}

inline auto Selectable(ImID id, char const* name, bool selected, ImGuiSelectableFlags flags,
    ImPlus::TextBlock const& v) -> InteractState
{
    return SelectableBox(id, name, selected, flags, v.Size, {},
        [&](ImDrawList* dl, ImVec2 const& tl, ImVec2 const& br, ImPlus::ColorSet const& cs) {
            v.Render(dl, tl, br, cs.Content);
        });
}

inline auto Selectable(ImID id, char const* name, bool selected, ImGuiSelectableFlags flags,
    ImPlus::ICDBlock const& v) -> InteractState
{
    return SelectableBox(id, name, selected, flags, v.Size, {},
        [&](ImDrawList* dl, ImVec2 const& tl, ImVec2 const& br, ImPlus::ColorSet const& cs) {
            v.Render(dl, tl, br, cs.Content);
        });
}

} // namespace ImPlus