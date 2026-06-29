#pragma once
// V41 Saeule-1 (Doku 24 §6, Increment 2) — ComposedStore<N,L,A>: 3-Achsen-Storage-Organ.
//
// @topic nodes @achse 04 @schicht composable-bridge (node_type ⊕ layout ⊕ allocator ALS Storage-Organ)
//
// **Zweck (Doku 24 §6, Fortsetzung von NodeTypeSlotStore<N>):** Holt zusaetzlich zur node_type-Achse
// die **allocator-Achse (axis_06)** und die **layout-Achse (axis_05)** real in das komponierbare
// Storage-Organ. Erfuellt WEITER das StorageOrgan-Concept (gemeinsamer uint64-Key, 8 Methoden) und ist
// damit Drop-in-Substrat fuer ComposedSearch<Traversal, .> → ein Such-Algorithmus =
// Traversal-Organ ⊕ ComposedStore<N,L,A>.
//
// **Was real wirkt (Increment 2):**
//   - A (allocator): der Slot-Speicher wird REAL ueber die Allocator-Achse bezogen
//     (std::vector<Slot, A::StdAllocatorAdapter<Slot>>, via as_std_allocator<Slot>()).
//   - L (layout): fliesst in den Organ-TYP ein (cache_line_size als static constexpr + Provenienz) und
//     ist ueber die optionale, NICHT-Vertrags-Methode organ_scan_field_sum() operativ messbar
//     (L::scan_field_sum ueber das Slot-Backing — F15-Mess-Bruecke).
//   - N (node_type): Kapazitaets-Provenienz (node_capacity()=N::max_capacity()).
//
// **Bewusst NICHT in diesem Increment (Folge-Increments, Doku 24 §6 + Session-Doku 2026-05-29):**
//   - ECHTE physische Layout-getragene Slot-Anordnung (L::slot_index/AoS-SoA-Interleaving): das
//     MemoryLayoutStrategy-Concept fordert NUR cache_line_size(); eine Slot-Permutation braeuchte eine
//     Concept-Erweiterung (broke 5 Wrapper) → eigener Increment. Die 8 Methoden greifen daher linear.
//   - bounded ComposedArrayStore<N,L,A> (N::max_capacity() als hartes Limit) — hier unbounded vector.
//   - statistics()/snapshot_t/ObservableAxis (Doku 24 §2.2) — bleibt aus dem StorageOrgan-Vertrag.

#include "axis_04_node_type_node4.hpp" // Pilot-NodeType (Selbstbeweis)
#include "concepts/axis_04_node_type_concept.hpp"
#include <topics/memory_layout/axis_05_memory_layout/concepts/axis_05_memory_layout_concept.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp> // Pilot-Layout
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_concept.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp> // Pilot-Allocator
#include <topics/traversal/axis_03a_search_algo/composable/storage_organ_concept.hpp>
#include <topics/traversal/axis_03a_search_algo/composable/composable_search.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::node {

namespace _ml = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace _al = ::comdare::cache_engine::allocator::axis_06_allocator;

/// 3-Achsen-Storage-Organ: node_type(N) ⊕ layout(L) ⊕ allocator(A) hinter dem uint64-StorageOrgan-Interface.
/// Der Slot-Speicher kommt REAL aus der Allocator-Achse A (via std::vector<Slot, A::StdAllocatorAdapter<Slot>>).
template <class N, class L, class A>
    requires concepts::NodeTypeStrategy<N> && _ml::concepts::MemoryLayoutStrategy<L> &&
             _al::concepts::AllocatorStrategy<A>
class ComposedStore {
private:
    // Frueh deklariert (vor allen Verwendungen in Ctor/Methoden/Membern) — MSVC-robust, kein
    // Verlass auf complete-class-context fuer den Member-Initializer.
    using slot_t     = std::pair<std::uint64_t, std::uint64_t>;
    using slot_alloc = typename A::template StdAllocatorAdapter<slot_t>;

public:
    using key_type       = std::uint64_t; // GEMEINSAMER breiter Key (Doku-24-§5.5)
    using value_type     = std::uint64_t;
    using node_type      = N;
    using layout_type    = L;
    using allocator_type = A;

    static constexpr std::size_t node_capacity_v = N::max_capacity();
    static constexpr std::size_t cache_line_size = L::cache_line_size(); // Layout floss in den Typ ein

    // Provenienz (NICHT Teil des StorageOrgan-Vertrags) — weist die 3 beteiligten Achsen aus.
    [[nodiscard]] static constexpr std::size_t      node_capacity() noexcept { return node_capacity_v; }
    [[nodiscard]] static constexpr std::string_view organ_name() noexcept { return "composed"; }
    [[nodiscard]] static constexpr std::string_view node_name() noexcept { return N::name(); }
    [[nodiscard]] static constexpr std::string_view layout_name() noexcept { return L::name(); }
    [[nodiscard]] static constexpr std::string_view allocator_name() noexcept { return A::name(); }

    // --- StorageOrgan-Concept: 8 Methoden (linear, semantisch == RawSlotStore → std::map-aequivalent) ---
    [[nodiscard]] std::size_t slot_count() const noexcept { return slots_.size(); }
    [[nodiscard]] key_type    key_at(std::size_t i) const noexcept { return slots_[i].first; }
    [[nodiscard]] value_type  value_at(std::size_t i) const noexcept { return slots_[i].second; }
    void                      set_value_at(std::size_t i, value_type v) noexcept { slots_[i].second = v; }
    void                      append_slot(key_type k, value_type v) { slots_.emplace_back(k, v); }
    void                      insert_slot_at(std::size_t i, key_type k, value_type v) {
        slots_.emplace(slots_.begin() + static_cast<std::ptrdiff_t>(i), k, v);
    }
    void erase_slot_at(std::size_t i) { slots_.erase(slots_.begin() + static_cast<std::ptrdiff_t>(i)); }
    void clear() noexcept { slots_.clear(); }

