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
#include <vector>

namespace comdare::cache_engine::lookup::composable {

class SurfFstMapPoolStore {
public:
    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;

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

private:
    std::vector<key_type>   keys_{}; // aufsteigend sortiert, duplikatfrei
    std::vector<value_type> vals_{}; // parallel zu keys_
};

// Selbstbeweis: das Substrat erfuellt das SurfFstMapPool-Concept.
static_assert(SurfFstMapPool<SurfFstMapPoolStore>);

} // namespace comdare::cache_engine::lookup::composable
