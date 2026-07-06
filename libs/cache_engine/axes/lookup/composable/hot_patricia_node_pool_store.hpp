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
#include <memory>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

namespace detail {

struct HotPatriciaLeaf {
    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;

    key_type   key{};
    value_type val{};
};

struct HotPatriciaInternal {
    std::uint8_t               crit_bit{};
    std::array<std::size_t, 2> child{};
}; // crit_bit 0..63 passt in uint8

} // namespace detail

template <class A = std::allocator<detail::HotPatriciaLeaf>>
class HotPatriciaNodePoolStore {
public:
    using node_type           = detail::HotPatriciaLeaf;
    using key_type            = typename node_type::key_type;
    using value_type          = typename node_type::value_type;
    using allocator_type      = A;
    using leaf_allocator_type = typename std::allocator_traits<A>::template rebind_alloc<detail::HotPatriciaLeaf>;
    using internal_allocator_type =
        typename std::allocator_traits<A>::template rebind_alloc<detail::HotPatriciaInternal>;
    using index_allocator_type        = typename std::allocator_traits<A>::template rebind_alloc<std::size_t>;
    static constexpr std::size_t kNil = std::numeric_limits<std::size_t>::max();

    enum Kind : std::uint8_t { kLeaf = 0, kInternal = 1 };
    [[nodiscard]] static constexpr std::size_t make_ref(Kind kind, std::size_t idx) noexcept {
        return (static_cast<std::size_t>(kind) << 56U) | idx;
    }
    [[nodiscard]] static constexpr Kind        ref_kind(std::size_t r) noexcept { return static_cast<Kind>(r >> 56U); }
    [[nodiscard]] static constexpr std::size_t ref_idx(std::size_t r) noexcept { return r & 0x00FF'FFFF'FFFF'FFFFULL; }

    // ── Wurzel + Groesse ──
    [[nodiscard]] std::size_t root() const noexcept { return root_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    void                      set_root(std::size_t r) noexcept { root_ = r; }
    void                      inc_size() noexcept { ++size_; }
    void                      dec_size() noexcept { --size_; }
    void                      clear() noexcept {
        leaves_.clear();
        internals_.clear();
        fl_leaf_.clear();
        fl_internal_.clear();
        root_ = kNil;
        size_ = 0;
    }

    // ── Leaf ──
    [[nodiscard]] bool        is_leaf(std::size_t r) const noexcept { return ref_kind(r) == kLeaf; }
    [[nodiscard]] key_type    leaf_key(std::size_t r) const noexcept { return leaves_[ref_idx(r)].key; }
    [[nodiscard]] value_type  leaf_value(std::size_t r) const noexcept { return leaves_[ref_idx(r)].val; }
    void                      set_leaf_value(std::size_t r, value_type v) noexcept { leaves_[ref_idx(r)].val = v; }
    [[nodiscard]] std::size_t new_leaf(key_type k, value_type v) {
        std::size_t idx;
        if (!fl_leaf_.empty()) {
            idx = fl_leaf_.back();
            fl_leaf_.pop_back();
            leaves_[idx] = node_type{k, v};
        } else {
            idx                            = leaves_.size();
            std::size_t const old_capacity = leaves_.capacity();
            leaves_.push_back(node_type{k, v});
            record_capacity_growth_(old_capacity, leaves_.capacity(), sizeof(node_type));
        }
        return make_ref(kLeaf, idx);
    }

    // ── Internal (crit-bit) ──
    [[nodiscard]] unsigned    crit_bit(std::size_t r) const noexcept { return internals_[ref_idx(r)].crit_bit; }
    [[nodiscard]] std::size_t child(std::size_t r, unsigned bit) const noexcept {
        return internals_[ref_idx(r)].child[bit & 1U];
    }
    void set_child(std::size_t r, unsigned bit, std::size_t c) noexcept { internals_[ref_idx(r)].child[bit & 1U] = c; }
    [[nodiscard]] std::size_t new_internal(unsigned crit_bit, std::size_t c0, std::size_t c1) {
        std::size_t idx;
        if (!fl_internal_.empty()) {
            idx = fl_internal_.back();
            fl_internal_.pop_back();
            internals_[idx] = detail::HotPatriciaInternal{};
        } else {
            idx                            = internals_.size();
            std::size_t const old_capacity = internals_.capacity();
            internals_.push_back(detail::HotPatriciaInternal{});
            record_capacity_growth_(old_capacity, internals_.capacity(), sizeof(detail::HotPatriciaInternal));
        }
        detail::HotPatriciaInternal& x = internals_[idx];
        x.crit_bit                     = static_cast<std::uint8_t>(crit_bit);
        x.child[0]                     = c0;
        x.child[1]                     = c1;
        return make_ref(kInternal, idx);
    }
    void free_node(std::size_t r) noexcept {
        if (ref_kind(r) == kLeaf) {
            std::size_t const old_capacity = fl_leaf_.capacity();
            fl_leaf_.push_back(ref_idx(r));
            record_capacity_growth_(old_capacity, fl_leaf_.capacity(), sizeof(std::size_t));
        } else {
            std::size_t const old_capacity = fl_internal_.capacity();
            fl_internal_.push_back(ref_idx(r));
            record_capacity_growth_(old_capacity, fl_internal_.capacity(), sizeof(std::size_t));
        }
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    struct allocator_statistics_snapshot {
        std::uint64_t alloc_calls     = 0;
        std::uint64_t bytes_allocated = 0;
        std::uint64_t live_nodes      = 0;
    };

    [[nodiscard]] allocator_statistics_snapshot store_allocator_statistics() const noexcept {
        return allocator_statistics_snapshot{
            alloc_calls_,
            bytes_allocated_,
            static_cast<std::uint64_t>((leaves_.size() + internals_.size()) - (fl_leaf_.size() + fl_internal_.size())),
        };
    }
#endif

private:
    using Leaf     = detail::HotPatriciaLeaf;
    using Internal = detail::HotPatriciaInternal;

#ifdef COMDARE_CE_ENABLE_STATISTICS
    // Ehrliche Allokator-Metrik: gezaehlt werden nur erfolgreiche vector-capacity-Zuwaechse, als Capacity-Delta
    // mal Elementgroesse. Reuse/clear ohne Capacity-Wachstum erzeugt bewusst keine kuenstlichen Werte.
    void record_capacity_growth_(std::size_t old_capacity, std::size_t new_capacity, std::size_t elem_bytes) noexcept {
        if (new_capacity <= old_capacity) return;
        ++alloc_calls_;
        bytes_allocated_ +=
            static_cast<std::uint64_t>(new_capacity - old_capacity) * static_cast<std::uint64_t>(elem_bytes);
    }
#else
    static void record_capacity_growth_(std::size_t, std::size_t, std::size_t) noexcept {}
#endif

    std::vector<Leaf, leaf_allocator_type>         leaves_{};
    std::vector<Internal, internal_allocator_type> internals_{};
    std::vector<std::size_t, index_allocator_type> fl_leaf_{};
    std::vector<std::size_t, index_allocator_type> fl_internal_{};
    std::size_t                                    root_ = kNil;
    std::size_t                                    size_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    std::uint64_t alloc_calls_     = 0;
    std::uint64_t bytes_allocated_ = 0;
#endif
};

// Selbstbeweis: das Substrat erfuellt das HotPatriciaNodePool-Concept.
static_assert(HotPatriciaNodePool<HotPatriciaNodePoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
