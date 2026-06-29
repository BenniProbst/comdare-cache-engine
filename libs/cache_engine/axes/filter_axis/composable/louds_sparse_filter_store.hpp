#pragma once
// V41 SuRF S2 Inkrement 1 (Task #42-Folge) — LoudsSparseFilterStore: succinct LOUDS-Sparse-Substrat + Single-Scan-Builder.
//
// @topic filter @achse axis_filter @schicht composable (Organ-statt-Tier) @paper P10 SuRF (Zhang et al. SIGMOD 2018)
//
// **Original-getreue Portierung** von surf_builder.hpp (buildSparse/insertKeyByte/insertKeyBytesToTrieUntilUnique)
// + louds_sparse.hpp (LoudsSparse-Ctor + lookupKey-Accessoren) aus ext/traversal/P10-SuRF (Apache-2.0).
// is_original=false ([[pseudocode-papers-fallback]]). SPARSE-ONLY (sparse_start_level=0, KEIN LoudsDense — reine
// Space-Opt, Inkrement 2; veraendert Membership/no-FN NICHT). node_count_dense_=child_count_dense_=0 fest verdrahtet.
//
// Keys = uint64 -> 8-Byte-BIG-ENDIAN (std::byteswap), damit numerische == lexikografische Trie-Ordnung (Range-Basis).

#include "surf_louds_bitvector.hpp"
#include "surf_suffix_bits.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace comdare::cache_engine::filter_axis::composable {

inline constexpr std::uint8_t  kSurfTerminator      = 255; // config.hpp Z.24
inline constexpr std::uint32_t kSurfRankBlockSize   = 512; // louds_sparse.hpp Z.161
inline constexpr std::uint32_t kSurfSelectSampleIvl = 64;  // louds_sparse.hpp Z.162

template <SurfSuffixType ST, unsigned HashLen, unsigned RealLen>
class LoudsSparseFilterStore {
public:
    using key_type    = std::uint64_t;
    using key_bytes_t = std::array<std::uint8_t, 8>;

    [[nodiscard]] static key_bytes_t to_bytes(std::uint64_t k) noexcept {
        std::uint64_t const be = std::byteswap(k); // config.hpp uint64ToString
        key_bytes_t         b{};
        for (unsigned i = 0; i < 8; ++i) b[i] = static_cast<std::uint8_t>((be >> (i * 8)) & 0xFFU);
        return b;
    }

    /// Single-Scan-Build aus SORTIERTEN uint64-Keys (defensiv kopiert+sortiert+dedupliziert wie S1).
    void build_from_sorted_keys(std::span<std::uint64_t const> keys) {
        clear();
        std::vector<std::uint64_t> ks(keys.begin(), keys.end());
        std::sort(ks.begin(), ks.end());
        ks.erase(std::unique(ks.begin(), ks.end()), ks.end());
        key_count_ = ks.size();
        if (key_count_ == 0) return;

        std::vector<key_bytes_t> kb(ks.size());
        for (std::size_t i = 0; i < ks.size(); ++i) kb[i] = to_bytes(ks[i]);

        build_sparse(kb);
        finalize();
    }

    // ── Query-Accessoren (sparse-only: node_count_dense_=child_count_dense_=0) ──
    [[nodiscard]] std::size_t key_count() const noexcept { return key_count_; }
    [[nodiscard]] bool        empty() const noexcept { return key_count_ == 0; }

    [[nodiscard]] std::uint8_t  label_at(std::uint32_t pos) const noexcept { return labels_flat_[pos]; }
    [[nodiscard]] bool          child_bit(std::uint32_t pos) const noexcept { return child_indicator_.read_bit(pos); }
    [[nodiscard]] std::uint32_t child_node_num(std::uint32_t pos) const noexcept {
        return child_indicator_.rank(pos);
    } // +child_count_dense_(0)
    [[nodiscard]] std::uint32_t first_label_pos(std::uint32_t node) const noexcept {
        return louds_bits_.select(node + 1);
    }
    [[nodiscard]] std::uint32_t suffix_pos(std::uint32_t pos) const noexcept {
        return pos - child_indicator_.rank(pos);
    }
    [[nodiscard]] std::uint32_t node_size(std::uint32_t pos) const noexcept {
        return louds_bits_.distance_to_next_set_bit(pos);
    }
    [[nodiscard]] std::uint32_t num_label_bits() const noexcept { return louds_bits_.num_bits(); }

    [[nodiscard]] bool suffix_check_equality(std::uint32_t idx, key_bytes_t const& kb, unsigned level) const noexcept {
        return suffix_.check_equality(idx, kb, level);
    }
    [[nodiscard]] int suffix_compare(std::uint32_t idx, key_bytes_t const& kb, unsigned level) const noexcept {
        return suffix_.compare(idx, kb, level);
    }

