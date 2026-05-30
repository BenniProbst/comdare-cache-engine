#pragma once
// V41 SuRF S2 Inkrement 1 (Task #42-Folge) — portables succinct Substrat: SurfBitVector + SurfRank + SurfSelect.
//
// @topic filter @achse axis_filter @schicht composable (Organ-statt-Tier) @paper P10 SuRF (Zhang et al. SIGMOD 2018)
//
// **Original-getreue Portierung** von ext/traversal/P10-SuRF/SuRF/include/{bitvector,rank,select,popcount}.hpp
// (efficient/SuRF, Apache-2.0). is_original=false ([[pseudocode-papers-fallback]]): GCC-Builtins -> C++23 <bit>
// (std::popcount/std::countl_zero), KEIN unsigned __int128, KEIN SSE, KEIN inline-asm, KEIN __builtin_prefetch.
//
// **MSB-first Bit-Layout (PFLICHT-Vertrag):** Bit `pos` lebt in `words[pos/64] & (kMsbMask >> (pos%64))`
// — IDENTISCH zu surf_builder.hpp readBit/setBit (Z.32-44) und bitvector.hpp readBit (Z.70-75). Jede
// Abweichung desynct rank/select gegen den Builder. comdare::succinct::BitVector ist LSB-first + O(n)-select1
// und wird BEWUSST NICHT wiederverwendet.

#include <bit>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace comdare::cache_engine::filter_axis::composable {

inline constexpr std::uint64_t kSurfMsbMask  = 0x8000000000000000ULL;   // config.hpp Z.18
inline constexpr unsigned      kSurfWordSize = 64U;

// popcountLinear: Anzahl 1-Bits in den ERSTEN `nbits` Bits ab Wort-Offset `x` (MSB-first). popcount.h Z.70-87.
[[nodiscard]] inline std::uint64_t surf_popcount_linear(std::uint64_t const* bits, std::uint64_t x, std::uint64_t nbits) noexcept {
    if (nbits == 0) return 0;
    std::uint64_t const lastword = (nbits - 1) / kSurfWordSize;
    std::uint64_t p = 0;
    for (std::uint64_t i = 0; i < lastword; ++i) p += static_cast<std::uint64_t>(std::popcount(bits[x + i]));
    // Restwort: oberste ((nbits-1)%64)+1 Bits behalten (MSB-first), Rest wegschieben.
    std::uint64_t const lastshifted = bits[x + lastword] >> (63 - ((nbits - 1) & (kSurfWordSize - 1)));
    p += static_cast<std::uint64_t>(std::popcount(lastshifted));
    return p;
}

// Index des k-ten gesetzten Bits in x, MSB-first (Position 0 == MSB). popcount.h Z.104-119 (binaerer Abstieg).
[[nodiscard]] inline int surf_select64(std::uint64_t x, int k) noexcept {
    int loc = -1;
    for (int testbits = 32; testbits > 0; testbits >>= 1) {
        int const lcount = std::popcount(x >> testbits);
        if (k > lcount) { x &= ((1ULL << testbits) - 1); loc += testbits; k -= lcount; }
        else            { x >>= testbits; }
    }
    return loc + k;
}

/// MSB-first Flachbitvektor ueber std::vector<uint64_t>. Portiert surf::Bitvector (bitvector.hpp).
class SurfBitVector {
public:
    SurfBitVector() = default;

    /// Konstruiert aus per-Level-Wortvektoren (concatenateBitvectors, bitvector.hpp Z.141-169, BYTE-GENAU).
    SurfBitVector(std::vector<std::vector<std::uint64_t>> const& bitvector_per_level,
                  std::vector<std::uint32_t> const& num_bits_per_level) {
        for (std::uint32_t n : num_bits_per_level) num_bits_ += n;
        // +1 Carry-Wort: concatenate (und distance_to_next_set_bit) hat ein One-Past-End-Muster (bitvector.hpp
        // Z.165 schreibt bits_[numWords()] wenn ein Level mit Carry exakt eine Wort-Grenze auffuellt). Das
        // Original nutzt new word_t[numWords()] (UB, faultet meist nicht); die std::vector-Portierung braucht
        // das Extra-Wort, sonst heap-buffer-overflow (ASan-bestaetigt, Verifikation wuegyse1h). num_words()/
        // bit_size_bits()/rank/select bleiben auf num_bits_ basiert — das Extra-Wort ist reines Padding.
        bits_.assign(static_cast<std::size_t>(num_words()) + 1, 0);
        concatenate(bitvector_per_level, num_bits_per_level);
    }

