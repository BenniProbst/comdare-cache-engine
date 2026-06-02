#pragma once
// D14 / L-CLUSTER-E2E (gate-frei, 2026-06-02) — perm_runner: der lokale Mess-Runner je Binary. Auf dem Cluster
// fährt jede SLURM-Task EINEN perm_runner mit EINER geladenen perm-DLL (AnatomyModuleLoader), treibt den
// Mess-Workload über das Tier-Sub-Interface, zieht den Observer und emittiert EINE Ergebnis-Zeile im
// result_ingest-Format (binary_id + 13 Felder). Schließt die gate-freie Kette: perm_runner (Mess→Format) ↔
// result_ingest (Format→Baum-NodeValue). LOKAL verifizierbar (Mock/in-process); echte Cluster-Submission = GATE-MAXIMAL.
//
// Format-Identität mit result_ingest.hpp garantiert den Round-Trip (Mess → Zeile → Ingest → identischer NodeValue).

#include "experiment_tree.hpp"                      // NodeObserverSnapshot
#include "../../anatomy/observable_tier.hpp"        // IObservableTier + ComdareTierObserverSnapshotV1

#include <cstdint>
#include <string>

namespace comdare::cache_engine::builder::experiment {

/// Formatiert einen Observer-Snapshot als result_ingest-Zeile (binary_id + 13 ';'-Felder, NodeObserverSnapshot-
/// Reihenfolge). EXAKT das von ingest_result_line erwartete Format → Round-Trip-Garant.
[[nodiscard]] inline std::string format_perm_result(std::string const& binary_id,
                                                    anatomy::ComdareTierObserverSnapshotV1 const& s) {
    std::string out = binary_id;
    auto add = [&](std::uint64_t v) { out += ';'; out += std::to_string(v); };
    add(s.search_lookup_count);   add(s.search_hit_count);       add(s.search_miss_count);
    add(s.search_insert_count);   add(s.search_erase_count);     add(s.search_peak_occupancy);
    add(s.alloc_bytes_allocated); add(s.alloc_bytes_in_use);     add(s.alloc_allocation_count);
    add(s.alloc_deallocation_count); add(s.alloc_failure_count); add(s.observable_axis_count);
    add(s.tier_fill_level);
    return out;
}

/// Treibt ein geladenes IObservableTier (SearchAlgorithm-Mess-Pfad): n_ops insert + n_ops lookup, zieht
/// tier_observe und liefert die result_ingest-Zeile. Der host-/cluster-seitige Unikat-Mess-Lauf je Binary.
[[nodiscard]] inline std::string run_observable_perm(anatomy::IObservableTier& tier,
                                                     std::string const& binary_id, std::uint64_t n_ops) {
    for (std::uint64_t i = 0; i < n_ops; ++i) (void)tier.tier_insert(i, i * 7u + 1u);
    for (std::uint64_t i = 0; i < n_ops; ++i) { std::uint64_t v = 0; (void)tier.tier_lookup(i, &v); }
    anatomy::ComdareTierObserverSnapshotV1 snap{};
    tier.tier_observe(&snap);
    return format_perm_result(binary_id, snap);
}

}  // namespace comdare::cache_engine::builder::experiment