    // labels_->search (label_vector.hpp Z.107-150): Terminator-Skip + linear(<3)/binary(>=3, OHNE SSE).
    [[nodiscard]] bool label_search(std::uint8_t target, std::uint32_t& pos, std::uint32_t search_len) const noexcept {
        if (search_len > 1 && labels_flat_[pos] == kSurfTerminator) {
            ++pos;
            --search_len;
        }
        if (search_len < 3) {
            for (std::uint32_t i = 0; i < search_len; ++i)
                if (labels_flat_[pos + i] == target) {
                    pos += i;
                    return true;
                }
            return false;
        }
        std::uint32_t l = pos, r = pos + search_len;
        while (l < r) {
            std::uint32_t const m = (l + r) >> 1;
            if (target < labels_flat_[m])
                r = m;
            else if (target == labels_flat_[m]) {
                pos = m;
                return true;
            } else
                l = m + 1;
        }
        return false;
    }

    // labels_->searchGreaterThan (label_vector.hpp Z.122-133/193-216) — fuer Range moveToLeftInNextSubtrie.
    [[nodiscard]] bool label_search_greater_than(std::uint8_t target, std::uint32_t& pos,
                                                 std::uint32_t search_len) const noexcept {
        if (search_len > 1 && labels_flat_[pos] == kSurfTerminator) {
            ++pos;
            --search_len;
        }
        std::uint32_t const base = pos;
        if (search_len < 3) {
            for (std::uint32_t i = 0; i < search_len; ++i)
                if (labels_flat_[pos + i] > target) {
                    pos += i;
                    return true;
                }
            return false;
        }
        std::uint32_t l = base, r = base + search_len;
        while (l < r) {
            std::uint32_t const m = (l + r) >> 1;
            if (target < labels_flat_[m])
                r = m;
            else if (target == labels_flat_[m]) {
                if (m < base + search_len - 1) {
                    pos = m + 1;
                    return true;
                }
                return false;
            } else
                l = m + 1;
        }
        if (l < base + search_len) {
            pos = l;
            return true;
        }
        return false;
    }

    // ── bit_size / Tuning-Observer ──
    [[nodiscard]] std::size_t bit_size() const noexcept {
        return labels_flat_.size() * 8 // Labels (1 Byte/Item)
               + child_indicator_.bit_size_bits() + child_indicator_.lut_bits() + louds_bits_.bit_size_bits() +
               louds_bits_.lut_bits() + suffix_.bit_size();
    }
    [[nodiscard]] double bits_per_key() const noexcept {
        return key_count_ ? static_cast<double>(bit_size()) / static_cast<double>(key_count_) : 0.0;
    }

    void clear() noexcept {
        labels_lv_.clear();
        child_ind_lv_.clear();
        louds_lv_.clear();
        suffix_words_lv_.clear();
        node_counts_.clear();
        is_last_term_.clear();
        labels_flat_.clear();
        child_indicator_ = SurfRank{};
        louds_bits_      = SurfSelect{};
        suffix_.clear();
        key_count_ = 0;
    }

private:
    // ── Build-Hilfen (port surf_builder.hpp) ──
    [[nodiscard]] std::uint32_t tree_height() const noexcept { return static_cast<std::uint32_t>(labels_lv_.size()); }
    [[nodiscard]] std::uint32_t num_items(std::uint32_t level) const noexcept {
        return static_cast<std::uint32_t>(labels_lv_[level].size());
    }

    static void set_bit(std::vector<std::uint64_t>& bits, std::uint32_t pos) noexcept {
        bits[pos / 64] |= (kSurfMsbMask >> (pos % 64));
    }
    static bool read_bit(std::vector<std::uint64_t> const& bits, std::uint32_t pos) noexcept {
        return (bits[pos / 64] & (kSurfMsbMask >> (pos % 64))) != 0;
    }

    void add_level() {
        labels_lv_.emplace_back();
        child_ind_lv_.emplace_back();
        louds_lv_.emplace_back();
        suffix_words_lv_.emplace_back();
        node_counts_.push_back(0);
        is_last_term_.push_back(0);
        child_ind_lv_.back().push_back(0);
        louds_lv_.back().push_back(0);
    }

    [[nodiscard]] bool is_char_common_prefix(std::uint8_t c, std::uint32_t level) const noexcept {
        return (level < tree_height()) && (is_last_term_[level] == 0) && (c == labels_lv_[level].back());
    }
    [[nodiscard]] bool is_level_empty(std::uint32_t level) const noexcept {
        return (level >= tree_height()) || labels_lv_[level].empty();
    }
    void move_to_next_item_slot(std::uint32_t level) {
        if (num_items(level) % 64 == 0) {
            child_ind_lv_[level].push_back(0);
            louds_lv_[level].push_back(0);
        }
    }
    void insert_key_byte(std::uint8_t c, std::uint32_t level, bool is_start_of_node, bool is_term) {
        if (level >= tree_height()) add_level();
        if (level > 0) set_bit(child_ind_lv_[level - 1], num_items(level - 1) - 1);
        labels_lv_[level].push_back(c);
        if (is_start_of_node) {
            set_bit(louds_lv_[level], num_items(level) - 1);
            ++node_counts_[level];
        }
        is_last_term_[level] = is_term ? 1 : 0;
        move_to_next_item_slot(level);
    }

