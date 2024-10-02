#pragma once

#include <imgui.h>

#include <string>
#include <string_view>

#include "../alignment.hpp"
#include "../combo.hpp"
#include "../input.hpp"
#include "../length.hpp"
#include "../listbox.hpp"
#include "../pagination.hpp"
#include "../text.hpp"
#include "../toggler.hpp"
#include "../toolbar.hpp"

namespace ImPlus::Demo {

template <typename T> struct parameter {
    std::string id;
    T value;
};

template <typename T> struct opt_parameter {
    std::string id;
    T value;
    bool use = false;

    auto get() -> std::optional<T>
    {
        if (use)
            return value;
        return {};
    }
};

inline auto caption_prefix(char const* caption)
{
    auto s = std::string(caption);
    if (!s.empty()) {
        s += ":";
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(s.c_str());
        ImGui::SameLine();
    }
}

inline auto string_id(std::string const& id) { return std::string("##") + id; }
inline auto string_id(std::string const& id, char const* suffix)
{
    return std::string("##") + id + "_" + suffix;
}

inline void editor(char const* caption, parameter<bool>& param)
{
    caption_prefix(caption);
    ImPlus::Toggler(string_id(param.id).c_str(), param.value);
}

inline void editor(char const* caption, parameter<Axis>& param)
{
    caption_prefix(caption);
    auto v = std::size_t(param.value);
    if (ImPlus::MultiToggler(string_id(param.id).c_str(), {"Horz", "Vert"}, v))
        param.value = Axis(v);
}

inline void editor(char const* caption, parameter<Pagination::Placement>& param)
{
    caption_prefix(caption);
    auto v = std::size_t(param.value);
    if (ImPlus::MultiToggler(string_id(param.id).c_str(), {"Near", "Center", "Far", "Stretch"}, v))
        param.value = Pagination::Placement(v);
}

inline void editor(char const* caption, parameter<Content::Layout>& param)
{
    caption_prefix(caption);
    auto v = std::size_t(param.value);
    if (ImPlus::MultiToggler(
            string_id(param.id).c_str(), {"HorzNear", "HorzCenter", "VertNear", "VertCenter"}, v))
        param.value = Content::Layout(v);
}

inline void editor(char const* caption, parameter<ImGuiDir>& param)
{
    caption_prefix(caption);
    auto v = std::size_t(param.value);
    if (ImPlus::MultiToggler(string_id(param.id).c_str(), {"Left", "Right", "Up", "Down"}, v))
        param.value = ImGuiDir(v);
}

inline void editor(char const* caption, parameter<ButtonAppearance>& param)
{
    caption_prefix(caption);
    auto v = std::size_t(param.value);
    if (ImPlus::MultiToggler(
            string_id(param.id).c_str(), {"Regular", "Flat", "Link", "Transparent"}, v))
        param.value = ButtonAppearance(v);
}

inline void editor(char const* caption, parameter<std::optional<Axis>>& param)
{
    caption_prefix(caption);
    auto idx = std::size_t((!param.value) ? 0 : *param.value == Axis::Horz ? 1 : 2);
    if (ImPlus::MultiToggler(string_id(param.id).c_str(), {"Auto (nullopt)", "Horz", "Vert"}, idx))
        switch (idx) {
        case 0: param.value = std::nullopt; break;
        case 1: param.value = Axis::Horz; break;
        case 2: param.value = Axis::Vert; break;
        }
}

inline void editor(char const* caption, parameter<std::optional<Content::Layout>>& param)
{
    caption_prefix(caption);

    auto idx = std::size_t((!param.value) ? 0 : 1 + int(*param.value));
    if (ImPlus::MultiToggler(string_id(param.id).c_str(),
            {"Auto (nullopt)", "HorzNear", "HorzCenter", "VertNear", "VertCenter"}, idx))
        switch (idx) {
        case 0: param.value = std::nullopt; break;
        case 1: param.value = Content::Layout::HorzNear; break;
        case 2: param.value = Content::Layout::HorzCenter; break;
        case 3: param.value = Content::Layout::VertNear; break;
        case 4: param.value = Content::Layout::VertCenter; break;
        }
}

inline void editor(char const* caption, parameter<Toolbar::TextVisibility>& param)
{
    caption_prefix(caption);
    auto v = std::size_t(param.value);
    if (ImPlus::MultiToggler(string_id(param.id).c_str(), {"Always", "Auto"}, v))
        param.value = Toolbar::TextVisibility(v);
}

inline void editor(
    char const* caption, parameter<length>& param, em_length const& min_em, em_length const& max_em)
{
    caption_prefix(caption);

    auto const spacing = ImGui::GetStyle().ItemSpacing.x;

    auto combo_w = to_pt<rounded>(3_em);
    auto slider_w = ImGui::GetContentRegionAvail().x - combo_w - spacing;
    slider_w = std::max(slider_w, to_pt<rounded>(3_em));
    slider_w = std::min(slider_w, to_pt<rounded>(10_em));

    ImGui::SetNextItemWidth(slider_w);

    auto curr_units = units_of(param.value);
    auto numeric = numeric_value_of(param.value);
    if (curr_units == em_units) {
        if (ImPlus::Slider(string_id(param.id).c_str(), numeric, min_em.numeric_value(),
                max_em.numeric_value()))
            assign_numeric_value(param.value, numeric);
    }
    else {
        if (ImPlus::Slider(string_id(param.id).c_str(), numeric, to_pt(min_em), to_pt(max_em)))
            assign_numeric_value(param.value, numeric);
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(combo_w);
    auto u = curr_units;
    ImPlus::Combo::Enums(string_id(param.id, "units").c_str(), {em_units, pt_units}, u,
        [](ImPlus::length_units const& b) { return b ? "em" : "pt"; });

    if (u != curr_units) {
        if (u == em_units)
            param.value = to_em(param.value);
        else
            param.value = to_pt(param.value);
    }
}

inline void editor(char const* caption, parameter<std::size_t>& param, std::size_t const& min_v,
    std::size_t const& max_v)
{
    caption_prefix(caption);

    auto const spacing = ImGui::GetStyle().ItemSpacing.x;

    auto slider_w = ImGui::GetContentRegionAvail().x - spacing;
    slider_w = std::max(slider_w, to_pt<rounded>(3_em));
    slider_w = std::min(slider_w, to_pt<rounded>(10_em));

    ImGui::SetNextItemWidth(slider_w);

    auto numeric = param.value;
    if (ImPlus::Slider(string_id(param.id).c_str(), numeric, min_v, max_v))
        param.value = numeric;
}

inline void editor(char const* caption, opt_parameter<length>& param, em_length const& min_em,
    em_length const& max_em)
{
    caption_prefix(caption);

    auto const spacing = ImGui::GetStyle().ItemSpacing.x;

    auto check_w = ImGui::GetFrameHeight();
    auto combo_w = to_pt<rounded>(3_em);
    auto slider_w = ImGui::GetContentRegionAvail().x - combo_w - spacing - spacing;
    slider_w = std::max(slider_w, to_pt<rounded>(3_em));
    slider_w = std::min(slider_w, to_pt<rounded>(10_em));

    ImGui::Checkbox(string_id(param.id, "use").c_str(), &param.use);
    ImGui::SameLine();

    ImGui::BeginDisabled(!param.use);

    ImGui::SetNextItemWidth(slider_w);
    auto curr_units = units_of(param.value);
    auto numeric = numeric_value_of(param.value);
    if (curr_units == em_units) {
        if (ImPlus::Slider(string_id(param.id).c_str(), numeric, min_em.numeric_value(),
                max_em.numeric_value()))
            assign_numeric_value(param.value, numeric);
    }
    else {
        if (ImPlus::Slider(string_id(param.id).c_str(), numeric, to_pt(min_em), to_pt(max_em)))
            assign_numeric_value(param.value, numeric);
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(combo_w);
    auto u = curr_units;
    ImPlus::Combo::Enums(string_id(param.id, "units").c_str(), {em_units, pt_units}, u,
        [](bool b) { return b ? "em" : "pt"; });

    if (u != curr_units) {
        if (u == em_units)
            param.value = to_em(param.value);
        else
            param.value = to_pt(param.value);
    }

    ImGui::EndDisabled();
}

inline void editor_alignment(char const* caption, parameter<float>& param)
{
    caption_prefix(caption);
    ImGui::TextUnformatted("X");
    ImGui::SameLine();
    ImGui::PushItemWidth(to_pt<rounded>(4_em));
    ImGui::SliderFloat(string_id(param.id).c_str(), &param.value, 0.0f, 1.0f);
    ImGui::PopItemWidth();
}

inline void editor_alignment_xy(char const* caption, parameter<ImVec2>& param)
{
    caption_prefix(caption);
    ImGui::TextUnformatted("X");
    ImGui::SameLine();
    ImGui::PushItemWidth(to_pt<rounded>(4_em));
    ImGui::SliderFloat(string_id(param.id, "x").c_str(), &param.value.x, 0.0f, 1.0f);
    ImGui::SameLine();
    ImGui::TextUnformatted(" Y");
    ImGui::SameLine();
    ImGui::SliderFloat(string_id(param.id, "y").c_str(), &param.value.y, 0.0f, 1.0f);
    ImGui::PopItemWidth();
}

inline void editOverflowPolicy(
    std::string const& label, char const* title, ImPlus::Text::OverflowPolicy& op)
{
    if (title) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(title);
        ImGui::SameLine();
    }
    auto v = std::size_t(op.Behavior);
    if (ImPlus::MultiToggler(label.c_str(), {"None", "Ellipsify", "Wrap", "ForceWrap"}, v))
        op.Behavior = Text::OverflowBehavior(v);

    ImGui::SameLine();
    ImGui::TextUnformatted(" max lines:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(std::max(
        to_pt<rounded>(3_em), std::min(to_pt<rounded>(12_em), ImGui::GetContentRegionAvail().x)));
    ImPlus::Slider<unsigned>((label + "_maxlines").c_str(), op.MaxLines, 0, 32);
}

} // namespace ImPlus::Demo