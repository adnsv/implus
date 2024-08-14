#pragma once

#include <imgui.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "implus/alignment.hpp"
#include "implus/content.hpp"
#include "implus/icd.hpp"
#include "implus/overridable.hpp"
#include "implus/text.hpp"

namespace ImPlus {

struct line_view : public std::string_view {
    float advance = 0.0f;
    line_view(line_view const&) noexcept = default;
    line_view(char const* text_begin, char const* text_end, float adv) noexcept
        : std::string_view{text_begin, std::size_t(text_end - text_begin)}
        , advance{adv}
    {
    }
};

namespace Style::ICD {

// DescrOffset returns pixel length of an indent that is added to the stacked description
// when doing near-aligned vertical stacking.
//
//   [CAPTION_CONTENT]
//   <--->[DESCR_CONTENT]
//      ⭦ DescrOffset()
//
inline auto DescrOffset = overridable<length>(0_pt);

// DescrSpacing returns pixel length of a vertical gap between the caption and
// the descr when description is placed below the caption content.
//
//   [CAPTION_CONTENT]
//     ↕ DescrSpacing()
//   [DESCR_CONTENT]
//
inline auto DescrSpacing = overridable<length>(0.1_em);

// DescrOpacity is the relative opacity of the descr when rendered within ICD
inline auto DescrOpacity = overridable<float>(0.7f);

// Stacking - default direction of stacking ICD content
inline auto Stacking = overridable<Axis>(Axis::Horz);

// VertIconSpacing returns pixel length of a gap between an icon and associated
// texts (caption, descr) for layouts when icon is placed above the description.
//
// [ICON] ↕ VertIconSpacing() [TEXTS]
//
inline auto VertIconSpacing = overridable<length>(0.1_em);

} // namespace Style::ICD

struct BlockBase {
    ImVec2 Size = {0, 0};
};

struct IconBlock : public BlockBase {
protected:
    ImPlus::Icon icon_ = {};

public:
    IconBlock() {}
    IconBlock(IconBlock const&) = default;
    IconBlock(ImPlus::Icon const&);
    IconBlock(ImPlus::Icon&&);

    auto Empty() const { return icon_.Empty(); }

    // pos is top-left corner.
    void RenderXY(ImDrawList* dl, ImVec2 const& pos, ImU32 c32) const;
    void RenderXY(ImDrawList* dl, ImVec2 const& pos, ImVec4 const& clr) const;

    void Display(ImVec4 const& clr) const;
    void Display() const;
};

struct CDBlock;

// TextBlock - a measured wrapped/ellipsified block of text.
//
// - content needs to be pre-stripped from `##suffix` if required
//
struct TextBlock : public BlockBase {
    struct LineInfo : public line_view {
        std::optional<float> ellipsis;
    };

protected:
    friend struct CDBlock;
    friend auto MakeContentDrawCallback(TextBlock const&) -> Content::DrawCallback;
    friend auto MakeContentDrawCallback(TextBlock&&) -> Content::DrawCallback;

    std::string_view content_ = {};
    ImVec2 align_ = {0, 0};
    Text::OverflowPolicy overflow_policy_ = {};
    std::optional<pt_length> overflow_width_ = {}; // in screen points
    ImFont* font_ = nullptr;
    float font_scale_ = 1.0f;
    std::vector<LineInfo> lines_;

public:
    TextBlock() {}
    TextBlock(TextBlock const&) = default;
    TextBlock(std::string_view content, ImVec2 const& align, Text::OverflowPolicy const& = {},
        std::optional<pt_length> const& overflow_width = {}, ImFont* font = nullptr, float font_scale = 1.0f);

    auto Empty() const { return content_.empty(); }

    void Reflow(std::optional<pt_length> const& overflow_width);

    void Render(ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max, ImU32 clr32) const;
    void Render(
        ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max, ImVec4 const& clr) const;

    void Display(ImVec4 const& clr, ImVec2 const& size_arg = {}) const;
    void Display(ImVec2 const& size_arg = {}) const;
};

// CDBlock contains caption and description
//
//    [CAPTION_CONTENT]
//          ↕ VertGap
//    <---->[DESCR_CONTENT]
//       ⭦ DescrOffset
//
// note: DescrOffset and VertGap are used only when both Caption and Descr are non-empty.
//
struct CDBlock : public BlockBase {
private:
    pt_length descr_offset_ = 0.0f;  // horz descr offset
    pt_length descr_spacing_ = 0.0f; // vertical gap begween caption and descr
    float descr_opacity_ = 1.0f;
    float vert_align_ = 0.0f; // horz alignment is picked up from caption and descr blocks

public:
    TextBlock Caption = {};
    TextBlock Descr = {};

    CDBlock() = default;
    CDBlock(std::string_view caption, std::string_view descr, ImVec2 const& align,
        CDOptions const& opts = {}, Text::CDOverflowPolicy const& op = {},
        std::optional<pt_length> const& overflow_width = {});
    CDBlock(CD_view const& content, ImVec2 const& align, CDOptions const& opts,
        Text::CDOverflowPolicy const& op, std::optional<pt_length> const& overflow_width)
        : CDBlock{content.Caption, content.Descr, align, opts, op, overflow_width}
    {
    }

    auto Empty() const { return Caption.Empty() && Descr.Empty(); }

    void Reflow(std::optional<float> desired_overflow_width);
    void Render(
        ImDrawList* dl, ImVec2 const& bb_min, ImVec2 const& bb_max, ImVec4 const& clr) const;

    auto NameForTestEngine() const -> std::string;
};

// ICDBlock contains icon, caption, and description
struct ICDBlock : public BlockBase {
protected:
    Content::Layout layout_;
    pt_length icon_gap = 0.0f;
    pt_length dropdown_width_ = {}; // in screen pixels
    IconBlock icon_block_;
    CDBlock ld_block_; // both caption and descr

public:
    ICDBlock(ICD_view const& content, Content::Layout layout, ICDOptions const& opts = {},
        Text::CDOverflowPolicy const& op = {}, std::optional<pt_length> const& overflow_width = {});

    void Reflow(std::optional<pt_length> desired_overflow_width);
    void Render(
        ImDrawList* draw_list, ImVec2 const& bb_min, ImVec2 const& bb_max, ImVec4 const& clr) const;

    void Display(ImVec4 const& clr, ImVec2 const& size_arg = {}) const;
    void Display(ImVec2 const& size_arg = {}) const;

    auto Empty() const -> bool { return icon_block_.Empty() && ld_block_.Empty(); }

    auto NameForTestEngine() const -> std::string;
};

auto MakeContentDrawCallback(ICDBlock const& block) -> Content::DrawCallback;
auto MakeContentDrawCallback(ICDBlock&& block) -> Content::DrawCallback;

} // namespace ImPlus