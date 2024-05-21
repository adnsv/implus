#pragma once

#include "implus/checkbox.hpp"
#include "implus/combo.hpp"
#include "implus/font.hpp"
#include "implus/geometry.hpp"
#include "implus/icd.hpp"
#include "implus/icon.hpp"
#include "implus/id.hpp"
#include "implus/input.hpp"
#include "implus/length.hpp"
#include "implus/overridable.hpp"
#include "implus/toggler.hpp"
#include <concepts>
#include <functional>
#include <optional>
#include <string_view>

namespace ImPlus {

enum class KeyValPlacement {
    Inline,
    Block,
};

struct ParagraphOptions {
    Text::OverflowBehavior OverflowBehavior = Text::OverflowWrap;
    unsigned MaxLines = 64;
    Font::Resource Font = {};
    std::optional<ImVec4> Color = {};
    length ReserveExtraWidth = 0_em;
};

struct ValueWidthSpec {
    length MinWidth = 4_em;
    length DesiredWidth = 0_em; // effective is max(DesiredWidth, MinWidth)
    length StretchWidth = 0_em; // effective is max(MinWidth, DesiredWidth, StretchWidth)
};

namespace Style::Flow {
inline auto KeyValPlacement = overridable<ImPlus::KeyValPlacement>({KeyValPlacement::Inline});
inline auto KeyColor = overridable<ImVec4>(ImPlus::Color::FromStyle<ImGuiCol_Text>);

// inline placement
inline auto InlineValSpacing = overridable<length>(0.5_em);
inline auto MinInlineLabelIndent = overridable<length>(2_em);
inline auto MaxInlineLabelIndent = overridable<length>(10_em);

// input/combo field sizing
inline auto InputTextFieldWidth = overridable<ValueWidthSpec>({4_em, 8_em, 16_em});
inline auto InputNumericFieldWidth = overridable<ValueWidthSpec>({5_em, 8_em, 8_em});
inline auto ComboFieldWidth = overridable<ValueWidthSpec>({6_em, 8_em, 16_em});
inline auto SliderFieldWidth = overridable<ValueWidthSpec>({5_em, 8_em, 16_em});

} // namespace Style::Flow

struct FlowOptions {
    bool AutoSizing = false;
};

struct Flow {
private:
    ImGuiID flow_id = 0;
    pt_length desired_width_delta = 0; // diff between initial avail and desired widths
    pt_length min_effective_overflow_width = 0;
    pt_length initial_indent = 0;
    pt_length this_frame_inline_indent = 0;
    pt_length next_frame_inline_indent = 0;
    pt_length max_inline_indent = 0;
    bool auto_sizing = false;
    ImVec2 start_cursor_pos = {0, 0}; // local windo coords upon flow creation

    // available space in the last collapsing header (next to the label)
    ImPlus::Rect last_collapsing_header_frame = {{0, 0}, {0, 0}};
    float last_collapsing_header_label_xmax = 0;

public:
    Flow(length const& desired_overflow_width);
    Flow(ImID id, length const& desired_overflow_width);
    Flow(ImID id, length const& desired_overflow_width, FlowOptions const& opts);
    ~Flow();

    auto AvailWidth() const -> pt_length;    // same as ImGui::GetContentRegionAvail().x
    auto OverflowWidth() const -> pt_length; //
    auto StretchWidth() const -> pt_length;  // 0 when autosizing, AvailWidth if not

    void Spacing();
    void Spacing(length dy);
    void Paragraph(std::string_view s, ParagraphOptions const& opts = {});
    void ICD(ICD_view const&);

    auto CollapsingHeader(ImID id, std::string_view caption, ImGuiTreeNodeFlags flags = 0) -> bool;
    void CollapsingHeaderOverlayText(std::string_view s);

    auto FieldPrefix(ICD_view const& key, ValueWidthSpec const&, pt_length reserve_extra_width = 0,
        bool key_with_frame_padding = true) -> pt_length;
    void TextField(ICD_view const& key, std::string_view val, Text::OverflowPolicy const& op = {},
        pt_length reserve_extra_width = 0, bool key_with_frame_padding = false);

