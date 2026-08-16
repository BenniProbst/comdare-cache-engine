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
//
// A8-S5 Familie 02b (2026-08-04) -- SCHNITT-REGEL (Dossier 20260803-a8_f2 Abschn. 3.4: "Speicher NUR ueber
// das Allokator-Achsen-Interface"): bits_, rank_lut_ und select_lut_ liefen bis hierher ueber den
// Default-Allokator, also an der Achse vorbei. Seit dem Schnitt haengen sie an der Strategie-Instanz des
// BESITZENDEN Organs (LoudsSparseFilterStore), die im Konstruktor hereingereicht wird -- EINE Instanz je
// Organ (surf_axis_allocator.hpp). Der Papertreue-Vertrag ist davon UNBERUEHRT: es wechselt allein die
// Speicher-QUELLE, kein Bit des MSB-first-Layouts, keine Wort-Arithmetik, keine Anker-Zeile. Die
// Zeilen-Anker in die Original-Header (bitvector.hpp Z.141-169, rank.hpp Z.33-40, select.hpp Z.34-66,
// popcount.h Z.70-87/Z.104-119) sitzen unveraendert an den Rumpf-Zeilen, die sie beschreiben -- der Schnitt
// hat sie MITGEFUEHRT, wo er eine Zeile verschoben hat (siehe build_from_levels unten: der Rumpf des
// Original-Konstruktors ist in eine benannte Methode gezogen, DAMIT das Neu-Bauen ohne Temporaer-Objekt
// geht; der Anker ist mitgewandert, nicht verwaist).

#include "surf_axis_allocator.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::filter_axis::composable {

/// Ein Wort-Vektor EINER Trie-Ebene (die innere Ebene des verschachtelten Build-Containers).
using surf_word_vec_t = surf_vec_t<std::uint64_t>;
/// Die per-Level-Wortvektoren (die AEUSSERE Ebene) -- beide Ebenen haengen an der Achse.
using surf_word_vec_per_level_t = surf_vec_t<surf_word_vec_t>;
/// Bit-Zahlen je Level bzw. die Rank-/Select-Lookup-Tabellen.
using surf_u32_vec_t = surf_vec_t<std::uint32_t>;

inline constexpr std::uint64_t kSurfMsbMask  = 0x8000000000000000ULL; // config.hpp Z.18
inline constexpr unsigned      kSurfWordSize = 64U;

// popcountLinear: Anzahl 1-Bits in den ERSTEN `nbits` Bits ab Wort-Offset `x` (MSB-first). popcount.h Z.70-87.
[[nodiscard]] inline std::uint64_t surf_popcount_linear(std::uint64_t const* bits, std::uint64_t x,
                                                        std::uint64_t nbits) noexcept {
    if (nbits == 0) return 0;
    std::uint64_t const lastword = (nbits - 1) / kSurfWordSize;
    std::uint64_t       p        = 0;
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
        if (k > lcount) {
            x &= ((1ULL << testbits) - 1);
            loc += testbits;
            k -= lcount;
        } else {
            x >>= testbits;
        }
    }
    return loc + k;
}

/// MSB-first Flachbitvektor ueber einen Wort-Vektor der Allokator-Achse. Portiert surf::Bitvector (bitvector.hpp).
class SurfBitVector {
public:
    /// A8-S5-02b Form-B-Ausweis: der Speicher dieses Substrats laeuft ueber die Allokator-Achse.
    using allocator_type = filter_surf_allocator_t;

    /// EINZIGER Weg, einen leeren Bitvektor anzulegen: mit der Strategie-Instanz des besitzenden Organs.
    /// Ein Default-Ctor existiert BEWUSST nicht -- der StdAllocatorAdapter ist nicht default-konstruierbar,
    /// ein stilles Zurueckfallen auf einen Default-Allokator ist damit compile-hart ausgeschlossen.
    explicit SurfBitVector(allocator_type& strat) : bits_(strat.template as_std_allocator<std::uint64_t>()) {}

