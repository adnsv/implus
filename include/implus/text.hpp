#pragma once

#include "alignment.hpp"
#include "length.hpp"
#include <imgui.h>
#include <optional>
#include <string_view>
#include <variant>

namespace ImPlus::Text {

enum OverflowBehavior {
    OverflowNone,
    OverflowEllipsify, // ellipsifies each line
    OverflowWrap,
    OverflowForceWrap, // wrap at unwrappable points to force-fit
};

// OverflowPolicy defines how text lines are wrapped/ellipsified/trimmed.
// MaxLines allows to limit the number of text lines produced due to wrapping, or EOL handling.
// If the number of actual lines exceeds MaxLines, the last allowed line then is ellipsified.
struct OverflowPolicy {
    OverflowBehavior Behavior = OverflowNone;
    unsigned MaxLines = 16;
};

struct CDOverflowPolicy {
    Text::OverflowPolicy Caption;
    Text::OverflowPolicy Descr;

    constexpr CDOverflowPolicy() noexcept = default;

    constexpr CDOverflowPolicy(CDOverflowPolicy const&) noexcept = default;

    constexpr CDOverflowPolicy(
        Text::OverflowPolicy const& caption_op, Text::OverflowPolicy const& descr_op) noexcept
        : Caption{caption_op}
        , Descr{descr_op}
    {
    }

    constexpr CDOverflowPolicy(Text::OverflowPolicy const& op_for_both) noexcept
        : Caption{op_for_both}
        , Descr{op_for_both}
    {
    }

    constexpr CDOverflowPolicy(
        OverflowBehavior caption_ob, OverflowBehavior descr_ob, unsigned max_lines = 16)
        : Caption{caption_ob, max_lines}
        , Descr{descr_ob, max_lines}
    {
    }

    constexpr CDOverflowPolicy(OverflowBehavior ob_for_both, unsigned max_lines = 16)
        : Caption{ob_for_both, max_lines}
        , Descr{ob_for_both, max_lines}
    {
    }
};

using Align = ::ImPlus::Align;

} // namespace ImPlus::Text