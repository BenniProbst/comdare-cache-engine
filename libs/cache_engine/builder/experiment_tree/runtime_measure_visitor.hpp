#pragma once
// D13 / L-MEAS (2026-06-02) — RuntimeMeasureVisitor: verbindet den RuntimeVariableLoop (KF-7, dyn. Variablen-
// Kartesik auf EINER geladenen Binary) mit der MESSUNG. Der bestehende Loop wendet je Einstellung nur die
// Resource-Control an — er MISST nichts. Dieser Visitor schließt die Lücke: je dynamischer Einstellung fährt er
// einen Workload (n_ops insert+lookup über IObservableTier) + zieht tier_observe, je `repeats` (Default 3, KF-10 —
// separat, nie interpoliert), OHNE Neu-Laden der Binary (Doc 26 §2). Ergebnis: korrelierte (Setting × Wdh →
// Observer)-Punkte je Binary. Testdaten RAM-resident, <1s je Binary (Doc 28 §5). Header-only, C++23.

#include "runtime_variable_loop.hpp"         // RuntimeVariableLoop / RuntimeSetting / DynamicDim
#include "experiment_tree.hpp"               // NodeObserverSnapshot
#include "../../anatomy/observable_tier.hpp" // IObservableTier + ComdareTierObserverSnapshot (I1)

#include <cstdint>
#include <string>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// Ein gemessener dynamischer Blatt-Punkt: dyn. Einstellung × Wiederholung → korrelierter Observer-Snapshot.
struct RuntimeMeasurePoint {
    std::string          setting_label;          // dyn. Belegung "axis.var=value/…"
    std::uint64_t        repeat_index       = 0; // 0..repeats-1 (separate Roh-Messung, KF-10)
    std::uint64_t        applied_axis_count = 0; // real angewandte Resource-Control-Achsen
    NodeObserverSnapshot observer{};             // tier_observe-Ergebnis (echt getrieben)
};

/// RuntimeMeasureVisitor — die host-seitige Mess-Schleife je geladener Binary. TierT erfüllt
/// IResourceControllableTier (für den Loop) UND IObservableTier (tier_insert/lookup/observe).
class RuntimeMeasureVisitor {
public:
    explicit RuntimeMeasureVisitor(anatomy::ComdareResourceControlV1 env_limits = {}) noexcept : env_{env_limits} {}

    template <class TierT>
    [[nodiscard]] std::vector<RuntimeMeasurePoint> measure(TierT& tier, std::vector<DynamicDim> const& dims,
                                                           std::uint64_t n_ops, std::uint64_t repeats = 3) const {
        std::vector<RuntimeMeasurePoint> points;
        RuntimeVariableLoop              loop{env_};
        loop.run(tier, dims, [&](RuntimeSetting const& s) {
            for (std::uint64_t r = 0; r < repeats; ++r) {
                // Workload je Einstellung (RAM-resident, <1s): insert + lookup über die geladene Binary (kein Reload).
                for (std::uint64_t i = 0; i < n_ops; ++i) (void)tier.tier_insert(i, i * 7u + 1u);
                for (std::uint64_t i = 0; i < n_ops; ++i) {
                    std::uint64_t v = 0;
                    (void)tier.tier_lookup(i, &v);
                }
                anatomy::ComdareTierObserverSnapshot pod{};
                tier.tier_observe(&pod); // echter Observer-Snapshot (korreliert mit der Einstellung)
                RuntimeMeasurePoint p;
                p.setting_label      = s.setting_label;
                p.repeat_index       = r;
                p.applied_axis_count = s.applied_axis_count;
                // I1: V1-Projektion aus dem konsolidierten POD (search→axis_stats[0], alloc→axis_stats[6]).
                p.observer.search_lookup_count      = pod.axis_stats[0][0];
                p.observer.search_hit_count         = pod.axis_stats[0][1];
                p.observer.search_miss_count        = pod.axis_stats[0][2];
                p.observer.search_insert_count      = pod.axis_stats[0][3];
                p.observer.search_erase_count       = pod.axis_stats[0][4];
                p.observer.search_peak_occupancy    = pod.axis_stats[0][5];
                p.observer.alloc_bytes_allocated    = pod.axis_stats[6][0];
                p.observer.alloc_bytes_in_use       = pod.axis_stats[6][1];
                p.observer.alloc_allocation_count   = pod.axis_stats[6][2];
                p.observer.alloc_deallocation_count = pod.axis_stats[6][3];
                p.observer.alloc_failure_count      = pod.axis_stats[6][4];
                p.observer.observable_axis_count    = pod.observable_axis_count;
                p.observer.tier_fill_level          = pod.tier_fill_level;
                points.push_back(std::move(p));
            }
        });
        return points;
    }

private:
    anatomy::ComdareResourceControlV1 env_;
};

} // namespace comdare::cache_engine::builder::experiment
