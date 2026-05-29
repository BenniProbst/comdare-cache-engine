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

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable {

/// Index-stabiler Mehrwege-Knoten-Pool (Free-List-Recycling); block-orientiert (alignas(64)).
class BTreeNodePoolStore {
public:
    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;
    static constexpr int         kT           = 4;            // Minimum-Degree
    static constexpr int         kMaxKeys     = 2 * kT - 1;   // 7
    static constexpr int         kMaxChildren = 2 * kT;       // 8
    static constexpr std::size_t kNil         = 0xFFFFFFFFu;

    [[nodiscard]] std::size_t root() const noexcept { return root_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] int  node_n(std::size_t i)    const noexcept { return static_cast<int>(nodes_[i].n); }
    [[nodiscard]] bool node_leaf(std::size_t i) const noexcept { return nodes_[i].leaf; }
    [[nodiscard]] key_type   node_key_at(std::size_t i, int j)   const noexcept { return nodes_[i].key[static_cast<std::size_t>(j)]; }
    [[nodiscard]] value_type node_value_at(std::size_t i, int j) const noexcept { return nodes_[i].val[static_cast<std::size_t>(j)]; }
    [[nodiscard]] std::size_t node_child_at(std::size_t i, int j) const noexcept {
        return static_cast<std::size_t>(nodes_[i].child[static_cast<std::size_t>(j)]);
    }

    /// SONDERFALL [[allocation-failure-exception]]: nodes_-Wachstum kann std::bad_alloc werfen.
    [[nodiscard]] std::size_t new_node(bool leaf) {
        std::size_t idx;
        if (!free_.empty()) { idx = free_.back(); free_.pop_back(); nodes_[idx] = Node{}; }
        else { nodes_.push_back(Node{}); idx = nodes_.size() - 1; }
        nodes_[idx].leaf = leaf;
        nodes_[idx].child.fill(kNilU32);
        return idx;
    }
    void free_node(std::size_t i)               noexcept { free_.push_back(i); }
    void set_root(std::size_t i)                noexcept { root_ = i; }
    void set_node_n(std::size_t i, int n)       noexcept { nodes_[i].n = static_cast<std::int16_t>(n); }
    void set_node_leaf(std::size_t i, bool b)   noexcept { nodes_[i].leaf = b; }
    void set_node_key_at(std::size_t i, int j, key_type k)     noexcept { nodes_[i].key[static_cast<std::size_t>(j)] = k; }
    void set_node_value_at(std::size_t i, int j, value_type v) noexcept { nodes_[i].val[static_cast<std::size_t>(j)] = v; }
    void set_node_child_at(std::size_t i, int j, std::size_t c) noexcept {
        nodes_[i].child[static_cast<std::size_t>(j)] = static_cast<std::uint32_t>(c);
    }
    void inc_size() noexcept { ++size_; }
    void dec_size() noexcept { --size_; }

    void clear() noexcept { nodes_.clear(); free_.clear(); root_ = kNil; size_ = 0; }

private:
    static constexpr std::uint32_t kNilU32 = 0xFFFFFFFFu;

    // alignas(64): block-orientierter Knoten ⇒ has_cache_line_alignment (B-Baum-Merkmal, F15).
    struct alignas(64) Node {
        std::int16_t n = 0;          // belegte Schluessel
        bool         leaf = true;
        std::array<key_type,      kMaxKeys>     key{};
        std::array<value_type,    kMaxKeys>     val{};
        std::array<std::uint32_t, kMaxChildren> child{};   // in new_node auf kNil gesetzt
    };

    std::vector<Node>        nodes_{};
    std::vector<std::size_t> free_{};
    std::size_t              root_ = kNil;
    std::size_t              size_ = 0;
};

// Selbstbeweis: das Substrat erfuellt das BTreeNodePool-Concept.
static_assert(BTreeNodePool<BTreeNodePoolStore>);

}  // namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable
