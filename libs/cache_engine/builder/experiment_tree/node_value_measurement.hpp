#pragma once
// BR-3 (2026-06-02, Doc 27 §3) — node_value_measurement: füllt NodeValue mit einem ECHTEN ObserverAggregate-
// Snapshot einer REALEN Komposition (kein 4-uint64-Stub).
//
// Pfad B in-process (Doku 24 §8.6): instanziiert den realen genus-ABI-Adapter SearchAlgorithmAbiAdapter<
// SearchAlgorithmAnatomy<Comp>>, treibt das ECHTE Such-Organ (tier_insert/tier_lookup) + den allocator-
// ComposedStore, zieht tier_observe → ComdareTierObserverSnapshotV1 (search_algo + allocator REAL), flacht ihn
// in NodeValue.observer. Der Knoten trägt damit den echten Per-Achsen-Observer + (via CompositionRegistry) die
// read-only Achsen-Definition.
//
// R5.B-GRENZE (EHRLICH, Doku 21 §4a / Doku 24 §5.5): operativ getrieben sind search_algo + allocator; die
// übrigen 15 Komposition-Achsen sind heute passive Compile-Time-Deskriptoren (Default-Snapshot). observer.
// observable_axis_count macht das transparent — KEINE Behauptung, alle 17/22 Observer seien voll.
//
// 22-OBSERVER (Doc 27 §0.1 / BR-3-OBS-22): dieser Treiber liefert den SearchAlgorithm-Gattungs-Observer (17er-
// Komposition). Die 5 außerhalb-Achsen (page_type/09b/12 = Definition-statt-Observer bzw. Build-Variante;
// q1/q2 = eigener Container-Gattungs-Observer) werden NICHT hier, sondern je nach Natur separat getragen.
//
// ⚠️ tier_observe existiert nur bei COMDARE_MEASUREMENT_ON; ohne das Flag bleibt observer=0/observer_real=false
// (ehrlich: kein Mess-Build → kein echter Observer). C++23.

#include "experiment_tree.hpp"             // NodeValue / NodeObserverSnapshot (umbrella-unabhängig)
#include "anatomy/composition_factory.hpp" // CompositionFromPermTuple
#include "anatomy/search_algorithm_anatomy.hpp"
#include "anatomy/abi_adapter.hpp"         // SearchAlgorithmAbiAdapter (treibt + tier_observe)
#include "anatomy/observable_tier.hpp"     // ComdareTierObserverSnapshotV1

#include <cstdint>

namespace comdare::cache_engine::builder::experiment {

/// Misst EINE reale Komposition (PermTuple<17> P) in-process über den realen genus-ABI-Adapter und liefert
/// NodeValue mit echtem Observer-Snapshot. n_keys = Treib-Last (insert + lookup).
template <class P>
[[nodiscard]] inline NodeValue measure_composition(std::uint64_t n_keys = 256) {
    namespace an = ::comdare::cache_engine::anatomy;
    using Comp    = an::CompositionFromPermTuple<P>;
    using Anatomy = an::SearchAlgorithmAnatomy<Comp>;

    NodeValue nv;
    an::SearchAlgorithmAbiAdapter<Anatomy> adapter;   // realer genus-Adapter (Mammal/SearchAlgorithm)
    adapter.warm_up();
    adapter.run();
    for (std::uint64_t k = 0; k < n_keys; ++k) (void)adapter.tier_insert(k, k * 7u + 1u);  // treibt Organ + Store
    for (std::uint64_t k = 0; k < n_keys; ++k) { std::uint64_t v = 0; (void)adapter.tier_lookup(k, &v); }

    nv.measured_setting_count = 1;
    nv.has_result             = true;
    nv.sum_op_count           = 2u * n_keys;

#if COMDARE_MEASUREMENT_ON
    an::ComdareTierObserverSnapshotV1 pod{};
    adapter.tier_observe(&pod);   // observe_all → flacher POD (search_algo + allocator real)
    nv.observer.search_lookup_count      = pod.search_lookup_count;
    nv.observer.search_hit_count         = pod.search_hit_count;
    nv.observer.search_miss_count        = pod.search_miss_count;
    nv.observer.search_insert_count      = pod.search_insert_count;
    nv.observer.search_erase_count       = pod.search_erase_count;
    nv.observer.search_peak_occupancy    = pod.search_peak_occupancy;
    nv.observer.alloc_bytes_allocated    = pod.alloc_bytes_allocated;
    nv.observer.alloc_bytes_in_use       = pod.alloc_bytes_in_use;
    nv.observer.alloc_allocation_count   = pod.alloc_allocation_count;
    nv.observer.alloc_deallocation_count = pod.alloc_deallocation_count;
    nv.observer.alloc_failure_count      = pod.alloc_failure_count;
    nv.observer.observable_axis_count    = pod.observable_axis_count;
    nv.observer.tier_fill_level          = pod.tier_fill_level;
    nv.observer_real                     = true;
#else
    // Kein Mess-Build (COMDARE_MEASUREMENT_ON aus) → kein echter Observer. observable_count() ist dennoch
    // compile-time abrufbar (Diagnose, ohne Treiben): wie viele Achsen WÄREN beobachtbar.
    nv.observer.observable_axis_count = Anatomy::observable_axis_count();
    nv.observer.tier_fill_level       = n_keys;
    nv.observer_real                  = false;
#endif
    return nv;
}

/// Compile-time-Diagnose (auch ohne Mess-Build): wie viele Achsen der Komposition sind ObservableAxis.
template <class P>
[[nodiscard]] inline constexpr std::size_t composition_observable_axis_count() noexcept {
    return ::comdare::cache_engine::anatomy::SearchAlgorithmAnatomy<
        ::comdare::cache_engine::anatomy::CompositionFromPermTuple<P>>::observable_axis_count();
}

}  // namespace comdare::cache_engine::builder::experiment
