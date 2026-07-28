// Unit tests for systemrdl::BitVector.
//
// Plain assertions, no test framework: the library has no test dependency and
// this keeps the offline build path unchanged.

#include "systemrdl_bitvector.h"

#include <cstdio>
#include <string>

using systemrdl::BitVector;

namespace {

int failures = 0;

void check(bool condition, const char *what)
{
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

void check_string(const std::string &actual, const std::string &expected, const char *what)
{
    if (actual != expected) {
        std::printf("FAIL: %s (expected '%s', got '%s')\n", what, expected.c_str(), actual.c_str());
        ++failures;
    }
}

// Build a value the way the literal parser does: one digit at a time.
BitVector accumulate(const std::string &digits, uint32_t base)
{
    BitVector value;
    for (char digit : digits) {
        uint32_t weight = 0;
        if (digit >= '0' && digit <= '9') {
            weight = static_cast<uint32_t>(digit - '0');
        } else {
            weight = static_cast<uint32_t>((digit | 0x20) - 'a' + 10);
        }
        value.mul_add_small(base, weight);
    }
    return value;
}

void test_construction()
{
    BitVector zero(32);
    check(zero.width() == 32, "explicit width is kept");
    check(zero.is_zero(), "new vector is zero");
    check(zero.significant_bits() == 0, "zero has no significant bits");
    check_string(zero.to_hex(), "0x00000000", "zero formats with full width padding");

    BitVector empty;
    check(empty.width() == 0, "default vector has zero width");
    check(empty.is_zero(), "default vector is zero");
}

void test_bit_access()
{
    BitVector value(8);
    value.set_bit(0, true);
    value.set_bit(7, true);
    check(value.get_bit(0), "bit 0 is set");
    check(value.get_bit(7), "bit 7 is set");
    check(!value.get_bit(3), "unset bit reads as zero");
    check(value.significant_bits() == 8, "significant bits reach the top set bit");
    check_string(value.to_binary(), "10000001", "binary formatting is width exact");

    value.set_bit(7, false);
    check(!value.get_bit(7), "bit can be cleared");
    check(value.significant_bits() == 1, "significant bits shrink after clearing");

    // Out of range access is defined: reads are zero, writes are dropped.
    check(!value.get_bit(64), "out of range read is zero");
    value.set_bit(64, true);
    check(value.significant_bits() == 1, "out of range write is ignored");
}

void test_from_uint64()
{
    BitVector small = BitVector::from_uint64(0xFFULL, 8);
    check_string(small.to_hex(), "0xff", "8-bit all ones");

    BitVector full = BitVector::from_uint64(0xFFFFFFFFFFFFFFFFULL, 64);
    check_string(full.to_hex(), "0xffffffffffffffff", "64-bit all ones survives");
    check(full.significant_bits() == 64, "64-bit value has 64 significant bits");

    // The source value is wider than the requested width: the high bits are
    // dropped rather than silently widening the result.
    BitVector clipped = BitVector::from_uint64(0x1FFULL, 8);
    check_string(clipped.to_hex(), "0xff", "value wider than width is truncated");
}

void test_uint64_roundtrip()
{
    uint64_t  out   = 0;
    BitVector value = BitVector::from_uint64(0xDEADBEEFULL, 32);
    check(value.to_uint64(out), "32-bit value converts");
    check(out == 0xDEADBEEFULL, "32-bit value round trips");

    BitVector wide = accumulate("deadbeef123456789abcdef011223344", 16);
    wide.resize(128);
    out = 0x5A5A5A5AULL;
    check(!wide.to_uint64(out), "128-bit value refuses to convert");
    check(out == 0x5A5A5A5AULL, "failed conversion leaves the output untouched");
}

void test_resize()
{
    BitVector value = BitVector::from_uint64(0xABCDULL, 16);

    value.resize(8);
    check_string(value.to_hex(), "0xcd", "resize down truncates to the low bits");

    value.resize(16);
    check_string(value.to_hex(), "0x00cd", "resize up zero extends");

    // Truncation is destructive: the discarded bits do not come back.
    check(value.significant_bits() == 8, "discarded high bits stay gone");
}

void test_decimal_accumulation()
{
    BitVector value = accumulate("200", 10);
    check(value.significant_bits() == 8, "200 needs 8 bits");
    uint64_t out = 0;
    check(value.to_uint64(out) && out == 200ULL, "decimal accumulation is exact");

    // A value far beyond 64 bits, accumulated one decimal digit at a time.
    BitVector huge = accumulate("340282366920938463463374607431768211456", 10);
    check(huge.significant_bits() == 129, "2^128 needs 129 bits");
}

void test_wide_hex()
{
    BitVector value = accumulate("deadbeef123456789abcdef011223344", 16);
    value.resize(128);
    check_string(value.to_hex(), "0xdeadbeef123456789abcdef011223344", "128-bit hex round trips");
    check(value.width() == 128, "width is 128");

    BitVector value256 = accumulate("ff", 16);
    value256.resize(256);
    check(value256.to_hex().size() == 2 + 64, "256-bit value formats as 64 hex digits");
    check(value256.significant_bits() == 8, "padding does not add significant bits");
}

void test_fits_in()
{
    BitVector value = BitVector::from_uint64(0xFFULL, 32);
    check(value.fits_in(8), "255 fits in 8 bits");
    check(!value.fits_in(7), "255 does not fit in 7 bits");
    check(value.fits_in(64), "255 fits in a wider field");

    BitVector zero(32);
    check(zero.fits_in(0), "zero fits in zero bits");

    // The check has no 64-bit boundary: it works the same above and below.
    BitVector wide = accumulate("1", 16);
    wide.resize(200);
    wide.set_bit(199, true);
    check(!wide.fits_in(199), "bit 199 requires 200 bits");
    check(wide.fits_in(200), "bit 199 fits in 200 bits");
}

void test_equality()
{
    BitVector left  = BitVector::from_uint64(0x1234ULL, 16);
    BitVector right = BitVector::from_uint64(0x1234ULL, 16);
    check(left == right, "equal values compare equal");

    BitVector other_width = BitVector::from_uint64(0x1234ULL, 32);
    check(left != other_width, "different widths are not equal");

    right.set_bit(0, !right.get_bit(0));
    check(left != right, "different values are not equal");
}

} // namespace

int main()
{
    test_construction();
    test_bit_access();
    test_from_uint64();
    test_uint64_roundtrip();
    test_resize();
    test_decimal_accumulation();
    test_wide_hex();
    test_fits_in();
    test_equality();

    if (failures != 0) {
        std::printf("FAILED: %d check(s)\n", failures);
        return 1;
    }
    std::printf("PASS: all BitVector checks\n");
    return 0;
}
