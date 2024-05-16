#pragma once

#include "implus/interact.hpp"
#include <algorithm>
#include <optional>
#include <vector>

namespace ImPlus::Itemizer {

// Key is used to uniquely identify the payload
// Payload would typically be a pointer to user's data
template <typename Key, typename Payload> struct Item {
    using key_type = Key;
    using payload_type = Payload;

    Item(key_type const& key, payload_type const& payload)
        : key{key}
        , payload{payload}
    {
    }

    Item(key_type const& key, payload_type&& payload = {})
        : key{key}
        , payload{std::move(payload)}
    {
    }

    Item(Item&&) = default;
    auto operator=(Item&&) -> Item& = default;

    key_type key;
    payload_type payload;
    bool visible = true;
    bool selected = false;
};

template <typename Key, typename Payload> struct List : public std::vector<Item<Key, Payload>> {
    using key_type = Key;
    using payload_type = Payload;
    using vector_type = typename std::vector<Item<Key, Payload>>;
    using value_type = typename vector_type::value_type;
    using item_type = typename vector_type::value_type;
    using iterator = typename vector_type::iterator;
    using const_iterator = typename vector_type::const_iterator;

private:
    std::optional<key_type> focused_ = {};
    std::optional<key_type> anchor_ = {};

public:
    auto contains(key_type const& key) const -> bool
    {
        for (auto& item : *this)
            if (item.key == key)
                return true;
        return false;
    }

    auto clear()
    {
        focused_.reset();
        anchor_.reset();
        vector_type::clear();
    }

    auto clear_selection()
    {
        for (auto& item : *this)
            item.selected = false;
    }

    auto find_iter(key_type const& key) -> iterator
    {
        return std::find_if(
            this->begin(), this->end(), [&key](item_type const& v) { return v.key == key; });
    }

    auto find_iter(key_type const& key) const -> const_iterator
    {
        return std::find_if(
            this->begin(), this->end(), [&key](item_type const& v) { return v.key == key; });
    }

    auto find(key_type const& key) const -> item_type const*
    {
        if (auto it = find_iter(key); it != this->end())
            return &(*it);
        return nullptr;
    }

    auto find(key_type const& key) -> item_type*
    {
        if (auto it = find_iter(key); it != this->end())
            return &(*it);
        return nullptr;
    }

    auto first_visible() -> item_type*
    {
        for (auto& item : *this)
            if (item.visible)
                return &item;
        return nullptr;
    }

    auto first_selected() -> item_type*
    {
        for (auto& item : *this)
            if (item.selected)
                return &item;
        return nullptr;
    }

    auto selection_if_single() -> item_type*
    {
        item_type* ret = nullptr;
        for (auto& item : *this)
            if (item.selected) {
                if (!ret)
                    ret = &item;
                else
                    return nullptr;
            }
        return ret;
    }

    auto focused() -> item_type* { return focused_ ? find(*focused_) : nullptr; }

    auto select_all(bool visible_only = true)
    {
        if (this->empty())
            return;
        for (auto& it : *this)
            it.selected = !visible_only || it.visible;

        if (focused_ >= this->size()) {
            focused_.reset();
            anchor_.reset();
        }
        else if (anchor_ >= this->size()) {
            anchor_ = focused_;
        }
    }

    auto is_selected(key_type const& key) const -> bool
    {
        auto it = find(key);
        return it && it->selected;
    }

    auto select(key_type const& key, SelectionModifier modifier = SelectionModifier::Regular)
    {
        item_type* it = find(key);

        switch (modifier) {

        case SelectionModifier::Regular: {
            clear_selection();
            focused_.reset();
            anchor_.reset();
            if (it) {
                it->selected = true;
                focused_ = key;
                anchor_ = key;
            }
        } break;

        case SelectionModifier::Toggle: {
            if (it) {
                it->selected = !it->selected;
                focused_ = key;
                anchor_ = key;
            }
        } break;

        case SelectionModifier::Range: {
            if (!it)
                return;
            if (!first_selected()) {
                focused_.reset();
                anchor_.reset();
            }
            clear_selection();

            item_type* anchor_it = nullptr;
            if (anchor_)
                anchor_it = find(*anchor_);

            if (!anchor_it) {
                it->selected = true;
                focused_ = key;
                anchor_ = key;
                return;
            }

            focused_ = key;
            if (it <= anchor_it) {
                for (; it <= anchor_it; ++it)
                    it->selected = it->visible;
            }
            else {
                for (; anchor_it <= it; ++anchor_it)
                    anchor_it->selected = anchor_it->visible;
            }
        } break;

        case SelectionModifier::Context: {
            if (!it || it->selected)
                return;
            clear_selection();
            it->selected = true;
            focused_ = key;
            anchor_ = key;
        }
        }
    }

    auto sel_range() const
    {
        auto f = this->end(); // first selected
        auto l = this->end(); // last selected
        std::size_t n_selected = 0;
        for (auto it = this->begin(); it != this->end(); ++it)
            if (it->selected) {
                if (!n_selected)
                    f = it;
                l = it;
                ++n_selected;
            }

        if (n_selected)
            ++l; // make l exclusive

        auto disjoint = n_selected < (l - f);

        return std::make_tuple(f, l, disjoint);
    }

    auto test_reorder(typename vector_type::const_iterator insert_before) const -> bool
    {
        auto [b, e, disjoint] = sel_range();
        if (b == this->end())
            return false;
        if (disjoint)
            return true;
        return (insert_before < b || insert_before > e);
    }
};

} // namespace ImPlus::Itemizer