    [[nodiscard]] std::uint32_t skip_common_prefix(key_bytes_t const& key) {
        std::uint32_t level = 0;
        while (level < 8 && is_char_common_prefix(key[level], level)) {
            set_bit(child_ind_lv_[level], num_items(level) - 1);
            ++level;
        }
        return level;
    }
    // next_len==0 => kein Nachfolger (letzter Key). insertKeyBytesToTrieUntilUnique (surf_builder.hpp Z.208-244).
    [[nodiscard]] std::uint32_t insert_until_unique(key_bytes_t const& key, key_bytes_t const& next, unsigned next_len,
                                                    std::uint32_t start_level) {
        std::uint32_t level            = start_level;
        bool          is_start_of_node = false;
        if (is_level_empty(level)) is_start_of_node = true;
        insert_key_byte(key[level], level, is_start_of_node, false);
        ++level;
        // level > next_len  ODER  Praefix [0,level) unterscheidet sich von next
        bool prefix_same = (level <= next_len);
        if (prefix_same)
            for (unsigned i = 0; i < level; ++i)
                if (key[i] != next[i]) {
                    prefix_same = false;
                    break;
                }
        if (level > next_len || !prefix_same) return level;

        is_start_of_node = true;
        while (level < 8 && level < next_len && key[level] == next[level]) {
            insert_key_byte(key[level], level, is_start_of_node, false);
            ++level;
        }
        if (level < 8) {
            insert_key_byte(key[level], level, is_start_of_node, false);
        } else {
            insert_key_byte(kSurfTerminator, level, is_start_of_node, true);
        }
        ++level;
        return level;
    }
    void insert_suffix(key_bytes_t const& key, std::uint32_t level) {
        if (level >= tree_height()) add_level();
        std::uint64_t const w = SurfSuffixBits<ST, HashLen, RealLen>::construct_suffix(key, level);
        suffix_words_lv_[level - 1].push_back(w);
    }

    void build_sparse(std::vector<key_bytes_t> const& keys) { // surf_builder.hpp Z.185-197
        for (std::size_t i = 0; i < keys.size(); ++i) {
            std::uint32_t     level  = skip_common_prefix(keys[i]);
            std::size_t const curpos = i;
            while ((i + 1 < keys.size()) && keys[curpos] == keys[i + 1]) ++i; // (bei dedup nie wahr)
            if (i < keys.size() - 1)
                level = insert_until_unique(keys[curpos], keys[i + 1], 8, level);
            else
                level = insert_until_unique(keys[curpos], key_bytes_t{}, 0, level);
            insert_suffix(keys[curpos], level);
        }
    }

    void finalize() {
        std::uint32_t const        height = tree_height();
        std::vector<std::uint32_t> nbpl(height);
        for (std::uint32_t L = 0; L < height; ++L) nbpl[L] = num_items(L);

        // Labels level-major flach legen.
        for (std::uint32_t L = 0; L < height; ++L)
            labels_flat_.insert(labels_flat_.end(), labels_lv_[L].begin(), labels_lv_[L].end());

        child_indicator_ = SurfRank(kSurfRankBlockSize, child_ind_lv_, nbpl);
        louds_bits_      = SurfSelect(kSurfSelectSampleIvl, louds_lv_, nbpl);

        // Suffixe level-major anhaengen (passend zu getSuffixPos = pos - rank(pos)).
        if constexpr (ST != SurfSuffixType::kNone) {
            for (std::uint32_t L = 0; L < height; ++L)
                for (std::uint64_t w : suffix_words_lv_[L]) suffix_.append_word(w);
        }
    }

    // per-Level Build-Vektoren
    std::vector<std::vector<std::uint8_t>>  labels_lv_{};
    std::vector<std::vector<std::uint64_t>> child_ind_lv_{};
    std::vector<std::vector<std::uint64_t>> louds_lv_{};
    std::vector<std::vector<std::uint64_t>> suffix_words_lv_{};
    std::vector<std::uint32_t>              node_counts_{};
    std::vector<std::uint8_t>               is_last_term_{};

    // finalisierte succinct Strukturen
    std::vector<std::uint8_t>            labels_flat_{};
    SurfRank                             child_indicator_{};
    SurfSelect                           louds_bits_{};
    SurfSuffixBits<ST, HashLen, RealLen> suffix_{};
    std::size_t                          key_count_ = 0;
};

} // namespace comdare::cache_engine::filter_axis::composable
