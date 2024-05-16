#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "implus/dlg.hpp"
#include "implus/flow.hpp"
#include <unordered_map>

// #define DBG_FLOW_VISUALS

namespace ImPlus {

struct flow_state {
    pt_length inline_indent = 0;
};

std::unordered_map<ImGuiID, flow_state> flow_states;

#ifdef DBG_FLOW_VISUALS
void dbg_box(ImVec2 const& bb_min, ImVec2 const& bb_max, ImVec4 const& clr)
{
    auto window = ImGui::GetCurrentWindow();
    auto c32 = ImGui::GetColorU32(clr);
    window->DrawList->AddRectFilled(bb_min, bb_max, c32);
};
#endif

Flow::Flow(ImID id, length const& desired_overflow_width, FlowOptions const& opts)
    : flow_id{id}
    , auto_sizing{opts.AutoSizing}
{
    auto avail_w = ImGui::GetContentRegionAvail().x;
    auto want_w = to_pt(desired_overflow_width);
    desired_width_delta = want_w - avail_w;
    start_cursor_pos = ImGui::GetCursorPos();

    initial_indent = ImGui::GetCurrentWindowRead()->DC.Indent.x;

    auto const min_inline_indent = to_pt<rounded>(Style::Flow::MinInlineLabelIndent());
    max_inline_indent = to_pt<rounded>(Style::Flow::MaxInlineLabelIndent());
    this_frame_inline_indent = min_inline_indent;
    next_frame_inline_indent = min_inline_indent;

    if (flow_id) {
        auto f = flow_states.find(flow_id);
        if (f != flow_states.end())
            this_frame_inline_indent = std::max(this_frame_inline_indent, f->second.inline_indent);
    }
}

Flow::Flow(ImID id, length const& desired_wrap_width)
    : Flow{id, desired_wrap_width, FlowOptions{.AutoSizing = Dlg::IsAutosizing()}}
{
}

Flow::Flow(length const& desired_wrap_width)
    : Flow{ImID{ImGuiID(0)}, desired_wrap_width, FlowOptions{.AutoSizing = Dlg::IsAutosizing()}}
{
}

Flow::~Flow()
{
    if (flow_id) {
        next_frame_inline_indent = std::min(next_frame_inline_indent, max_inline_indent);

        flow_states[flow_id] = flow_state{
            .inline_indent = next_frame_inline_indent,
        };
    }
}

auto Flow::AvailWidth() const -> pt_length { return ImGui::GetContentRegionAvail().x; }

auto Flow::OverflowWidth() const -> pt_length
{
    auto avail_w = ImGui::GetContentRegionAvail().x;
    auto w = avail_w + desired_width_delta;
    if (!auto_sizing && w > avail_w)
        w = avail_w;

    if (w < min_effective_overflow_width)
        w = min_effective_overflow_width;
    return w;
}

auto Flow::StretchWidth() const -> pt_length { return auto_sizing ? 0.0f : AvailWidth(); }

void Flow::Spacing() { ImGui::Spacing(); }

void Flow::Spacing(length dy)
{
    auto y = ImGui::GetCursorPosY();
    ImGui::SetCursorPosY(y + to_pt<rounded>(dy));
}

void Flow::Paragraph(std::string_view s, ParagraphOptions const& opts)
{
    auto const reserve_pt = to_pt(opts.ReserveExtraWidth);

    auto overflow_w = OverflowWidth() - reserve_pt;

    auto blk = ImPlus::TextBlock{s, ImVec2{0, 0},
        Text::OverflowPolicy{.Behavior = opts.OverflowBehavior, .MaxLines = opts.MaxLines},
        overflow_w, opts.Font.imfont()};

    auto const clr = opts.Color.value_or(ImGui::GetStyleColorVec4(ImGuiCol_Text));
    auto sz_x = 0.0f;
    if (!auto_sizing) {
        sz_x = AvailWidth() - reserve_pt;
        if (sz_x > blk.Size.x)
            sz_x = blk.Size.x;
    }

    blk.Display(clr, ImVec2{sz_x, 0});
}

void Flow::ICD(ICD_view const& v)
{
    auto blk = ImPlus::ICDBlock{v, Content::Layout::HorzNear, {},
        Text::OverflowPolicy{.Behavior = Text::OverflowWrap, .MaxLines = 10}, OverflowWidth()};
    blk.Display(ImVec2{StretchWidth(), 0});
}

auto Flow::CollapsingHeader(ImID id, std::string_view caption, ImGuiTreeNodeFlags flags) -> bool
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    auto const label_size = ImGui::CalcTextSize(caption.data(), caption.data() + caption.size());

    flags |= ImGuiTreeNodeFlags_CollapsingHeader;

