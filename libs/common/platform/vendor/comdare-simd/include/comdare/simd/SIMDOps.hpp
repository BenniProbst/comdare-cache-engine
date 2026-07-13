#pragma once
/**
 * @file SIMDOps.hpp
 * @brief SIMD-accelerated operations (Scalar, AVX2, AVX-512, ARM NEON)
 *
 * Part of comdare-simd (ModuleBaseline0-Foundation)
 * Backends: scalar (always), avx2 (x86), avx512 (x86), neon (ARM)
 */

#include <cstdint>
#include <cstddef>
#include <cstring>

#include <comdare/simd/SIMDCapabilities.hpp>
#include <comdare/simd/SIMDProfiler.hpp>

#if defined(__AVX2__) || defined(__AVX512F__)
#include <immintrin.h>
#endif

#if defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>
#define COMDARE_SIMD_NEON 1
#endif

namespace comdare::simd {

// =============================================================================
// Scalar Fallback Operations
// =============================================================================

namespace scalar {

/**
 * @brief Add two arrays with carry propagation
 * @return Final carry (0 or 1)
 */
inline uint64_t add_u64(const uint64_t* a, const uint64_t* b,
                        uint64_t* result, size_t count) {
    uint64_t carry = 0;
    for (size_t i = 0; i < count; ++i) {
        uint64_t sum = a[i] + carry;
        carry = (sum < carry) ? 1 : 0;
        sum += b[i];
        carry += (sum < b[i]) ? 1 : 0;
        result[i] = sum;
    }
    return carry;
}

/**
 * @brief Subtract two arrays with borrow propagation
 * @return Final borrow (0 or 1)
 */
inline uint64_t sub_u64(const uint64_t* a, const uint64_t* b,
                        uint64_t* result, size_t count) {
    uint64_t borrow = 0;
    for (size_t i = 0; i < count; ++i) {
        uint64_t diff = a[i] - borrow;
        borrow = (diff > a[i]) ? 1 : 0;
        uint64_t bdiff = diff - b[i];
        borrow += (bdiff > diff) ? 1 : 0;
        result[i] = bdiff;
    }
    return borrow;
}

/**
 * @brief Lexicographic comparison (MSB first)
 * @return -1 if a<b, 0 if a==b, 1 if a>b
 */
inline int compare_u64(const uint64_t* a, const uint64_t* b, size_t count) {
    for (size_t i = count; i > 0; --i) {
        if (a[i-1] < b[i-1]) return -1;
        if (a[i-1] > b[i-1]) return 1;
    }
    return 0;
}

inline void bitwise_and_u64(const uint64_t* a, const uint64_t* b,
                            uint64_t* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] & b[i];
    }
}

inline void bitwise_or_u64(const uint64_t* a, const uint64_t* b,
                           uint64_t* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] | b[i];
    }
}

inline void bitwise_xor_u64(const uint64_t* a, const uint64_t* b,
                            uint64_t* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] ^ b[i];
    }
}

inline void bitwise_not_u64(const uint64_t* a, uint64_t* result, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        result[i] = ~a[i];
    }
}

inline uint32_t clz_u64(uint64_t x) {
    if (x == 0) return 64;
#ifdef _MSC_VER
    unsigned long idx;
    _BitScanReverse64(&idx, x);
    return 63 - idx;
#else
    return __builtin_clzll(x);
#endif
}

inline uint32_t ctz_u64(uint64_t x) {
    if (x == 0) return 64;
#ifdef _MSC_VER
    unsigned long idx;
    _BitScanForward64(&idx, x);
    return idx;
#else
    return __builtin_ctzll(x);
#endif
}

inline uint32_t popcount_u64(uint64_t x) {
#ifdef _MSC_VER
    return static_cast<uint32_t>(__popcnt64(x));
#else
    return __builtin_popcountll(x);
#endif
}

inline size_t find_nonzero(const uint8_t* data, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (data[i] != 0) return i;
    }
    return count;
}

