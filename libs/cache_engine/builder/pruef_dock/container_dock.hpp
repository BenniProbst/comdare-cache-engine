#pragma once
// Container-Gattungs-Prüf-Dock (2026-06-02, Doc 24 §8.8, User-Option-B Schritt 3) — die CacheEngineBuilder-Seite
// für die CONTAINER-Gattung (Adapter), analog SearchAlgorithmDock für die SearchAlgorithm-Gattung.
//
// Doc 24 §8.8: ein Prüf-Dock ist der per-Gattung ABI-stabile Mess-Übergang — es (a) hält/treibt ein Tier der
// Gattung über die Gattungs-API, (b) misst dessen eingebaute Observer, (c) persistiert. Hier in-process über die
// ContainerAnatomy (put/get-Workload → ContainerObserverSnapshot → CSV). Gattungs-Constraint: genus()==Adapter
// (Cross-Genus type-unmöglich, Doku 14 §32). Der DLL-Pfad (AnatomyModuleLoader) ist analog BR-4 ein Folgeschritt.
// C++23, header-only.

#include "anatomy/container_anatomy.hpp"   // ContainerAnatomy / ContainerObserverSnapshot
#include "anatomy/anatomy_base.hpp"        // AnatomyGenus

#include <cstdint>
#include <string>

namespace comdare::cache_engine::builder::pruef_dock {

/// ContainerDock<ContainerAnatomyT> — treibt ein Container-Tier (Queue) über die Gattungs-API + misst den
/// eingebauten Container-Observer + persistiert. Per-Gattung gebunden (genus()==Adapter).
template <class ContainerAnatomyT>
class ContainerDock {
public:
    struct MeasureResult {
        ::comdare::cache_engine::anatomy::ContainerObserverSnapshot observer{};
        std::uint64_t total_ops = 0;
    };

    /// Diese Dock-Seite bedient die Container/Adapter-Gattung (Doc 24 §8.8 Gattungs-Bindung).
    [[nodiscard]] static constexpr ::comdare::cache_engine::anatomy::AnatomyGenus genus() noexcept {
        return ::comdare::cache_engine::anatomy::AnatomyGenus::Adapter;
    }

    /// Treibt das Container-Tier mit n_puts put + n_gets get (Zustands-Manipulation, Doku 24 §8.7b) und zieht
    /// den eingebauten Container-Observer. Reines in-process-Treiben BEIDER Organe (Q1 buffer + Q2 flush).
    /// capacity > 0 aktiviert die Q2-Flush-Policy (Bezugsgröße); 0 (Default) = kein Auto-Flush.
    [[nodiscard]] MeasureResult measure(std::uint64_t n_puts, std::uint64_t n_gets,
                                        std::size_t capacity = 0) const {
        ContainerAnatomyT tier(capacity);
        for (std::uint64_t i = 0; i < n_puts; ++i)
            tier.put(static_cast<typename ContainerAnatomyT::element_type>(i));
        for (std::uint64_t i = 0; i < n_gets; ++i) (void)tier.get();
        return MeasureResult{ tier.observe_all(), n_puts + n_gets };
    }

    /// Persistierung (Doc 24 §8.8 Schritt c): eine CSV-Zeile mit den korrelierten Container-Observer-Werten
    /// (Q1 buffer + Q2 flush). Die Flush-Felder stehen am Zeilenende (stabile Präfix-Spalten für Bestands-Parser).
    [[nodiscard]] static std::string serialize_csv(MeasureResult const& r) {
        std::string s = "genus,total_ops,put_count,get_count,peak_occupancy,current_occupancy,"
                        "flush_decisions,full_flush,flush_complete\n";
        s += "Container," + std::to_string(r.total_ops) + ","
           + std::to_string(r.observer.put_count) + "," + std::to_string(r.observer.get_count) + ","
           + std::to_string(r.observer.peak_occupancy) + "," + std::to_string(r.observer.current_occupancy) + ","
           + std::to_string(r.observer.flush_decisions_evaluated) + "," + std::to_string(r.observer.full_flush_count) + ","
           + std::to_string(r.observer.flush_complete_count) + "\n";
        return s;
    }
};

}  // namespace comdare::cache_engine::builder::pruef_dock
