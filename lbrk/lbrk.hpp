#pragma once

// DO NOT EDIT: Automatically-generated file
//
// original UCD file: LineBreak-13.0.0.txt
// original UCD timestamp: 2020-02-17, 07:43:02 GMT [KW, LI]

// clang-format off

#include <cstdint>

namespace lbrk {

// ba is a break action that indicates whether the
// line break is allowed, forced, or fodbidden.
enum class lba {
    allow, // break allowed
    force, // mandatory break
    forbid // no break allowed
};

// lbc corresponds to the line-break class as described
// in Unicode Standard Annex #14 (Table 1) available
// at https://www.unicode.org/reports/tr14/
//
// The original line-break class values from Table 1
// have the followng modifications applied:
// - SOT was added for start-of-text processing
// - AI was resolved to AL [LB1]
// - CJ was resolved to NS [LB1]
// - SA was resolved to CM (for GC=Mn and GC=Mc); AL (other GCs) [LB1]
// - SG was resolved to AL [LB1]
// - XX was resolved to AL [LB1]
//
enum class lbc : uint8_t {
    SOT, // Start-of-text
    AL,  // Alphabetic
    B2,  // Break Opportunity Before and After
    BA,  // Break After
    BB,  // Break Before
    BK,  // Mandatory Break
    CB,  // Contingent Break Opportunity
    CL,  // Close Punctuation
    CM,  // Combining Mark
    CP,  // Close Parenthesis
    CR,  // Carriage Return
    EB,  // Emoji Base
    EM,  // Emoji Modifier
    EX,  // Exclamation/Interrogation
    GL,  // Non-breaking ("Glue")
    H2,  // Hangul LV Syllable
    H3,  // Hangul LVT Syllable
    HL,  // Hebrew Letter
    HY,  // Hyphen
    ID,  // Ideographic
    IN,  // Inseparable
    IS,  // Infix Numeric Separator
    JL,  // Hangul L Jamo
    JT,  // Hangul T Jamo
    JV,  // Hangul V Jamo
    LF,  // Line Feed
    NL,  // Next Line
    NS,  // Nonstarter
    NU,  // Numeric
    OP,  // Open Punctuation
    PO,  // Postfix Numeric
    PR,  // Prefix Numeric
    QU,  // Quotation
    RI,  // Regional Indicator
    SP,  // Space
    SY,  // Symbols Allowing Break After
    WJ,  // Word Joiner
    ZW,  // Zero Width Space
    ZWJ, // Zero Width Joiner
};

auto get_class(char32_t cp) -> lbc;

constexpr auto is_trimmable(lbc c)
{
    return c == lbc::SP || c == lbc::CR || c == lbc::LF || c == lbc::NL || c == lbc::CM ||
           c == lbc::BK;
}

inline auto is_trimmable(char32_t cp) {
    return is_trimmable(get_class(cp));
}


// context internally tracks the state required for
// applying line breaking rules to an incoming
// sequence of line-break classes
struct context {
    lbc curr_a; // real current
    lbc curr_e; // effective current (after LB9)
    lbc prev;   // one before current

    // calc_action produces the break action that needs
    // to be applied before an incoming codepoint of the
    // specified line-break class.
    auto calc_action(lbc n) -> lba; 
};

} // namespace tint::lbrk