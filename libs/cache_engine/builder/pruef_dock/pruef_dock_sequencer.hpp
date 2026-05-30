#pragma once
// V41.F.6.1.R6 — Prüf-Dock-Sequenzierung: Default sequentiell-pro-Gattung (Doku 24 §8.8).
//
// Realisiert die vom User (2026-05-30) beschriebene Default-Mess-Ordnung: "Binaries derselben Gattung
// werden sequentiell durchgemessen, gefolgt von der Sequenz der nachfolgenden Gattung". Die geladenen
// Module-Handles werden STABIL nach der IM MODUL deklarierten Gattung (anatomy()->genus()) gruppiert
// (gleiche Gattung zusammen; die Cartesian-Ordnung INNERHALB einer Gattung — z. B. Allokator-Varianten auf
// demselben Such-Algo — bleibt erhalten), dann wird je Handle das passende Prüf-Dock gewählt + gemessen.
// Cross-genus ist implizit möglich: jedes Handle zieht über die Registry sein eigenes, gattungs-passendes
// Dock. KEIN Hot-Path, reine builder-seitige Orchestrierung VOR dem Workload.
//
// Bewusst NICHT in den Legacy-ExperimentDriver-phase5 verdrahtet (der nutzt die ALTE module_loader-Welt,
// nicht anatomy_module_loader — Blueprint-Risiko "zwei Loader-Welten"). Dieser Sequenzierer arbeitet rein
// über anatomy_loader::AnatomyModuleHandle (die Pfad-B-/Prüf-Dock-Welt).
//
// @doku docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md §8.8

#include "pruef_dock_registry.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::pruef_dock {

/// Ergebnis EINER Modul-Messung durch ein Prüf-Dock.
struct PruefDockResult {
    std::string           dock_name{};                          // z. B. "SearchAlgorithmDock" ("" wenn kein Dock)
    anatomy::AnatomyGenus genus = anatomy::AnatomyGenus::SearchAlgorithm;
    int                   status = dock_status_no_anatomy;
    std::string           csv{};
    std::string           json{};
};

namespace detail {
/// Sortier-Schlüssel: im-Modul-deklarierte Gattung; anatomie-lose Handles ans Ende (0xFF).
[[nodiscard]] inline std::uint16_t genus_sort_key(anatomy_loader::AnatomyModuleHandle const& h) noexcept {
    return h.anatomy() ? static_cast<std::uint16_t>(h.anatomy()->genus()) : std::uint16_t{0xFF};
}
}  // namespace detail

/// Misst eine Menge geladener Module GATTUNGS-SEQUENTIELL (Doku 24 §8.8 Default). Die Handles werden
/// IN-PLACE stabil nach Gattung gruppiert; je Handle das passende Prüf-Dock gewählt + gemessen.
[[nodiscard]] inline std::vector<PruefDockResult>
measure_genus_sequential(PruefDockRegistry const& registry,
                         std::vector<anatomy_loader::AnatomyModuleHandle>& handles,
                         PruefDockMeasureOptions const& opts) {
    std::stable_sort(handles.begin(), handles.end(),
        [](anatomy_loader::AnatomyModuleHandle const& a, anatomy_loader::AnatomyModuleHandle const& b) noexcept {
            return detail::genus_sort_key(a) < detail::genus_sort_key(b);
        });

    std::vector<PruefDockResult> results;
    results.reserve(handles.size());
    for (auto& h : handles) {
        PruefDockResult r;
        if (h.anatomy() != nullptr) r.genus = h.anatomy()->genus();
        IPruefDock* dock = registry.select_for(h);
        if (dock == nullptr) {
            r.status = dock_status_no_anatomy;   // kein passendes Dock (Gattung nicht unterstützt / kein Anatomie)
            results.push_back(std::move(r));
            continue;
        }
        r.dock_name = dock->dock_name();
        r.status    = dock->measure(h, opts, r.csv, r.json);
        results.push_back(std::move(r));
    }
    return results;
}

}  // namespace comdare::cache_engine::builder::pruef_dock
