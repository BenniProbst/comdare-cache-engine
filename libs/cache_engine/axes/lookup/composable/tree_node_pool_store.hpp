#pragma once
// V41 Roadmap-2 INC-2b — TreeNodePoolStore: index-stabiles Knoten-Pool-Substrat (erfuellt TreeNodePool).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Such-Logik (1:1-Struktur nach dem bewaehrten axis_03a_search_algo_bst.hpp:
// std::vector<Node> + Free-List + root_), aber generisch ueber uint64-Key und mit getrennter
// Verantwortung: die Baum-NAVIGATION lebt im Tree-Traversal-Organ, NICHT hier (genetisches Experiment).

#include "tree_node_pool_concept.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

/// Index-stabiler Knoten-Pool: Knoten behalten ihren Index ueber ihre Lebensdauer (Free-List-Recycling).
class TreeNodePoolStore {
public:
    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;
    static constexpr std::size_t kNil = std::numeric_limits<std::size_t>::max();   // "kein Knoten"

    [[nodiscard]] std::size_t root()        const noexcept { return root_; }
    [[nodiscard]] std::size_t node_count()  const noexcept { return size_; }
    [[nodiscard]] key_type    node_key(std::size_t i)   const noexcept { return nodes_[i].key; }
    [[nodiscard]] value_type  node_value(std::size_t i) const noexcept { return nodes_[i].val; }
    [[nodiscard]] std::size_t left(std::size_t i)       const noexcept { return nodes_[i].left; }
    [[nodiscard]] std::size_t right(std::size_t i)      const noexcept { return nodes_[i].right; }

    void set_node_key(std::size_t i, key_type k)     noexcept { nodes_[i].key = k; }
    void set_node_value(std::size_t i, value_type v) noexcept { nodes_[i].val = v; }
    void set_left(std::size_t i, std::size_t c)      noexcept { nodes_[i].left = c; }
    void set_right(std::size_t i, std::size_t c)     noexcept { nodes_[i].right = c; }
    void set_root(std::size_t i)                     noexcept { root_ = i; }

    /// Allokiert einen Knoten (Free-List-Recycling oder Anhang) — darf via vector werfen (kein noexcept).
    std::size_t allocate_node(key_type k, value_type v) {
        std::size_t idx;
        if (!free_.empty()) {
            idx = free_.back();
            free_.pop_back();
            nodes_[idx] = Node{k, v, kNil, kNil};
        } else {
            idx = nodes_.size();
            nodes_.push_back(Node{k, v, kNil, kNil});
        }
        ++size_;
        return idx;
    }
    void free_node(std::size_t i) noexcept {
        free_.push_back(i);
        --size_;
    }
    void clear() noexcept {
        nodes_.clear();
        free_.clear();
        root_ = kNil;
        size_ = 0;
    }

private:
    struct Node {
        key_type    key{};
        value_type  val{};
        std::size_t left{kNil};
        std::size_t right{kNil};
    };
    std::vector<Node>        nodes_{};
    std::vector<std::size_t> free_{};
    std::size_t              root_ = kNil;
    std::size_t              size_ = 0;
};

// Selbstbeweis: das Substrat erfuellt das TreeNodePool-Concept.
static_assert(TreeNodePool<TreeNodePoolStore>);

}  // namespace comdare::cache_engine::lookup::composable