    /// Konstruiert aus per-Level-Wortvektoren (concatenateBitvectors, bitvector.hpp Z.141-169, BYTE-GENAU).
    SurfBitVector(allocator_type& strat, surf_word_vec_per_level_t const& bitvector_per_level,
                  surf_u32_vec_t const& num_bits_per_level)
        : SurfBitVector(strat) {
        build_from_levels(bitvector_per_level, num_bits_per_level);
    }

    /// ALLOKATOR-ERWEITERTE Kopie: rebindet auf die Strategie-Instanz des ZIEL-Organs. Die gewoehnliche
    /// Kopie ist geloescht, weil sie den Adapter der QUELLE mitschleppte -- ein dangling Zeiger, sobald
    /// die Quelle stirbt (dieselbe Falle, die 02a fuer die verschachtelten Chunk-Vektoren dokumentiert).
    SurfBitVector(SurfBitVector const& o, allocator_type& strat)
        : bits_(o.bits_, strat.template as_std_allocator<std::uint64_t>()), num_bits_(o.num_bits_) {}
    SurfBitVector(SurfBitVector const&) = delete;
    /// Kopier-ZUWEISUNG bleibt zulaessig und ist sicher: propagate_on_container_copy_assignment ist am
    /// StdAllocatorAdapter false, das Ziel BEHAELT also seinen eigenen Adapter und kopiert nur Elemente.
    SurfBitVector& operator=(SurfBitVector const&) = default;
    ~SurfBitVector()                               = default;

    /// Rumpf des Original-Konstruktors (concatenateBitvectors, bitvector.hpp Z.141-169) als benannte
    /// Methode -- MITGEFUEHRTER Anker: der Schnitt hat die Zeilen aus dem Ctor hierher gezogen, damit
    /// LoudsSparseFilterStore::finalize() IN PLACE neu bauen kann statt ein Temporaer-Objekt zuzuweisen
    /// (das haette den Puffer ein zweites Mal ueber die Achse alloziert und die Achsen-Statistik der
    /// Organ-Instanz verfaelscht). Verhalten identisch zum Ctor-Pfad: num_bits_ startet bei 0.
    void build_from_levels(surf_word_vec_per_level_t const& bitvector_per_level,
                           surf_u32_vec_t const&            num_bits_per_level) {
        num_bits_ = 0;
        for (std::uint32_t n : num_bits_per_level) num_bits_ += n;
        // +1 Carry-Wort: concatenate (und distance_to_next_set_bit) hat ein One-Past-End-Muster (bitvector.hpp
        // Z.165 schreibt bits_[numWords()] wenn ein Level mit Carry exakt eine Wort-Grenze auffuellt). Das
        // Original nutzt new word_t[numWords()] (UB, faultet meist nicht); die Vektor-Portierung braucht
        // das Extra-Wort, sonst heap-buffer-overflow (ASan-bestaetigt, Verifikation wuegyse1h). num_words()/
        // bit_size_bits()/rank/select bleiben auf num_bits_ basiert — das Extra-Wort ist reines Padding.
        // A8-S5-02b: diese EINE Ersetzung des Original-`new word_t[]` durch einen besitzenden Vektor ist
        // seit dem Schnitt zugleich die Naht zur Allokator-Achse -- der Vektor traegt den
        // StdAllocatorAdapter der Organ-Strategie, das Extra-Wort wird also ebenfalls ueber die Achse
        // besorgt. Die bewusste Abweichung vom Original (Extra-Wort statt UB) bleibt inhaltlich unveraendert.
        bits_.assign(static_cast<std::size_t>(num_words()) + 1, 0);
        concatenate(bitvector_per_level, num_bits_per_level);
    }

