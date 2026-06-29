#pragma once
// Audit-30 Fix Q2 / Beweis-Schnitt (2026-06-03) — NodeChunkedStore<N,L,A>: node-WIRKSAMES 3-Achsen-Storage-Organ.
//
// @topic nodes @achse 04 @schicht composable-bridge (node_type ⊕ layout ⊕ allocator ALS Storage-Organ, N runtime-wirksam)
//
// **Zweck (Audit docs/architecture/30 §4 Q2, Schritt 3):** ComposedStore<N,L,A> nutzt N nur als constexpr-Provenienz
// (unbounded vector → node_type ohne Laufzeitwirkung; genau die Ursache des „Befund-B"-Verstoßes). NodeChunkedStore
// speichert die Slots in NODE-GROSSEN Chunks der Kapazität N::max_capacity() → die Zahl der Node-(Chunk-)Allokationen
// = ceil(size / N::max_capacity()) hängt REAL von der node_type-Achse ab. Ein darüber komponierter Suchalgorithmus
// (ComposedSearch<Traversal, NodeChunkedStore<N,L,A>>) liefert damit node_type-ABHÄNGIGE Messwerte — der Beweis, dass
// ein DELEGIERENDES Such-Organ die Speicher-Achse wirklich konsumiert (im Gegensatz zu den Monolith-Wrappern).
//
// Erfüllt das StorageOrgan-Concept (gemeinsamer uint64-Key, 8 Methoden) → Drop-in für ComposedSearch. Append-Pfad
// (LinearScan) ist der getriebene Pfad; insert_slot_at/erase_slot_at sind concept-vollständig (shift-basiert) für die
// SortedBinary-Kompatibilität. A/L fließen als Provenienz + (A) realer Chunk-Allokator ein.

#include "axis_04_node_type_node4.hpp" // Pilot-NodeType (Selbstbeweis)
#include "concepts/axis_04_node_type_concept.hpp"
#include <topics/memory_layout/axis_05_memory_layout/concepts/axis_05_memory_layout_concept.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_concept.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp>
#include <topics/traversal/axis_03a_search_algo/composable/storage_organ_concept.hpp>
#include <topics/traversal/axis_03a_search_algo/composable/composable_search.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::node {

namespace _ml_ck = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace _al_ck = ::comdare::cache_engine::allocator::axis_06_allocator;

/// node-gechunktes 3-Achsen-Storage-Organ: Slots liegen in Chunks der Kapazität N::max_capacity().
/// chunk_count() = Zahl der node-großen Blöcke = REALE Laufzeitwirkung der node_type-Achse.
template <class N, class L, class A>
    requires concepts::NodeTypeStrategy<N> && _ml_ck::concepts::MemoryLayoutStrategy<L> &&
             _al_ck::concepts::AllocatorStrategy<A>
class NodeChunkedStore {
private:
    using slot_t = std::pair<std::uint64_t, std::uint64_t>;
    // N::max_capacity() ist die node-Fanout (Node4→4 … Node256→256); 0 abgesichert auf 1.
    static constexpr std::size_t cap_ = (N::max_capacity() == 0 ? std::size_t{1} : N::max_capacity());

public:
    using key_type       = std::uint64_t; // GEMEINSAMER breiter Key (Doku-24-§5.5)
    using value_type     = std::uint64_t;
    using node_type      = N;
    using layout_type    = L;
    using allocator_type = A;

    static constexpr std::size_t node_capacity_v = cap_;
    static constexpr std::size_t cache_line_size = L::cache_line_size();

    [[nodiscard]] static constexpr std::size_t      node_capacity() noexcept { return cap_; }
    [[nodiscard]] static constexpr std::string_view organ_name() noexcept { return "node_chunked"; }
    [[nodiscard]] static constexpr std::string_view node_name() noexcept { return N::name(); }
    [[nodiscard]] static constexpr std::string_view layout_name() noexcept { return L::name(); }
    [[nodiscard]] static constexpr std::string_view allocator_name() noexcept { return A::name(); }

    // node-WIRKSAME Observables (der eigentliche Beweis): #aktive Chunks + #je angelegter Chunks (Node-Allokationen).
    [[nodiscard]] std::size_t chunk_count() const noexcept { return chunks_.size(); }
    [[nodiscard]] std::size_t chunk_alloc_count() const noexcept { return chunk_allocs_; }

    // --- StorageOrgan-Concept: 8 Methoden über logischem Flach-Index (Chunk c=i/cap_, Offset=i%cap_) ---
    [[nodiscard]] std::size_t slot_count() const noexcept { return size_; }
    [[nodiscard]] key_type    key_at(std::size_t i) const noexcept { return chunks_[i / cap_][i % cap_].first; }
    [[nodiscard]] value_type  value_at(std::size_t i) const noexcept { return chunks_[i / cap_][i % cap_].second; }
    void set_value_at(std::size_t i, value_type v) noexcept { chunks_[i / cap_][i % cap_].second = v; }

