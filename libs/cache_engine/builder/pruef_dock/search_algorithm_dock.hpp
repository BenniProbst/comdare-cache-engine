#pragma once
// V41.F.6.1.R6 — SearchAlgorithmDock: das Prüf-Dock für die SearchAlgorithm-Gattung (Mammal).
//
// **KEIN NEUBAU (User 2026-05-30):** Ein dünner, gattungs-typisierter Orchestrierungs-Wrapper, der die DREI
// bereits existierenden, bereits build-grünen Zahnräder uniform hinter IPruefDock zusammenhält:
//   (1) anatomy::IObservableTier  — das ABI-stabile SearchAlgorithm-Gattungs-Antriebs-Sub-Interface (CE-Schicht),
//   (2) anatomy_loader::AnatomyModuleHandle — das geladene Tier-Modul (Loader, CEB-Schicht),
//   (3) anatomy_cmds::drive_tier_observe_trace_abi + serialize_abi_tier_trace_csv/json — der Füllstands-
//       Mess-/Persistier-Treiber (CEB-Schicht).
// Erfindet NICHTS neu; der compile-time-Gattungs-Constraint bleibt unangetastet im SearchAlgorithmAbiAdapter<A>
// in der DLL (static_assert genus==SearchAlgorithm). Das Dock validiert nur runtime-seitig per genus()-Abfrage,
// dass es das richtige Modul vor sich hat, und zieht den Antrieb per dynamic_cast (bewährtes Pfad-B-Probing).
//
// @doku docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md §8.8

#include "pruef_dock.hpp"
#include <anatomy/observable_tier.hpp>   // IObservableTier (SearchAlgorithm-Gattungs-Antrieb)

namespace comdare::cache_engine::builder::pruef_dock {

/// SearchAlgorithmDock — Prüf-Dock der SearchAlgorithm-Gattung. Header-only (verkabelt inline-Funktionen).
class SearchAlgorithmDock final : public IPruefDock {
public:
    [[nodiscard]] anatomy::AnatomyGenus dock_genus() const noexcept override {
        return anatomy::AnatomyGenus::SearchAlgorithm;
    }

    [[nodiscard]] std::string_view dock_name() const noexcept override { return "SearchAlgorithmDock"; }

    [[nodiscard]] bool accepts(anatomy_loader::AnatomyModuleHandle const& h) const noexcept override {
        return h.anatomy() != nullptr && h.anatomy()->genus() == dock_genus();
    }

    [[nodiscard]] int measure(anatomy_loader::AnatomyModuleHandle& h,
                              PruefDockMeasureOptions const& opts,
                              std::string& out_csv, std::string& out_json) override {
        auto* base = h.anatomy();
        if (base == nullptr)                  return dock_status_no_anatomy;
        if (base->genus() != dock_genus())    return dock_status_wrong_genus;
        // Pfad-B-Probing (bewährtes Muster, vgl. IMeasurableWorkload): das gattungs-eigene Antriebs-
        // Sub-Interface aus IAnatomyBase ziehen. nullptr = altes Modul ohne Pfad B → sauber degradieren.
        auto* tier = dynamic_cast<anatomy::IObservableTier*>(base);
        if (tier == nullptr)                  return dock_status_subinterface_missing;
        // Der bestehende, unveränderte Füllstands-Treiber: r/w/d-Wall-Clock + tier_observe-POD Wall-Clock-korreliert.
        auto const trace = anatomy_cmds::drive_tier_observe_trace_abi(*tier, opts);
        out_csv  = anatomy_cmds::serialize_abi_tier_trace_csv(trace);
        out_json = anatomy_cmds::serialize_abi_tier_trace_json(trace);
        return dock_status_ok;
    }
};

}  // namespace comdare::cache_engine::builder::pruef_dock
