#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/selbox.hpp"

#include <algorithm>

namespace ImPlus {

auto SelectableBox(ImID id, char const* name, bool selected, ImGuiSelectableFlags flags,
    ImVec2 size, InteractColorSetCallback on_color,
    Content::DrawCallback draw_callback) -> InteractState
{
    auto state = InteractState{};

    auto window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return state;

    auto& g = *GImGui;
    auto const& style = g.Style;

    auto pos = window->DC.CursorPos;
    pos.y += window->DC.CurrLineTextBaseOffset;
    ImGui::ItemSize(size, 0.0f);

    // Fill horizontal space
    // We don't support (size < 0.0f) in Selectable() because the ItemSpacing
    // extension would make explicitly right-aligned sizes not visibly match
    // other widgets.
    auto const span_all_columns = (flags & ImGuiSelectableFlags_SpanAllColumns) != 0;
    auto const min_x = span_all_columns ? window->ParentWorkRect.Min.x : pos.x;
    auto const max_x = span_all_columns ? window->ParentWorkRect.Max.x : window->WorkRect.Max.x;
    if (span_all_columns || (flags & ImGuiSelectableFlags_SpanAvailWidth))
        size.x = std::max(size.x, max_x - min_x);

    // Text stays at the submission position, but bounding box may be extended
    // on both sides
    auto const content_min = pos;
    auto content_max = ImVec2{min_x + size.x, pos.y + size.y};

    // Selectables are meant to be tightly packed together with no click-gap, so
    // we extend their box to cover spacing between selectable.
    ImRect bb(min_x, pos.y, content_max.x, content_max.y);
    if ((flags & ImGuiSelectableFlags_NoPadWithHalfSpacing) == 0) {
        const float spacing_x = span_all_columns ? 0.0f : style.ItemSpacing.x;
        const float spacing_y = style.ItemSpacing.y;
        const float spacing_L = IM_TRUNC(spacing_x * 0.50f);
        const float spacing_U = IM_TRUNC(spacing_y * 0.50f);
        bb.Min.x -= spacing_L;
        bb.Min.y -= spacing_U;
        bb.Max.x += (spacing_x - spacing_L);
        bb.Max.y += (spacing_y - spacing_U);
    }

    if (span_all_columns) {
        content_max.x = window->WorkRect.Max.x;
    }

    // Modify ClipRect for the ItemAdd(), faster than doing a
    // PushColumnsBackground/PushTableBackground for every Selectable..
    const float backup_clip_rect_min_x = window->ClipRect.Min.x;
    const float backup_clip_rect_max_x = window->ClipRect.Max.x;
    if (span_all_columns) {
        window->ClipRect.Min.x = window->ParentWorkRect.Min.x;
        window->ClipRect.Max.x = window->ParentWorkRect.Max.x;
    }

    bool item_add;
    const bool disabled_item = (flags & ImGuiSelectableFlags_Disabled) != 0;
    if (disabled_item) {
        ImGuiItemFlags backup_item_flags = g.CurrentItemFlags;
        g.CurrentItemFlags |= ImGuiItemFlags_Disabled;
        item_add = ImGui::ItemAdd(bb, id);
        g.CurrentItemFlags = backup_item_flags;
    }
    else {
        item_add = ImGui::ItemAdd(bb, id);
    }

    if (span_all_columns) {
        window->ClipRect.Min.x = backup_clip_rect_min_x;
        window->ClipRect.Max.x = backup_clip_rect_max_x;
    }

    if (!item_add)
        return state;

    const bool disabled_global = (g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
    if (disabled_item && !disabled_global)
        ImGui::BeginDisabled();

    // FIXME: We can standardize the behavior of those two, we could also keep
    // the fast path of override ClipRect + full push on render only, which
    // would be advantageous since most selectable are not selected.
    if (span_all_columns && window->DC.CurrentColumns)
        ImGui::PushColumnsBackground();
    else if (span_all_columns && g.CurrentTable)
        ImGui::TablePushBackgroundChannel();

    // We use NoHoldingActiveID on menus so user can click and _hold_ on a menu
    // then drag to browse child entries
    ImGuiButtonFlags button_flags = 0;
    if (flags & ImGuiSelectableFlags_NoHoldingActiveID)
        button_flags |= ImGuiButtonFlags_NoHoldingActiveId;
    if (flags & ImGuiSelectableFlags_SelectOnClick)
        button_flags |= ImGuiButtonFlags_PressedOnClick;
    if (flags & ImGuiSelectableFlags_SelectOnRelease)
        button_flags |= ImGuiButtonFlags_PressedOnRelease;
    if (flags & ImGuiSelectableFlags_AllowDoubleClick)
        button_flags |=
            ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
    if (flags & ImGuiSelectableFlags_AllowOverlap)
        button_flags |= ImGuiButtonFlags_AllowOverlap;

    auto const was_selected = selected;
    state.Pressed = ImGui::ButtonBehavior(bb, id, &state.Hovered, &state.Held, button_flags);

    // Auto-select when moved into
    //
    // - This will be more fully fleshed in the range-select branch
    //
    // - This is not exposed as it won't nicely work with some user side
    // handling of shift/control
    //
    // - We cannot do 'if (g.NavJustMovedToId != id) { selected=false;
    // pressed=was_selected; }' for two reasons
    //
    //   - (1) it would require focus scope to be set, need exposing
    //   PushFocusScope() or equivalent (e.g. BeginSelection() calling
    //   PushFocusScope())
    //
    //   - (2) usage will fail with clipped items
    //   The multi-select API aim to fix those issues, e.g. may be replaced with
    //   a BeginSelection() API.
    //
    if ((flags & ImGuiSelectableFlags_SelectOnNav) && g.NavJustMovedToId != 0 &&
        g.NavJustMovedToFocusScopeId == g.CurrentFocusScopeId)
        if (g.NavJustMovedToId == id) {
            selected = true;
            state.Pressed = true;
        }

    // Update NavId when clicking or when Hovering (this doesn't happen on most
    // widgets), so navigation can be resumed with gamepad/keyboard
    if (state.Pressed || (state.Hovered && (flags & ImGuiSelectableFlags_SetNavIdOnHover))) {
        if (!g.NavDisableMouseHover && g.NavWindow == window &&
            g.NavLayer == window->DC.NavLayerCurrent) {
            ImGui::SetNavID(id, window->DC.NavLayerCurrent, g.CurrentFocusScopeId,
                ImGui::WindowRectAbsToRel(window, bb));
            g.NavDisableHighlight = true;
        }
    }
    if (state.Pressed)
        ImGui::MarkItemEdited(id);

    // In this branch, Selectable() cannot toggle the selection so this will
    // never trigger.
    if (selected != was_selected) //-V547
        g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

    // delay color appearance by one frame to avoid visual jitter when
    // item's selection changes as a result of the mouse being released
    auto disp_state = state;
    if (state.Hovered && !state.Held && ImGui::IsMouseReleased(ImGuiMouseButton_Left, id))
        disp_state.Held = true;

    // Render
    if (!NeedsHoverHighlight() && !disp_state.Held && !disp_state.Pressed)
        disp_state.Hovered = false;

    auto color_set = on_color   ? on_color(disp_state)
                     : selected ? ColorSets_SelectedSelectable(disp_state)
                                : ColorSets_RegularSelectable(disp_state);

    auto frame = bb;
    if (flags & ImGuiSelectableFlags_ExtendFrameHorz) {
        frame.Min.x = window->OuterRectClipped.Min.x;
        frame.Max.x = window->OuterRectClipped.Max.x;
    }
    if (flags & ImGuiSelectableFlags_ExtendFrameVert) {
        frame.Min.y = window->OuterRectClipped.Min.y;
        frame.Max.y = window->OuterRectClipped.Max.y;
    }

    ImGui::RenderFrame(frame.Min, frame.Max, ImGui::GetColorU32(color_set.Background), false, 0.0f);

    ImGui::RenderNavHighlight(
        bb, id, ImGuiNavHighlightFlags_Compact | ImGuiNavHighlightFlags_NoRounding);

    if (span_all_columns && window->DC.CurrentColumns)
        ImGui::PopColumnsBackground();
    else if (span_all_columns && g.CurrentTable)
        ImGui::TablePopBackgroundChannel();

    if (draw_callback && window->ClipRect.Overlaps({content_min, content_max}))
        draw_callback(window->DrawList, content_min, content_max, color_set);

    // Automatically close popups
    if (state.Pressed && (window->Flags & ImGuiWindowFlags_Popup) &&
        !(flags & ImGuiSelectableFlags_NoAutoClosePopups) &&
        (g.LastItemData.ItemFlags & ImGuiItemFlags_AutoClosePopups))
        ImGui::CloseCurrentPopup();

    if (disabled_item && !disabled_global)
        ImGui::EndDisabled();

    IMGUI_TEST_ENGINE_ITEM_INFO(id, name, window->DC.LastItemStatusFlags);

    return state;
}

} // namespace ImPlus