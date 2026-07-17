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
#include "conformance_gate.hpp"                   // V5-I4: std::map-Konformitäts-Gate vor der Messung
#include <anatomy/observable_tier.hpp>            // IObservableTier (SearchAlgorithm-Gattungs-Antrieb)
#include <anatomy/resource_controllable_tier.hpp> // INC-2a: IResourceControllableTier (Prüf-Dock-Settings)
#include <anatomy/rollbackable_tier.hpp>          // V5-I6/I7: IRollbackableTier (memento_all) für Zwei-Phasen-Messung
#include <anatomy/scannable_tier.hpp>             // INC-2a: IScannableTier (YCSB-E Range-Scan)

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

    [[nodiscard]] int measure(anatomy_loader::AnatomyModuleHandle& h, PruefDockMeasureOptions const& opts,
                              std::string& out_csv, std::string& out_json) override {
        auto* base = h.anatomy();
        if (base == nullptr) return dock_status_no_anatomy;
        if (base->genus() != dock_genus()) return dock_status_wrong_genus;
        // Pfad-B-Probing (bewährtes Muster, vgl. IMeasurableWorkload): das gattungs-eigene Antriebs-
        // Sub-Interface aus IAnatomyBase ziehen. nullptr = altes Modul ohne Pfad B → sauber degradieren.
        auto* tier = dynamic_cast<anatomy::IObservableTier*>(base);
        if (tier == nullptr) return dock_status_subinterface_missing;
        // V5-I4: Konformitäts-Gate gegen std::map VOR der Messung — eine nicht-konforme Hülle wird NICHT gemessen
        // (Reihenfolge import → GATE → messen). tier IS-A IDriveableTier (Split); das Gate leert den Tier am Ende
        // → saubere Ausgangslage für die anschließende Füllstands-Messung.
        if (!run_conformance_gate(*tier).passed()) return dock_status_conformance_failed;
        // V5-I6/I7: memento_all-Sub-Interface derselben Tier-Instanz ziehen (nullbar). Vorhanden → der Treiber
        // misst je Op ZWEI-PHASIG (save→warmup→rollback→measure, Mess-Architektur §4 Default); nullptr (altes
        // Modul / nicht-rollbackbares Organ) → der Treiber fällt intern auf Einphasen-Kalt-Messung zurück =
        // exakt das bisherige Verhalten. Eine RAII-untaugliche Slot-Aliasing-Gefahr besteht nicht (gleiche Instanz).
        auto*      rollback = dynamic_cast<anatomy::IRollbackableTier*>(base);
        auto const trace    = anatomy_cmds::drive_two_phase_tier_trace_abi(*tier, rollback, opts);
        out_csv             = anatomy_cmds::serialize_abi_tier_trace_csv(trace);
        out_json            = anatomy_cmds::serialize_abi_tier_trace_json(trace);
        return dock_status_ok;
    }
};

/// INC-2a (Q4, Prüf-Dock scharf): das dock-vertragliche Antriebs-Bündel der SearchAlgorithm-Gattung.
/// obs = Mess-Antrieb (Pflicht fuer Messung), ctrl/rbk/scn = optionale Sub-Antriebe (alte DLLs → nullptr).
struct SearchAlgorithmDrive {
    anatomy::IObservableTier*           obs  = nullptr;
    anatomy::IResourceControllableTier* ctrl = nullptr;
    anatomy::IRollbackableTier*         rbk  = nullptr;
    anatomy::IScannableTier*            scn  = nullptr;
};

/// INC-2a (Q4): die EINE dock-vertragliche Antriebs-Beschaffung — ersetzt die rohen dynamic_cast-
/// Bypaesse des Lazy-Iterators (cache_engine_builder_iterator). Semantik BEWUSST identisch zum
/// bisherigen Iterator-Verhalten (kein Gattungs-Reject hier — der scharfe Gattungs-Match kommt mit
/// den Multi-Gattungs-Docks in INC-2d/2e ueber accepts()): base fehlt → no_anatomy; Mess-Antrieb
/// fehlt → subinterface_missing; sonst ok mit vollem Buendel.
[[nodiscard]] inline int acquire_search_algorithm_drive(anatomy_loader::AnatomyModuleHandle& h,
                                                        SearchAlgorithmDrive&                out) noexcept {
    anatomy::IAnatomyBase* base = h.anatomy();
    if (base == nullptr) return dock_status_no_anatomy;
    out.obs  = dynamic_cast<anatomy::IObservableTier*>(base);
    out.ctrl = dynamic_cast<anatomy::IResourceControllableTier*>(base);
    out.rbk  = dynamic_cast<anatomy::IRollbackableTier*>(base);
    out.scn  = dynamic_cast<anatomy::IScannableTier*>(base);
    return (out.obs == nullptr) ? dock_status_subinterface_missing : dock_status_ok;
}

} // namespace comdare::cache_engine::builder::pruef_dock
