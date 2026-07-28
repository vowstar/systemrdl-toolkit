#include "systemrdl_number.h"

namespace systemrdl {

namespace {

NumberLiteral make_error(const std::string &message)
{
    NumberLiteral result;
    result.error         = true;
    result.error_message = message;
    return result;
}

uint32_t radix_of(NumberBase base)
{
    switch (base) {
    case NumberBase::Binary:
        return 2;
    case NumberBase::Decimal:
        return 10;
    case NumberBase::Hexadecimal:
        return 16;
    default:
        return 0;
    }
}

// Digit weight for the given radix, or -1 when the character is not a digit of
// that radix.
int digit_weight(char c, uint32_t radix)
{
    int weight = -1;
    if (c >= '0' && c <= '9') {
        weight = c - '0';
    } else if (c >= 'a' && c <= 'f') {
        weight = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        weight = c - 'A' + 10;
    }
    if (weight < 0 || static_cast<uint32_t>(weight) >= radix) {
        return -1;
    }
    return weight;
}

const char *base_name(NumberBase base)
{
    switch (base) {
    case NumberBase::Binary:
        return "binary";
    case NumberBase::Decimal:
        return "decimal";
    case NumberBase::Hexadecimal:
        return "hexadecimal";
    default:
        return "unknown";
    }
}

// Accumulate digits, ignoring underscore separators. The grammar allows an
// underscore anywhere except as the first character, which the caller has
// already checked.
bool accumulate_digits(const std::string &digits, NumberBase base, BitVector &out)
{
    const uint32_t radix = radix_of(base);
    if (radix == 0 || digits.empty()) {
        return false;
    }

    BitVector value;
    bool      saw_digit = false;
    for (char c : digits) {
        if (c == '_') {
            continue;
        }
        const int weight = digit_weight(c, radix);
        if (weight < 0) {
            return false;
        }
        value.mul_add_small(radix, static_cast<uint32_t>(weight));
        saw_digit = true;
    }
    if (!saw_digit) {
        return false;
    }
    out = value;
    return true;
}

} // namespace

NumberLiteral parse_number(const std::string &text)
{
    if (text.empty()) {
        return make_error("empty numeric literal");
    }
    if (text.size() > kMaxLiteralChars) {
        return make_error(
            "numeric literal is longer than " + std::to_string(kMaxLiteralChars) + " characters");
    }

    NumberLiteral result;
    std::string   digits;

    const std::string::size_type tick = text.find('\'');
    if (tick != std::string::npos) {
        // Sized literal: [0-9]+ ' [bdh] digits
        if (tick == 0) {
            return make_error("sized literal '" + text + "' has no width");
        }

        size_t width = 0;
        for (std::string::size_type i = 0; i < tick; ++i) {
            const char c = text[i];
            if (c < '0' || c > '9') {
                return make_error("sized literal '" + text + "' has a non-numeric width");
            }
            width = width * 10 + static_cast<size_t>(c - '0');
            if (width > kMaxLiteralWidth) {
                return make_error(
                    "sized literal '" + text + "' declares a width above "
                    + std::to_string(kMaxLiteralWidth));
            }
        }
        if (width == 0) {
            return make_error("sized literal '" + text + "' declares a zero width");
        }

        if (tick + 1 >= text.size()) {
            return make_error("sized literal '" + text + "' has no base specifier");
        }
        switch (text[tick + 1]) {
        case 'b':
        case 'B':
            result.base = NumberBase::Binary;
            break;
        case 'd':
        case 'D':
            result.base = NumberBase::Decimal;
            break;
        case 'h':
        case 'H':
            result.base = NumberBase::Hexadecimal;
            break;
        default:
            return make_error(
                std::string("sized literal '") + text + "' uses unsupported base '" + text[tick + 1]
                + "'");
        }

        digits                    = text.substr(tick + 2);
        result.width              = width;
        result.has_explicit_width = true;
    } else if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        result.base = NumberBase::Hexadecimal;
        digits      = text.substr(2);
    } else {
        result.base = NumberBase::Decimal;
        digits      = text;
    }

    if (digits.empty()) {
        return make_error("numeric literal '" + text + "' has no digits");
    }
    if (digits.front() == '_') {
        return make_error("numeric literal '" + text + "' starts with a digit separator");
    }
    if (!accumulate_digits(digits, result.base, result.value)) {
        return make_error(
            "numeric literal '" + text + "' is not a valid " + base_name(result.base) + " value");
    }

    if (result.has_explicit_width) {
        // The reference compiler rejects a value that does not fit its declared
        // width instead of truncating it. Do the same: a literal that has to be
        // clipped to be usable is a defect in the source, not an input to guess at.
        if (!result.value.fits_in(result.width)) {
            return make_error(
                "numeric literal '" + text + "' needs "
                + std::to_string(result.value.significant_bits()) + " bits but declares a width of "
                + std::to_string(result.width));
        }
        result.value.resize(result.width);
    } else {
        const size_t needed = result.value.significant_bits();
        if (needed > kMaxLiteralWidth) {
            return make_error(
                "numeric literal '" + text + "' needs more than " + std::to_string(kMaxLiteralWidth)
                + " bits");
        }
        result.width = needed == 0 ? 1 : needed;
        result.value.resize(result.width);
    }

    return result;
}

} // namespace systemrdl
