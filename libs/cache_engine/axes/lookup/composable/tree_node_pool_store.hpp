#pragma once
// V41 Roadmap-2 INC-2b — TreeNodePoolStore: index-stabiles Knoten-Pool-Substrat (erfuellt TreeNodePool).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Such-Logik (1:1-Struktur nach dem bewaehrten axis_03a_search_algo_bst.hpp:
// std::vector<Node> + Free-List + root_), aber generisch ueber uint64-Key und mit getrennter
// Verantwortung: die Baum-NAVIGATION lebt im Tree-Traversal-Organ, NICHT hier (genetisches Experiment).

#include "tree_node_pool_concept.hpp"
#include <topics/nodes/axis_bst_shape/axis_bst_shape_ptr_size_t.hpp>
#include <topics/nodes/axis_bst_shape/concepts/axis_bst_shape_concept.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

namespace detail {

template <typename Shape>
struct TreeNodePoolNode {
    using index_type                      = typename Shape::index_type;
    static constexpr index_type kNilIndex = std::numeric_limits<index_type>::max();

    std::uint64_t key{};
    std::uint64_t val{};
    index_type    left{kNilIndex};
    index_type    right{kNilIndex};
};

} // namespace detail

/// Index-stabiler Knoten-Pool: Knoten behalten ihren Index ueber ihre Lebensdauer (Free-List-Recycling).
template <typename Shape = ::comdare::cache_engine::nodes::axis_bst_shape::BstPtrSizeT,
          class A        = std::allocator<detail::TreeNodePoolNode<Shape>>>
class TreeNodePoolStore {
    static_assert(::comdare::cache_engine::nodes::axis_bst_shape::concepts::BstShape<Shape>);

public:
    using key_type                         = std::uint64_t;
    using value_type                       = std::uint64_t;
    using index_type                       = typename Shape::index_type;
    using node_type                        = detail::TreeNodePoolNode<Shape>;
    using allocator_type                   = A;
    static constexpr index_type  kNilIndex = std::numeric_limits<index_type>::max();
    static constexpr std::size_t kNil      = static_cast<std::size_t>(kNilIndex); // Level-0: size_t max (#234-K)

    [[nodiscard]] std::size_t root() const noexcept { return static_cast<std::size_t>(root_); }
    [[nodiscard]] std::size_t node_count() const noexcept { return size_; }
    [[nodiscard]] key_type    node_key(std::size_t i) const noexcept { return nodes_[i].key; }
    [[nodiscard]] value_type  node_value(std::size_t i) const noexcept { return nodes_[i].val; }
    [[nodiscard]] std::size_t left(std::size_t i) const noexcept { return static_cast<std::size_t>(nodes_[i].left); }
    [[nodiscard]] std::size_t right(std::size_t i) const noexcept { return static_cast<std::size_t>(nodes_[i].right); }

    void set_node_key(std::size_t i, key_type k) noexcept { nodes_[i].key = k; }
    void set_node_value(std::size_t i, value_type v) noexcept { nodes_[i].val = v; }
    void set_left(std::size_t i, std::size_t c) noexcept { nodes_[i].left = static_cast<index_type>(c); }
    void set_right(std::size_t i, std::size_t c) noexcept { nodes_[i].right = static_cast<index_type>(c); }
    void set_root(std::size_t i) noexcept { root_ = static_cast<index_type>(i); }

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

    /// Allokiert einen Knoten (Free-List-Recycling oder Anhang) — darf via vector werfen (kein noexcept).
    std::size_t allocate_node(key_type k, value_type v) {
        std::size_t idx;
        if (!free_.empty()) {
            idx = free_.back();
            free_.pop_back();
            nodes_[idx] = node_type{k, v, kNilIndex, kNilIndex};
        } else {
            idx = nodes_.size();
            // #234-F3 [[allocation-failure-exception]]: der Sentinel kNilIndex ist NIE ein gueltiger Knoten-Index —
            // bei schmalem index_type (U16: max 65535 Knoten, Indizes 0..65534; 65535 = kNilIndex-Sentinel) ist Ueberlauf ein harter Fehler, kein stilles Wrappen.
            if (idx >= static_cast<std::size_t>(kNilIndex))
                throw std::length_error("TreeNodePoolStore: index_type-Kapazitaet erschoepft (#234-F3)");
            std::size_t const old_capacity = nodes_.capacity();
            nodes_.push_back(node_type{k, v, kNilIndex, kNilIndex});
            record_capacity_growth_(old_capacity, nodes_.capacity(), sizeof(node_type));
        }
        ++size_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++live_nodes_;
#endif
        return idx;
    }
    /// Contract (Substrat/Organ-Vertrauensgrenze, wie alle Setter): i MUSS ein aktuell lebendiger, genau einmal
    /// freigegebener Knoten-Index sein — das Traversal-Organ garantiert das; der Store validiert nicht (#234-F3).
    void free_node(std::size_t i) noexcept {
        std::size_t const old_capacity = free_.capacity();
        free_.push_back(i);
        record_capacity_growth_(old_capacity, free_.capacity(), sizeof(std::size_t));
        --size_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        --live_nodes_;
#endif
    }
    void clear() noexcept {
        nodes_.clear();
        free_.clear();
        root_ = kNilIndex;
        size_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        live_nodes_ = 0;
#endif
    }

private:
    using Node                = node_type;
    using node_allocator_type = typename std::allocator_traits<A>::template rebind_alloc<Node>;
    using free_allocator_type = typename std::allocator_traits<A>::template rebind_alloc<std::size_t>;

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

    // sizeof(Node) je Packing: size_t 32 B; u32 24 B; u16 20->24 B mit Padding, alignof(u64)-bedingt;
    // U16-Gewinn liegt im Index-Wertebereich, nicht im sizeof.
    std::vector<Node, node_allocator_type>        nodes_{};
    std::vector<std::size_t, free_allocator_type> free_{};
    index_type                                    root_ = kNilIndex;
    std::size_t                                   size_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    std::uint64_t alloc_calls_     = 0;
    std::uint64_t bytes_allocated_ = 0;
    std::uint64_t live_nodes_      = 0;
#endif
};

// Selbstbeweis: das Substrat erfuellt das TreeNodePool-Concept.
static_assert(TreeNodePool<TreeNodePoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