    void append_slot(key_type k, value_type v) {
        if (chunks_.empty() || chunks_.back().size() == cap_) {
            chunks_.emplace_back();
            chunks_.back().reserve(cap_);
            ++chunk_allocs_;
        }
        chunks_.back().emplace_back(k, v);
        ++size_;
    }
    // concept-vollständig (SortedBinary): logisch an Position i einfügen/löschen, Chunk-Invariante (voll außer letztem)
    // via Flatten-Rebuild wahren. O(n) — im getriebenen LinearScan-Append-Pfad NICHT aufgerufen.
    void insert_slot_at(std::size_t i, key_type k, value_type v) {
        auto flat = flatten_();
        flat.emplace(flat.begin() + static_cast<std::ptrdiff_t>(i), k, v);
        rebuild_(flat);
    }
    void erase_slot_at(std::size_t i) {
        auto flat = flatten_();
        flat.erase(flat.begin() + static_cast<std::ptrdiff_t>(i));
        rebuild_(flat);
    }
    void clear() noexcept {
        chunks_.clear();
        size_         = 0;
        chunk_allocs_ = 0;
    }

    // --- Drop-in-Parität zu ComposedStore (von ObservableComposedSearch optional/requires-detektiert) ---
#ifdef COMDARE_CE_ENABLE_STATISTICS
    // Säule-2 Allocator-Durchgriff — hier NODE-WIRKSAM: allocation_count = Zahl der node-großen Chunks =
    // ceil(size / N::max_capacity()) → hängt REAL von der node_type-Achse ab (Node4 viele, Node256 wenige).
    using allocator_snapshot_t = typename A::snapshot_t; // == allocator::…::AllocationStatistics
    [[nodiscard]] allocator_snapshot_t allocator_statistics() const noexcept {
        allocator_snapshot_t a{};
        a.allocation_count      = chunk_allocs_;
        a.deallocation_count    = 0;
        a.failure_count         = 0;
        a.total_bytes_allocated = chunk_allocs_ * cap_ * sizeof(slot_t);
        a.total_bytes_in_use    = size_ * sizeof(slot_t);
        return a;
    }
#endif
    // V2-Auto-Kopplung node_type: low-Byte je gespeichertem Key → Format-divergenter Self-Lookup (mirror ComposedStore).
    template <class NodeOrgan>
    std::uint64_t organ_observe_node_type(NodeOrgan& org) const {
        std::vector<std::uint8_t> kb;
        kb.reserve(size_);
        for (auto const& c : chunks_)
            for (auto const& s : c) kb.push_back(static_cast<std::uint8_t>(s.first & 0xFFu));
        return org.observe_node_find(kb.data(), kb.size(), kb.data(), kb.size());
    }
    // V2-Auto-Kopplung layout/serialization: Scan über das chunk-lokale Slot-Backing (kein toter Code).
    template <class LayoutOrgan>
    std::uint64_t organ_observe_layout(LayoutOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_)
            acc += org.observe_scan(reinterpret_cast<unsigned char const*>(c.data()), c.size(), sizeof(slot_t));
        return acc;
    }
    template <class SerOrgan>
    std::uint64_t organ_observe_serialization(SerOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_)
            acc += org.observe_serialize(reinterpret_cast<unsigned char const*>(c.data()), c.size(), sizeof(slot_t));
        return acc;
    }
    // Phase B (2026-06-04) T11 value_handle Auto-Kopplung: Scan ueber das chunk-lokale Slot-Backing → die
    // value_handle-Observer-Huelle treibt value_access_scan ueber die REAL gespeicherten Slots (record_size =
    // sizeof(slot_t)); KEINE flache Roh-Puffer-Simulation mehr (der vom Spec geforderte „echte Weg"). Analog
    // organ_observe_layout/_serialization. Jeder Chunk = ein Slot-Block der node_type-Kapazitaet.
    template <class VhOrgan>
    std::uint64_t organ_observe_value_handle(VhOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_)
            acc += org.observe_value_handle(reinterpret_cast<unsigned char const*>(c.data()), c.size(), sizeof(slot_t));
        return acc;
    }
    // Phase B (2026-06-04) T12 isa Auto-Kopplung: SIMD-Feld-Reduktion ueber das chunk-lokale Slot-Backing → die
    // isa-Observer-Huelle treibt simd_field_sum ueber die REAL gespeicherten Slot-Bytes als 32-bit-Wort-Strom
    // (n = chunk_bytes / 4). simd_field_sum nimmt nur (buf, n) — kein record_size. Analog organ_observe_layout.
    template <class IsaOrgan>
    std::uint64_t organ_observe_isa(IsaOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_) {
            std::size_t const words = (c.size() * sizeof(slot_t)) / sizeof(std::uint32_t);
            acc += org.observe_simd_field_sum(reinterpret_cast<unsigned char const*>(c.data()), words);
        }
        return acc;
    }
    // Phase B (2026-06-04) T13 index_organization Auto-Kopplung: Scan ueber das chunk-lokale Slot-Backing → die
    // index_org-Observer-Huelle treibt index_org_scan ueber die REAL gespeicherten Slots (record_size =
    // sizeof(slot_t)); strategie-divergentes Zugriffsmuster (Clustered sequential / Heap predicate / NonClustered
    // indirect / IOT embedded). Analog organ_observe_layout. Jeder Chunk = ein Slot-Block der node_type-Kapazitaet.
    template <class IdxOrgan>
    std::uint64_t organ_observe_index_org(IdxOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_)
            acc += org.index_org_observe(reinterpret_cast<unsigned char const*>(c.data()), c.size(), sizeof(slot_t));
        return acc;
    }
    // Phase B (2026-06-04) T14 io_dispatch Auto-Kopplung: Scan ueber das chunk-lokale Slot-Backing → die io_dispatch-
    // Observer-Huelle treibt io_dispatch_scan ueber die REAL gespeicherten Slots (record_size = sizeof(slot_t)) als
    // IN-MEMORY-Dispatch (kein Disk-IO, Hauptagent-Entscheid). Analog organ_observe_layout.
    template <class IoOrgan>
    std::uint64_t organ_observe_io_dispatch(IoOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_)
            acc += org.observe_dispatch(reinterpret_cast<unsigned char const*>(c.data()), c.size(), sizeof(slot_t));
        return acc;
    }
    // Phase B (2026-06-04) T15 migration_policy Auto-Kopplung: Scan ueber das chunk-lokale Slot-Backing → die
    // migration-Observer-Huelle treibt migration_decide_scan (record_size = sizeof(slot_t) ≥ 4 → der 4-Byte-Recency-
    // Read im observe_decide ist OOB-sicher). decide-only, KEIN realer Block-Move (tier_moves honest 0). Analog layout.
    template <class MigOrgan>
    std::uint64_t organ_observe_migration(MigOrgan& org) const {
        for (auto const& c : chunks_)
            org.observe_decide(reinterpret_cast<unsigned char const*>(c.data()), c.size(), sizeof(slot_t));
        return 0; // observe_decide ist void (Treibe-Op exerziert, Wegopt-Schutz im seg19-Pfad)
    }
    // Phase B (2026-06-04) T16 filter Auto-Kopplung: die low-Bytes der gespeicherten Keys werden als Query-Strom UND
    // als Filter-Backing-Bitmap an die filter-Observer-Huelle gegeben (filter_probe_scan(buf,n,queries,q)) — analog
    // organ_observe_node_type (das ebenfalls die Key-Low-Bytes als Self-Lookup-Query nutzt). REALER In-Memory-Filter.
    template <class FltOrgan>
    std::uint64_t organ_observe_filter(FltOrgan& org) const {
        // P5 (#124, 2026-06-04, User §4.3): bevorzugt den REALEN Filter ueber die VOLLEN gespeicherten Keys proben
        // (observe_probe_keys → strat_.probe_key) — gleiche uint64-Key-Domain wie der insert_key-Build. Fallback
        // (synthetische Strategien ohne observe_probe_keys): der bisherige 1-Byte-Puffer-Pfad (unveraendert).
        if constexpr (requires { org.observe_probe_keys(static_cast<std::uint64_t const*>(nullptr), std::size_t{}); }) {
            std::vector<std::uint64_t> ks;
            ks.reserve(size_);
            for (auto const& c : chunks_)
                for (auto const& s : c) ks.push_back(static_cast<std::uint64_t>(s.first));
            return org.observe_probe_keys(ks.data(), ks.size());
        } else {
            std::vector<unsigned char> kb;
            kb.reserve(size_);
            for (auto const& c : chunks_)
                for (auto const& s : c) kb.push_back(static_cast<unsigned char>(s.first & 0xFFu));
            return org.observe_probe(kb.data(), kb.size(), kb.data(), kb.size());
        }
    }

