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

#include "btree_node_pool_concept.hpp"
#include <topics/nodes/axis_btree_order/axis_btree_order_kt4.hpp>
#include <topics/nodes/axis_btree_order/concepts/axis_btree_order_concept.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
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
          class A        = std::allocator<detail::BTreeNodePoolNode<Shape>>>
class BTreeNodePoolStore {
    static_assert(::comdare::cache_engine::nodes::axis_btree_order::concepts::BtreeOrderShape<Shape>);

public:
    using key_type                            = std::uint64_t;
    using value_type                          = std::uint64_t;
    using node_type                           = detail::BTreeNodePoolNode<Shape>;
    using allocator_type                      = A;
    static constexpr int         kT           = Shape::kT;  // Minimum-Degree (#234-K shape carrier)
    static constexpr int         kMaxKeys     = 2 * kT - 1; // Level-0: 7
    static constexpr int         kMaxChildren = 2 * kT;     // Level-0: 8
    static constexpr std::size_t kNil         = 0xFFFFFFFFu;

    static_assert(kMaxKeys == Shape::kMaxKeys);
    static_assert(kMaxChildren == Shape::kMaxChildren);

    [[nodiscard]] std::size_t root() const noexcept { return root_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
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
    struct allocator_statistics_snapshot {
        std::uint64_t alloc_calls     = 0;
        std::uint64_t bytes_allocated = 0;
        std::uint64_t live_nodes      = 0;
    };

    [[nodiscard]] allocator_statistics_snapshot store_allocator_statistics() const noexcept {
        return allocator_statistics_snapshot{
            alloc_calls_,
            bytes_allocated_,
            live_nodes_,
        };
    }
#endif

    /// SONDERFALL [[allocation-failure-exception]]: nodes_-Wachstum kann std::bad_alloc werfen.
    [[nodiscard]] std::size_t new_node(bool leaf) {
        std::size_t idx;
        if (!free_.empty()) {
            idx = free_.back();
            free_.pop_back();
            nodes_[idx] = Node{};
        } else {
            std::size_t const old_capacity = nodes_.capacity();
            nodes_.push_back(Node{});
            record_capacity_growth_(old_capacity, nodes_.capacity(), sizeof(Node));
            idx = nodes_.size() - 1;
        }
        nodes_[idx].leaf = leaf;
        nodes_[idx].child.fill(kNilU32);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++live_nodes_;
#endif
        return idx;
    }
    void free_node(std::size_t i) noexcept {
        std::size_t const old_capacity = free_.capacity();
        free_.push_back(i);
        record_capacity_growth_(old_capacity, free_.capacity(), sizeof(std::size_t));
#ifdef COMDARE_CE_ENABLE_STATISTICS
        --live_nodes_;
#endif
    }
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
#ifdef COMDARE_CE_ENABLE_STATISTICS
        live_nodes_ = 0;
#endif
    }

private:
    using Node                = node_type;
    using node_allocator_type = typename std::allocator_traits<A>::template rebind_alloc<Node>;
    using free_allocator_type = typename std::allocator_traits<A>::template rebind_alloc<std::size_t>;

    static constexpr std::uint32_t kNilU32 = Node::kNilU32;

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

    // alignas(64): block-orientierter Knoten => has_cache_line_alignment (B-Baum-Merkmal, F15).
    std::vector<Node, node_allocator_type>        nodes_{};
    std::vector<std::size_t, free_allocator_type> free_{};
    std::size_t                                   root_ = kNil;
    std::size_t                                   size_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    std::uint64_t alloc_calls_     = 0;
    std::uint64_t bytes_allocated_ = 0;
    std::uint64_t live_nodes_      = 0;
#endif
};

// Selbstbeweis: das Substrat erfuellt das BTreeNodePool-Concept.
static_assert(BTreeNodePool<BTreeNodePoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