    template <typename T>
    requires arithmetic<T>
    void NumericField(ICD_view const& key, T const& v, std::string_view units = {})
    {
        constexpr std::size_t buf_capacity = 128;
        char buf[buf_capacity];
        int n = 0;
        if constexpr (std::is_floating_point_v<T>) {
            auto const spec = sizeof(T) >= 8 ? "%.16g" : "%.7g";
            n = std::snprintf(buf, buf_capacity - 1, spec, v);
            auto decimal = Style::Numeric::DecimalSeparator();
            if (decimal != '.') {
                for (auto i = 0; i < n; ++i)
                    if (buf[i] == '.') {
                        buf[i] = decimal;
                        break;
                    }
            }
        }
        else {
            n = std::to_chars(buf, buf + buf_capacity - 1, v).ptr - buf;
        }
        if (!units.empty() && units.size() + 1 <= buf_capacity - n) {
            buf[n++] = ' ';
            for (auto c : units)
                buf[n++] = c;
        }
        TextField(key, std::string_view(buf, n));
    }

    struct InputTextFieldOpts {
        char const* Hint = nullptr;
        ImGuiInputTextFlags Flags = ImGuiInputTextFlags_None;
        ImPlus::pt_length ReserveExtraWidth = 0.0f;
    };

    auto InputTextField(ImID id, ICD_view const& key, std::string& str,
        InputTextFieldOpts const& opts = {nullptr, ImGuiInputTextFlags_None, 0.0f}) -> bool
    {
        auto w = FieldPrefix(key, Style::Flow::InputTextFieldWidth(), opts.ReserveExtraWidth);
        ImGui::SetNextItemWidth(w);
        return ImPlus::InputTextWithHint(id, opts.Hint, str, opts.Flags);
    }

    // InputNumericField uses ImPlus::Style::Numeric to format
    template <typename T>
    requires maybe_optional_arithmetic<T>
    auto InputNumericField(ImID id, ICD_view const& key, T& v, std::string_view units,
        InputValidator<maybe_optional_value_type<T>>&& on_validate = {},
        std::span<sentinel<T> const> const& sentinels = {}) -> bool
    {
        auto const u_size = ImGui::CalcTextSize(units.data(), units.data() + units.size()).x;
        auto w = FieldPrefix(key, Style::Flow::InputNumericFieldWidth(), u_size);
        ImGui::SetNextItemWidth(w);
        return ImPlus::InputNumeric(id, v, units,
            std::forward<InputValidator<maybe_optional_value_type<T>>>(on_validate), sentinels);
    }

    template <typename T>
    requires maybe_optional_arithmetic<T>
    auto InputNumericField(ImID id, ICD_view const& key, T& v,
        InputValidator<maybe_optional_value_type<T>>&& on_validate = {},
        std::span<sentinel<T> const> const& sentinels = {}) -> bool
    {
        auto w = FieldPrefix(key, Style::Flow::InputNumericFieldWidth());
        ImGui::SetNextItemWidth(w);
        return ImPlus::InputNumeric(id, v,
            std::forward<InputValidator<maybe_optional_value_type<T>>>(on_validate), sentinels);
    }

    template <typename T>
    requires std::is_arithmetic_v<T>
    auto SliderField(ImID id, ICD_view const& key, T& v, T const& v_min, T const& v_max,
        std::string_view units, char const* fmt = Arithmetics::DefaultFmtSpec<T>(),
        ImGuiSliderFlags flags = 0) -> bool
    {
        auto const u_size = ImGui::CalcTextSize(units.data(), units.data() + units.size()).x;
        auto w = FieldPrefix(key, Style::Flow::SliderFieldWidth(), u_size);
        ImGui::SetNextItemWidth(w);
        return ImPlus::Slider(id, v, v_min, v_max, units, fmt, flags);
    }

    template <typename T>
    requires std::is_arithmetic_v<T>
    auto SliderField(ImID id, ICD_view const& key, T& v, T const& v_min, T const& v_max,
        char const* fmt = Arithmetics::DefaultFmtSpec<T>(), ImGuiSliderFlags flags = 0) -> bool
    {
        auto w = FieldPrefix(key, Style::Flow::SliderFieldWidth());
        ImGui::SetNextItemWidth(w);
        return ImPlus::Slider(id, v, v_min, v_max, fmt, flags);
    }

