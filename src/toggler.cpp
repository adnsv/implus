#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/toggler.hpp"

#include "implus/length.hpp"
#include <cmath>
#include <vector>

namespace ImPlus {

inline auto width_of(std::string_view s) -> float
{
    return ImGui::CalcTextSize(s.data(), s.data() + s.size()).x;
}

auto Measure::Toggler(TogglerOptions const& opts) -> ImVec2
{
    auto const& style = ImGui::GetStyle();

    ImVec2 sz;
    sz.y = ImGui::GetFrameHeight();
    sz.x = std::round(sz.y * 2);

    auto caption_width = 0.0f;
    auto have_captions = !opts.ActiveCaption.empty() || !opts.InactiveCaption.empty();
    if (have_captions) {
        auto w1 = width_of(opts.InactiveCaption) + style.ItemInnerSpacing.x;
        auto w2 = width_of(opts.ActiveCaption) + style.ItemInnerSpacing.x;
        caption_width = std::max(w1, w2);
    }

    sz.x += caption_width;
    return sz;
};

auto Toggler(ImID id, bool& active, TogglerOptions const& opts) -> bool
{
    auto const& g = *GImGui;
    auto const* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    auto const& style = g.Style;

    ImVec2 sz;
    sz.y = ImGui::GetFrameHeight();
    sz.x = std::round(sz.y * 2);
    auto radius = sz.y * 0.5f;

    auto caption_width = 0.0f;
    auto have_captions = !opts.ActiveCaption.empty() || !opts.InactiveCaption.empty();
    if (have_captions) {
        auto w1 = width_of(opts.InactiveCaption) + style.ItemInnerSpacing.x;
        auto w2 = width_of(opts.ActiveCaption) + style.ItemInnerSpacing.x;
        caption_width = std::max(w1, w2);
    }
    sz.x += caption_width;

    auto pos = window->DC.CursorPos;
    auto bb = ImRect{pos, pos + sz};
    ImGui::ItemSize(sz, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return {};

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(
        bb, id, &hovered, &held, ImGuiButtonFlags_PressedOnClickReleaseAnywhere);

    ImGui::RenderNavHighlight(bb, id);
    auto dl = window->DrawList;

    auto text_clr32 = ImGui::GetColorU32(ImGuiCol_Text);

    if (have_captions) {
        auto s = active ? opts.ActiveCaption : opts.InactiveCaption;
        if (!s.empty()) {
            auto x = bb.Min.x;
            auto y = bb.Min.y + style.FramePadding.y;
            dl->AddText(ImVec2{x, y}, text_clr32, s.data(), s.data() + s.size());
        }
        bb.Min.x += caption_width;
    }

    auto enabled = !(g.CurrentItemFlags & ImGuiItemFlags_Disabled);

    auto bg_clr =
        active && enabled ? ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive) : ImVec4{0, 0, 0, 0};
    auto frame_clr =
        active && enabled ? ImVec4{0, 0, 0, 0} : ImGui::GetStyleColorVec4(ImGuiCol_Text);
    auto dot_clr = active && enabled ? ImGui::GetStyleColorVec4(ImGuiCol_Text)
                                     : ImGui::GetStyleColorVec4(ImGuiCol_Text);

    auto frame_thickness = ImPlus::to_pt(1.5_pt);

    if (bg_clr.w > 0)
        dl->AddRectFilled(bb.Min, bb.Max, ImGui::ColorConvertFloat4ToU32(bg_clr), radius);

    if (frame_clr.w > 0) {
        auto r = bb;
        r.Expand({-frame_thickness * 0.5f, -frame_thickness * 0.5f});
        dl->AddRect(
            r.Min, r.Max, ImGui::ColorConvertFloat4ToU32(frame_clr), radius, 0, frame_thickness);
    }

    auto inner_box = bb;
    auto inner_radius = std::round((radius - frame_thickness) * 0.75f);
    inner_box.Expand({inner_radius - radius, inner_radius - radius});

    auto dot_box = inner_box;
    auto dot_w = dot_box.GetHeight();

    auto& io = ImGui::GetIO();

    auto owns_mouse = ImGui::GetKeyOwner(ImGui::MouseButtonToKey(ImGuiMouseButton_Left)) == id;

    if (held && owns_mouse) {
        auto dt = 0.1f;
        auto t = io.MouseDownDuration[ImGuiMouseButton_Left] / dt;
        if (t > 1.0f)
            t = 1.0f;
        dot_w = std::round(dot_w * (1.0f + t * 0.25f));
    }
    auto dot_lo = dot_box.Min.x;
    auto dot_hi = dot_box.Max.x - dot_w;
    auto dot_x = dot_box.Min.x;
    if (active)
        dot_x = dot_box.Max.x - dot_w;

    if (held && owns_mouse && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        auto dx = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x;
        dot_x += dx;
        if (dot_x < dot_lo)
            dot_x = dot_lo;
        else if (dot_x > dot_hi)
            dot_x = dot_hi;
    }

    dl->AddRectFilled({dot_x, dot_box.Min.y}, {dot_x + dot_w, dot_box.Max.y},
        ImGui::ColorConvertFloat4ToU32(dot_clr), radius);

    if (pressed) {
        if (owns_mouse && ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Left)) {
            auto dx = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x;
            auto dot_x = dot_box.Min.x;
            if (active)
                dot_x = dot_box.Max.x - dot_w;
            dot_x += dx;
            auto want_active = dot_x * 2.0f > dot_lo + dot_hi;
            if (want_active != active) {
                ImGui::MarkItemEdited(id);
                active = want_active;
                return true;
            }
        }
        else {
            active = !active;
            ImGui::MarkItemEdited(id);
            return true;
        }
    }

    return false;
}

struct interval {
    float lo;
    float hi;
    float dx;
};

auto Measure::MultiToggler(std::initializer_list<std::string_view> items) -> ImVec2
{
    auto const& style = ImGui::GetStyle();

    auto const padding = style.ItemSpacing.x;
    auto sz = ImVec2{0, ImGui::GetFrameHeight()};

    auto radius = sz.y * 0.5f;
    auto const min_item_width = sz.y;

    auto x = 0.0f;
    for (auto&& s : items) {
        auto const text_w = width_of(s);
        auto const adj_w = std::max(text_w, min_item_width);
        auto const w = adj_w + padding * 2.0f;
        x += w;
    }
    sz.x = x;
    return sz;
};

auto MultiToggler(ImID id, std::initializer_list<std::string_view> items, std::size_t& sel_index)
    -> bool
{
    auto const& g = *GImGui;
    auto const* window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    auto const& style = g.Style;

    auto intervals = std::vector<interval>{};
    intervals.reserve(items.size());

    auto const padding = style.ItemSpacing.x;
    auto sz = ImVec2{0, ImGui::GetFrameHeight()};
    auto radius = sz.y * 0.5f;
    auto const min_item_width = sz.y;

    auto x = 0.0f;
    for (auto&& s : items) {
        auto const text_w = width_of(s);
        auto const adj_w = std::max(text_w, min_item_width);
        auto const w = adj_w + padding * 2.0f;
        intervals.push_back(interval{x, x + w, padding + (adj_w - text_w) * 0.5f});
        x += w;
    }
    sz.x = x;

    auto pos = window->DC.CursorPos;
    auto bb = ImRect{pos, pos + sz};
    ImGui::ItemSize(sz, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return {};

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    ImGui::RenderNavHighlight(bb, id);

    auto dl = window->DrawList;
    auto text_clr32 = ImGui::GetColorU32(ImGuiCol_Text);
    auto line_clr32 = ImGui::GetColorU32(ImGuiCol_Text, 0.15f);
    auto fill_clr32 = ImGui::GetColorU32(ImGuiCol_FrameBg);
    auto hovered_clr32 = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
    auto active_clr32 = ImGui::GetColorU32(ImGuiCol_ButtonActive);

    auto const thumb_range = interval{bb.Min.x, bb.Max.x};
    auto thumb = thumb_range;
    if (sel_index < intervals.size()) {
        thumb = intervals[sel_index];
    }

    auto owns_mouse = ImGui::GetKeyOwner(ImGui::MouseButtonToKey(ImGuiMouseButton_Left)) == id;
    auto real_hovered = hovered && ImGui::IsItemHovered(ImGuiHoveredFlags_NoNavOverride);

    auto hot_index = -1;
    if (real_hovered) {
        auto x = ImGui::GetMousePos().x;
        if (x <= bb.Min.x)
            hot_index = 0;
        else if (x >= bb.Max.x)
            hot_index = intervals.size() - 1;
        else {
            x -= bb.Min.x;
            for (std::size_t i = 0; i < intervals.size(); ++i) {
                if (x < intervals[i].hi) {
                    hot_index = i;
                    break;
                }
            }
        }
    }

    {
        auto d = std::round(g.Style.FramePadding.y * 0.25f);
        auto offset = ImVec2{d, d};
        auto r = std::max(0.0f, radius - d);
        dl->AddRectFilled(bb.Min + offset, bb.Max - offset, fill_clr32, r);
    }

    {
        if (hot_index >= 0 && hot_index != sel_index) {
            auto offset = ImVec2{ImPlus::to_pt(2.5_pt), 0};
            offset.y = offset.x;
            auto hot_thumb = intervals[hot_index];
            dl->AddRectFilled(ImVec2{bb.Min.x + hot_thumb.lo, bb.Min.y} + offset,
                ImVec2{bb.Min.x + hot_thumb.hi, bb.Max.y} - offset, hovered_clr32, radius);
        }

        dl->AddRectFilled(ImVec2{bb.Min.x + thumb.lo, bb.Min.y},
            ImVec2{bb.Min.x + thumb.hi, bb.Max.y}, active_clr32, radius);
    }

    auto idx = std::size_t{0};
    auto y = bb.Min.y + style.FramePadding.y;
    auto const line_half_width = ImPlus::to_pt(1_pt);
    auto dy = style.FramePadding.y;

    for (auto&& s : items) {
        auto const& p = intervals[idx];
        dl->AddText({bb.Min.x + p.lo + p.dx, y}, text_clr32, s.data(), s.data() + s.size());

        if (idx > 0 && (idx < sel_index || idx > sel_index + 1)) {
            auto x = std::round(bb.Min.x + p.lo);
            dl->AddRectFilled({x - line_half_width, bb.Min.y + dy},
                {x + line_half_width, bb.Max.y - dy}, line_clr32);
        }
        ++idx;
    }

    if (pressed) {
        auto want_index = sel_index;
        if (owns_mouse) {
            if (hot_index >= 0)
                want_index = hot_index;
        }
        else
            want_index = std::size_t(sel_index + 1) >= items.size() ? 0 : sel_index + 1;

        if (want_index != sel_index) {
            sel_index = want_index;
            return true;
        }
    }

    return false;
}

} // namespace ImPlus