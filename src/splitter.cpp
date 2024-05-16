#include "implus/splitter.hpp"
#include <imgui_internal.h>
#include <string>

namespace ImPlus {

class scope_guard {
    std::function<void()> func_;

public:
    template <typename T>
    explicit scope_guard(T func)
        : func_(std::move(func))
    {
    }
    ~scope_guard() { func_(); }
};

auto Splitter::CalcSizes(pt_length target_sum) -> float
{
    if (empty())
        return target_sum;

    // minimum size of all bands
    auto minimum_fixed_sum = 0.0f;
    auto minimum_sprng_sum = 0.0f;
    auto n_sprng = size_t{0};
    auto n_fixed = size_t{0};
    auto sprng_ref = 0.0f;
    for (auto& b : *this) {
        b.minimum_size_ = std::max(1.0f, to_pt(b.Size.Minimum));
        if (auto v = spring_numeric_value(b.Size.Desired)) {
            sprng_ref += *v;
            ++n_sprng;
            minimum_sprng_sum += b.minimum_size_;
        }
        else {
            ++n_fixed;
            minimum_fixed_sum += b.minimum_size_;
        }
    }
    const auto minimum_sum = minimum_fixed_sum + minimum_sprng_sum;
    target_sum = std::max(target_sum, minimum_sum);

    for (auto& b : *this)
        b.size_ = b.minimum_size_;

    if (minimum_sum >= target_sum)
        return target_sum;

    if (n_fixed && !n_sprng) {
        auto sum = 0.0f;
        for (auto& b : *this) {
            if (auto v = to_pt(b.Size.Desired))
                b.size_ = std::max(1.0f, *v);
            sum += b.size_;
        }
        auto factor = target_sum / sum;
        for (auto& b : *this)
            b.size_ *= factor;
    }
    else if (!n_fixed) {
        auto factor = target_sum / sprng_ref;
        for (auto& b : *this)
            if (auto v = spring_numeric_value(b.Size.Desired))
                b.size_ = factor * std::max(0.001f, *v);
    }
    else if (n_fixed && n_sprng) {
        auto fixed_sum = 0.0f;
        for (auto& b : *this)
            if (auto v = to_pt(b.Size.Desired)) {
                b.size_ = std::max(1.0f, *v);
                fixed_sum += b.size_;
            }
        auto factor = std::max(target_sum - fixed_sum, minimum_sprng_sum) / sprng_ref;
        auto spring_sum = 0.0f;
        for (auto& b : *this)
            if (auto v = spring_numeric_value(b.Size.Desired)) {
                b.size_ = factor * std::max(0.001f, *v);
                spring_sum += b.size_;
            }
        factor = target_sum / (fixed_sum + spring_sum);
        for (auto& b : *this)
            b.size_ *= factor;
    }

    // apply minimum constraints
    auto sum = 0.0f;
    for (auto& b : *this) {
        b.size_ = std::max(b.size_, b.minimum_size_);
        sum += b.size_;
    }

    // linear scale all
    auto factor = target_sum / sum;
    sum = 0.0f;
    for (auto& b : *this) {
        b.size_ = std::max(b.size_ * factor, b.minimum_size_);
        sum += b.size_;
    }

    // lerp all
    if (sum > minimum_sum) {
        auto factor = (target_sum - minimum_sum) / (sum - minimum_sum);
        for (auto& b : *this)
            b.size_ = b.minimum_size_ + (b.size_ - b.minimum_size_) * factor;
    }

    // rounding
    sum = 0;
    for (auto& b : *this) {
        b.size_ = std::max(b.minimum_size_, std::round(b.size_));
        sum += b.size_;
    }
    auto delta = std::round(target_sum - sum);
    if (delta > 0)
        back().size_ += delta;
    else if (delta < 0)
        for (size_t i = size(); i > 0; --i) {
            auto& b = (*this)[i - 1];
            auto sz = std::max(b.size_ + delta, b.minimum_size_);
            delta -= sz - b.size_;
            b.size_ = sz;
            if (!delta)
                break;
        }

    return target_sum;
}

auto is_child_of(ImGuiWindow* child, ImGuiWindow* parent)
{
    while (child)
        if (child->ParentWindow == parent)
            return true;
        else
            child = child->ParentWindow;

    return false;
}

void Splitter::Display(std::initializer_list<std::function<void()>> display_procs)
{
    if (empty())
        return;

    auto& g = *ImGui::GetCurrentContext();

    auto const area_size = ImGui::GetContentRegionAvail();
    ImGui::PushID(this);
    auto when_done = scope_guard{[] { ImGui::PopID(); }};

    auto parent_window = g.CurrentWindow;

    ImGui::BeginChildEx("splt", parent_window->GetID(this), area_size, false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoSavedSettings);
    auto when_done_2 = scope_guard{[] { ImGui::EndChild(); }};

    const auto thickness = to_pt<rounded_up>(Style::Splitter::Line::Thickness());
    const auto hitextend = to_pt<rounded_up>(Style::Splitter::Line::HitExtend());

    const auto area_avail = StackingDirection == Axis::Horz ? area_size.x : area_size.y;
    const auto target_sum = CalcSizes(area_avail - thickness * (size() - 1));

    auto window = g.CurrentWindow;

    auto sum_mins = [&](size_t first, size_t last) -> float {
        auto ret = -thickness;
        for (auto i = first; i <= last; ++i)
            ret += (*this)[i].minimum_size_ + thickness;
        return ret;
    };

    auto drag_index = std::optional<size_t>{};
    auto delta = 0.0f;
    auto const imaxis = (StackingDirection == Axis::Horz) ? ImGuiAxis_X : ImGuiAxis_Y;
    auto scr_org = ImGui::GetCursorScreenPos();
    auto bb = ImRect{scr_org, {scr_org.x + area_size.x, scr_org.y + area_size.y}};

    auto const wnd_org = ImGui::GetCursorPos();

    auto band_offset = -thickness;

    ImGui::PushStyleColor(ImGuiCol_Separator, Style::Splitter::Line::Color::Regular());
    ImGui::PushStyleColor(ImGuiCol_SeparatorActive, Style::Splitter::Line::Color::Active());
    ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, Style::Splitter::Line::Color::Hovered());

