// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// comdare::succinct::BitVector — eigene C++23-Portierung der SDSL-Lite-Kern-
// Datenstruktur (Aufgabe #105).
//
// Ersetzt SDSL-Lite (Simon Gog) durch eine Comdare-eigene Implementation
// mit den klassischen Operationen:
//   rank1(i)   - Anzahl gesetzter Bits in [0, i)
//   rank0(i)   - Anzahl nicht-gesetzter Bits in [0, i)
//   select1(k) - Position des k-ten gesetzten Bits (1-based)
//   select0(k) - Position des k-ten nicht-gesetzten Bits
//
// REV 7 §6 Concept-Anbindung: dient als Baustein fuer LOUDS (P09) und
// SuRF (P10) Re-Implementations.
//
// Implementations-Variante: 64-bit-Words mit precomputed Rank-Lookup-Table
// pro 256-Bit-Block (4 Words). Naive select via linearer Suche — die echte
// SDSL-Lite-Optimization mit constant-time select kommt in Phase 5+.

#pragma once

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace comdare::succinct {

class BitVector {
public:
    using word_t = std::uint64_t;
    static constexpr std::size_t kWordBits  = 64;
    static constexpr std::size_t kBlockBits = 256;            // 4 words per rank-block
    static constexpr std::size_t kWordsPerBlock = kBlockBits / kWordBits;

    BitVector() = default;
    explicit BitVector(std::size_t num_bits) { resize(num_bits); }

    void resize(std::size_t num_bits) {
        bits_ = num_bits;
        words_.assign((num_bits + kWordBits - 1) / kWordBits, 0);
        rank_index_dirty_ = true;
    }

    [[nodiscard]] std::size_t size() const noexcept { return bits_; }
    [[nodiscard]] bool        empty() const noexcept { return bits_ == 0; }

    // Set bit at position i to value v.
    int set(std::size_t i, bool v) noexcept {
        if (i >= bits_) return 7;                              // status_out_of_range
        std::size_t const w = i / kWordBits;
        std::size_t const b = i % kWordBits;
        word_t const mask = word_t{1} << b;
        if (v) words_[w] |=  mask;
        else   words_[w] &= ~mask;
        rank_index_dirty_ = true;
        return 0;
    }

    [[nodiscard]] bool get(std::size_t i) const noexcept {
        if (i >= bits_) return false;
        std::size_t const w = i / kWordBits;
        std::size_t const b = i % kWordBits;
        return ((words_[w] >> b) & word_t{1}) != 0;
    }

    // Aufbau des Rank-Index (Pflicht vor rank/select-Aufrufen)
    void build_rank_index() {
        std::size_t const nblocks = (words_.size() + kWordsPerBlock - 1) / kWordsPerBlock;
        rank_block_prefix_.assign(nblocks + 1, 0);
        std::uint64_t prefix = 0;
        for (std::size_t b = 0; b < nblocks; ++b) {
            rank_block_prefix_[b] = prefix;
            std::size_t const word_begin = b * kWordsPerBlock;
            std::size_t const word_end   = std::min(word_begin + kWordsPerBlock, words_.size());
            for (std::size_t w = word_begin; w < word_end; ++w) {
                prefix += std::popcount(words_[w]);
            }
        }
        rank_block_prefix_.back() = prefix;
        rank_index_dirty_ = false;
    }

    // rank1(i) = Anzahl gesetzter Bits in [0, i)
    [[nodiscard]] std::uint64_t rank1(std::size_t i) const noexcept {
        assert(!rank_index_dirty_ && "build_rank_index() must be called first");
        if (i >= bits_) i = bits_;
        std::size_t const block = i / kBlockBits;
        std::uint64_t count = rank_block_prefix_[block];
        std::size_t const word_begin = block * kWordsPerBlock;
        std::size_t const target_word = i / kWordBits;
        for (std::size_t w = word_begin; w < target_word; ++w) {
            count += std::popcount(words_[w]);
        }
        if (target_word < words_.size()) {
            std::size_t const bit_in_word = i % kWordBits;
            if (bit_in_word > 0) {
                word_t const mask = (word_t{1} << bit_in_word) - 1;
                count += std::popcount(words_[target_word] & mask);
            }
        }
        return count;
    }

    [[nodiscard]] std::uint64_t rank0(std::size_t i) const noexcept {
        if (i > bits_) i = bits_;
        return i - rank1(i);
    }

    [[nodiscard]] std::uint64_t total_ones() const noexcept {
        return rank_index_dirty_ ? 0 : rank_block_prefix_.back();
    }

    // select1(k) — Position des k-ten gesetzten Bits (1-based).
    // Naive O(n) Implementation; Phase 5+ ersetzt durch O(1) mit superblock-index.
    [[nodiscard]] std::size_t select1(std::uint64_t k) const noexcept {
        if (k == 0) return bits_;
        std::uint64_t count = 0;
        for (std::size_t i = 0; i < bits_; ++i) {
            if (get(i)) {
                if (++count == k) return i;
            }
        }
        return bits_;  // not found
    }

    [[nodiscard]] std::size_t select0(std::uint64_t k) const noexcept {
        if (k == 0) return bits_;
        std::uint64_t count = 0;
        for (std::size_t i = 0; i < bits_; ++i) {
            if (!get(i)) {
                if (++count == k) return i;
            }
        }
        return bits_;
    }

    void clear() noexcept {
        std::fill(words_.begin(), words_.end(), word_t{0});
        rank_index_dirty_ = true;
    }

    [[nodiscard]] std::vector<word_t> const& raw_words() const noexcept { return words_; }

private:
    std::size_t                bits_              = 0;
    std::vector<word_t>        words_{};
    std::vector<std::uint64_t> rank_block_prefix_{};
    bool                       rank_index_dirty_  = true;
};

}  // namespace comdare::succinct
