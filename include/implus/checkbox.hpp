#pragma once

#include <imgui.h>
#include <string_view>

#include "implus/id.hpp"
#include "implus/icd.hpp"
#include "implus/length.hpp"
#include "implus/text.hpp"
#include "implus/overridable.hpp"

namespace ImPlus {

namespace Style::Checkbox {
inline auto OverflowPolicy =
    overridable<Text::CDOverflowPolicy>({Text::OverflowWrap, Text::OverflowWrap});
inline auto OverflowWidth = overridable<std::optional<length>>{};

// see also (in blocks.hpp):
// - Style::ICD::DescrOffset
// - Style::ICD::DescrSpacing
// - Style::ICD::DescrOpacity

} // namespace Style::Checkbox

enum class CheckboxFlags {
    SmallCheck = 1 << 0,
};

constexpr auto operator|(CheckboxFlags a, CheckboxFlags b) -> CheckboxFlags
{
    return CheckboxFlags(unsigned(a) | unsigned(b));
};
constexpr auto operator&(CheckboxFlags a, CheckboxFlags b) -> CheckboxFlags
{
    return CheckboxFlags(unsigned(a) & unsigned(b));
};
constexpr auto operator&&(CheckboxFlags a, CheckboxFlags b) -> bool { return bool(a) && bool(b); };
constexpr auto operator&&(CheckboxFlags a, bool b) -> bool { return bool(a) && b; };
constexpr auto operator&&(bool a, CheckboxFlags b) -> bool { return a && bool(b); };

// Checkbox with explicit style specs
auto Checkbox(ImID, CD_view const& content, bool& checked,
    Text::CDOverflowPolicy const& overflow_policty, std::optional<length> const& overflow_width,
    CheckboxFlags flags = {}) -> bool;

// Checkbox using Style::Checkbox
auto Checkbox(ImID, CD_view const& content, bool& checked, CheckboxFlags flags = {}) -> bool;

} // namespace ImPlus