inline size_t find_byte(const uint8_t* data, size_t count, uint8_t value) {
    const void* ptr = std::memchr(data, value, count);
    return ptr ? static_cast<size_t>(static_cast<const uint8_t*>(ptr) - data) : count;
}

inline void memset_u64(uint64_t* dest, uint64_t value, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        dest[i] = value;
    }
}

} // namespace scalar

// =============================================================================
// AVX2 Operations (256-bit)
// =============================================================================

#ifdef __AVX2__
namespace avx2 {

inline uint64_t add_u64(const uint64_t* a, const uint64_t* b,
                        uint64_t* result, size_t count) {
    // Process 4 limbs at a time, handle carry with scalar
    size_t i = 0;
    uint64_t carry = 0;

    // Main vectorized loop (carry-save for internal, final propagation)
    for (; i + 4 <= count; i += 4) {
        __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(a + i));
        __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(b + i));
        __m256i vsum = _mm256_add_epi64(va, vb);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(result + i), vsum);
    }

    // Scalar carry propagation over the full array. Der frühere Zweig las für die Tail-Limbs (j >= i) das
    // noch UNINITIALISIERTE result[j] (UB) und absorbierte den Übertrag des vektorisierten Präfix in diese
    // Garbage-Limbs, statt ihn in die Rest-Schleife zu tragen → empirisch r[4]==12 statt 13. Jetzt exakt die
    // Carry-Save-Logik der avx512-/neon-Fassung: für j < i (vektorisiert) den in result[j] bereits abgelegten
    // Roh-Sum a[j]+b[j] mit dem eingehenden Carry kombinieren, sonst (Tail) den vollständigen Skalar-Add.
    carry = 0;
    for (size_t j = 0; j < count; ++j) {
        if (j < i) {
            uint64_t orig = a[j] + b[j];
            uint64_t sum = result[j] + carry;
            carry = (orig < a[j]) ? 1 : 0;
            carry += (sum < result[j]) ? 1 : 0;
            result[j] = sum;
        } else {
            uint64_t sum = a[j] + carry;
            carry = (sum < carry) ? 1 : 0;
            sum += b[j];
            carry += (sum < b[j]) ? 1 : 0;
            result[j] = sum;
        }
    }

    return carry;
}

inline void bitwise_and_u64(const uint64_t* a, const uint64_t* b,
                            uint64_t* result, size_t count) {
    size_t i = 0;
    for (; i + 4 <= count; i += 4) {
        __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(a + i));
        __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(b + i));
        __m256i vr = _mm256_and_si256(va, vb);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(result + i), vr);
    }
    for (; i < count; ++i) {
        result[i] = a[i] & b[i];
    }
}

inline void bitwise_or_u64(const uint64_t* a, const uint64_t* b,
                           uint64_t* result, size_t count) {
    size_t i = 0;
    for (; i + 4 <= count; i += 4) {
        __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(a + i));
        __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(b + i));
        __m256i vr = _mm256_or_si256(va, vb);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(result + i), vr);
    }
    for (; i < count; ++i) {
        result[i] = a[i] | b[i];
    }
}

inline void bitwise_xor_u64(const uint64_t* a, const uint64_t* b,
                            uint64_t* result, size_t count) {
    size_t i = 0;
    for (; i + 4 <= count; i += 4) {
        __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(a + i));
        __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(b + i));
        __m256i vr = _mm256_xor_si256(va, vb);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(result + i), vr);
    }
    for (; i < count; ++i) {
        result[i] = a[i] ^ b[i];
    }
}

inline void bitwise_not_u64(const uint64_t* a, uint64_t* result, size_t count) {
    __m256i ones = _mm256_set1_epi64x(static_cast<int64_t>(-1));
    size_t i = 0;
    for (; i + 4 <= count; i += 4) {
        __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(a + i));
        __m256i vr = _mm256_xor_si256(va, ones);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(result + i), vr);
    }
    for (; i < count; ++i) {
        result[i] = ~a[i];
    }
}