    // do same calculations as ImGui::TreeNodeBehavior to figure out the layout
    auto& g = *GImGui;
    auto const& style = g.Style;
    auto const display_frame = (flags & ImGuiTreeNodeFlags_Framed) != 0;
    auto const padding = (display_frame || (flags & ImGuiTreeNodeFlags_FramePadding))
                             ? style.FramePadding
                             : ImVec2(style.FramePadding.x,
                                   ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));

    auto const frame_height =
        std::max(std::min(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2),
            label_size.y + padding.y * 2);
    ImRect frame_bb;
    frame_bb.Min.x = (flags & ImGuiTreeNodeFlags_SpanFullWidth) ? window->WorkRect.Min.x
                                                                : window->DC.CursorPos.x;
    frame_bb.Min.y = window->DC.CursorPos.y;
    frame_bb.Max.x = window->WorkRect.Max.x;
    frame_bb.Max.y = window->DC.CursorPos.y + frame_height;
    if (display_frame) {
        frame_bb.Min.x -= IM_FLOOR(window->WindowPadding.x * 0.5f - 1.0f);
        frame_bb.Max.x += IM_FLOOR(window->WindowPadding.x * 0.5f);
    }

    auto const text_offset_x = g.FontSize + (display_frame ? padding.x * 3 : padding.x * 2);
    auto const text_offset_y = std::max(padding.y, window->DC.CurrLineTextBaseOffset);
    auto const text_width =
        g.FontSize + (label_size.x > 0.0f ? label_size.x + padding.x * 2 : 0.0f);
    auto const text_pos =
        ImVec2(window->DC.CursorPos.x + text_offset_x, window->DC.CursorPos.y + text_offset_y);

    auto const expanded =
        ImGui::TreeNodeBehavior(id, flags, caption.data(), caption.data() + caption.size());

    last_collapsing_header_frame.Min = frame_bb.Min;
    last_collapsing_header_frame.Max = frame_bb.Max;
    last_collapsing_header_label_xmax = text_pos.x + label_size.x;

    return expanded;
}

void Flow::CollapsingHeaderOverlayText(std::string_view s)
{
    if (s.empty())
        return;
    auto r = last_collapsing_header_frame;
    r.Min.x = last_collapsing_header_label_xmax + to_pt(1.0_em);
    r.Max.x -= ImGui::GetStyle().FramePadding.x;

    if (r.Empty())
        return;

    auto blk = ImPlus::TextBlock(
        s, {0, 0.5f}, Text::OverflowPolicy{Text::OverflowEllipsify, 1}, r.Width());

    auto indented_x = ImGui::GetCurrentWindowRead()->Pos.x + this_frame_inline_indent;

    r.Min.x = std::max(r.Min.x, std::min(indented_x, r.Max.x - blk.Size.x));
    auto dl = ImGui::GetWindowDrawList();
    blk.Render(dl, r.Min, r.Max, ImGui::GetColorU32(ImGuiCol_Text));
}

