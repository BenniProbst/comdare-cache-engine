#pragma once
// V41 Umstufung-A (Task #41) — BTreeNodePoolStore: Mehrwege-Knoten-Substrat (erfuellt BTreeNodePool).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Such-Logik — 1:1-Port der Knoten-Struktur aus axis_03a_search_algo_btree.hpp
// (struct alignas(64) Node{n; leaf; key[7]; val[7]; child[8]}; nodes_; free_-List; root_=kNil; size_;
// new_node mit child.fill(kNil)), aber generisch ueber uint64-Key. Split/Merge/Borrow leben im
// BTreeTraversalOrgan, NICHT hier (Transformations-Organe = Traversal, Substrat = nur Slot-Verwaltung).
//
// **alignas(64) BLEIBT erhalten** — der block-orientierte Knoten ist das messbare B-Baum-Merkmal
// (has_cache_line_alignment, Doku 14 §13, F15). Kind-Indizes intern als uint32 (block-orientierte
// Speichercharakteristik), nach aussen als std::size_t (Pool-API). size_ = logische Schluesselzahl,
// vom Organ ueber inc_size/dec_size gefuehrt (B-Baum-Knoten != Schluessel).
//
// Phase 0.3a (Hebel B, Doc 21 §F): der Knoten-/Free-List-Speicher kommt REAL aus der Allocator-Achse (axis_06),
// analog TreeNodePoolStore (BST). store_allocator_statistics() liefert die Strategie-Statistik -> T6 reflektiert
// den ECHTEN Allocator. Default ExgenAllocator (real=std bei disabled). COW-Sicherheit via Memento (Copy-Ctor/
// Assign rebinden den StdAllocatorAdapter an das eigene allocator_ + verwerfen die COW-Kopier-Pollution per
// restore_statistics; Move nicht deklariert -> degradiert zu Copy). Siehe tree_node_pool_store.hpp fuer Details.

#include "btree_node_pool_concept.hpp"
#include <axes/alloc/axis_06_allocator_exgen.hpp>
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <topics/nodes/axis_btree_order/axis_btree_order_kt4.hpp>
#include <topics/nodes/axis_btree_order/concepts/axis_btree_order_concept.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

namespace detail {

template <typename Shape>
struct alignas(64) BTreeNodePoolNode {
    using key_type                              = std::uint64_t;
    using value_type                            = std::uint64_t;
    static constexpr int           kMaxKeys     = Shape::kMaxKeys;
    static constexpr int           kMaxChildren = Shape::kMaxChildren;
    static constexpr std::uint32_t kNilU32      = 0xFFFFFFFFu;

    std::int16_t                            n    = 0; // belegte Schluessel
    bool                                    leaf = true;
    std::array<key_type, kMaxKeys>          key{};
    std::array<value_type, kMaxKeys>        val{};
    std::array<std::uint32_t, kMaxChildren> child{}; // in new_node auf kNil gesetzt
};

} // namespace detail

/// Index-stabiler Mehrwege-Knoten-Pool (Free-List-Recycling); block-orientiert (alignas(64)).
template <typename Shape = ::comdare::cache_engine::nodes::axis_btree_order::BtreeOrderKt4,
          class Alloc    = ::comdare::cache_engine::alloc::ExgenAllocator>
    requires ::comdare::cache_engine::alloc::concepts::AllocatorStrategy<Alloc>
class BTreeNodePoolStore {
    static_assert(::comdare::cache_engine::nodes::axis_btree_order::concepts::BtreeOrderShape<Shape>);

public:
    using key_type                            = std::uint64_t;
    using value_type                          = std::uint64_t;
    using node_type                           = detail::BTreeNodePoolNode<Shape>;
    using allocator_type                      = Alloc;
    static constexpr int         kT           = Shape::kT;  // Minimum-Degree (#234-K shape carrier)
    static constexpr int         kMaxKeys     = 2 * kT - 1; // Level-0: 7
    static constexpr int         kMaxChildren = 2 * kT;     // Level-0: 8
    static constexpr std::size_t kNil         = 0xFFFFFFFFu;

    static_assert(kMaxKeys == Shape::kMaxKeys);
    static_assert(kMaxChildren == Shape::kMaxChildren);

private:
    using Node       = node_type;
    using node_alloc = typename Alloc::template StdAllocatorAdapter<node_type>;
    using free_alloc = typename Alloc::template StdAllocatorAdapter<std::size_t>;

