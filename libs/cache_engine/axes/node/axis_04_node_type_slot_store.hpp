#pragma once
// V41 Saeule-1 (Doku 24 §5.4/§5.5, Doku 14 §3) — NodeTypeSlotStore: node_type-getriebenes STORAGE-Organ.
//
// @topic nodes @achse 04 @schicht composable-bridge (node_type ALS Storage-Organ)
//
// **Zweck (Korrektur Doku 24 §5.4):** Bisher waren die vier NodeType-Wrapper (Node4/16/48/256)
// reine Tag-Klassen mit nur `max_capacity()` — KEIN Speicher-Substrat. Dieses Organ macht den
// node_type zu einem echten, vom ComposedSearch konsumierbaren Storage-Organ: `N::max_capacity()`
// dimensioniert ein bounded `std::array`-Slot-Substrat ueber GEMEINSAMEM uint64-Key. Damit ist der
// node_type kein "Tier" (ganze Struktur), sondern ein austauschbares ORGAN (Doku 14 §1.2): ein
// Suchalgorithmus = Traversal-Organ ⊕ NodeTypeSlotStore<N>.
//
// **Increment 1 (Scope):** NUR NodeType (Kapazitaet). Layout (axis_05) + Allocator (axis_06) als
// echte Slot-Stride/Backing-Achsen folgen als `NodeTypeSlotStore<N, L, A>` im Folge-Increment.
// statistics()/snapshot_t (ObservableAxis, Doku 24 §2.2) sind ebenfalls Folge-Increment.
//
// **uint64-Key:** loest den Doku-24-§5.5-Blocker (Array256=uint8, BST=uint16) strukturell — die
// node_type-Kapazitaet bestimmt die Slot-Anzahl, NICHT die Key-Breite.

#include "axis_04_node_type_node4.hpp" // Pilot-NodeType + zieht concept/base/subaxes/flags mit
#include "concepts/axis_04_node_type_concept.hpp"
#include <topics/traversal/axis_03a_search_algo/composable/storage_organ_concept.hpp>
#include <topics/traversal/axis_03a_search_algo/composable/composable_search.hpp> // Traversal-Organe (Selbstbeweis)

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::node {

/// node_type-getriebenes Storage-Organ: bounded Slots der Kapazitaet N::max_capacity() ueber uint64-Key.
/// Erfuellt das StorageOrgan-Concept und ist damit Drop-in-Substrat fuer ComposedSearch<Traversal, .>.
template <class N>
    requires concepts::NodeTypeStrategy<N>
class NodeTypeSlotStore {
public:
    using key_type   = std::uint64_t; // GEMEINSAMER breiter Key (Doku-24-§5.5)
    using value_type = std::uint64_t;

    static constexpr std::size_t capacity = N::max_capacity();

    // Provenienz (NICHT Teil des StorageOrgan-Vertrags, aber weist das Organ als node_type-getrieben aus).
    [[nodiscard]] static constexpr std::size_t      node_capacity() noexcept { return capacity; }
    [[nodiscard]] static constexpr std::string_view organ_name() noexcept { return N::name(); }

    [[nodiscard]] std::size_t slot_count() const noexcept { return size_; }
    [[nodiscard]] key_type    key_at(std::size_t i) const noexcept { return slots_[i].first; }
    [[nodiscard]] value_type  value_at(std::size_t i) const noexcept { return slots_[i].second; }
    void                      set_value_at(std::size_t i, value_type v) noexcept { slots_[i].second = v; }

    void append_slot(key_type k, value_type v) {
        if (size_ >= capacity) throw std::length_error("NodeTypeSlotStore: capacity exceeded");
        slots_[size_++] = {k, v};
    }
    void insert_slot_at(std::size_t i, key_type k, value_type v) {
        if (size_ >= capacity) throw std::length_error("NodeTypeSlotStore: capacity exceeded");
        for (std::size_t j = size_; j > i; --j) slots_[j] = slots_[j - 1]; // rechts schieben
        slots_[i] = {k, v};
        ++size_;
    }
    void erase_slot_at(std::size_t i) noexcept {
        for (std::size_t j = i + 1; j < size_; ++j) slots_[j - 1] = slots_[j]; // links schieben
        --size_;
    }
    void clear() noexcept { size_ = 0; }

private:
    std::array<std::pair<key_type, value_type>, capacity> slots_{};
    std::size_t                                           size_ = 0;
};

// Compile-Time-Selbstbeweis (Regel "Compile-Time-Only NO Runtime"): der node_type-getriebene Store
// erfuellt das StorageOrgan-Concept UND ist von BEIDEN Traversal-Organen nutzbar (Organ-Swappability).
namespace ce_cmp = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable;
static_assert(ce_cmp::StorageOrgan<NodeTypeSlotStore<Node4NodeType>>);
static_assert(ce_cmp::TraversalOrgan<ce_cmp::LinearScanTraversal, NodeTypeSlotStore<Node4NodeType>>);
static_assert(ce_cmp::TraversalOrgan<ce_cmp::SortedBinaryTraversal, NodeTypeSlotStore<Node4NodeType>>);

} // namespace comdare::cache_engine::node