    for (size_t i = 0; i + 1 < size(); ++i) {
        auto id = window->GetID(i);
        band_offset += (*this)[i].size_ + thickness;
        if (StackingDirection == Axis::Horz) {
            bb.Min.x = scr_org.x + band_offset;
            bb.Max.x = bb.Min.x + thickness;
        }
        else {
            bb.Min.y = scr_org.y + band_offset;
            bb.Max.y = bb.Min.y + thickness;
        }
        auto sz1 = band_offset;
        auto sz2 = area_avail - band_offset;
        auto min1 = sum_mins(0, i);
        auto min2 = sum_mins(i + 1, size() - 1);

        if (g.HoveredWindow &&
            (g.HoveredWindow == window || is_child_of(g.HoveredWindow, window))) {
            auto save_hw = g.HoveredWindow;
            g.HoveredWindow = window;
            auto held = ImGui::SplitterBehavior(bb, id, imaxis, &sz1, &sz2, min1, min2, hitextend);
            g.HoveredWindow = save_hw;
            if (held) {
                drag_index = i;
                delta = sz1 - band_offset;
            }
        }
        else {
            ImGui::SplitterBehavior(bb, id, imaxis, &sz1, &sz2, min1, min2, hitextend);
        }
    }

    ImGui::PopStyleColor(3);