inline size_t find_nonzero(const uint8_t* data, size_t count) {
    size_t i = 0;
    __m256i zero = _mm256_setzero_si256();
    for (; i + 32 <= count; i += 32) {
        __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + i));
        int mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(v, zero));
        if (mask != -1) {
            // At least one non-zero byte
            return i + static_cast<size_t>(__builtin_ctz(~static_cast<uint32_t>(mask)));
        }
    }
    for (; i < count; ++i) {
        if (data[i] != 0) return i;
    }
    return count;
}

inline void memset_u64(uint64_t* dest, uint64_t value, size_t count) {
    __m256i vval = _mm256_set1_epi64x(static_cast<int64_t>(value));
    size_t i = 0;
    for (; i + 4 <= count; i += 4) {
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dest + i), vval);
    }
    for (; i < count; ++i) {
        dest[i] = value;
    }
}

} // namespace avx2
#endif

// =============================================================================
// AVX-512 Operations (512-bit)
// =============================================================================

#ifdef __AVX512F__
namespace avx512 {

inline uint64_t add_u64(const uint64_t* a, const uint64_t* b,
                        uint64_t* result, size_t count) {
    size_t i = 0;
    for (; i + 8 <= count; i += 8) {
        __m512i va = _mm512_loadu_si512(a + i);
        __m512i vb = _mm512_loadu_si512(b + i);
        __m512i vsum = _mm512_add_epi64(va, vb);
        _mm512_storeu_si512(result + i, vsum);
    }
    // Scalar carry propagation over the full array
    uint64_t carry = 0;
    for (size_t j = 0; j < count; ++j) {
        if (j < i) {
            uint64_t orig = a[j] + b[j];
            uint64_t sum = result[j] + carry;
            carry = (orig < a[j]) ? 1 : 0;
            carry += (sum < result[j]) ? 1 : 0;
            result[j] = sum;
        } else {
            uint64_t sum = a[j] + carry;
            carry = (sum < carry) ? 1 : 0;
            sum += b[j];
            carry += (sum < b[j]) ? 1 : 0;
            result[j] = sum;
        }
    }
    return carry;
}

inline void bitwise_and_u64(const uint64_t* a, const uint64_t* b,
                            uint64_t* result, size_t count) {
    size_t i = 0;
    for (; i + 8 <= count; i += 8) {
        __m512i va = _mm512_loadu_si512(a + i);
        __m512i vb = _mm512_loadu_si512(b + i);
        _mm512_storeu_si512(result + i, _mm512_and_si512(va, vb));
    }
    for (; i < count; ++i) result[i] = a[i] & b[i];
}

inline void bitwise_or_u64(const uint64_t* a, const uint64_t* b,
                           uint64_t* result, size_t count) {
    size_t i = 0;
    for (; i + 8 <= count; i += 8) {
        __m512i va = _mm512_loadu_si512(a + i);
        __m512i vb = _mm512_loadu_si512(b + i);
        _mm512_storeu_si512(result + i, _mm512_or_si512(va, vb));
    }
    for (; i < count; ++i) result[i] = a[i] | b[i];
}

inline void bitwise_xor_u64(const uint64_t* a, const uint64_t* b,
                            uint64_t* result, size_t count) {
    size_t i = 0;
    for (; i + 8 <= count; i += 8) {
        __m512i va = _mm512_loadu_si512(a + i);
        __m512i vb = _mm512_loadu_si512(b + i);
        _mm512_storeu_si512(result + i, _mm512_xor_si512(va, vb));
    }
    for (; i < count; ++i) result[i] = a[i] ^ b[i];
}

inline void memset_u64(uint64_t* dest, uint64_t value, size_t count) {
    __m512i vval = _mm512_set1_epi64(static_cast<int64_t>(value));
    size_t i = 0;
    for (; i + 8 <= count; i += 8) {
        _mm512_storeu_si512(dest + i, vval);
    }
    for (; i < count; ++i) dest[i] = value;
}

} // namespace avx512
#endif