    [[nodiscard]] std::uint32_t num_bits()  const noexcept { return num_bits_; }
    [[nodiscard]] std::uint32_t num_words() const noexcept {
        return (num_bits_ % kSurfWordSize == 0) ? (num_bits_ / kSurfWordSize) : (num_bits_ / kSurfWordSize + 1);
    }
    [[nodiscard]] std::uint64_t const* data() const noexcept { return bits_.data(); }
    [[nodiscard]] std::size_t bit_size_bits() const noexcept { return static_cast<std::size_t>(num_words()) * kSurfWordSize; }

    [[nodiscard]] bool read_bit(std::uint32_t pos) const noexcept {
        return (bits_[pos / kSurfWordSize] & (kSurfMsbMask >> (pos & (kSurfWordSize - 1)))) != 0;
    }

    // Distanz vom Bit `pos` zum naechsten gesetzten Bit (>pos). bitvector.hpp Z.77-102 mit std::countl_zero.
    [[nodiscard]] std::uint32_t distance_to_next_set_bit(std::uint32_t pos) const noexcept {
        std::uint32_t distance = 1;
        std::uint32_t word_id = (pos + 1) / kSurfWordSize;
        std::uint32_t offset  = (pos + 1) % kSurfWordSize;
        // Defensiv: pos+1 kann genau auf die Wort-Grenze jenseits num_words() fallen (pos==num_bits-1,
        // num_bits%64==0). Dann existiert kein naechstes Set-Bit -> der letzte Knoten reicht bis num_bits.
        if (word_id >= num_words()) return num_bits_ - pos;
        std::uint64_t test_bits = (offset == 0) ? bits_[word_id] : (bits_[word_id] << offset);
        if (test_bits > 0) return distance + static_cast<std::uint32_t>(std::countl_zero(test_bits));
        if (word_id == num_words() - 1) return num_bits_ - pos;
        distance += (kSurfWordSize - offset);
        while (word_id < num_words() - 1) {
            ++word_id;
            test_bits = bits_[word_id];
            if (test_bits > 0) return distance + static_cast<std::uint32_t>(std::countl_zero(test_bits));
            distance += kSurfWordSize;
        }
        // KORREKTUR ggue. surf::Bitvector (bitvector.hpp Z.101 `return distance;`): bei FEHLENDEM naechsten
        // Set-Bit (letzter LOUDS-Knoten) wuerde `distance` ins Padding jenseits num_bits ueberschiessen und
        // node_size + Label-Binaersuche korrumpieren. Der letzte Knoten reicht bis num_bits => num_bits - pos.
        return num_bits_ - pos;
    }

protected:
    void concatenate(std::vector<std::vector<std::uint64_t>> const& bvpl, std::vector<std::uint32_t> const& nbpl) {
        std::uint32_t bit_shift = 0;
        std::uint32_t word_id   = 0;
        for (std::size_t level = 0; level < nbpl.size(); ++level) {
            if (nbpl[level] == 0) continue;
            std::uint32_t const num_complete_words = nbpl[level] / kSurfWordSize;
            for (std::uint32_t word = 0; word < num_complete_words; ++word) {
                bits_[word_id] |= (bvpl[level][word] >> bit_shift);
                ++word_id;
                if (bit_shift > 0) bits_[word_id] |= (bvpl[level][word] << (kSurfWordSize - bit_shift));
            }
            std::uint32_t const bits_remain = nbpl[level] - num_complete_words * kSurfWordSize;
            if (bits_remain > 0) {
                std::uint64_t const last_word = bvpl[level][num_complete_words];
                bits_[word_id] |= (last_word >> bit_shift);
                if (bit_shift + bits_remain < kSurfWordSize) {
                    bit_shift += bits_remain;
                } else {
                    ++word_id;
                    bits_[word_id] |= (last_word << (kSurfWordSize - bit_shift));
                    bit_shift = bit_shift + bits_remain - kSurfWordSize;
                }
            }
        }
    }

    std::vector<std::uint64_t> bits_{};
    std::uint32_t              num_bits_ = 0;
};

/// Rank-Bitvektor (rank(pos) = Anzahl 1-Bits in [0,pos]). Portiert surf::BitvectorRank (rank.hpp).
class SurfRank : public SurfBitVector {
public:
    SurfRank() = default;
    SurfRank(std::uint32_t basic_block_size,
             std::vector<std::vector<std::uint64_t>> const& bvpl, std::vector<std::uint32_t> const& nbpl)
        : SurfBitVector(bvpl, nbpl), basic_block_size_(basic_block_size) { init_rank_lut(); }

