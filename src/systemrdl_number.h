#pragma once

#include "systemrdl_bitvector.h"

#include <cstddef>
#include <string>

namespace systemrdl {

// Largest width a sized literal may declare, per the SystemVerilog limit.
constexpr size_t kMaxLiteralWidth = 16777215;

// Longest literal text accepted. Anything beyond this is rejected before any
// digits are accumulated so a pathological input cannot drive allocation.
constexpr size_t kMaxLiteralChars = 65536;

// Numeric base a literal was written in.
enum class NumberBase { Unknown, Binary, Decimal, Hexadecimal };

// Result of parsing one SystemRDL numeric literal.
//
// error is the only thing a caller must check. A failed parse never yields a
// usable value: the whole point of this type is that a malformed or oversized
// literal cannot be mistaken for a small number.
struct NumberLiteral
{
    BitVector   value;
    size_t      width              = 0;
    NumberBase  base               = NumberBase::Unknown;
    bool        has_explicit_width = false;
    bool        error              = false;
    std::string error_message;
};

// Parse a SystemRDL numeric literal.
//
// Accepts exactly the three forms the grammar defines in SystemRDL.g4:
//
//   INT      [0-9][0-9_]*                       42, 1_000
//   HEX_INT  0[xX][0-9a-fA-F][0-9a-fA-F_]*      0xFF, 0x1_0000_0000
//   VLOG_INT [0-9]+'[bdh]<digits>               8'hFF, 8'b1010_1010, 16'd200
//
// Underscores are digit separators and are ignored. A sized literal whose
// value does not fit in the declared width is an error, matching the reference
// compiler, which rejects 4'hFF rather than truncating it.
//
// Anything else sets error. There is no fallback value and no partial parse:
// std::stoll stopping at an apostrophe is exactly the behaviour this replaces.
NumberLiteral parse_number(const std::string &text);

} // namespace systemrdl