auto Flow::FieldPrefix(ICD_view const& key, ValueWidthSpec const& value_width,
    pt_length extra_label_width, bool key_with_frame_padding) -> pt_length
{
    auto window = ImGui::GetCurrentWindowRead();
    auto const& sty = ImGui::GetStyle();

    auto const kvp = Style::Flow::KeyValPlacement();

    auto op = Text::OverflowPolicy{.Behavior = Text::OverflowEllipsify, .MaxLines = 4};
    auto ow = OverflowWidth();
    auto const hide_descr = kvp == KeyValPlacement::Inline;

    auto const key_blk = ImPlus::ICDBlock{
        hide_descr ? ICD_view{key.Icon, key.Caption} : key, Content::Layout::HorzNear, {}, op, ow};

    auto const key_w = key_blk.Size.x;

    if (extra_label_width)
        extra_label_width = std::round(sty.ItemInnerSpacing.x + extra_label_width + 0.5f);

    auto const val_min_width = to_pt<rounded>(value_width.MinWidth) + extra_label_width;
    auto const val_desired_width =
        std::max(val_min_width, to_pt<rounded>(value_width.DesiredWidth) + extra_label_width);
    auto const val_max_width =
        std::max(val_desired_width, to_pt<rounded>(value_width.StretchWidth) + extra_label_width);

    auto const inline_spacing = to_pt<rounded>(Style::Flow::InlineValSpacing());
    auto const inline_indent = this_frame_inline_indent;

    // block placement
    if (kvp == KeyValPlacement::Block) {
        auto p = ImGui::GetCursorPos();
        if (!key.Empty()) {
            if (key_with_frame_padding)
                ImGui::AlignTextToFramePadding();
            key_blk.Display(Style::Flow::KeyColor());
            p = ImGui::GetCursorPos();
            p.y -= sty.ItemSpacing.y;
            ImGui::SetCursorPos(p);
        }

        if (auto_sizing)
            return val_desired_width - extra_label_width;

        auto const avail_w = std::max(val_min_width, AvailWidth());
        return std::min(avail_w, val_max_width) - extra_label_width;
    }

    // inline placement
    auto xmin = ImGui::GetCursorPosX();

    auto key_r = xmin;
    auto same_line = true;
    if (!key.Empty()) {
        key_r = xmin + key_w + inline_spacing;
        if (key_r > initial_indent + max_inline_indent)
            same_line = false;
        else if (key_r > initial_indent + next_frame_inline_indent)
            next_frame_inline_indent = key_r - initial_indent;
    }

    auto field_l = std::max(same_line ? key_r : xmin, initial_indent + inline_indent);
    auto field_r = field_l + val_desired_width;
    if (!auto_sizing) {
        auto const region_r = xmin + AvailWidth();
        field_r = std::min(field_l + val_max_width, region_r);
        field_l = std::max(xmin, std::min(field_l, field_r - val_min_width));
        if (same_line && field_l < key_r)
            same_line = false;

        field_r = std::max(field_r, val_min_width);
    }
#ifdef DBG_FLOW_VISUALS
    auto const dx = window->Pos.x - window->Scroll.x;
    auto const y = window->DC.CursorPos.y;
    dbg_box({dx + field_l, y}, {dx + field_r, y + 4}, {0, 1, 0, 0.7f});
#endif

    auto x = xmin;
    if (!key.Empty()) {
        if (key_with_frame_padding)
            ImGui::AlignTextToFramePadding();
        key_blk.Display(Style::Flow::KeyColor());
        if (same_line)
            ImGui::SameLine();
        else {
            auto p = ImGui::GetCursorPos();
            p.y -= sty.ItemSpacing.y;
            ImGui::SetCursorPos(p);
        }
    }

    ImGui::SetCursorPosX(field_l);
    return field_r - field_l - extra_label_width;
}

void Flow::TextField(ICD_view const& key, std::string_view val, Text::OverflowPolicy const& op,
    pt_length reserve_extra_width, bool key_with_frame_padding)
{
    auto blk = ImPlus::TextBlock{val, {0, 0}, op};
    auto width_spec = ImPlus::ValueWidthSpec{.DesiredWidth = blk.Size.x};
    if (op.Behavior != ImPlus::Text::OverflowNone)
        width_spec.MinWidth = 4_em;

    auto const content_w =
        FieldPrefix(key, width_spec, reserve_extra_width, key_with_frame_padding);
    if (op.Behavior != ImPlus::Text::OverflowNone)
        blk.Reflow(content_w);

    blk.Display();
}

FlowContent::FlowContent(display_callback&& on_display)
    : on_display_{std::move(on_display)}
{
}

FlowContent::FlowContent(std::string_view sv)
{
    if (sv.empty())
        return;
    auto s = std::string{sv};
    on_display_ = [s = std::move(s)](Flow& f) { f.Paragraph(s); };
}

auto FlowContent::Empty() const -> bool { return !on_display_; }

void FlowContent::Display(Flow& f) const
{
    if (on_display_)
        on_display_(f);
}

void BeginMainInstructionGroup(
    ImPlus::Icon const& icon, std::string const& main_instruction, FlowContent const& description)
{
    auto indent = 0.0f;

    if (!icon.Empty()) {
        ImGui::BeginGroup();
        auto blk = ImPlus::IconBlock{icon};
        auto save_pos = ImGui::GetCursorPosY();
        if (!main_instruction.empty()) {
            if (auto f = GetMainInstructionFont().imfont(); f && f->FontSize > blk.Size.y) {
                auto dy = (f->FontSize - blk.Size.y) * 0.5f;
                ImGui::SetCursorPosY(save_pos + dy);
            }
        }
        blk.Display();
        indent = std::round(blk.Size.x + ImGui::GetStyle().ItemSpacing.x);
        ImGui::SetCursorPosY(save_pos);
        ImGui::EndGroup();
        ImGui::SameLine(indent);
    }

    ImGui::BeginGroup();

    auto ctx = Flow{"##dlg.main.instruction.flow", 20_em};
    if (!main_instruction.empty())
        ctx.Paragraph(main_instruction, {.Font = GetMainInstructionFont()});

    if (!description.Empty()) {
        if (!main_instruction.empty())
            ImGui::Spacing();
        description.Display(ctx);
    }
}

auto main_instruction_font = Font::Resource{};

void SetMainInstructionFont(Font::Resource font) { main_instruction_font = font; }
auto GetMainInstructionFont() -> Font::Resource { return main_instruction_font; }

} // namespace ImPlus