    // 1-basiert: rank(pos) zaehlt 1-Bits in Positionen [0,pos]. rank.hpp Z.33-40.
    [[nodiscard]] std::uint32_t rank(std::uint32_t pos) const noexcept {
        std::uint32_t const words_per_block = basic_block_size_ / kSurfWordSize;
        std::uint32_t const block_id = pos / basic_block_size_;
        std::uint32_t const offset   = pos & (basic_block_size_ - 1);
        return rank_lut_[block_id]
             + static_cast<std::uint32_t>(surf_popcount_linear(data(), static_cast<std::uint64_t>(block_id) * words_per_block, offset + 1));
    }

    [[nodiscard]] std::size_t lut_bits() const noexcept { return rank_lut_.size() * sizeof(std::uint32_t) * 8; }

private:
    void init_rank_lut() {
        std::uint32_t const words_per_block = basic_block_size_ / kSurfWordSize;
        std::uint32_t const num_blocks = num_bits_ / basic_block_size_ + 1;
        rank_lut_.assign(num_blocks, 0);
        std::uint32_t cumu = 0;
        for (std::uint32_t i = 0; i < num_blocks - 1; ++i) {
            rank_lut_[i] = cumu;
            cumu += static_cast<std::uint32_t>(surf_popcount_linear(data(), static_cast<std::uint64_t>(i) * words_per_block, basic_block_size_));
        }
        rank_lut_[num_blocks - 1] = cumu;
    }

    std::uint32_t              basic_block_size_ = 0;
    std::vector<std::uint32_t> rank_lut_{};
};

/// Select-Bitvektor (select(rank) = Position des rank-ten 1-Bits). Portiert surf::BitvectorSelect (select.hpp).
/// ANNAHME (wie Paper): das erste Bit ist 1 (initSelectLut Z.139). Bei LOUDS-bits ist das per Konstruktion erfuellt.
class SurfSelect : public SurfBitVector {
public:
    SurfSelect() = default;
    SurfSelect(std::uint32_t sample_interval,
               std::vector<std::vector<std::uint64_t>> const& bvpl, std::vector<std::uint32_t> const& nbpl)
        : SurfBitVector(bvpl, nbpl), sample_interval_(sample_interval) { init_select_lut(); }

    [[nodiscard]] std::uint32_t num_ones() const noexcept { return num_ones_; }

    // 0-basierte Position des rank-ten (1-basiert) 1-Bits. select.hpp Z.34-66.
    [[nodiscard]] std::uint32_t select(std::uint32_t rank) const noexcept {
        std::uint32_t lut_idx   = rank / sample_interval_;
        std::uint32_t rank_left = rank % sample_interval_;
        if (lut_idx == 0) --rank_left;
        std::uint32_t pos = select_lut_[lut_idx];
        if (rank_left == 0) return pos;

        std::uint32_t word_id = pos / kSurfWordSize;
        std::uint32_t offset  = pos % kSurfWordSize;
        if (offset == kSurfWordSize - 1) { ++word_id; offset = 0; }
        else                             { ++offset; }
        std::uint64_t word = (bits_[word_id] << offset) >> offset;   // oberste `offset` MSBs nullen
        std::uint32_t ones_in_word = static_cast<std::uint32_t>(std::popcount(word));
        while (ones_in_word < rank_left) {
            ++word_id;
            word = bits_[word_id];
            rank_left -= ones_in_word;
            ones_in_word = static_cast<std::uint32_t>(std::popcount(word));
        }
        return word_id * kSurfWordSize + static_cast<std::uint32_t>(surf_select64(word, static_cast<int>(rank_left)));
    }

    [[nodiscard]] std::size_t lut_bits() const noexcept { return select_lut_.size() * sizeof(std::uint32_t) * 8; }

private:
    void init_select_lut() {
        std::uint32_t num_words = num_bits_ / kSurfWordSize;
        if (num_bits_ % kSurfWordSize != 0) ++num_words;
        select_lut_.clear();
        select_lut_.push_back(0);   // ASSERT: erstes Bit ist 1 (select.hpp Z.139)
        std::uint32_t sampling_ones = sample_interval_;
        std::uint32_t cumu_ones = 0;
        for (std::uint32_t i = 0; i < num_words; ++i) {
            std::uint32_t const num_ones_in_word = static_cast<std::uint32_t>(std::popcount(bits_[i]));
            while (sampling_ones <= cumu_ones + num_ones_in_word) {
                int const diff = static_cast<int>(sampling_ones - cumu_ones);
                select_lut_.push_back(i * kSurfWordSize + static_cast<std::uint32_t>(surf_select64(bits_[i], diff)));
                sampling_ones += sample_interval_;
            }
            cumu_ones += num_ones_in_word;
        }
        num_ones_ = cumu_ones;
    }

    std::uint32_t              sample_interval_ = 0;
    std::uint32_t              num_ones_ = 0;
    std::vector<std::uint32_t> select_lut_{};
};

}  // namespace comdare::cache_engine::filter_axis::composable
