#pragma once
// V41.F.6.1.R6 — PruefDockRegistry: Config-getriebene per-Gattung Dock-Auswahl (Doku 24 §8.8).
//
// Hält genau EIN Prüf-Dock pro implementierter Gattung (initial nur SearchAlgorithmDock; Set/Sequence/
// Adapter/View kommen mit V42). select_for(handle) liefert das erste Dock, dessen Gattung zur IM MODUL
// deklarierten Gattung (anatomy()->genus()) passt — KEINE Dateinamen-Heuristik. Damit kann ein Mess-Lauf
// mehrere Gattungen QUER messen (jedes Handle zieht sein passendes Dock); der Default ist sequentiell-pro-
// Gattung (die Sortierung der Handles nach Gattung erfolgt builder-seitig im Mess-Treiber, separater Increment).
//
// @doku docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md §8.8

#include "pruef_dock.hpp"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::pruef_dock {

/// PruefDockRegistry — builder-seitige Registry der verfügbaren Prüf-Docks (eines je Gattung).
class PruefDockRegistry {
public:
    /// Registriert ein Dock (Ownership wandert in die Registry).
    void register_dock(std::unique_ptr<IPruefDock> dock) { docks_.push_back(std::move(dock)); }

    /// Liefert das erste Dock, das das geladene Modul akzeptiert (per im Modul deklarierter Gattung), sonst nullptr.
    [[nodiscard]] IPruefDock* select_for(anatomy_loader::AnatomyModuleHandle const& h) const noexcept {
        for (auto const& d : docks_) {
            if (d->accepts(h)) return d.get();
        }
        return nullptr;
    }

    /// Liefert das Dock für eine konkrete Gattung (oder nullptr), unabhängig von einem geladenen Modul.
    [[nodiscard]] IPruefDock* dock_for_genus(anatomy::AnatomyGenus g) const noexcept {
        for (auto const& d : docks_) {
            if (d->dock_genus() == g) return d.get();
        }
        return nullptr;
    }

    [[nodiscard]] std::size_t size() const noexcept { return docks_.size(); }

private:
    std::vector<std::unique_ptr<IPruefDock>> docks_;
};

}  // namespace comdare::cache_engine::builder::pruef_dock
