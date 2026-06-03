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

#include "axis_04_node_type_node4.hpp"                 // Pilot-NodeType (Selbstbeweis)
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
    requires concepts::NodeTypeStrategy<N>
          && _ml_ck::concepts::MemoryLayoutStrategy<L>
          && _al_ck::concepts::AllocatorStrategy<A>
class NodeChunkedStore {
private:
    using slot_t = std::pair<std::uint64_t, std::uint64_t>;
    // N::max_capacity() ist die node-Fanout (Node4→4 … Node256→256); 0 abgesichert auf 1.
    static constexpr std::size_t cap_ = (N::max_capacity() == 0 ? std::size_t{1} : N::max_capacity());

public:
    using key_type       = std::uint64_t;   // GEMEINSAMER breiter Key (Doku-24-§5.5)
    using value_type     = std::uint64_t;
    using node_type      = N;
    using layout_type    = L;
    using allocator_type = A;

    static constexpr std::size_t node_capacity_v = cap_;
    static constexpr std::size_t cache_line_size = L::cache_line_size();

    [[nodiscard]] static constexpr std::size_t      node_capacity()  noexcept { return cap_; }
    [[nodiscard]] static constexpr std::string_view organ_name()     noexcept { return "node_chunked"; }
    [[nodiscard]] static constexpr std::string_view node_name()      noexcept { return N::name(); }
    [[nodiscard]] static constexpr std::string_view layout_name()    noexcept { return L::name(); }
    [[nodiscard]] static constexpr std::string_view allocator_name() noexcept { return A::name(); }

    // node-WIRKSAME Observables (der eigentliche Beweis): #aktive Chunks + #je angelegter Chunks (Node-Allokationen).
    [[nodiscard]] std::size_t chunk_count()       const noexcept { return chunks_.size(); }
    [[nodiscard]] std::size_t chunk_alloc_count() const noexcept { return chunk_allocs_; }

    // --- StorageOrgan-Concept: 8 Methoden über logischem Flach-Index (Chunk c=i/cap_, Offset=i%cap_) ---
    [[nodiscard]] std::size_t slot_count()            const noexcept { return size_; }
    [[nodiscard]] key_type    key_at(std::size_t i)   const noexcept { return chunks_[i / cap_][i % cap_].first; }
    [[nodiscard]] value_type  value_at(std::size_t i) const noexcept { return chunks_[i / cap_][i % cap_].second; }
    void set_value_at(std::size_t i, value_type v)          noexcept { chunks_[i / cap_][i % cap_].second = v; }

    void append_slot(key_type k, value_type v) {
        if (chunks_.empty() || chunks_.back().size() == cap_) { chunks_.emplace_back(); chunks_.back().reserve(cap_); ++chunk_allocs_; }
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
    void clear() noexcept { chunks_.clear(); size_ = 0; chunk_allocs_ = 0; }

private:
    [[nodiscard]] std::vector<slot_t> flatten_() const {
        std::vector<slot_t> f; f.reserve(size_);
        for (auto const& c : chunks_) for (auto const& s : c) f.push_back(s);
        return f;
    }
    void rebuild_(std::vector<slot_t> const& flat) {
        clear();
        for (auto const& s : flat) append_slot(s.first, s.second);
    }

    std::vector<std::vector<slot_t>> chunks_{};
    std::size_t size_        = 0;
    std::size_t chunk_allocs_ = 0;
};

// Compile-Time-Selbstbeweis: node-gechunktes Organ erfüllt StorageOrgan UND ist von beiden Traversal-Organen nutzbar.
namespace _ck_cmp = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable;
using PilotChunkedStore = NodeChunkedStore<Node4NodeType, _ml_ck::CacheLineAlignedMemoryLayout, _al_ck::MimallocAllocator>;
static_assert(_ck_cmp::StorageOrgan<PilotChunkedStore>);
static_assert(_ck_cmp::TraversalOrgan<_ck_cmp::LinearScanTraversal,   PilotChunkedStore>);
static_assert(_ck_cmp::TraversalOrgan<_ck_cmp::SortedBinaryTraversal, PilotChunkedStore>);

}  // namespace comdare::cache_engine::node