// =============================================================================
// ARM NEON Operations (128-bit)
// =============================================================================

#ifdef COMDARE_SIMD_NEON
namespace neon {

inline uint64_t add_u64(const uint64_t* a, const uint64_t* b,
                        uint64_t* result, size_t count) {
    size_t i = 0;
    for (; i + 2 <= count; i += 2) {
        uint64x2_t va = vld1q_u64(a + i);
        uint64x2_t vb = vld1q_u64(b + i);
        uint64x2_t vsum = vaddq_u64(va, vb);
        vst1q_u64(result + i, vsum);
    }
    // Scalar carry propagation
    uint64_t carry = 0;
    for (size_t j = 0; j < count; ++j) {
        if (j < i) {
            uint64_t orig = a[j] + b[j];
            uint64_t sum = result[j] + carry;
            carry = (orig < a[j]) ? 1 : 0;
            carry += (sum < result[j]) ? 1 : 0;
            result[j] = sum;
        } else {
            uint64_t sum = a[j] + carry;
            carry = (sum < carry) ? 1 : 0;
            sum += b[j];
            carry += (sum < b[j]) ? 1 : 0;
            result[j] = sum;
        }
    }
    return carry;
}

inline uint64_t sub_u64(const uint64_t* a, const uint64_t* b,
                        uint64_t* result, size_t count) {
    size_t i = 0;
    for (; i + 2 <= count; i += 2) {
        uint64x2_t va = vld1q_u64(a + i);
        uint64x2_t vb = vld1q_u64(b + i);
        uint64x2_t vdiff = vsubq_u64(va, vb);
        vst1q_u64(result + i, vdiff);
    }
    // Scalar borrow propagation
    uint64_t borrow = 0;
    for (size_t j = 0; j < count; ++j) {
        if (j < i) {
            uint64_t orig_diff = a[j] - b[j];
            uint64_t diff = result[j] - borrow;
            borrow = (a[j] < b[j]) ? 1 : 0;
            borrow += (diff > result[j]) ? 1 : 0;
            result[j] = diff;
        } else {
            uint64_t diff = a[j] - borrow;
            borrow = (diff > a[j]) ? 1 : 0;
            uint64_t bdiff = diff - b[j];
            borrow += (bdiff > diff) ? 1 : 0;
            result[j] = bdiff;
        }
    }
    return borrow;
}

inline void bitwise_and_u64(const uint64_t* a, const uint64_t* b,
                            uint64_t* result, size_t count) {
    size_t i = 0;
    for (; i + 2 <= count; i += 2) {
        uint64x2_t va = vld1q_u64(a + i);
        uint64x2_t vb = vld1q_u64(b + i);
        vst1q_u64(result + i, vandq_u64(va, vb));
    }
    for (; i < count; ++i) result[i] = a[i] & b[i];
}

inline void bitwise_or_u64(const uint64_t* a, const uint64_t* b,
                           uint64_t* result, size_t count) {
    size_t i = 0;
    for (; i + 2 <= count; i += 2) {
        uint64x2_t va = vld1q_u64(a + i);
        uint64x2_t vb = vld1q_u64(b + i);
        vst1q_u64(result + i, vorrq_u64(va, vb));
    }
    for (; i < count; ++i) result[i] = a[i] | b[i];
}

inline void bitwise_xor_u64(const uint64_t* a, const uint64_t* b,
                            uint64_t* result, size_t count) {
    size_t i = 0;
    for (; i + 2 <= count; i += 2) {
        uint64x2_t va = vld1q_u64(a + i);
        uint64x2_t vb = vld1q_u64(b + i);
        vst1q_u64(result + i, veorq_u64(va, vb));
    }
    for (; i < count; ++i) result[i] = a[i] ^ b[i];
}

inline void bitwise_not_u64(const uint64_t* a, uint64_t* result, size_t count) {
    size_t i = 0;
    for (; i + 2 <= count; i += 2) {
        uint64x2_t va = vld1q_u64(a + i);
        vst1q_u64(result + i, vreinterpretq_u64_u32(vmvnq_u32(vreinterpretq_u32_u64(va))));
    }
    for (; i < count; ++i) result[i] = ~a[i];
}

inline size_t find_nonzero(const uint8_t* data, size_t count) {
    size_t i = 0;
    uint8x16_t zero = vdupq_n_u8(0);
    for (; i + 16 <= count; i += 16) {
        uint8x16_t v = vld1q_u8(data + i);
        uint8x16_t cmp = vceqq_u8(v, zero);
        // Check if any lane is non-zero (not all equal to zero)
        uint64x2_t reduced = vreinterpretq_u64_u8(cmp);
        uint64_t lo = vgetq_lane_u64(reduced, 0);
        uint64_t hi = vgetq_lane_u64(reduced, 1);
        if (lo != 0xFFFFFFFFFFFFFFFFULL || hi != 0xFFFFFFFFFFFFFFFFULL) {
            for (size_t j = i; j < i + 16 && j < count; ++j) {
                if (data[j] != 0) return j;
            }
        }
    }
    for (; i < count; ++i) {
        if (data[i] != 0) return i;
    }
    return count;
}

inline void memset_u64(uint64_t* dest, uint64_t value, size_t count) {
    uint64x2_t vval = vdupq_n_u64(value);
    size_t i = 0;
    for (; i + 2 <= count; i += 2) {
        vst1q_u64(dest + i, vval);
    }
    for (; i < count; ++i) dest[i] = value;
}

} // namespace neon
#endif

// =============================================================================
// Unified Operations (Compile-Time Platform Selection)
// =============================================================================

namespace ops {

/// Helper: determine active backend at compile time
inline constexpr Backend active_backend() {
#ifdef __AVX512F__
    return Backend::AVX512;
#elif defined(__AVX2__)
    return Backend::AVX2;
#elif defined(COMDARE_SIMD_NEON)
    return Backend::NEON;
#else
    return Backend::Scalar;
#endif
}

inline uint64_t add_u64(const uint64_t* a, const uint64_t* b,
                        uint64_t* result, size_t count) {
    constexpr auto B = active_backend();
    global_counters().increment(B);
    ProfileScope ps("add_u64", B, count);
#ifdef __AVX512F__
    return avx512::add_u64(a, b, result, count);
#elif defined(__AVX2__)
    return avx2::add_u64(a, b, result, count);
#elif defined(COMDARE_SIMD_NEON)
    return neon::add_u64(a, b, result, count);
#else
    return scalar::add_u64(a, b, result, count);
#endif
}

inline uint64_t sub_u64(const uint64_t* a, const uint64_t* b,
                        uint64_t* result, size_t count) {
    constexpr auto B =
#ifdef COMDARE_SIMD_NEON
        Backend::NEON;
#else
        Backend::Scalar;
#endif
    global_counters().increment(B);
    ProfileScope ps("sub_u64", B, count);
#ifdef COMDARE_SIMD_NEON
    return neon::sub_u64(a, b, result, count);
#else
    return scalar::sub_u64(a, b, result, count);
#endif
}

inline int compare_u64(const uint64_t* a, const uint64_t* b, size_t count) {
    global_counters().increment(Backend::Scalar);
    ProfileScope ps("compare_u64", Backend::Scalar, count);
    return scalar::compare_u64(a, b, count);
}

inline void bitwise_and_u64(const uint64_t* a, const uint64_t* b,
                            uint64_t* result, size_t count) {
    constexpr auto B = active_backend();
    global_counters().increment(B);
    ProfileScope ps("bitwise_and_u64", B, count);
#ifdef __AVX512F__
    avx512::bitwise_and_u64(a, b, result, count);
#elif defined(__AVX2__)
    avx2::bitwise_and_u64(a, b, result, count);
#elif defined(COMDARE_SIMD_NEON)
    neon::bitwise_and_u64(a, b, result, count);
#else
    scalar::bitwise_and_u64(a, b, result, count);
#endif
}

inline void bitwise_or_u64(const uint64_t* a, const uint64_t* b,
                           uint64_t* result, size_t count) {
    constexpr auto B = active_backend();
    global_counters().increment(B);
    ProfileScope ps("bitwise_or_u64", B, count);
#ifdef __AVX512F__
    avx512::bitwise_or_u64(a, b, result, count);
#elif defined(__AVX2__)
    avx2::bitwise_or_u64(a, b, result, count);
#elif defined(COMDARE_SIMD_NEON)
    neon::bitwise_or_u64(a, b, result, count);
#else
    scalar::bitwise_or_u64(a, b, result, count);
#endif
}

inline void bitwise_xor_u64(const uint64_t* a, const uint64_t* b,
                            uint64_t* result, size_t count) {
    constexpr auto B = active_backend();
    global_counters().increment(B);
    ProfileScope ps("bitwise_xor_u64", B, count);
#ifdef __AVX512F__
    avx512::bitwise_xor_u64(a, b, result, count);
#elif defined(__AVX2__)
    avx2::bitwise_xor_u64(a, b, result, count);
#elif defined(COMDARE_SIMD_NEON)
    neon::bitwise_xor_u64(a, b, result, count);
#else
    scalar::bitwise_xor_u64(a, b, result, count);
#endif
}

inline void bitwise_not_u64(const uint64_t* a, uint64_t* result, size_t count) {
    constexpr auto B =
#ifdef __AVX2__
        Backend::AVX2;
#elif defined(COMDARE_SIMD_NEON)
        Backend::NEON;
#else
        Backend::Scalar;
#endif
    global_counters().increment(B);
    ProfileScope ps("bitwise_not_u64", B, count);
#ifdef __AVX2__
    avx2::bitwise_not_u64(a, result, count);
#elif defined(COMDARE_SIMD_NEON)
    neon::bitwise_not_u64(a, result, count);
#else
    scalar::bitwise_not_u64(a, result, count);
#endif
}

inline uint32_t clz_u64(uint64_t x) {
    global_counters().increment(Backend::Scalar);
    return scalar::clz_u64(x);
}
inline uint32_t ctz_u64(uint64_t x) {
    global_counters().increment(Backend::Scalar);
    return scalar::ctz_u64(x);
}
inline uint32_t popcount_u64(uint64_t x) {
    global_counters().increment(Backend::Scalar);
    return scalar::popcount_u64(x);
}

inline size_t find_nonzero(const uint8_t* data, size_t count) {
    constexpr auto B =
#ifdef __AVX2__
        Backend::AVX2;
#elif defined(COMDARE_SIMD_NEON)
        Backend::NEON;
#else
        Backend::Scalar;
#endif
    global_counters().increment(B);
    ProfileScope ps("find_nonzero", B, count);
#ifdef __AVX2__
    return avx2::find_nonzero(data, count);
#elif defined(COMDARE_SIMD_NEON)
    return neon::find_nonzero(data, count);
#else
    return scalar::find_nonzero(data, count);
#endif
}

inline size_t find_byte(const uint8_t* data, size_t count, uint8_t value) {
    global_counters().increment(Backend::Scalar);
    return scalar::find_byte(data, count, value);
}

inline void memset_u64(uint64_t* dest, uint64_t value, size_t count) {
    constexpr auto B = active_backend();
    global_counters().increment(B);
    ProfileScope ps("memset_u64", B, count);
#ifdef __AVX512F__
    avx512::memset_u64(dest, value, count);
#elif defined(__AVX2__)
    avx2::memset_u64(dest, value, count);
#elif defined(COMDARE_SIMD_NEON)
    neon::memset_u64(dest, value, count);
#else
    scalar::memset_u64(dest, value, count);
#endif
}

} // namespace ops

} // namespace comdare::simd