    if (drag_index && delta) {
        if (delta > 0) {
            for (size_t i = *drag_index + 1; i < size(); ++i) {
                auto sz = std::max((*this)[i].size_ - delta, (*this)[i].minimum_size_);
                delta += sz - (*this)[i].size_;
                (*this)[i].size_ = sz;
                if (delta <= 0)
                    break;
            }
            auto sz = target_sum;
            for (size_t i = 0; i < size(); ++i)
                if (i != *drag_index)
                    sz -= (*this)[i].size_;
            (*this)[*drag_index].size_ = sz;
        }
        else {
            for (size_t i = *drag_index + 1; i > 0; --i) {
                auto& b = (*this)[i - 1];
                auto sz = std::max(b.size_ + delta, b.minimum_size_);
                delta -= sz - b.size_;
                b.size_ = sz;
                if (delta >= 0)
                    break;
            }
            auto sz = target_sum;
            for (size_t i = 0; i < size(); ++i)
                if (i != *drag_index + 1)
                    sz -= (*this)[i].size_;
            (*this)[*drag_index + 1].size_ = sz;
        }

        auto remaining = target_sum;

        for (auto& b : *this)
            if (auto pv = std::get_if<length>(&b.Size.Desired)) {
                remaining -= b.size_;
                assign_from_pt(*pv, b.size_);
            };

        if (remaining > 0) {
            for (auto& b : *this)
                if (auto pv = std::get_if<spring>(&b.Size.Desired))
                    pv->numeric_value() = b.size_;
        }
    }

    auto idx = size_t{0};
    band_offset = (StackingDirection == Axis::Horz) ? wnd_org.x : wnd_org.y;
    for (auto& proc : display_procs) {
        if (idx > size())
            break;

        auto& band = (*this)[idx];

        auto band_size = band.size_;
        if (StackingDirection == Axis::Horz)
            ImGui::SetCursorPos({band_offset, wnd_org.y});
        else
            ImGui::SetCursorPos({wnd_org.x, band_offset});

        band_offset += band_size + thickness;

        auto sz =
            (StackingDirection == Axis::Horz) ? ImVec2(band_size, -1.0f) : ImVec2(-1.0, band_size);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        auto bg_color = GetColor(band.BackgroundColor).value_or(ImVec4{});
        if (bg_color.w)
            ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_color);

        auto s = std::to_string(idx);
        auto showing = ImGui::BeginChildEx(s.c_str(), window->GetID(&(*this)[idx]), sz,
            band.Flags.ChildFlags, band.Flags.WindowFlags);
        ImGui::PopStyleVar();

        if (showing && proc)
            proc();

        ImGui::EndChild();
        ++idx;

        if (bg_color.w)
            ImGui::PopStyleColor();
    }
}

/*
auto to_string(detail::Band::desired_sz_t const& sz) -> std::string
{
    char buf[64] = {0};
    if (auto ln = std::get_if<Length>(&sz)) {
        auto f = ln->AsFloat();
        switch (ln->GetUnit()) {
        case Length::Unit::Pixel: snprintf(buf, 64, "%.0g px", f); break;
        case Length::Unit::FontH: snprintf(buf, 64, "%.2f fh", f);
        }
    }
    else if (auto spr = std::get_if<Spring>(&sz)) {
        auto f = spr->AsFloat();
        snprintf(buf, 64, "%.2f %%", f);
    }
    return buf;
}

auto from_string(std::string_view str, detail::Band::desired_sz_t& sz) -> bool
{
    auto end = str.data() + str.size();
    auto p = (char*)end;
    auto f = strtof(str.data(), &p);
    if (p == str.data())
        return false;
    auto u = std::string_view(p, end - p);
    while (!u.empty() && u.front() == ' ')
        u.remove_prefix(1);
    if (u == "%")
        sz = ImPlus::Spring(f);
    else if (u == "fh")
        sz = f * ImPlus::FontH;
    else if (u == "px")
        sz = f * ImPlus::Pixel;
    else
        return false;
    return true;
}
*/

} // namespace ImPlus