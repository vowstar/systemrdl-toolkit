// Unit tests for systemrdl::parse_number.
//
// Every accepted case is a literal form the SystemRDL grammar can produce, and
// every rejection case is one it cannot. Both matter equally: the defect this
// parser replaces was std::stoll silently returning the width prefix of
// "8'hFF" as the value 8.

#include "systemrdl_number.h"

#include <cstdio>
#include <string>

using systemrdl::NumberBase;
using systemrdl::NumberLiteral;
using systemrdl::parse_number;

namespace {

int failures = 0;

void check(bool condition, const std::string &what)
{
    if (!condition) {
        std::printf("FAIL: %s\n", what.c_str());
        ++failures;
    }
}

// Assert that text parses and yields the given hex image and width.
void accepts(const std::string &text, const std::string &expected_hex, size_t expected_width)
{
    const NumberLiteral parsed = parse_number(text);
    if (parsed.error) {
        std::printf("FAIL: '%s' was rejected (%s)\n", text.c_str(), parsed.error_message.c_str());
        ++failures;
        return;
    }
    if (parsed.value.to_hex() != expected_hex) {
        std::printf(
            "FAIL: '%s' value expected %s, got %s\n",
            text.c_str(),
            expected_hex.c_str(),
            parsed.value.to_hex().c_str());
        ++failures;
    }
    if (parsed.width != expected_width) {
        std::printf(
            "FAIL: '%s' width expected %zu, got %zu\n", text.c_str(), expected_width, parsed.width);
        ++failures;
    }
}

void rejects(const std::string &text)
{
    const NumberLiteral parsed = parse_number(text);
    if (!parsed.error) {
        std::printf(
            "FAIL: '%s' should have been rejected, got %s\n",
            text.c_str(),
            parsed.value.to_hex().c_str());
        ++failures;
    }
}

void test_decimal()
{
    accepts("0", "0x0", 1);
    accepts("42", "0x2a", 6);
    accepts("255", "0xff", 8);
    accepts("200", "0xc8", 8);
    accepts("1_000", "0x3e8", 10);
    accepts("00042", "0x2a", 6);
}

void test_hex()
{
    accepts("0xFF", "0xff", 8);
    accepts("0xff", "0xff", 8);
    accepts("0X5A", "0x5a", 7);
    accepts("0x0FF", "0xff", 8);
    // The underscore case that std::stoull truncated to 1.
    accepts("0x1_0000_0000", "0x100000000", 33);
    accepts("0xDEADBEEF", "0xdeadbeef", 32);
}

void test_verilog_sized()
{
    // The core defect: these all used to evaluate to their width prefix.
    accepts("8'hFF", "0xff", 8);
    accepts("8'b1010_1010", "0xaa", 8);
    accepts("8'd200", "0xc8", 8);
    accepts("16'd200", "0x00c8", 16);
    accepts("32'h1234_5678", "0x12345678", 32);
    accepts("1'b0", "0x0", 1);
    accepts("1'b1", "0x1", 1);

    // Case insensitive base specifier.
    accepts("8'HFF", "0xff", 8);
    accepts("8'B11111111", "0xff", 8);
    accepts("8'D255", "0xff", 8);
}

void test_wide_values()
{
    accepts("64'hFFFF_FFFF_FFFF_FFFF", "0xffffffffffffffff", 64);
    accepts("128'hDEAD_BEEF_1234_5678_9ABC_DEF0_1122_3344", "0xdeadbeef123456789abcdef011223344", 128);
    accepts("256'h1", std::string("0x") + std::string(63, '0') + "1", 256);

    // A value beyond 64 bits written without a width still parses exactly.
    const NumberLiteral unsized = parse_number("0xDEADBEEF123456789ABCDEF011223344");
    check(!unsized.error, "unsized 128-bit hex parses");
    check(unsized.width == 128, "unsized 128-bit hex infers width 128");
    check(!unsized.has_explicit_width, "unsized literal reports no explicit width");
}

void test_width_semantics()
{
    const NumberLiteral sized = parse_number("16'd200");
    check(sized.has_explicit_width, "sized literal reports an explicit width");
    check(sized.width == 16, "declared width wins over the value size");
    check(sized.base == NumberBase::Decimal, "base is recorded");

    const NumberLiteral unsized = parse_number("200");
    check(!unsized.has_explicit_width, "unsized literal reports no explicit width");
    check(unsized.width == 8, "unsized width is the significant bit count");

    // Zero is one bit wide, not zero bits.
    const NumberLiteral zero = parse_number("0");
    check(zero.width == 1, "zero is one bit wide");
}

void test_overflow_is_rejected()
{
    // The reference compiler rejects these rather than truncating.
    rejects("4'hFF");
    rejects("8'hFFF");
    rejects("1'b11");
    rejects("8'd256");
}

void test_malformed_is_rejected()
{
    rejects("");
    rejects("'");
    rejects("'h10");    // no width
    rejects("8'");      // no base
    rejects("8'q10");   // unsupported base
    rejects("8'h");     // no digits
    rejects("0'h1");    // zero width
    rejects("8'b12");   // 2 is not a binary digit
    rejects("8'dZZ");   // not decimal digits
    rejects("0xGG");    // not hex digits
    rejects("0x");      // too short for the 0x prefix; rejected as a decimal literal
    rejects("_8");      // leading separator
    rejects("8'h_FF");  // leading separator in digits
    rejects("WIDTH");   // an identifier, not a number
    rejects("WIDTH-1"); // an expression, not a number
    rejects("1e5");     // no exponent form in SystemRDL
    rejects("x'hFF");   // non-numeric width

    // Octal exists in Verilog but not in the SystemRDL grammar.
    rejects("8'o17");

    // A width above the SystemVerilog limit is refused rather than wrapped.
    rejects("16777216'h1");
}

void test_length_limit()
{
    const std::string too_long = "0x" + std::string(systemrdl::kMaxLiteralChars, 'F');
    rejects(too_long);
}

} // namespace

int main()
{
    test_decimal();
    test_hex();
    test_verilog_sized();
    test_wide_values();
    test_width_semantics();
    test_overflow_is_rejected();
    test_malformed_is_rejected();
    test_length_limit();

    if (failures != 0) {
        std::printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    std::printf("PASS: all number parser checks\n");
    return 0;
}