    /// Zuruecksetzen ohne Allokator-Wechsel (ersetzt das fruehere `bv = SurfBitVector{}`, das mit einem
    /// achsen-gebundenen Puffer nicht mehr moeglich waere, ohne eine fremde Strategie mitzubringen).
    void clear() noexcept {
        bits_.clear();
        num_bits_ = 0;
    }

    [[nodiscard]] std::uint32_t num_bits() const noexcept { return num_bits_; }
    [[nodiscard]] std::uint32_t num_words() const noexcept {
        return (num_bits_ % kSurfWordSize == 0) ? (num_bits_ / kSurfWordSize) : (num_bits_ / kSurfWordSize + 1);
    }
    [[nodiscard]] std::uint64_t const* data() const noexcept { return bits_.data(); }
    [[nodiscard]] std::size_t          bit_size_bits() const noexcept {
        return static_cast<std::size_t>(num_words()) * kSurfWordSize;
    }

    [[nodiscard]] bool read_bit(std::uint32_t pos) const noexcept {
        return (bits_[pos / kSurfWordSize] & (kSurfMsbMask >> (pos & (kSurfWordSize - 1)))) != 0;
    }

    // Distanz vom Bit `pos` zum naechsten gesetzten Bit (>pos). bitvector.hpp Z.77-102 mit std::countl_zero.
    [[nodiscard]] std::uint32_t distance_to_next_set_bit(std::uint32_t pos) const noexcept {
        std::uint32_t distance = 1;
        std::uint32_t word_id  = (pos + 1) / kSurfWordSize;
        std::uint32_t offset   = (pos + 1) % kSurfWordSize;
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
    void concatenate(surf_word_vec_per_level_t const& bvpl, surf_u32_vec_t const& nbpl) {
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

    // A8-S5-02b: der Wort-Puffer haengt an der Achse (surf_word_vec_t == std::vector<uint64,Achsen-Adapter>).
    surf_word_vec_t bits_;
    std::uint32_t   num_bits_ = 0;
};

/// Rank-Bitvektor (rank(pos) = Anzahl 1-Bits in [0,pos]). Portiert surf::BitvectorRank (rank.hpp).
class SurfRank : public SurfBitVector {
public:
    explicit SurfRank(allocator_type& strat)
        : SurfBitVector(strat), rank_lut_(strat.template as_std_allocator<std::uint32_t>()) {}
    SurfRank(allocator_type& strat, std::uint32_t basic_block_size, surf_word_vec_per_level_t const& bvpl,
             surf_u32_vec_t const& nbpl)
        : SurfRank(strat) {
        build(basic_block_size, bvpl, nbpl);
    }
    SurfRank(SurfRank const& o, allocator_type& strat)
        : SurfBitVector(o, strat), basic_block_size_(o.basic_block_size_),
          rank_lut_(o.rank_lut_, strat.template as_std_allocator<std::uint32_t>()) {}
    SurfRank(SurfRank const&)            = delete;
    SurfRank& operator=(SurfRank const&) = default;
    ~SurfRank()                          = default;

    /// IN-PLACE-Neubau (ersetzt `rank = SurfRank(bbs, bvpl, nbpl)` im Store): derselbe Rumpf wie der Ctor,
    /// nur ohne Temporaer-Objekt -- s. build_from_levels-Begruendung in SurfBitVector.
    void build(std::uint32_t basic_block_size, surf_word_vec_per_level_t const& bvpl, surf_u32_vec_t const& nbpl) {
        basic_block_size_ = basic_block_size;
        build_from_levels(bvpl, nbpl);
        init_rank_lut();
    }
    void clear() noexcept {
        SurfBitVector::clear();
        rank_lut_.clear();
        basic_block_size_ = 0;
    }

    // 1-basiert: rank(pos) zaehlt 1-Bits in Positionen [0,pos]. rank.hpp Z.33-40.
    [[nodiscard]] std::uint32_t rank(std::uint32_t pos) const noexcept {
        std::uint32_t const words_per_block = basic_block_size_ / kSurfWordSize;
        std::uint32_t const block_id        = pos / basic_block_size_;
        std::uint32_t const offset          = pos & (basic_block_size_ - 1);
        return rank_lut_[block_id] + static_cast<std::uint32_t>(surf_popcount_linear(
                                         data(), static_cast<std::uint64_t>(block_id) * words_per_block, offset + 1));
    }

    [[nodiscard]] std::size_t lut_bits() const noexcept { return rank_lut_.size() * sizeof(std::uint32_t) * 8; }

private:
    void init_rank_lut() {
        std::uint32_t const words_per_block = basic_block_size_ / kSurfWordSize;
        std::uint32_t const num_blocks      = num_bits_ / basic_block_size_ + 1;
        rank_lut_.assign(num_blocks, 0);
        std::uint32_t cumu = 0;
        for (std::uint32_t i = 0; i < num_blocks - 1; ++i) {
            rank_lut_[i] = cumu;
            cumu += static_cast<std::uint32_t>(
                surf_popcount_linear(data(), static_cast<std::uint64_t>(i) * words_per_block, basic_block_size_));
        }
        rank_lut_[num_blocks - 1] = cumu;
    }

    std::uint32_t  basic_block_size_ = 0;
    surf_u32_vec_t rank_lut_; // A8-S5-02b: ueber die Achse
};

/// Select-Bitvektor (select(rank) = Position des rank-ten 1-Bits). Portiert surf::BitvectorSelect (select.hpp).
/// ANNAHME (wie Paper): das erste Bit ist 1 (initSelectLut Z.139). Bei LOUDS-bits ist das per Konstruktion erfuellt.
class SurfSelect : public SurfBitVector {
public:
    explicit SurfSelect(allocator_type& strat)
        : SurfBitVector(strat), select_lut_(strat.template as_std_allocator<std::uint32_t>()) {}
    SurfSelect(allocator_type& strat, std::uint32_t sample_interval, surf_word_vec_per_level_t const& bvpl,
               surf_u32_vec_t const& nbpl)
        : SurfSelect(strat) {
        build(sample_interval, bvpl, nbpl);
    }
    SurfSelect(SurfSelect const& o, allocator_type& strat)
        : SurfBitVector(o, strat), sample_interval_(o.sample_interval_), num_ones_(o.num_ones_),
          select_lut_(o.select_lut_, strat.template as_std_allocator<std::uint32_t>()) {}
    SurfSelect(SurfSelect const&)            = delete;
    SurfSelect& operator=(SurfSelect const&) = default;
    ~SurfSelect()                            = default;

    /// IN-PLACE-Neubau (ersetzt `sel = SurfSelect(ivl, bvpl, nbpl)` im Store), s. SurfRank::build.
    void build(std::uint32_t sample_interval, surf_word_vec_per_level_t const& bvpl, surf_u32_vec_t const& nbpl) {
        sample_interval_ = sample_interval;
        build_from_levels(bvpl, nbpl);
        init_select_lut();
    }
    void clear() noexcept {
        SurfBitVector::clear();
        select_lut_.clear();
        sample_interval_ = 0;
        num_ones_        = 0;
    }

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
        if (offset == kSurfWordSize - 1) {
            ++word_id;
            offset = 0;
        } else {
            ++offset;
        }
        std::uint64_t word         = (bits_[word_id] << offset) >> offset; // oberste `offset` MSBs nullen
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
        select_lut_.push_back(0); // ASSERT: erstes Bit ist 1 (select.hpp Z.139)
        std::uint32_t sampling_ones = sample_interval_;
        std::uint32_t cumu_ones     = 0;
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

    std::uint32_t  sample_interval_ = 0;
    std::uint32_t  num_ones_        = 0;
    surf_u32_vec_t select_lut_; // A8-S5-02b: ueber die Achse
};

} // namespace comdare::cache_engine::filter_axis::composable
