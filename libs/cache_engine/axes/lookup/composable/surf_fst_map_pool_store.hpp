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

#include "surf_fst_map_pool_concept.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

template <class A = std::allocator<std::uint64_t>>
class SurfFstMapPoolStore {
public:
    using node_type            = std::uint64_t;
    using key_type             = node_type;
    using value_type           = node_type;
    using allocator_type       = A;
    using key_allocator_type   = typename std::allocator_traits<A>::template rebind_alloc<key_type>;
    using value_allocator_type = typename std::allocator_traits<A>::template rebind_alloc<value_type>;

    [[nodiscard]] std::size_t size() const noexcept { return keys_.size(); }
    [[nodiscard]] std::size_t lower_bound(key_type k) const noexcept {
        return static_cast<std::size_t>(std::lower_bound(keys_.begin(), keys_.end(), k) - keys_.begin());
    }
    [[nodiscard]] key_type   key_at(std::size_t i) const noexcept { return keys_[i]; }
    [[nodiscard]] value_type value_at(std::size_t i) const noexcept { return vals_[i]; }
    void                     set_value_at(std::size_t i, value_type v) noexcept { vals_[i] = v; }
    /// Sortiert einfuegen an Index i (Aufrufer garantiert die Sortier-Position) — darf via vector werfen.
    void insert_at(std::size_t i, key_type k, value_type v) {
        std::size_t const old_key_capacity = keys_.capacity();
        keys_.insert(keys_.begin() + static_cast<std::ptrdiff_t>(i), k);
        record_capacity_growth_(old_key_capacity, keys_.capacity(), sizeof(std::uint64_t));
        std::size_t const old_value_capacity = vals_.capacity();
        vals_.insert(vals_.begin() + static_cast<std::ptrdiff_t>(i), v);
        record_capacity_growth_(old_value_capacity, vals_.capacity(), sizeof(std::uint64_t));
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
    struct allocator_statistics_snapshot {
        std::uint64_t alloc_calls     = 0;
        std::uint64_t bytes_allocated = 0;
        std::uint64_t live_nodes      = 0;
    };

    [[nodiscard]] allocator_statistics_snapshot store_allocator_statistics() const noexcept {
        return allocator_statistics_snapshot{
            alloc_calls_,
            bytes_allocated_,
            static_cast<std::uint64_t>(keys_.size()),
        };
    }
#endif

private:
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

    std::vector<key_type, key_allocator_type>     keys_{}; // aufsteigend sortiert, duplikatfrei
    std::vector<value_type, value_allocator_type> vals_{}; // parallel zu keys_
#ifdef COMDARE_CE_ENABLE_STATISTICS
    std::uint64_t alloc_calls_     = 0;
    std::uint64_t bytes_allocated_ = 0;
#endif
};

// Selbstbeweis: das Substrat erfuellt das SurfFstMapPool-Concept.
static_assert(SurfFstMapPool<SurfFstMapPoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
