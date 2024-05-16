#pragma once

#include <implus/color.hpp>
#include <optional>

namespace ImPlus {

template <typename ItemID> struct SelectionHelper {
    using id_type = ItemID;
    struct item_ref_pair {
        id_type id;
        std::size_t index;
    };
    struct index_interval {
        std::size_t lo;
        std::size_t hi;
    };
    struct Configuration {
        std::optional<ImVec4> BackgroundColor;
    };

protected:
    std::size_t n_selected = 0;
    std::size_t n_unselected = 0;
    std::optional<id_type> anchor_id;
    std::optional<std::size_t> anchor_index;

    std::optional<item_ref_pair> click;
    bool click_was_selected = false;
    bool click_was_context = false;

    std::optional<item_ref_pair> first_sel;
    std::optional<item_ref_pair> last_sel;

public:
    struct SelectionResult {
        bool ClearAll = false;
        std::optional<item_ref_pair> ClearOne;
        std::optional<item_ref_pair> SelectOne;
        std::optional<index_interval> SelectInterval;
        bool WantContextMenu = false;

        std::optional<item_ref_pair> ClickSelected; // clicked, transitioned to selected
    };

    auto IsAnySelected() const { return n_selected > 0; }
    auto IsOneSelected() const { return n_selected == 1; }
    auto IsMultiSelected() const { return n_selected > 0; }
    auto IsAllSelected() const { return n_selected > 0 && n_unselected == 0; }

    auto FirstSel() const { return *first_sel; }

    auto Process() -> SelectionResult
    {
        auto r = SelectionResult{};
        if (!click)
            return r;

        if (click_was_context) {
            if (!click_was_selected) {
                r.ClearAll = true;
                r.SelectOne = click;
                r.ClickSelected = click;
                anchor_id = click->id;
            }
            r.WantContextMenu = true;
            return r;
        }

        auto const ctrl = ImGui::GetIO().KeyCtrl;
        auto const shift = ImGui::GetIO().KeyShift;

        if (ctrl && !shift) {
            if (click_was_selected)
                r.ClearOne = click;
            else {
                r.SelectOne = click;
                r.ClickSelected = click;
                anchor_id = click->id;
            }
        }
        else if (shift && !ctrl) {
            if (!anchor_index) {
                r.ClearAll = true;
                r.SelectOne = click;
                anchor_id = click->id;
            }
            else if (*anchor_index < click->index)
                r.SelectInterval = {*anchor_index, click->index};
            else
                r.SelectInterval = {click->index, *anchor_index};
            r.ClickSelected = click;
        }
        else if (!shift && !ctrl) {
            r.ClearAll = true;
            r.SelectOne = click;
            r.ClickSelected = click;
            anchor_id = click->id;
        }
        return r;
    }

    void Start(Configuration const& config = {})
    {
        n_selected = 0;
        n_unselected = 0;

        anchor_index.reset();

        click.reset();
        click_was_selected = false;
        click_was_context = false;

        first_sel.reset();
        last_sel.reset();

        auto bkgnd_color =
            config.BackgroundColor.value_or(ImPlus::Color::FromStyle(ImGuiCol_WindowBg));
    }

    void Stop()
    {
        if (first_sel && last_sel && first_sel->index == last_sel->index) {
            anchor_index = first_sel->index;
            anchor_id = first_sel->id;
        }
        if (!anchor_index)
            anchor_id.reset();
    }

    void RegisterItem(std::size_t item_index, id_type const& item_id, bool selected)
    {
        if (selected) {
            ++n_selected;
            if (!first_sel)
                first_sel = {item_id, item_index};
            last_sel = {item_id, item_index};

            if (anchor_id && *anchor_id == item_id)
                anchor_index = item_index;
        }
        else {
            ++n_unselected;
            if (anchor_id && *anchor_id == item_id) {
                anchor_id.reset();
                anchor_index.reset();
            }
        }
    }

    void RegisterClick(std::size_t item_index, id_type const& item_id, bool was_selected)
    {
        click = {item_id, item_index};
        click_was_selected = was_selected;
        click_was_context = false;
    }

    void RegisterContextClick(std::size_t item_index, id_type const& item_id, bool was_selected)
    {
        click = {item_id, item_index};
        click_was_selected = was_selected;
        click_was_context = true;
    }
};

} // namespace ImPlus