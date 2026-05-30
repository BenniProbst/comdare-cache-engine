#pragma once
// V41 Umstufung-A s4 (Task #43) — StartTrieNodePoolStore: Multibyte-Span-Radix-Substrat (erfuellt StartTrieNodePool).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Such-Logik — die START-Korrektheits-Basis (Fent et al. ICDEW 2020, is_original=false
// [[pseudocode-papers-fallback]]): Leaf (Key+Value inline) + Inner-Knoten mit per-Node SPAN (1/2/3 Bytes
// Diskriminator-Breite) + SPARSE Diskriminator->Kind-Dispatch (sortierte Paare, Binaersuche) + ByteWiseKeyPrefix-
// Path-Compression (axis_02). Der span-1-Fall degeneriert zu ART (Korrektheits-Anker); span-2/3 sind ARTs
// CHARAKTERISTISCHE START-Erweiterung (Rewired64K/16M als portable C++23-Sparse-Dispatch statt Linux-Page-
// Rewiring). Der span-aware Descent lebt im StartTrieTraversalOrgan, NICHT hier.
//
// **Vereinfachung ggue. ART (bewusst, S1):** EINE sparse-sortierte Kind-Liste je Inner-Knoten (statt ARTs
// adaptiver Node4/16/48/256) — die adaptiven Knoten-TYPEN sind eine axis_04-Dimension, die START mit ART
// teilt (Folge-Refinement). Die START-DISTINKTION ist der Multibyte-Span. Cost-DP-Span-Wahl = axis_03t (Folge).

