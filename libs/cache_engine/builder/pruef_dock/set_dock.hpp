#pragma once
// L-76a (2026-06-03, Doc 24 §8.8) — Set-Gattungs-Prüf-Dock: die CacheEngineBuilder-Seite für die SET-Gattung
// (Vogel, genus()==Set), analog AdapterDock für Adapter + SearchAlgorithmDock für SearchAlgorithm.
//
// Doc 24 §8.8: ein Prüf-Dock ist der per-Gattung Mess-Übergang — es (a) hält/treibt ein Tier der Gattung über die
// Gattungs-API, (b) misst dessen eingebauten Observer, (c) persistiert. Hier in-process über die SetAnatomy
// (insert/contains/erase-Workload → SetObserverSnapshot → CSV). Gattungs-Constraint: genus()==Set (Cross-Genus
// type-unmöglich, Doku 14 §32). Der DLL-Pfad (AnatomyModuleLoader, analog BR-4 + IPruefDock+AnatomyModuleHandle)
// ist ein Folgeschritt — exakt wie bei AdapterDock dokumentiert. C++23, header-only.
//
// @related [[anatomie-gattungen]] [[gattungs-constraint-pruefling-merge]]

#include "anatomy/set_anatomy.hpp"  // SetAnatomy / SetObserverSnapshot
#include "anatomy/anatomy_base.hpp" // AnatomyGenus

#include <cstdint>
#include <string>

namespace comdare::cache_engine::builder::pruef_dock {

/// SetDock<SetAnatomyT> — treibt ein Set-Tier (Menge) über die Gattungs-API (insert/contains/erase) + misst den
/// eingebauten Set-Observer + persistiert. Per-Gattung gebunden (genus()==Set).
template <class SetAnatomyT>
class SetDock {
public:
    struct MeasureResult {
        ::comdare::cache_engine::anatomy::SetObserverSnapshot observer{};
        std::uint64_t                                         total_ops = 0;
    };

    /// Diese Dock-Seite bedient die Set-Gattung (Doc 24 §8.8 Gattungs-Bindung).
    [[nodiscard]] static constexpr ::comdare::cache_engine::anatomy::AnatomyGenus genus() noexcept {
        return ::comdare::cache_engine::anatomy::AnatomyGenus::Set;
    }

    /// Treibt das Set-Tier (Zustands-Manipulation, Doku 24 §8.7b): erst n_inserts insert(0..n_inserts-1) (alle neu),
    /// dann n_contains contains(0..n_contains-1) (i<n_inserts = Hit, sonst Miss → gemischte Trefferquote), dann
    /// n_erases erase(0..n_erases-1). Zieht den eingebauten Set-Observer über das echte search_algo-Kern-Organ.
    [[nodiscard]] MeasureResult measure(std::uint64_t n_inserts, std::uint64_t n_contains,
                                        std::uint64_t n_erases) const {
        SetAnatomyT tier;
        for (std::uint64_t i = 0; i < n_inserts; ++i) (void)tier.insert(i);
        for (std::uint64_t i = 0; i < n_contains; ++i) (void)tier.contains(i); // i<n_inserts = Hit, sonst Miss
        for (std::uint64_t i = 0; i < n_erases; ++i) (void)tier.erase(i);
        return MeasureResult{tier.observe_all(), n_inserts + n_contains + n_erases};
    }

    /// Persistierung (Doc 24 §8.8 Schritt c): eine CSV-Zeile mit den korrelierten Set-Observer-Werten.
    [[nodiscard]] static std::string serialize_csv(MeasureResult const& r) {
        std::string s = "genus,total_ops,insert_count,contains_count,contains_hit,contains_miss,"
                        "erase_count,current_size,peak_size\n";
        s += "Set," + std::to_string(r.total_ops) + "," + std::to_string(r.observer.insert_count) + "," +
             std::to_string(r.observer.contains_count) + "," + std::to_string(r.observer.contains_hit_count) + "," +
             std::to_string(r.observer.contains_miss_count) + "," + std::to_string(r.observer.erase_count) + "," +
             std::to_string(r.observer.current_size) + "," + std::to_string(r.observer.peak_size) + "\n";
        return s;
    }
};

} // namespace comdare::cache_engine::builder::pruef_dock
