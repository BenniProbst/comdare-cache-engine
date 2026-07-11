#pragma once
// V41 Umstufung-A s4 (Task #43) — SurfFstMapPoolStore: exaktes sortiertes K->V-Substrat (erfuellt SurfFstMapPool).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Such-Logik — die SuRF-Map-Schale (S1 correctness-base, is_original=false): zwei
// parallele Vektoren (aufsteigend sortierte Keys + exakte Werte). AUTORITATIVES exaktes K->V, sodass das
// SuRF-Tier am std::map-Interface vergleichbar ist (das echte LOUDS-Filter-Organ in axis_filter liefert nur
// approximatives may-contain). insert/lookup/erase lebt im SurfMapTraversalOrgan, NICHT hier.
//
// S2-Pfad: dieser Store wird durch die echte LOUDS-FST (LoudsDense 256-Bitmaps + LoudsSparse rank/select +
// Suffix + value-Slot je Leaf) ersetzt; die hier exponierte Pflicht-API (sortierter Index-Zugriff) bleibt
// stabil, der Filter wird ein zweiter statischer View. Heute sortierter std::vector (Phase-5-Succinct spaeter).
//
// Phase 0.3a (Hebel B, Doc 21 §F): der Vektor-Speicher kommt REAL aus der Allocator-Achse (axis_06), analog
// TreeNodePoolStore (BST). store_allocator_statistics() liefert die Strategie-Statistik -> T6 reflektiert den
// ECHTEN Allocator. Default ExgenAllocator (real=std bei disabled). COW-Sicherheit via Memento (Copy-Ctor/
// Assign rebinden den StdAllocatorAdapter + verwerfen die COW-Kopier-Pollution; Move degradiert zu Copy).

#include "surf_fst_map_pool_concept.hpp"
#include <axes/alloc/axis_06_allocator_exgen.hpp>
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

template <class Alloc = ::comdare::cache_engine::alloc::ExgenAllocator>
    requires ::comdare::cache_engine::alloc::concepts::AllocatorStrategy<Alloc>
class SurfFstMapPoolStore {
public:
    using node_type      = std::uint64_t;
    using key_type       = node_type;
    using value_type     = node_type;
    using allocator_type = Alloc;

private:
    using key_alloc   = typename Alloc::template StdAllocatorAdapter<key_type>;
    using value_alloc = typename Alloc::template StdAllocatorAdapter<value_type>;

public:
    // Phase 0.3a (analog BST): die Vektoren allokieren real ueber die axis_06-Strategie; Copy-Ctor/Assign rebinden
    // den Adapter an das eigene allocator_ und verwerfen die COW-Kopier-Pollution per Memento-restore_statistics.
    SurfFstMapPoolStore()
        : keys_(allocator_.template as_std_allocator<key_type>()),
          vals_(allocator_.template as_std_allocator<value_type>()) {}
    SurfFstMapPoolStore(SurfFstMapPoolStore const& o)
        : allocator_(o.allocator_), keys_(o.keys_, allocator_.template as_std_allocator<key_type>()),
          vals_(o.vals_, allocator_.template as_std_allocator<value_type>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    SurfFstMapPoolStore& operator=(SurfFstMapPoolStore const& o) {
        if (this != &o) {
            keys_ = o.keys_;
            vals_ = o.vals_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~SurfFstMapPoolStore() = default;

    [[nodiscard]] std::size_t size() const noexcept { return keys_.size(); }
    [[nodiscard]] std::size_t lower_bound(key_type k) const noexcept {
        return static_cast<std::size_t>(std::lower_bound(keys_.begin(), keys_.end(), k) - keys_.begin());
    }
    [[nodiscard]] key_type   key_at(std::size_t i) const noexcept { return keys_[i]; }
    [[nodiscard]] value_type value_at(std::size_t i) const noexcept { return vals_[i]; }
    void                     set_value_at(std::size_t i, value_type v) noexcept { vals_[i] = v; }
    /// Sortiert einfuegen an Index i (Aufrufer garantiert die Sortier-Position) — darf via vector werfen.
    void insert_at(std::size_t i, key_type k, value_type v) {
        keys_.insert(keys_.begin() + static_cast<std::ptrdiff_t>(i), k);
        vals_.insert(vals_.begin() + static_cast<std::ptrdiff_t>(i), v);
    }
    void erase_at(std::size_t i) noexcept {
        keys_.erase(keys_.begin() + static_cast<std::ptrdiff_t>(i));
        vals_.erase(vals_.begin() + static_cast<std::ptrdiff_t>(i));
    }
    void clear() noexcept {
        keys_.clear();
        vals_.clear();
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using allocator_snapshot_t = typename Alloc::snapshot_t;
    /// T6-Route (Phase 0.3a): die ECHTE Allocator-Achsen-Statistik (rich AllocationStatistics, 5 Felder).
    [[nodiscard]] allocator_snapshot_t store_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    Alloc                                allocator_{};
    std::vector<key_type, key_alloc>     keys_; // aufsteigend sortiert, duplikatfrei
    std::vector<value_type, value_alloc> vals_; // parallel zu keys_
};

// Selbstbeweis: das Substrat erfuellt das SurfFstMapPool-Concept.
static_assert(SurfFstMapPool<SurfFstMapPoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