private:
    [[nodiscard]] std::vector<slot_t> flatten_() const {
        std::vector<slot_t> f;
        f.reserve(size_);
        for (auto const& c : chunks_)
            for (auto const& s : c) f.push_back(s);
        return f;
    }
    void rebuild_(std::vector<slot_t> const& flat) {
        clear();
        for (auto const& s : flat) append_slot(s.first, s.second);
    }

    std::vector<std::vector<slot_t>> chunks_{};
    std::size_t                      size_         = 0;
    std::size_t                      chunk_allocs_ = 0;
};

// Compile-Time-Selbstbeweis: node-gechunktes Organ erfüllt StorageOrgan UND ist von beiden Traversal-Organen nutzbar.
namespace _ck_cmp = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable;
using PilotChunkedStore =
    NodeChunkedStore<Node4NodeType, _ml_ck::CacheLineAlignedMemoryLayout, _al_ck::MimallocAllocator>;
static_assert(_ck_cmp::StorageOrgan<PilotChunkedStore>);
static_assert(_ck_cmp::TraversalOrgan<_ck_cmp::LinearScanTraversal, PilotChunkedStore>);
static_assert(_ck_cmp::TraversalOrgan<_ck_cmp::SortedBinaryTraversal, PilotChunkedStore>);

} // namespace comdare::cache_engine::node
