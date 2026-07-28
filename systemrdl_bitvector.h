#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace systemrdl {

// Fixed-width unsigned bit vector.
//
// A register field reset value is a bit pattern of a known width, not an
// integer. Representing it as uint64_t forces a special case at 64 bits and
// silently discards the high bits of a wider field. This type has no such
// boundary: a 1-bit field and a 256-bit field take the same code path.
//
// Only the operations the elaborator needs are provided: build a value from
// digits, place it into a register image, compare it against a width, and
// format it. There is no division, modulus or exponentiation.
class BitVector
{
public:
    BitVector() = default;

    // Zero value of the given width.
    explicit BitVector(size_t width);

    static BitVector from_uint64(uint64_t value, size_t width);

    size_t width() const { return width_; }

    // Truncate to the low bits or zero-extend. Bits above the new width are
    // discarded; this is the defining behaviour of a fixed-width value.
    void resize(size_t width);

    bool get_bit(size_t index) const;
    void set_bit(size_t index, bool value);

    bool is_zero() const;

    // Number of bits required to represent the value, i.e. the position of the
    // highest set bit plus one. Zero has zero significant bits.
    size_t significant_bits() const;

    // True when the value can be represented in the given number of bits.
    bool fits_in(size_t width) const;

    // Convert to uint64_t. Returns false and leaves out untouched when the
    // value needs more than 64 bits, so a caller can never truncate by
    // accident.
    bool to_uint64(uint64_t &out) const;

    // value = value * multiplier + addend, growing the width as needed.
    // This is the single primitive used to accumulate digits in any base.
    void mul_add_small(uint32_t multiplier, uint32_t addend);

    // Lowercase, "0x" prefixed, zero padded to ceil(width / 4) digits.
    std::string to_hex() const;

    // Zero padded to exactly width() characters.
    std::string to_binary() const;

    bool operator==(const BitVector &other) const;
    bool operator!=(const BitVector &other) const { return !(*this == other); }

private:
    static constexpr size_t kBitsPerWord = 32;

    // Clear the bits above width_ in the top word so the representation stays
    // canonical after every mutation.
    void normalize();

    std::vector<uint32_t> words_;
    size_t                width_ = 0;
};

} // namespace systemrdl
