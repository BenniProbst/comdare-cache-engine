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
#if COMDARE_MEASUREMENT_ON
// NAHT-1: die CEB-Haelfte der Mess-Naht. Der Include steht BEWUSST im Gate -- der Header traegt
// einen #error und kann in einem OFF-Kompilat nicht existieren. Genau das ist die CEB-Seite der
// UND-Bedingung: eine funktional-only gebaute CacheEngineBuilder kann den Mess-Visitor nicht
// einmal BENENNEN, geschweige denn uebergeben.
#include "genus_mess_naht.hpp"
#endif

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
#if !COMDARE_MEASUREMENT_ON
        // NAHT-1, DIE CEB-HAELFTE DER UND-BEDINGUNG: diese CacheEngineBuilder ist funktional-only
        // uebersetzt. Es gibt hier keinen Mess-Visitor -- der Typ existiert in diesem Kompilat nicht.
        // Also wird KEINER uebergeben, und es wird NICHT gemessen. Frueher lief der Mess-Versuch in
        // der CEB IMMER, unabhaengig vom eigenen Schalter; das war die fehlende Haelfte, wegen der
        // die Aktivierung einseitig blieb.
        // FLAECHE 3 / KON25-02-DECKUNG: dieser Zweig ist der Zustand CEB=AUS/Tier=beliebig des
        // measurement-Durchstichs. Uebersetzt und am Objekt belegt wird er von
        // tests/unit/test_flaeche3_deckung_ceb_aus_tier_an.cpp (vorher: von KEINEM Testziel).
        (void)opts;
        (void)out_csv;
        (void)out_json;
        return dock_status_mess_deaktiviert;
#else
        // Pfad-B-Probing (bewährtes Muster, vgl. IMeasurableWorkload): das gattungs-eigene Antriebs-
        // Sub-Interface aus IAnatomyBase ziehen.
        auto* tier = dynamic_cast<anatomy::IObservableTier*>(base);
        // NAHT-1: DIE WEICHE VOR DER MESSUNG. Bis hierher hiess `tier == nullptr` schlicht
        // "sauber degradieren" -- und genau dieses stille Degrade liess eine Reihe ehrlicher Nullen
        // wie eine Messung aussehen. Jetzt entscheidet der Abgleich Legende <-> Probe:
        //   Widerspruch          -> DEFEKT (5), hart. Nie ein gruenes Degrade.
        //   ehrlich deaktiviert  -> (6), unterscheidbar von einem Defekt.
        //   Flaeche fehlt, keine Legende -> (3) wie bisher (Alt-DLL; Wissensluecke, kein Defekt).
        switch (mess_gate_befund(tier != nullptr, opts.mess_legende_erwartung >= 0, opts.mess_legende_erwartung == 1)) {
            case MessGateBefund::Widerspruch: return dock_status_mess_gate_mismatch;
            case MessGateBefund::Tier_aus: return dock_status_mess_deaktiviert;
            case MessGateBefund::Flaeche_fehlt_stumm: return dock_status_subinterface_missing;
            case MessGateBefund::Beidseitig_an: break; // beide Seiten an -> der Visitor darf queren
        }
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
#endif // COMDARE_MEASUREMENT_ON
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