    static constexpr std::uint32_t kNilU32 = node_type::kNilU32;

public:
    // Phase 0.3a (analog BST): die Vektoren allokieren real ueber die axis_06-Strategie; Copy-Ctor/Assign rebinden
    // den Adapter an das eigene allocator_ und verwerfen die COW-Kopier-Pollution per Memento-restore_statistics.
    BTreeNodePoolStore()
        : nodes_(allocator_.template as_std_allocator<node_type>()),
          free_(allocator_.template as_std_allocator<std::size_t>()) {}
    BTreeNodePoolStore(BTreeNodePoolStore const& o)
        : allocator_(o.allocator_), nodes_(o.nodes_, allocator_.template as_std_allocator<node_type>()),
          free_(o.free_, allocator_.template as_std_allocator<std::size_t>()), root_(o.root_), size_(o.size_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    BTreeNodePoolStore& operator=(BTreeNodePoolStore const& o) {
        if (this != &o) {
            nodes_ = o.nodes_;
            free_  = o.free_;
            root_  = o.root_;
            size_  = o.size_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~BTreeNodePoolStore() = default;

    [[nodiscard]] std::size_t root() const noexcept { return root_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    // Pool-native LEBENDE Knotenzahl (allokiert minus freigegeben) — growth-bucket-UNABHAENGIG (Phase 0.3a:
    // ersetzt den entfallenen live_nodes_-Zaehler; die Allocator-Stats zaehlen jetzt echte vector-Reallokationen).
    [[nodiscard]] std::size_t pool_node_count() const noexcept { return nodes_.size() - free_.size(); }
    [[nodiscard]] int         node_n(std::size_t i) const noexcept { return static_cast<int>(nodes_[i].n); }
    [[nodiscard]] bool        node_leaf(std::size_t i) const noexcept { return nodes_[i].leaf; }
    [[nodiscard]] key_type    node_key_at(std::size_t i, int j) const noexcept {
        return nodes_[i].key[static_cast<std::size_t>(j)];
    }
    [[nodiscard]] value_type node_value_at(std::size_t i, int j) const noexcept {
        return nodes_[i].val[static_cast<std::size_t>(j)];
    }
    [[nodiscard]] std::size_t node_child_at(std::size_t i, int j) const noexcept {
        return static_cast<std::size_t>(nodes_[i].child[static_cast<std::size_t>(j)]);
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using allocator_snapshot_t = typename Alloc::snapshot_t;
    /// T6-Route (Phase 0.3a): die ECHTE Allocator-Achsen-Statistik (rich AllocationStatistics, 5 Felder).
    [[nodiscard]] allocator_snapshot_t store_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

    /// SONDERFALL [[allocation-failure-exception]]: nodes_-Wachstum kann std::bad_alloc werfen.
    [[nodiscard]] std::size_t new_node(bool leaf) {
        std::size_t idx;
        if (!free_.empty()) {
            idx = free_.back();
            free_.pop_back();
            nodes_[idx] = Node{};
        } else {
            nodes_.push_back(Node{});
            idx = nodes_.size() - 1;
        }
        nodes_[idx].leaf = leaf;
        nodes_[idx].child.fill(kNilU32);
        return idx;
    }
    // (F57/Muster B, WP-5 2026-07-16): NICHT noexcept — free_.push_back kann beim Free-List-Wachstum
    // allozieren/werfen ([[allocation-failure-exception]]: werfen statt terminate; Concept verlangt kein noexcept).
    void free_node(std::size_t i) { free_.push_back(i); }
    void set_root(std::size_t i) noexcept { root_ = i; }
    void set_node_n(std::size_t i, int n) noexcept { nodes_[i].n = static_cast<std::int16_t>(n); }
    void set_node_leaf(std::size_t i, bool b) noexcept { nodes_[i].leaf = b; }
    void set_node_key_at(std::size_t i, int j, key_type k) noexcept { nodes_[i].key[static_cast<std::size_t>(j)] = k; }
    void set_node_value_at(std::size_t i, int j, value_type v) noexcept {
        nodes_[i].val[static_cast<std::size_t>(j)] = v;
    }
    void set_node_child_at(std::size_t i, int j, std::size_t c) noexcept {
        nodes_[i].child[static_cast<std::size_t>(j)] = static_cast<std::uint32_t>(c);
    }
    void inc_size() noexcept { ++size_; }
    void dec_size() noexcept { --size_; }

    void clear() noexcept {
        nodes_.clear();
        free_.clear();
        root_ = kNil;
        size_ = 0;
    }

private:
    // alignas(64): block-orientierter Knoten => has_cache_line_alignment (B-Baum-Merkmal, F15).
    // allocator_ VOR den Vektoren (Adapter haelt &allocator_) — Phase 0.3a, analog BST.
    Alloc                                allocator_{};
    std::vector<Node, node_alloc>        nodes_;
    std::vector<std::size_t, free_alloc> free_;
    std::size_t                          root_ = kNil;
    std::size_t                          size_ = 0;
};

// Selbstbeweis: das Substrat erfuellt das BTreeNodePool-Concept.
static_assert(BTreeNodePool<BTreeNodePoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
