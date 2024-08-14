#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/blocks.hpp"
#include "internal/draw-utils.hpp"
#include <cmath>
#include <lbrk.hpp>

namespace ImPlus {

inline auto find_next_line(char const* text, char const* text_end) -> char const*
{
    while (text < text_end && ImCharIsBlankA(*text))
        text++;
    if (*text == '\r' && text + 1 < text_end && text[1] == '\n')
        text += 2;
    else if (*text == '\n')
        text++;
    return text;
}

auto unscaled_char_width(ImFont const& font, unsigned int c) -> float
{
    return int(c) < font.IndexAdvanceX.Size ? font.IndexAdvanceX.Data[c] : font.FallbackAdvanceX;
}

struct calc_line_result {
    line_view line;
    char const* next;
};

auto calc_line_wrap(ImFont const& font, float font_size, float wrap_width, char const* text,
    char const* text_end, bool allow_emergency_break) -> calc_line_result
{
    auto const scale = font_size / font.FontSize;
    wrap_width /= scale; // work with unscaled widths to avoid scaling every characters

    struct split_result {
        bool force_brk;
        char const* trim;
        char const* next;
        char const* avail;
        float trim_advance;
        float next_advance;
        float avail_advance;
    };

    auto split = [&](char const* curr, float avail_w) -> split_result {
        auto ctx = lbrk::context{};
        auto trim = curr;
        auto avail = curr;
        auto trim_advance = 0.0f;
        auto next_advance = 0.0f;
        auto avail_advance = 0.0f;
        auto first = true;
        while (curr != text_end) {
            auto c = static_cast<unsigned int>(*curr);
            auto const next = curr + ((c < 0x80) ? 1 : ImTextCharFromUtf8(&c, curr, text_end));
            auto const lbc = lbrk::get_class(char32_t(c));

            if (auto ba = ctx.calc_action(lbc); ba != lbrk::lba::forbid)
                return {ba == lbrk::lba::force, trim, curr, avail, trim_advance, next_advance,
                    avail_advance};

            curr = next;
            next_advance += unscaled_char_width(font, c);
            if (first || next_advance <= avail_w) {
                avail = curr;
                avail_advance = next_advance;
            }
            if (!lbrk::is_trimmable(lbc)) {
                trim = curr;
                trim_advance = next_advance;
            }

            first = false;
        }
        return {false, trim, curr, avail, trim_advance, next_advance, avail_advance};
    };

    // returned values
    auto content_end = text;
    auto content_advance = 0.0f;

    auto curr = text;
    auto curr_advance = 0.0f;
    auto first = true;
    while (curr < text_end) {
        auto splt = split(curr, wrap_width - curr_advance);

        if (curr_advance + splt.trim_advance > wrap_width) {
            // handle overflow
            if (allow_emergency_break) {
                if (first || (splt.trim_advance > wrap_width &&
                                 curr_advance + splt.avail_advance <= wrap_width)) {
                    curr = splt.avail;
                    content_end = curr;
                    content_advance = curr_advance + splt.avail_advance;
                    break;
                }
            }
            if (!first)
                break;
        }

        first = false;
        content_end = splt.trim;
        content_advance = curr_advance + splt.trim_advance;
        curr = splt.next;
        curr_advance += splt.next_advance;

        if (splt.force_brk) {
            break;
        }
    }
    return calc_line_result{line_view{text, content_end, content_advance * scale}, curr};
};

auto calc_next_line(
    ImFont const& font, float font_size, char const* first, char const* last) -> calc_line_result
{
    auto const scale = font_size / font.FontSize;

    auto line_advance = 0.0f;    // includes trailing white space
    auto content_advance = 0.0f; // excludes trailing white space
    auto content_end = first;    // excludes trailing white space

    auto curr = first;
    while (curr < last) {
        auto c = static_cast<unsigned int>(*curr);
        curr = c < 0x80 ? curr + 1 : curr + ImTextCharFromUtf8(&c, curr, last);

        if (c == '\r')
            continue;
        if (c == '\n')
            break;

        line_advance += unscaled_char_width(font, c);
        if (!ImCharIsBlankW(c)) {
            content_advance = line_advance;
            content_end = curr;
        }
    }

    return calc_line_result{line_view{first, content_end, scale * content_advance}, curr};
}

void ellipsify_line(
    ImFont const& font, float font_size, TextBlock::LineInfo& line, std::optional<float> avail_w)
{
    if (!avail_w.has_value()) {
        line.ellipsis = font.EllipsisWidth * font_size / font.FontSize;
        return;
    }

    auto const scale = font_size / font.FontSize;
    auto const ellipsis_width = font.EllipsisWidth * scale;
    auto const fit_w = std::max(1.0f, *avail_w - ellipsis_width);

    if (line.empty()) {
        if (ellipsis_width <= fit_w)
            line.ellipsis = ellipsis_width;
        return;
    }

    auto curr = line.data();
    auto const last = curr + line.size();
    auto curr_advance = 0.0f;
    auto content_advance = 0.0f;
    auto content_end = curr;

    auto first_char = true;

    while (curr < last) {
        auto c = static_cast<unsigned int>(*curr);
        curr += ((c < 0x80) ? 1 : ImTextCharFromUtf8(&c, curr, last));

        curr_advance += scale * unscaled_char_width(font, c);
        if (ImCharIsBlankW(c))
            continue;

        if (curr_advance > fit_w && !first_char)
            break;

        first_char = false;
        content_advance = curr_advance;
        content_end = curr;
    }

    line.remove_suffix(last - content_end);
    line.advance = content_advance;
    if (line.advance <= fit_w) {
        line.ellipsis = ellipsis_width;
    }
}

struct MeasureTextResult {
    ImVec2 size = {0, 0};
    std::vector<TextBlock::LineInfo> lines;
};

auto MeasureTextEx(ImFont* fnt, float fnt_size, std::string_view s, Text::OverflowPolicy const& op,
    std::optional<float> const& ow) -> MeasureTextResult
{
    auto ret = MeasureTextResult{};

    auto& font = fnt ? *fnt : *GImGui->Font;
    auto const font_size = fnt_size ? fnt_size : GImGui->FontSize;

    if (s.empty()) {
        ret.size.y = font_size;
        return ret;
    }

    auto first = s.data();
    auto last = first + s.size();

    auto ellipsify_each = op.Behavior == Text::OverflowBehavior::OverflowEllipsify;
    auto wrappable = op.Behavior == Text::OverflowBehavior::OverflowWrap ||
                     op.Behavior == Text::OverflowBehavior::OverflowForceWrap;

    auto needs_wrapping = false;

    unsigned line_count = 0;
    while (first != last) {
        if (op.MaxLines > 0 && line_count >= op.MaxLines) {
            ellipsify_line(font, font_size, ret.lines.back(), ow);
            break;
        }

        auto lr = calc_next_line(font, font_size, first, last);
        auto line_overflows = ow && (lr.line.advance > *ow);
        if (line_overflows && wrappable) {
            needs_wrapping = true;
            break;
        }
        ret.lines.push_back(TextBlock::LineInfo{lr.line});
        if (line_overflows && ellipsify_each)
            ellipsify_line(font, font_size, ret.lines.back(), ow);
        first = lr.next;
        ++line_count;
    }

    if (needs_wrapping) {
        first = s.data();
        auto const allow_emergency_break = op.Behavior == Text::OverflowBehavior::OverflowForceWrap;
        ret.lines.clear();

        line_count = 0;
        while (first != last) {
            if (op.MaxLines > 0 && line_count >= op.MaxLines) {
                ellipsify_line(font, font_size, ret.lines.back(), ow);
                break;
            }
            auto lr = calc_line_wrap(font, font_size, *ow, first, last, allow_emergency_break);
            ret.lines.push_back(TextBlock::LineInfo{lr.line});
            first = lr.next;
            ++line_count;
        }
    }

    const float line_height = std::round(font_size);
    ret.size.x = 0;
    ret.size.y = ret.lines.size() * line_height;
    for (auto&& ln : ret.lines) {
        auto w = ln.advance + ln.ellipsis.value_or(0.0f);
        if (w > ret.size.x)
            ret.size.x = w;
    }

    if (op.Behavior == Text::OverflowBehavior::OverflowNone)
        if (ow && ret.size.x > *ow)
            ret.size.x = *ow;

    return ret;
}

auto MeasureText(
    std::string_view s, Text::OverflowPolicy const& op, std::optional<float> const& ow) -> ImVec2
{
    auto ret = MeasureTextEx(nullptr, 0.0f, s, op, ow);
    return ret.size;
}

auto CalcStackedSize(std::initializer_list<ImVec2> box_sizes, Axis dir, float gap) -> ImVec2
{
    if (box_sizes.size() == 0)
        return {};

    auto ret = ImVec2{0, 0};
    auto n = 0;

    switch (dir) {
    case Axis::Horz:
        for (auto const& b : box_sizes) {
            ret.x += b.x;
            n += int(b.x > 0);
            ret.y = std::max(ret.y, b.y);
        }
        if (n > 1)
            ret.x += (n - 1) * gap;
        break;

    case Axis::Vert:
        for (auto const& b : box_sizes) {
            ret.x = std::max(ret.x, b.x);
            ret.y += b.y;
            n += int(b.y > 0);
        }
        if (n > 1)
            ret.y += (n - 1) * gap;
        break;
    }
    return ret;
}

auto vround(ImVec2 const& v) -> ImVec2 { return ImVec2{std::round(v.x), std::round(v.y)}; }
auto vceil(ImVec2 const& v) -> ImVec2 { return ImVec2{std::ceil(v.x), std::ceil(v.y)}; }

// ------ blocks --------

IconBlock::IconBlock(ImPlus::Icon const& icon)
    : icon_{icon}
{
    Size = vceil(icon_.Measure());
}

IconBlock::IconBlock(ImPlus::Icon&& icon)
    : icon_{std::move(icon)}
{
    Size = vceil(icon_.Measure());
}

void IconBlock::RenderXY(ImDrawList* dl, ImVec2 const& pos, ImU32 c32) const
{
    icon_.Draw(dl, vround(pos), c32);
}

void IconBlock::RenderXY(ImDrawList* dl, ImVec2 const& pos, ImVec4 const& clr) const
{
    icon_.Draw(dl, vround(pos), ImGui::GetColorU32(clr));
}

void IconBlock::Display(ImVec4 const& clr) const
{
    auto window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    auto pos = vround(window->DC.CursorPos);
    pos.y += window->DC.CurrLineTextBaseOffset;
    auto bb = ImRect{pos, pos + Size};

    // auto dy = std::max(0.0f, std::round((Size.y - GImGui->FontSize) * 0.5f));
    auto dy = 0.0f;

    ImGui::ItemSize(Size, dy);
    if (ImGui::ItemAdd(bb, 0))
        RenderXY(window->DrawList, bb.Min, clr);
}

void IconBlock::Display() const { Display(ImGui::GetStyleColorVec4(ImGuiCol_Text)); }

TextBlock::TextBlock(std::string_view content, ImVec2 const& align, Text::OverflowPolicy const& op,
    std::optional<pt_length> const& ow, ImFont* font, float font_scale)
    : content_{content}
    , align_{align}
    , overflow_policy_{op}
    , overflow_width_{ow}
    , font_{font}
    , font_scale_{font_scale}
{
    if (!content_.empty()) {
        auto prev = GImGui->Font;
        if (font_ && font_ != prev)
            ImGui::SetCurrentFont(font_);
        auto t = MeasureTextEx(GImGui->Font, GImGui->Font->FontSize * font_scale_, content_,
            overflow_policy_, overflow_width_);
        if (font_ && font_ != prev)
            ImGui::SetCurrentFont(prev);
        lines_ = std::move(t.lines);
        Size = vceil(t.size);
    }
}

void TextBlock::Reflow(std::optional<pt_length> const& ow)
{
    if (Empty() || overflow_width_ == ow)
        return;

    overflow_width_ = ow;
    auto prev = GImGui->Font;
    if (font_ && font_ != prev)
        ImGui::SetCurrentFont(font_);
    auto t = MeasureTextEx(GImGui->Font, GImGui->Font->FontSize * font_scale_, content_,
        overflow_policy_, overflow_width_);
    if (font_ && font_ != prev)
        ImGui::SetCurrentFont(prev);
    lines_ = std::move(t.lines);
    Size = vceil(t.size);
}

void TextBlock::Render(
    ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max, ImU32 clr32) const
{
    if (content_.empty())
        return;

    auto cr = ImRect{
        dl->GetClipRectMin(),
        dl->GetClipRectMax(),
    };
    auto y = Aligned(align_.y, bb_min.y, bb_max.y, Size.y, true);
    if (y > cr.Max.y || y + Size.y < cr.Min.y)
        return;

    if (font_)
        ImGui::PushFont(font_);

    auto& font = *GImGui->Font;
    auto const font_size = GImGui->FontSize * font_scale_;
    auto const ellipsis_char_w = font_size * font.EllipsisCharStep / font.FontSize;

    auto first = content_.data();
    auto const last = first + content_.size();

    const float line_height = std::round(font_size);

    for (auto&& ln : lines_) {
        if (y + line_height < cr.Min.y) {
            y += line_height;
            continue;
        }

        if (y > cr.Max.y)
            break;

        auto const sum_w = ln.advance + ln.ellipsis.value_or(0.0f);

        auto x = Aligned(align_.x, bb_min.x, bb_max.x, sum_w, true);

        if (x > cr.Max.x || x + sum_w < cr.Min.x)
            continue;

        font.RenderText(dl, font_size, {x, y}, clr32, *reinterpret_cast<ImVec4*>(&cr), ln.data(),
            ln.data() + ln.size(), 0.0f, true);
        if (ln.ellipsis) {
            x += ln.advance;
            for (std::size_t i = 0; i < font.EllipsisCharCount; ++i) {
                font.RenderChar(dl, font_size, {x, y}, clr32, font.EllipsisChar);
                x += ellipsis_char_w;
            }
        }
        y += line_height;
    }

    if (font_)
        ImGui::PopFont();
}

void TextBlock::Render(
    ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max, ImVec4 const& clr) const
{
    Render(dl, bb_min, bb_max, ImGui::GetColorU32(clr));
}

void TextBlock::Display(ImVec4 const& clr, ImVec2 const& size_arg) const
{
    auto window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    auto size = ImGui::CalcItemSize(size_arg, Size.x, Size.y);
    auto pos = window->DC.CursorPos;
    pos.y += window->DC.CurrLineTextBaseOffset;
    auto bb = ImRect{pos, pos + size};
    ImGui::ItemSize(size, 0.0f);
    if (ImGui::ItemAdd(bb, 0))
        Render(window->DrawList, bb.Min, bb.Max, clr);
}

void TextBlock::Display(ImVec2 const& size_arg) const
{
    Display(ImGui::GetStyleColorVec4(ImGuiCol_Text), size_arg);
}

auto MakeContentDrawCallback(TextBlock const& block) -> Content::DrawCallback
{
    return [block](ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max,
               ColorSet const& colors) { block.Render(dl, bb_min, bb_max, colors.Content); };
}

auto MakeContentDrawCallback(TextBlock&& block) -> Content::DrawCallback
{
    return [block = std::move(block)](ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max,
               ColorSet const& colors) { block.Render(dl, bb_min, bb_max, colors.Content); };
}

CDBlock::CDBlock(std::string_view caption, std::string_view descr, ImVec2 const& align,
    CDOptions const& opts, Text::CDOverflowPolicy const& op,
    std::optional<pt_length> const& overflow_width)
{
    vert_align_ = align.y;
    if (caption.empty()) {
        // note: do not indent if there is no caption
        if (!descr.empty()) {
            Descr = TextBlock{
                descr, {align.x, 0}, op.Descr, overflow_width, opts.DescrFont, opts.DescrFontScale};
            Size = Descr.Size;
        }
        return;
    }

    if (descr.empty()) {
        Caption = TextBlock{caption, {align.x, 0}, op.Caption, overflow_width, opts.CaptionFont,
            opts.CaptionFontScale};
        Size = Caption.Size;
        descr_offset_ = 0.0f;
    }
    else {
        descr_offset_ = align.x <= 0 ? to_pt<rounded>(Style::ICD::DescrOffset()) : 0.0f;
        descr_opacity_ = Style::ICD::DescrOpacity();
    }

    // wrapping both
    descr_spacing_ = to_pt<rounded>(Style::ICD::DescrSpacing());

    auto dw = overflow_width;
    if (dw)
        dw = std::max({*dw, descr_offset_});

    Caption =
        TextBlock{caption, {align.x, 0}, op.Caption, dw, opts.CaptionFont, opts.CaptionFontScale};

    if (dw)
        dw = std::max(*dw, Caption.Size.x) - descr_offset_;

    Descr = TextBlock{descr, {align.x, 0}, op.Descr, dw, opts.DescrFont, opts.DescrFontScale};

    if (dw && Caption.overflow_width_) {
        *dw += descr_offset_;
        if (*dw > *Caption.overflow_width_)
            Caption.Reflow(dw);
    }

    Size = vround(CalcStackedSize(
        {Caption.Size, ImVec2{std::abs(descr_offset_) + Descr.Size.x, Descr.Size.y}}, Axis::Vert,
        descr_spacing_));
}

void CDBlock::Reflow(std::optional<pt_length> desired_overflow_width)
{
    if (Caption.Empty()) {
        Descr.Reflow(desired_overflow_width);
    }
    else if (Descr.Empty()) {
        Caption.Reflow(desired_overflow_width);
    }
    else {
        auto dw = desired_overflow_width;
        if (dw)
            dw = std::max({*dw, descr_offset_});

        Caption.Reflow(dw);
        if (dw)
            dw = std::max(*dw, Caption.Size.x) - descr_offset_;

        Descr.Reflow(dw);

        if (dw && Caption.overflow_width_) {
            *dw += descr_offset_;
            if (*dw > *Caption.overflow_width_)
                Caption.Reflow(dw);
        }
    }

    Size = vround(CalcStackedSize(
        {Caption.Size, ImVec2{std::abs(descr_offset_) + Descr.Size.x, Descr.Size.y}}, Axis::Vert,
        descr_spacing_));
}

void CDBlock::Render(
    ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max, ImVec4 const& clr) const
{
    auto y = Aligned(vert_align_, bb_min.y, bb_max.y, Size.y, true);
    if (!Caption.Empty()) {
        Caption.Render(dl, ImVec2{bb_min.x, y}, ImVec2{bb_max.x, y + Caption.Size.y}, clr);
        y += Caption.Size.y + descr_spacing_;
    }
    if (!Descr.Empty()) {
        auto c = clr;
        c.w *= descr_opacity_;
        Descr.Render(
            dl, ImVec2{bb_min.x + descr_offset_, y}, ImVec2{bb_max.x, y + Descr.Size.y}, c);
    }
}

auto CDBlock::NameForTestEngine() const -> std::string
{
#ifdef IMGUI_ENABLE_TEST_ENGINE
    return std::string{content.Caption};
#else
    return std::string{};
#endif
}

auto calc_dropdown_arrow_extra() -> float { return to_pt<rounded>(0.5_em); }

auto draw_dropdown_arrow(
    ImDrawList* dl, ImVec2 const& ref_pos, bool draw_east_of_refpos, ImVec4 const& clr)
{
    auto const sz = calc_dropdown_arrow_extra();
    auto const dx = draw_east_of_refpos ? std::floor(sz * 0.65f) : 0.0f;
    DrawArrow(dl, ImGuiDir_Down, {ref_pos.x + dx, ref_pos.y}, sz, ImGui::GetColorU32(clr));
}

inline auto horz_icon_spacing() -> float
{
    return std::round(ImGui::GetStyle().ItemInnerSpacing.x);
}

ICDBlock::ICDBlock(ICD_view const& content, Content::Layout layout, ICDOptions const& opts,
    Text::CDOverflowPolicy const& op, std::optional<pt_length> const& overflow_width)
    : layout_{layout}
    , icon_block_{content.Icon}
{
    if (opts.WithDropdownArrow)
        dropdown_width_ = calc_dropdown_arrow_extra();

    if (content.Caption.empty() && content.Descr.empty()) {
        // just an icon, nothing else
        Size = icon_block_.Size;
        Size.x += dropdown_width_;
        return;
    }

    auto const horz_icon_placement =
        layout_ == Content::Layout::HorzNear || layout_ == Content::Layout::HorzCenter;

    icon_gap = icon_block_.Empty()   ? 0_pt
               : horz_icon_placement ? horz_icon_spacing()
                                     : to_pt<rounded>(Style::ICD::VertIconSpacing());

    auto const descr_indent = horz_icon_placement ? Style::ICD::DescrOffset() : pt_length(0);

    auto const cd_opts = CDOptions{
        .CaptionFont = opts.CaptionFont,
        .CaptionFontScale = opts.CaptionFontScale,
        .DescrFont = opts.DescrFont,
        .DescrFontScale = opts.DescrFontScale,
    };

    // remaining wrapping with for caption and description
    if (horz_icon_placement) {
        if (overflow_width && !icon_block_.Empty()) {
            //
            //         ⭩IconGap
            // [ICON]<-->[CAPTION_CONTENT]       | <- original wrapping point
            //                [DESCR_CONTENT]    |
            //                                   |
            //           <---------------------->|
            //                       ⭦ text_wrap_width
            //
            auto dw = *overflow_width;
            dw = std::max(1.0f, dw - icon_block_.Size.x - icon_gap);
            ld_block_ = CDBlock{content.Caption, content.Descr, {0, 0}, cd_opts, op, dw};
        }
        else
            ld_block_ =
                CDBlock{content.Caption, content.Descr, {0, 0}, cd_opts, op, overflow_width};
    }
    else {
        // Stacking == Axis::Vert
        ld_block_ = CDBlock{content.Caption, content.Descr, {0.5f, 0}, cd_opts, op, overflow_width};
    }

    if (icon_block_.Empty()) {
        // no icon, just caption and/or description, plus optional dropdown arrow
        Size = ld_block_.Size;
        Size.x += dropdown_width_;
        return;
    }

    // we have both icon and texts
    if (horz_icon_placement || !dropdown_width_) {
        Size = CalcStackedSize({icon_block_.Size, ld_block_.Size},
            horz_icon_placement ? Axis::Horz : Axis::Vert, icon_gap);
        Size.x += dropdown_width_; // drop down to the right of everything
    }
    else {
        // special case
        //
        //      [ICON] <arrow>
        //          Caption
        //        Description
        //
        Size = CalcStackedSize(
            {ImVec2{icon_block_.Size.x + dropdown_width_, icon_block_.Size.y}, ld_block_.Size},
            Axis::Vert, icon_gap);
    }
}

void ICDBlock::Reflow(std::optional<pt_length> desired_overflow_width)
{
    auto const horz_icon_placement =
        layout_ == Content::Layout::HorzNear || layout_ == Content::Layout::HorzCenter;

    if (horz_icon_placement || icon_block_.Empty() || ld_block_.Empty()) {
        ld_block_.Reflow(desired_overflow_width);
        Size = CalcStackedSize(
            {ImVec2{icon_block_.Size.x + dropdown_width_, icon_block_.Size.y}, ld_block_.Size},
            horz_icon_placement ? Axis::Horz : Axis::Vert, icon_gap);
        return;
    }

    // horizontal stacking both icon and texts
    auto dw = desired_overflow_width;
    if (dw)
        dw = std::max(1.0f, *dw - icon_block_.Size.x - icon_gap);
    ld_block_.Reflow(dw);
    Size = CalcStackedSize({icon_block_.Size, ld_block_.Size},
        horz_icon_placement ? Axis::Horz : Axis::Vert, icon_gap);
    Size.x += dropdown_width_; // drop down to the right of everything
}

inline auto rounded_vec(float x, float y) -> ImVec2 { return {std::round(x), std::round(y)}; }

void ICDBlock::Render(
    ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max, ImVec4 const& clr) const
{
    auto const horz_icon_placement =
        layout_ == Content::Layout::HorzNear || layout_ == Content::Layout::HorzCenter;

    if (!horz_icon_placement) {

        // center everything horizontally
        auto const cx = (bb_min.x + bb_max.x) * 0.5f;

        // align vertically near top (VertNear) or centered (VertCentered)
        auto y = (layout_ == Content::Layout::VertNear) ? bb_min.y
                                                        : (bb_min.y + bb_max.y - Size.y) * 0.5f;

        if (ld_block_.Empty() && icon_block_.Empty()) {
            if (dropdown_width_)
                draw_dropdown_arrow(dl, rounded_vec(cx, y + Size.y * 0.5f), false, clr);
            return;
        }

        auto adjusted_bb_max_x = bb_max.x;

        if (!icon_block_.Empty()) {
            auto w = icon_block_.Size.x;
            auto h = icon_block_.Size.y;
            auto x = std::min(cx - w * 0.5f, bb_max.x - dropdown_width_ - w);
            icon_block_.RenderXY(dl, rounded_vec(x, y), clr);
            if (dropdown_width_)
                draw_dropdown_arrow(dl, rounded_vec(x + w, y + h * 0.5f), true, clr);
            y += h + icon_gap;
        }
        else {
            if (dropdown_width_) {
                adjusted_bb_max_x -= dropdown_width_;
                draw_dropdown_arrow(dl,
                    rounded_vec(
                        cx + (ld_block_.Size.x - dropdown_width_) * 0.5f, y + Size.y * 0.5f),
                    true, clr);
            }
        }
        ld_block_.Render(dl, {bb_min.x, y}, {adjusted_bb_max_x, y + ld_block_.Size.y}, clr);
    }
    else {
        // horz_icon_placement
        // center everything vertically
        auto const cy = (bb_min.y + bb_max.y) * 0.5f;

        // content is alyays flush-left, then place the aligned content near the left edge
        // (HorzNear) or center it horizontally (HorzCentered)
        auto x = std::round((layout_ == Content::Layout::HorzNear)
                                ? bb_min.x
                                : (bb_min.x + bb_max.x - Size.x) * 0.5f);

        auto max_x = x + Size.x;
        auto const top_y = std::max(bb_min.y, (bb_min.y + bb_max.y - ld_block_.Size.y) * 0.5f);

        auto clip_for_arrow = false;

        if (dropdown_width_) {
            if (max_x > bb_max.x) {
                clip_for_arrow = true;
                max_x = bb_max.x;
            }
            max_x -= dropdown_width_;
            draw_dropdown_arrow(dl, {max_x, cy}, true, clr);
            if (clip_for_arrow)
                ImGui::PushClipRect(bb_min, {max_x, bb_max.y}, true);
        }

        if (!icon_block_.Empty()) {
            auto y2 = cy - icon_block_.Size.y * 0.5f;
            icon_block_.RenderXY(dl, {x, std::round(std::min(top_y, y2))}, clr);
            x = std::round(x + icon_block_.Size.x + icon_gap);
        }

        if (!ld_block_.Empty())
            ld_block_.Render(dl, {x, top_y}, {max_x, top_y + ld_block_.Size.y}, clr);

        if (clip_for_arrow)
            ImGui::PopClipRect();
    }
}

auto ICDBlock::NameForTestEngine() const -> std::string { return ld_block_.NameForTestEngine(); }

void ICDBlock::Display(ImVec4 const& clr, ImVec2 const& size_arg) const
{
    auto window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    auto size = ImGui::CalcItemSize(size_arg, Size.x, Size.y);
    auto pos = window->DC.CursorPos;
    pos.y += window->DC.CurrLineTextBaseOffset;
    auto bb = ImRect{pos, pos + size};
    ImGui::ItemSize(size, 0.0);
    if (ImGui::ItemAdd(bb, 0))
        Render(window->DrawList, bb.Min, bb.Max, clr);
}

void ICDBlock::Display(ImVec2 const& size_arg) const
{
    Display(ImGui::GetStyleColorVec4(ImGuiCol_Text), size_arg);
}

auto MakeContentDrawCallback(ICDBlock const& block) -> Content::DrawCallback
{
    return [block](ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max,
               ColorSet const& colors) { block.Render(dl, bb_min, bb_max, colors.Content); };
}

auto MakeContentDrawCallback(ICDBlock&& block) -> Content::DrawCallback
{
    return [block = std::move(block)](ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max,
               ColorSet const& colors) { block.Render(dl, bb_min, bb_max, colors.Content); };
}

} // namespace ImPlus