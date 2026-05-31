#pragma once
// V41 Umstufung-A s4 (Task #43) — HotPatriciaNodePoolStore: binary crit-bit-Substrat (erfuellt HotPatriciaNodePool).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Such-Logik — die HOT-Korrektheits-Basis (Binna et al. SIGMOD 2018), is_original=false
// ([[pseudocode-papers-fallback]]): Leaf (Key+Value inline) + Internal{crit_bit (0..63), child[2]}. Nur 2
// Kind-Typen (kein adaptives Growth wie ART — Single-Bit-Split erzeugt stets binaere Internals). NodeRef =
// std::size_t (Kind in Bits 56-63, Index in Bits 0-55), je Kind ein Vektor + Free-List (Index-Stabilitaet via
// Recycling). Der crit-bit-Descent + Leaf-Split + Erase-Collapse lebt im HotPatriciaTraversalOrgan, NICHT hier.

#include "hot_patricia_node_pool_concept.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

class HotPatriciaNodePoolStore {
public:
    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;
    static constexpr std::size_t kNil = std::numeric_limits<std::size_t>::max();

    enum Kind : std::uint8_t { kLeaf = 0, kInternal = 1 };
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
        leaves_.clear(); internals_.clear();
        fl_leaf_.clear(); fl_internal_.clear();
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

    // ── Internal (crit-bit) ──
    [[nodiscard]] unsigned    crit_bit(std::size_t r)            const noexcept { return internals_[ref_idx(r)].crit_bit; }
    [[nodiscard]] std::size_t child(std::size_t r, unsigned bit) const noexcept { return internals_[ref_idx(r)].child[bit & 1U]; }
    void set_child(std::size_t r, unsigned bit, std::size_t c)   noexcept { internals_[ref_idx(r)].child[bit & 1U] = c; }
    [[nodiscard]] std::size_t new_internal(unsigned crit_bit, std::size_t c0, std::size_t c1) {
        std::size_t idx;
        if (!fl_internal_.empty()) { idx = fl_internal_.back(); fl_internal_.pop_back(); internals_[idx] = Internal{}; }
        else { idx = internals_.size(); internals_.push_back(Internal{}); }
        Internal& x = internals_[idx];
        x.crit_bit = static_cast<std::uint8_t>(crit_bit);
        x.child[0] = c0; x.child[1] = c1;
        return make_ref(kInternal, idx);
    }
    void free_node(std::size_t r) noexcept {
        if (ref_kind(r) == kLeaf) fl_leaf_.push_back(ref_idx(r));
        else                      fl_internal_.push_back(ref_idx(r));
    }

private:
    struct Leaf     { key_type key{}; value_type val{}; };
    struct Internal { std::uint8_t crit_bit{}; std::array<std::size_t, 2> child{}; };  // crit_bit 0..63 passt in uint8

    std::vector<Leaf>        leaves_{};
    std::vector<Internal>    internals_{};
    std::vector<std::size_t> fl_leaf_{}, fl_internal_{};
    std::size_t root_ = kNil;
    std::size_t size_ = 0;
};

// Selbstbeweis: das Substrat erfuellt das HotPatriciaNodePool-Concept.
static_assert(HotPatriciaNodePool<HotPatriciaNodePoolStore>);

}  // namespace comdare::cache_engine::lookup::composable
