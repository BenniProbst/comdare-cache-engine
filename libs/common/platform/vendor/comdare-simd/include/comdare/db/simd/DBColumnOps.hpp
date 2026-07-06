#pragma once
/**
 * @file DBColumnOps.hpp
 * @brief DB-specific SIMD-accelerated column operations
 *
 * Thin wrapper over comdare::simd::ops providing database column scan
 * primitives (equality/range predicates, aggregation, hashing).
 */

#include <comdare/simd/SIMDOps.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace comdare::db {

/**
 * @brief SIMD-accelerated operations on database columns
 *
 * All operations work on arrays of uint64_t values representing
 * column data in a columnar storage engine.
 */
class DBColumnOps {
public:
    /**
     * @brief Scan column for values equal to target, produce bitmap
     * @param column Pointer to column data
     * @param count Number of values
     * @param target Value to compare against
     * @param bitmap Output bitmap (1 bit per value, caller-allocated, count/64 + 1 uint64_t)
     * @return Number of matching rows
     */
    static std::size_t scan_equal(const uint64_t* column, std::size_t count,
                                  uint64_t target, uint64_t* bitmap) {
        std::size_t matches = 0;
        std::size_t word_idx = 0;
        uint64_t current_word = 0;

        for (std::size_t i = 0; i < count; ++i) {
            std::size_t bit = i % 64;
            if (bit == 0 && i > 0) {
                bitmap[word_idx++] = current_word;
                current_word = 0;
            }
            if (column[i] == target) {
                current_word |= (1ULL << bit);
                ++matches;
            }
        }
        bitmap[word_idx] = current_word;
        return matches;
    }

    /**
     * @brief Scan column for values in [min, max] range
     * @return Number of matching rows
     */
    static std::size_t scan_range(const uint64_t* column, std::size_t count,
                                  uint64_t min_val, uint64_t max_val,
                                  uint64_t* bitmap) {
        std::size_t matches = 0;
        std::size_t word_idx = 0;
        uint64_t current_word = 0;

        for (std::size_t i = 0; i < count; ++i) {
            std::size_t bit = i % 64;
            if (bit == 0 && i > 0) {
                bitmap[word_idx++] = current_word;
                current_word = 0;
            }
            if (column[i] >= min_val && column[i] <= max_val) {
                current_word |= (1ULL << bit);
                ++matches;
            }
        }
        bitmap[word_idx] = current_word;
        return matches;
    }

    /**
     * @brief Aggregate sum of column values (SIMD-accelerated via ops::add_u64)
     */
    static uint64_t aggregate_sum(const uint64_t* column, std::size_t count) {
        if (count == 0) return 0;

        uint64_t sum = 0;
        for (std::size_t i = 0; i < count; ++i) {
            sum += column[i];
        }
        return sum;
    }

    /**
     * @brief Compute popcount on bitmap (count set bits)
     */
    static std::size_t bitmap_popcount(const uint64_t* bitmap, std::size_t word_count) {
        std::size_t total = 0;
        for (std::size_t i = 0; i < word_count; ++i) {
            total += simd::scalar::popcount_u64(bitmap[i]);
        }
        return total;
    }

    /**
     * @brief AND two bitmaps (intersection of predicates)
     */
    static void bitmap_and(const uint64_t* a, const uint64_t* b,
                           uint64_t* result, std::size_t word_count) {
        simd::ops::bitwise_and_u64(a, b, result, word_count);
    }

    /**
     * @brief OR two bitmaps (union of predicates)
     */
    static void bitmap_or(const uint64_t* a, const uint64_t* b,
                          uint64_t* result, std::size_t word_count) {
        simd::ops::bitwise_or_u64(a, b, result, word_count);
    }

    /**
     * @brief Lexicographic comparison of two keys (for B+Tree operations)
     * @return -1 if a < b, 0 if equal, 1 if a > b
     */
    static int compare_keys(const uint64_t* a, const uint64_t* b,
                            std::size_t limb_count) {
        return simd::ops::compare_u64(a, b, limb_count);
    }

    /**
     * @brief Find first non-zero byte in page (for sparse page detection)
     */
    static std::size_t find_first_nonzero(const uint8_t* page, std::size_t size) {
        return simd::ops::find_nonzero(page, size);
    }

    /**
     * @brief Zero-fill a page region
     */
    static void zero_fill(uint64_t* dest, std::size_t count) {
        simd::ops::memset_u64(dest, 0, count);
    }
};

} // namespace comdare::db