    template <typename R, typename Stringer>
    requires(std::ranges::random_access_range<R> && Listbox::detail::stringer<R, Stringer>)
    auto ComboStringField(ImID id, ICD_view const& key, R&& items, std::size_t& sel_index,
        Stringer&& to_string) -> bool
    {
        auto w = FieldPrefix(key, Style::Flow::ComboFieldWidth());
        ImGui::SetNextItemWidth(w);
        return Combo::Strings(
            id, std::forward<R>(items), sel_index, std::forward<Stringer>(to_string));
    }
    template <typename R>
    requires(std::ranges::random_access_range<R> &&
                std::is_convertible_v<std::ranges::range_value_t<R>, std::string>)
    auto ComboStringField(ImID id, ICD_view const& key, R&& items, std::size_t& sel_index) -> bool
    {
        auto w = FieldPrefix(key, Style::Flow::ComboFieldWidth());
        ImGui::SetNextItemWidth(w);
        return Combo::Strings(id, std::forward<R>(items), sel_index);
    }
    template <typename T>
    requires std::is_convertible_v<T, std::string>
    auto ComboStringField(ImID id, ICD_view const& key, std::initializer_list<T> items,
        std::size_t& sel_index) -> bool
    {
        auto w = FieldPrefix(key, Style::Flow::ComboFieldWidth());
        ImGui::SetNextItemWidth(w);
        return Combo::Strings(id, items, sel_index);
    }
    auto ComboStringItemsField(ImID id, ICD_view const& key, std::size_t count,
        std::size_t& sel_index, std::function<std::string(std::size_t idx)> on_item)
    {
        auto w = FieldPrefix(key, Style::Flow::ComboFieldWidth());
        ImGui::SetNextItemWidth(w);
        return Combo::StringItems(id, count, sel_index, on_item);
    }
    template <typename R, typename Stringer>
    requires(Listbox::detail::input<R> && Listbox::detail::stringer_unindexed<R, Stringer>)
    auto ComboEnumField(ImID id, ICD_view const& key, R&& items,
        std::remove_cv_t<std::ranges::range_value_t<R>>& v, Stringer&& to_string) -> bool
    {
        auto w = FieldPrefix(key, Style::Flow::ComboFieldWidth());
        ImGui::SetNextItemWidth(w);
        return Combo::Enums(id, std::forward<R>(items), v, std::forward<Stringer>(to_string));
    }
    template <typename T, typename Stringer>
    auto ComboEnumField(ImID id, ICD_view const& key, std::initializer_list<T> items, T& v,
        Stringer&& to_string) -> bool
    {
        auto w = FieldPrefix(key, Style::Flow::ComboFieldWidth());
        ImGui::SetNextItemWidth(w);
        return Combo::Enums(id, items, v, std::forward<Stringer>(to_string));
    }

    auto CheckboxField(ImID id, ICD_view const& key, bool& checked) -> bool
    {
        auto box_w = ImGui::GetFrameHeight();
        auto w = FieldPrefix(key, {box_w});
        ImGui::SetNextItemWidth(w);
        return ImPlus::Checkbox(id, {}, checked);
    }

    auto TogglerField(
        ImID id, ICD_view const& key, bool& active, TogglerOptions const& opts = {}) -> bool
    {
        auto box_w = ImPlus::Measure::Toggler(opts).x;
        auto w = FieldPrefix(key, {box_w});
        ImGui::SetNextItemWidth(w);
        return ImPlus::Toggler(id, active, opts);
    }
    template <typename T>
    auto MultiTogglerField(
        ImID id, ICD_view const& key, std::initializer_list<std::string_view> items, T& sel) -> bool
    {
        auto box_w = ImPlus::Measure::MultiToggler(items).x;
        auto w = FieldPrefix(key, {box_w});
        ImGui::SetNextItemWidth(w);
        return ImPlus::MultiToggler(id, items, sel);
    }
};

void SetMainInstructionFont(Font::Resource);
auto GetMainInstructionFont() -> Font::Resource;

struct FlowContent {
    using display_callback = std::function<void(Flow&)>;

    FlowContent(FlowContent const&) = default;
    FlowContent(FlowContent&&) = default;

    FlowContent(){};
    FlowContent(std::string_view);
    FlowContent(display_callback&&);

    template <typename T>
    requires std::is_convertible_v<T, std::string_view>
    FlowContent(T&& v)
        : FlowContent{std::string_view(v)}
    {
    }

    auto Empty() const -> bool;

    void Display(Flow&) const;

private:
    display_callback on_display_;
};

// BeginMainInstructionGroup displays icon, main_instruction and description
//
//  [ICON]  | <main_instruction>        |
//          | <description>             |
//          | .. <custom content> ...   |
//
// Note: this call must be followed by ImGui::EndGroup();
//
void BeginMainInstructionGroup(ImPlus::Icon const& icon, std::string const& main_instruction,
    FlowContent const& description = {});

} // namespace ImPlus