#include "start_trie_node_pool_concept.hpp"
#include "../../../nodes/axis_02_path_compression/axis_02_path_compression_byte_wise.hpp"  // ByteWiseKeyPrefix

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable {

class StartTrieNodePoolStore {
public:
    using key_type    = std::uint64_t;
    using value_type  = std::uint64_t;
    using prefix_type = ::comdare::cache_engine::nodes::axis_02_path_compression::ByteWiseKeyPrefix;
    static constexpr std::size_t kNil = std::numeric_limits<std::size_t>::max();

    enum Kind : std::uint8_t { kLeaf = 0, kInner = 1 };
    [[nodiscard]] static constexpr std::size_t make_ref(Kind kind, std::size_t idx) noexcept {
        return (static_cast<std::size_t>(kind) << 56U) | idx;
    }
    [[nodiscard]] static constexpr Kind        ref_kind(std::size_t r) noexcept { return static_cast<Kind>(r >> 56U); }
    [[nodiscard]] static constexpr std::size_t ref_idx(std::size_t r)  noexcept { return r & 0x00FF'FFFF'FFFF'FFFFULL; }

    // ── Wurzel + Groesse ──
    [[nodiscard]] std::size_t root() const noexcept { return root_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    void set_root(std::size_t r) noexcept { root_ = r; }
    void inc_size()              noexcept { ++size_; }
    void dec_size()              noexcept { --size_; }
    void clear() noexcept {
        leaves_.clear(); inners_.clear(); fl_leaf_.clear(); fl_inner_.clear();
        root_ = kNil; size_ = 0;
    }

    // ── Leaf ──
    [[nodiscard]] bool       is_leaf(std::size_t r)    const noexcept { return ref_kind(r) == kLeaf; }
    [[nodiscard]] key_type   leaf_key(std::size_t r)   const noexcept { return leaves_[ref_idx(r)].key; }
    [[nodiscard]] value_type leaf_value(std::size_t r) const noexcept { return leaves_[ref_idx(r)].val; }
    void set_leaf_value(std::size_t r, value_type v)   noexcept { leaves_[ref_idx(r)].val = v; }
    [[nodiscard]] std::size_t new_leaf(key_type k, value_type v) {
        std::size_t idx;
        if (!fl_leaf_.empty()) { idx = fl_leaf_.back(); fl_leaf_.pop_back(); leaves_[idx] = Leaf{k, v}; }
        else { idx = leaves_.size(); leaves_.push_back(Leaf{k, v}); }
        return make_ref(kLeaf, idx);
    }

    // ── Inner (Multibyte-Span) ──
    [[nodiscard]] std::size_t new_inner(unsigned sp) {
        std::size_t idx;
        if (!fl_inner_.empty()) { idx = fl_inner_.back(); fl_inner_.pop_back(); inners_[idx] = Inner{}; }
        else { idx = inners_.size(); inners_.push_back(Inner{}); }
        inners_[idx].span = static_cast<std::uint8_t>(sp);
        return make_ref(kInner, idx);
    }
    [[nodiscard]] unsigned    span(std::size_t r)      const noexcept { return inners_[ref_idx(r)].span; }
    [[nodiscard]] prefix_type prefix_of(std::size_t r) const noexcept { return inners_[ref_idx(r)].prefix; }
    void set_prefix(std::size_t r, prefix_type p)      noexcept { inners_[ref_idx(r)].prefix = p; }
    void prefix_cut(std::size_t r, unsigned n)         noexcept { inners_[ref_idx(r)].prefix.cut(n); }

    [[nodiscard]] std::size_t find_child(std::size_t r, std::uint32_t disc) const noexcept {
        Inner const& x = inners_[ref_idx(r)];
        auto it = std::lower_bound(x.disc.begin(), x.disc.end(), disc);
        if (it != x.disc.end() && *it == disc) return x.kids[static_cast<std::size_t>(it - x.disc.begin())];
        return kNil;
    }
    /// Fuegt (disc, child) sortiert ein (disc ist garantiert NICHT vorhanden) — darf via vector werfen.
    void add_child(std::size_t r, std::uint32_t disc, std::size_t child) {
        Inner& x = inners_[ref_idx(r)];
        auto it = std::lower_bound(x.disc.begin(), x.disc.end(), disc);
        std::size_t const pos = static_cast<std::size_t>(it - x.disc.begin());
        x.disc.insert(x.disc.begin() + static_cast<std::ptrdiff_t>(pos), disc);
        x.kids.insert(x.kids.begin() + static_cast<std::ptrdiff_t>(pos), child);
    }
    void set_child(std::size_t r, std::uint32_t disc, std::size_t child) noexcept {
        Inner& x = inners_[ref_idx(r)];
        auto it = std::lower_bound(x.disc.begin(), x.disc.end(), disc);
        if (it != x.disc.end() && *it == disc) x.kids[static_cast<std::size_t>(it - x.disc.begin())] = child;
    }
    void remove_child(std::size_t r, std::uint32_t disc) noexcept {
        Inner& x = inners_[ref_idx(r)];
        auto it = std::lower_bound(x.disc.begin(), x.disc.end(), disc);
        if (it != x.disc.end() && *it == disc) {
            std::size_t const pos = static_cast<std::size_t>(it - x.disc.begin());
            x.disc.erase(x.disc.begin() + static_cast<std::ptrdiff_t>(pos));
            x.kids.erase(x.kids.begin() + static_cast<std::ptrdiff_t>(pos));
        }
    }

    void free_node(std::size_t r) noexcept {
        if (ref_kind(r) == kLeaf) fl_leaf_.push_back(ref_idx(r));
        else                      fl_inner_.push_back(ref_idx(r));
    }

private:
    struct Leaf  { key_type key{}; value_type val{}; };
    struct Inner {
        prefix_type                prefix{};
        std::uint8_t               span = 1;
        std::vector<std::uint32_t> disc{};   // aufsteigend sortierte Diskriminatoren (span-breit)
        std::vector<std::size_t>   kids{};   // parallel: Kind-Refs
    };

    std::vector<Leaf>        leaves_{};
    std::vector<Inner>       inners_{};
    std::vector<std::size_t> fl_leaf_{}, fl_inner_{};
    std::size_t root_ = kNil;
    std::size_t size_ = 0;
};

// Selbstbeweis: das Substrat erfuellt das StartTrieNodePool-Concept.
static_assert(StartTrieNodePool<StartTrieNodePoolStore>);

}  // namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable
