#pragma once
// V42 L-74c — Cross-ABI-Brücke: ObserverAggregate<Composition> → flacher ComdareTierObserverSnapshotV2.
//
// Der ObserverAggregate ist Composition-ABHÄNGIG (sein Layout variiert je Composition); der V2-POD ist
// flach + versioniert + memcpy-fähig (observable_tier.hpp). Diese Brücke mappt die 5 voll observable+
// getriebenen Achsen (search_algo + die 4 OperativeCapable telemetry/memory_layout/serialization/node_type)
// aus dem Aggregate in den flachen POD — der erste Schritt der Cross-ABI-Verdrahtung (Doc 29 §3 Schritt 4).
// Jede Achse if-constexpr-geschützt: nicht-observable Achsen (EmptyAxisSnapshot) lassen ihre Felder 0.
//
// @doku docs/architecture/29_experiment_baum_generik_und_composition_driver.md §3c–§3f
// @task V42-L-74c

#include "observable_tier.hpp"     // ComdareTierObserverSnapshotV2
#include "observer_aggregate.hpp"  // ObserverAggregate, ObservableAxis

namespace comdare::cache_engine::anatomy {

/// Füllt den flachen V2-POD aus einem ObserverAggregate. out != nullptr. Generisch über jede Composition;
/// die Felder nicht-getriebener/-observabler Achsen bleiben 0 (if-constexpr — kein Runtime-Switch).
template <IsComposition Composition>
void fill_tier_observer_v2(ObserverAggregate<Composition> const& agg,
                           ComdareTierObserverSnapshotV2* out) noexcept {
    if (out == nullptr) return;
    ComdareTierObserverSnapshotV2 s{};

    // ── search_algo (axis_03a) ──
    if constexpr (ObservableAxis<typename Composition::search_algo>) {
        s.search_lookup_count   = agg.search_algo.total_lookup_count;
        s.search_hit_count      = agg.search_algo.total_hit_count;
        s.search_miss_count     = agg.search_algo.total_miss_count;
        s.search_insert_count   = agg.search_algo.total_insert_count;
        s.search_erase_count    = agg.search_algo.total_erase_count;
        s.search_peak_occupancy = agg.search_algo.peak_occupancy;
    }
    // ── telemetry (axis_11) ──
    if constexpr (ObservableAxis<typename Composition::telemetry>) {
        s.telemetry_total_events = agg.telemetry.total_events;
        s.telemetry_leaf_updates = agg.telemetry.leaf_updates;
        s.telemetry_node_updates = agg.telemetry.node_updates;
        s.telemetry_peak_tracked = agg.telemetry.peak_tracked;
    }
    // ── memory_layout (axis_05) ──
    if constexpr (ObservableAxis<typename Composition::memory_layout>) {
        s.layout_scan_count          = agg.memory_layout.scan_count;
        s.layout_records_scanned     = agg.memory_layout.records_scanned;
        s.layout_field_bytes_read    = agg.memory_layout.field_bytes_read;
        s.layout_cache_lines_touched = agg.memory_layout.cache_lines_touched;
        s.layout_last_checksum       = agg.memory_layout.last_checksum;
    }
    // ── serialization (axis_10) ──
    if constexpr (ObservableAxis<typename Composition::serialization>) {
        s.serialization_serialize_count    = agg.serialization.serialize_count;
        s.serialization_records_serialized = agg.serialization.records_serialized;
        s.serialization_bytes_serialized   = agg.serialization.bytes_serialized;
        s.serialization_last_checksum      = agg.serialization.last_checksum;
    }
    // ── node_type (axis_04) ──
    if constexpr (ObservableAxis<typename Composition::node_type>) {
        s.node_find_count    = agg.node_type.find_count;
        s.node_keys_stored   = agg.node_type.keys_stored;
        s.node_queries_run   = agg.node_type.queries_run;
        s.node_last_checksum = agg.node_type.last_checksum;
    }

    s.observable_axis_count = ObserverAggregate<Composition>::observable_count();
    *out = s;
}

}  // namespace comdare::cache_engine::anatomy
