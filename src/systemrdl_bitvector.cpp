#include "systemrdl_bitvector.h"

namespace systemrdl {

namespace {

constexpr size_t kBitsPerWord = 32;

size_t words_for_width(size_t width)
{
    return (width + kBitsPerWord - 1) / kBitsPerWord;
}

} // namespace

BitVector::BitVector(size_t width)
    : words_(words_for_width(width), 0)
    , width_(width)
{}

BitVector BitVector::from_uint64(uint64_t value, size_t width)
{
    BitVector result(width);
    for (size_t i = 0; i < 64 && i < width; ++i) {
        if ((value >> i) & 1ULL) {
            result.set_bit(i, true);
        }
    }
    return result;
}

void BitVector::normalize()
{
    words_.resize(words_for_width(width_), 0);
    // width_ % kBitsPerWord is 0 when the top word is fully used, in which case
    // there is nothing above width_ to clear.
    const size_t used_bits = width_ % kBitsPerWord;
    if (used_bits != 0 && !words_.empty()) {
        words_.back() &= (1U << used_bits) - 1U;
    }
}

void BitVector::resize(size_t width)
{
    width_ = width;
    normalize();
}

bool BitVector::get_bit(size_t index) const
{
    if (index >= width_) {
        return false;
    }
    const size_t word = index / kBitsPerWord;
    if (word >= words_.size()) {
        return false;
    }
    return ((words_[word] >> (index % kBitsPerWord)) & 1U) != 0U;
}

void BitVector::set_bit(size_t index, bool value)
{
    if (index >= width_) {
        return;
    }
    const size_t word = index / kBitsPerWord;
    if (word >= words_.size()) {
        words_.resize(word + 1, 0);
    }
    const uint32_t mask = 1U << (index % kBitsPerWord);
    if (value) {
        words_[word] |= mask;
    } else {
        words_[word] &= ~mask;
    }
}

bool BitVector::is_zero() const
{
    for (uint32_t word : words_) {
        if (word != 0U) {
            return false;
        }
    }
    return true;
}

size_t BitVector::significant_bits() const
{
    for (size_t word = words_.size(); word > 0; --word) {
        const uint32_t value = words_[word - 1];
        if (value == 0U) {
            continue;
        }
        for (size_t bit = kBitsPerWord; bit > 0; --bit) {
            if ((value >> (bit - 1)) & 1U) {
                return (word - 1) * kBitsPerWord + bit;
            }
        }
    }
    return 0;
}

bool BitVector::fits_in(size_t width) const
{
    return significant_bits() <= width;
}

bool BitVector::to_uint64(uint64_t &out) const
{
    if (significant_bits() > 64) {
        return false;
    }
    uint64_t value = 0;
    for (size_t word = 0; word < words_.size() && word < 2; ++word) {
        value |= static_cast<uint64_t>(words_[word]) << (word * kBitsPerWord);
    }
    out = value;
    return true;
}

void BitVector::mul_add_small(uint32_t multiplier, uint32_t addend)
{
    uint64_t carry = addend;
    for (size_t i = 0; i < words_.size(); ++i) {
        const uint64_t product = static_cast<uint64_t>(words_[i]) * multiplier + carry;
        words_[i]              = static_cast<uint32_t>(product & 0xFFFFFFFFULL);
        carry                  = product >> kBitsPerWord;
    }
    while (carry != 0U) {
        words_.push_back(static_cast<uint32_t>(carry & 0xFFFFFFFFULL));
        carry >>= kBitsPerWord;
    }

    // Growing during digit accumulation is expected: the final width is set by
    // the caller once the whole literal has been consumed.
    const size_t needed = significant_bits();
    if (needed > width_) {
        width_ = needed;
    }
    normalize();
}

std::string BitVector::to_hex() const
{
    const size_t digits = (width_ + 3) / 4;
    if (digits == 0) {
        return "0x0";
    }

    std::string result = "0x";
    result.reserve(2 + digits);
    for (size_t digit = digits; digit > 0; --digit) {
        const size_t base_bit = (digit - 1) * 4;
        uint32_t     nibble   = 0;
        for (size_t bit = 0; bit < 4; ++bit) {
            if (get_bit(base_bit + bit)) {
                nibble |= 1U << bit;
            }
        }
        result += static_cast<char>(nibble < 10 ? ('0' + nibble) : ('a' + nibble - 10));
    }
    return result;
}

std::string BitVector::to_binary() const
{
    if (width_ == 0) {
        return std::string();
    }
    std::string result(width_, '0');
    for (size_t i = 0; i < width_; ++i) {
        if (get_bit(i)) {
            result[width_ - 1 - i] = '1';
        }
    }
    return result;
}

bool BitVector::operator==(const BitVector &other) const
{
    if (width_ != other.width_) {
        return false;
    }
    const size_t count = words_for_width(width_);
    for (size_t i = 0; i < count; ++i) {
        const uint32_t left  = i < words_.size() ? words_[i] : 0U;
        const uint32_t right = i < other.words_.size() ? other.words_[i] : 0U;
        if (left != right) {
            return false;
        }
    }
    return true;
}

} // namespace systemrdl