    // Optionale, NICHT-Vertrags-Methode: macht die Layout-Achse operativ messbar (F15-Bruecke).
    // Ruft L::scan_field_sum ueber das rohe Slot-Backing (Layout-distinkter Read-Pfad; kein toter Code).
    [[nodiscard]] std::uint64_t organ_scan_field_sum() const noexcept {
        return L::scan_field_sum(reinterpret_cast<unsigned char const*>(slots_.data()), slots_.size(), sizeof(slot_t));
    }

    // V42 L-74c scan-Achsen-Auto-Kopplung (Pfad-B Zustand-Scan): treibt ein gegebenes Observer-Organ
    // (memory_layout/serialization-Huelle) ueber das ECHTE Slot-Backing → die scan-Achsen-statistics()
    // messen die realen Tier-Daten zum Observe-Zeitpunkt (nicht einen synthetischen Buffer). Nicht-Vertrags-
    // Methoden (analog organ_scan_field_sum). Templated → kein Achsen-Header-Include im Store noetig.
    template <class LayoutOrgan>
    std::uint64_t organ_observe_layout(LayoutOrgan& org) const {
        return org.observe_scan(reinterpret_cast<unsigned char const*>(slots_.data()), slots_.size(), sizeof(slot_t));
    }
    template <class SerOrgan>
    std::uint64_t organ_observe_serialization(SerOrgan& org) const {
        return org.observe_serialize(reinterpret_cast<unsigned char const*>(slots_.data()), slots_.size(),
                                     sizeof(slot_t));
    }

    // V42 L-74c node_type-Auto-Kopplung: extrahiert die niederwertigsten Key-Bytes (ART-Node256-Direkt-
    // Adressierung) aus dem Slot-Backing als `stored` + self-query (alle gespeicherten Keys nachschlagen) →
    // das node_type-Organ misst den Format-divergenten Lookup ueber die ECHTEN Tier-Keys. Templated.
    template <class NodeOrgan>
    std::uint64_t organ_observe_node_type(NodeOrgan& org) const {
        std::vector<std::uint8_t> kb;
        kb.reserve(slots_.size());
        for (auto const& s : slots_) kb.push_back(static_cast<std::uint8_t>(s.first & 0xFFu));
        return org.observe_node_find(kb.data(), kb.size(), kb.data(), kb.size());
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    // Saeule-2 (Doku 24 §2.2): Durchgriff auf die Allocator-Achsen-Statistik. NICHT-Vertrags-Methode
    // (StorageOrgan fordert statistics() bewusst nicht); A::statistics() ist unter STATISTICS Pflicht-API
    // (CacheEnginePermutationStrategy). Der Allocator wird durch slots_-Vector-Growth (insert/erase) real
    // getrieben → allocation_count/total_bytes_in_use sind workload-getrieben.
    using allocator_snapshot_t = typename A::snapshot_t; // == allocator::…::AllocationStatistics
    [[nodiscard]] allocator_snapshot_t allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

    // --- Lifetime: der StdAllocatorAdapter haelt einen Derived*-Zeiger auf allocator_ ---
    // allocator_ MUSS vor slots_ deklariert sein; slots_ wird mit allocator_.as_std_allocator<slot_t>()
    // konstruiert. Copy/Move bewusst geloescht — der Adapter haelt &allocator_, eine korrekte
    // Rematerialisierung am Ziel-allocator_ ist Folge-Increment und wird vom Pilot nicht benoetigt
    // (ComposedSearch haelt den Store by value, default-konstruiert, nie kopiert/bewegt).
    ComposedStore() : allocator_{}, slots_(allocator_.template as_std_allocator<slot_t>()) {}
    ComposedStore(ComposedStore const&)            = delete;
    ComposedStore& operator=(ComposedStore const&) = delete;
    ComposedStore(ComposedStore&&)                 = delete;
    ComposedStore& operator=(ComposedStore&&)      = delete;
    ~ComposedStore()                               = default;

private:
    // slot_t/slot_alloc sind oben (vor den public-Membern) deklariert — KEINE Re-Deklaration hier
    // (frueheres Doppel war MSVC-toleriert, aber GCC/Clang-brechend; Cleanup Roadmap-1).
    A                               allocator_; // VOR slots_ (Lifetime des Derived*-Zeigers im Adapter)
    std::vector<slot_t, slot_alloc> slots_;
};

// Compile-Time-Selbstbeweis (Regel "Compile-Time-Only NO Runtime"): das 3-Achsen-Organ erfuellt
// StorageOrgan UND ist von BEIDEN Traversal-Organen nutzbar. Pilot = (Node4, CacheLineAligned, Mimalloc).
namespace ce_cmp         = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable;
using PilotComposedStore = ComposedStore<Node4NodeType, _ml::CacheLineAlignedMemoryLayout, _al::MimallocAllocator>;
static_assert(ce_cmp::StorageOrgan<PilotComposedStore>);
static_assert(ce_cmp::TraversalOrgan<ce_cmp::LinearScanTraversal, PilotComposedStore>);
static_assert(ce_cmp::TraversalOrgan<ce_cmp::SortedBinaryTraversal, PilotComposedStore>);

} // namespace comdare::cache_engine::node
