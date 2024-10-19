#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/button.hpp"
#include "implus/buttonbar.hpp"
#include "implus/dlg.hpp"
#include "implus/length.hpp"
#include <cmath>
#include <numeric>
#include <vector>

namespace ImPlus::Buttonbar {

inline auto padded_size(ImVec2 const& inner, ImVec2 const& padding) -> ImVec2
{
    return {std::round(inner.x + padding.x * 2.0f), std::round(inner.y + padding.y * 2.0f)};
}

struct measurement_entry {
    ICDBlock block;
    ImVec2 size = {};         // includes padding
    pt_length spacing = 0.0f; // spacing gap between previous and this
    bool is_disabled = false;
    bool is_default = false;
    bool is_cancel = false;
    InteractColorSetCallback colors = nullptr;
};

static auto name_for_test_engine(ICD_view const& content)
{
#ifdef IMGUI_ENABLE_TEST_ENGINE
    return std::string{content.Caption};
#else
    return std::string{};
#endif
}

auto Display(
    ImID id, SourceCallback&& buttons, ImVec2 const& align_all) -> std::optional<std::size_t>
{
    auto& g = *GImGui;
    auto const& style = g.Style;

    auto window = g.CurrentWindow;
    if (window->SkipItems || !buttons)
        return {};

    auto clicked = std::optional<size_t>{};
    auto focused = std::optional<size_t>{};

    // sizing along and across the item alignment axis
    auto const vertical = Style::Buttonbar::ItemStacking() == Axis::Vert;
    auto along = [&](auto& sz) -> auto& { return vertical ? sz.y : sz.x; };
    auto across = [&](auto& sz) -> auto& { return vertical ? sz.x : sz.y; };

    // see if the caller is forcing us to a certain item width via PushItemWidth or
    // SetNextItemWidth:
    auto force_item_width = to_pt(Style::Buttonbar::ForceItemWidth());
    auto force_item_height = to_pt(Style::Buttonbar::ForceItemHeight());

    // calculate sizes
    auto entries = std::vector<measurement_entry>{};
    auto const item_padding = style.FramePadding;
    auto const default_spacing = std::round(along(g.Style.ItemSpacing));
    auto const icon_stacking = Style::Buttonbar::ItemLayout();
    auto const available_region = ImGui::GetContentRegionAvail();
    auto const available_along = along(available_region);

    auto const overflow_policy = Style::Buttonbar::OverflowPolicy();

    auto overflow_width = std::optional<pt_length>{};
    if (force_item_width)
        overflow_width = *force_item_width;
    else
        overflow_width = to_pt(Style::Buttonbar::Horizontal::OverflowWidth());

    if (overflow_width)
        *overflow_width -= item_padding.x * 2.0f;

    auto calc_spacing = [default_spacing](std::optional<length> const& v) -> float {
        if (!v.has_value())
            return default_spacing;
        else
            return to_pt<rounded>(*v);
    };

    auto spacing = 0.0f;
    for (auto n = 0; n < 100; ++n) {
        auto btn = Button{};
        if (!buttons(btn))
            break;

        if (n > 0)
            spacing = std::max(spacing, calc_spacing(btn.SpacingBefore));

        auto& en = entries.emplace_back(measurement_entry{
            .block = ICDBlock{btn.Content, icon_stacking, {}, overflow_policy, overflow_width},
            .spacing = spacing,
            .is_disabled = !btn.Enabled,
            .is_default = btn.Default,
            .is_cancel = btn.Cancel,
            .colors = btn.Colors,
        });
        en.size = padded_size(en.block.Size, item_padding);
        if (force_item_width)
            en.size.x = *force_item_width;
        if (force_item_height)
            en.size.y = *force_item_height;
        spacing = calc_spacing(btn.SpacingAfter);
    }

    auto max_measured = ImVec2{0, 0};
    auto spacing_sum = 0.0f;
    auto content_sum = 0.0f; // along the item stacking axis
    for (auto&& en : entries) {
        spacing_sum += en.spacing;
        content_sum += along(en.size);
        max_measured.x = std::max(max_measured.x, en.size.x);
        max_measured.y = std::max(max_measured.y, en.size.y);
    }

    auto const allow_stretch = (!vertical && !force_item_width) || (vertical && !force_item_height);

    if (allow_stretch) {
        auto const stretch_up_to =
            to_pt(vertical ? Style::Buttonbar::Vertical::StretchItemHeight()
                           : Style::Buttonbar::Horizontal::StretchItemWidth());
        if (stretch_up_to) {
            const auto target_content_sum = std::max(content_sum, available_along - spacing_sum);
            if (content_sum < target_content_sum) {
                // equalizing stretch
                auto desired_sum = 0.0f;
                for (auto&& en : entries)
                    desired_sum += std::max(*stretch_up_to, along(en.size));

                if (desired_sum <= target_content_sum) {
                    // stretch all to desired size
                    content_sum = 0;
                    for (auto&& en : entries)
                        along(en.size) = std::max(along(en.size), *stretch_up_to);
                }
                else {
                    // sorted stretch all to desired size starting from narrowest
                    auto target = target_content_sum;
                    auto sorted = std::vector<float*>{};
                    for (auto& en : entries)
                        if (auto sz = along(en.size); sz < stretch_up_to)
                            sorted.push_back(&along(en.size));
                        else
                            target -= sz;
                    std::sort(sorted.begin(), sorted.end(),
                        [](float const* a, float const* b) { return *a < *b; });
                    auto partial = 0.0f;
                    for (std::size_t i = 0; i < sorted.size(); ++i) {
                        auto actual = 0.0f;
                        for (auto ptr : sorted)
                            actual += *ptr;
                        if (actual >= target)
                            break;
                        partial += *sorted[i];
                        auto sz = (partial + target - actual) / (i + 1);
                        sz = std::min(sz, *stretch_up_to); // limit stretching to desired_size
                        if (i + 1 < sorted.size()) {
                            // don't stretch wider than any other wider item (during
                            // this iteration)
                            sz = std::min(sz, *sorted[i + 1]);
                        }
                        for (std::size_t j = 0; j <= i; ++j)
                            *sorted[j] = sz;
                        partial = sz * (i + 1);
                    }
                }

                content_sum = 0;
                for (auto&& en : entries)
                    content_sum += along(en.size);
                content_sum = std::round(content_sum);
            }
        }
    }

    // equalize across
    if (vertical) {
        if (!force_item_width)
            for (auto&& en : entries)
                en.size.x = max_measured.x;
    }
    else { // horizontal item placement
        if (!force_item_height)
            for (auto&& en : entries)
                en.size.y = max_measured.y;
    }

    auto const content_size = !vertical
                                  ? ImVec2{content_sum + spacing_sum, std::round(max_measured.y)}
                                  : ImVec2{std::round(max_measured.x), content_sum + spacing_sum};

    // alignment within parent
    auto cursor = ImGui::GetCursorPos();
    if (align_all.x > 0)
        cursor.x += std::max(0.0f, available_region.x - content_size.x) * align_all.x;
    if (align_all.y > 0)
        cursor.y += std::max(0.0f, available_region.y - content_size.y) * align_all.y;

    // default/focused

    auto default_index = std::optional<std::size_t>{};
    auto default_id = ImGuiKeyOwner_NoOwner;
    auto cancel_id = ImGuiKeyOwner_NoOwner;

    if (Style::Buttonbar::HighlightDefault() &&
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        auto active_id = g.ActiveId;
        auto focus_id = g.NavCursorVisible ? g.NavId : ImGuiID(0);

        for (std::size_t index = 0; index < entries.size(); ++index) {
            auto& en = entries[index];

            auto const id = window->GetID(index);
            if (!en.is_disabled) {
                if (en.is_default && default_id == ImGuiKeyOwner_NoOwner) {
                    default_index = index;
                    default_id = id;
                }

                if (en.is_cancel && cancel_id == ImGuiKeyOwner_NoOwner) {
                    cancel_id = id;
                }
            }

            if (focus_id == id || active_id == id) {
                default_index = {};
                default_id = 0;
            }
        }
    }

    if (default_id != ImGuiID(0)) {
        // see if the "Enter" shortcut is available
        auto avail = [default_id](ImGuiKeyChord kc) {
            auto d = ImGui::GetShortcutRoutingData(kc);
            return !d || d->RoutingCurr == ImGuiKeyOwner_NoOwner || d->RoutingCurr == default_id;
        };
        if (!avail(ImGuiKey_Enter) && !avail(ImGuiKey_KeypadEnter)) {
            default_index = {};
            default_id = 0;
        }
    }

    // render
    ImGui::PushID(id);
    auto index = std::size_t{0};
    for (auto&& en : entries) {
        auto const id = window->GetID(index);
        auto sz = en.size;

        if (!vertical) {
            cursor.x += en.spacing;
            ImGui::SetCursorPos({std::round(cursor.x), cursor.y});
            cursor.x += en.size.x;
        }
        else {
            cursor.y += en.spacing;
            ImGui::SetCursorPos({cursor.x, std::round(cursor.y)});
            cursor.y += en.size.y;
        }

        ImGui::BeginDisabled(en.is_disabled);

        auto btn_opts = ButtonOptions{
            .DefaultAction = (index == default_index),
            .Padding = item_padding,
        };

        auto draw_cb =
            MakeButtonDrawCallback(btn_opts, en.colors, MakeContentDrawCallback(en.block));

        auto st = CustomButton(id, en.block.NameForTestEngine().c_str(), sz, item_padding.y,
            ImGuiButtonFlags_None, draw_cb);

        if (st.Pressed)
            clicked = en.is_cancel ? CancelSentinel : index;

        ImGui::EndDisabled();
        ++index;
    }
    ImGui::PopID();

    // handle default action for modal dialogs
    if (!clicked && default_index) {
        if (ImPlus::Dlg::IsAcceptActionKeyPressed())
            clicked = default_index;
    }

    return clicked;
}

} // namespace ImPlus::Buttonbar