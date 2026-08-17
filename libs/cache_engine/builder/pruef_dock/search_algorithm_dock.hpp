#pragma once
// V41.F.6.1.R6 -- SearchAlgorithmDock: das Pruef-Dock fuer die SearchAlgorithm-Gattung (Mammal).
//
// **KEIN NEUBAU (User 2026-05-30):** Ein duenner, gattungs-typisierter Orchestrierungs-Wrapper, der die DREI
// bereits existierenden, bereits build-gruenen Zahnraeder uniform hinter IPruefDock zusammenhaelt:
//   (1) anatomy::IObservableTier  -- das ABI-stabile SearchAlgorithm-Gattungs-Antriebs-Sub-Interface (CE-Schicht),
//   (2) anatomy_loader::AnatomyModuleHandle -- das geladene Tier-Modul (Loader, CEB-Schicht),
//   (3) anatomy_cmds::drive_tier_observe_trace_abi + serialize_abi_tier_trace_csv/json -- der Fuellstands-
//       Mess-/Persistier-Treiber (CEB-Schicht).
// Erfindet NICHTS neu; der compile-time-Gattungs-Constraint bleibt unangetastet im SearchAlgorithmAbiAdapter<A>
// in der DLL (static_assert genus==SearchAlgorithm). Das Dock validiert nur runtime-seitig per genus()-Abfrage,
// dass es das richtige Modul vor sich hat, und zieht den Antrieb per dynamic_cast (bewaehrtes Pfad-B-Probing).
//
// @doku docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md Par.8.8

#include "pruef_dock.hpp"
#include <anatomy_drive/search_algorithm_drive.hpp> // K2: Drive + acquire, stufen-neutral
#include "conformance_gate.hpp"                     // V5-I4: std::map-Konformitaets-Gate vor der Messung
#include <anatomy/observable_tier.hpp>              // IObservableTier (SearchAlgorithm-Gattungs-Antrieb)
#include <anatomy/resource_controllable_tier.hpp>   // INC-2a: IResourceControllableTier (Pruef-Dock-Settings)
#include <anatomy/rollbackable_tier.hpp> // V5-I6/I7: IRollbackableTier (memento_all) fuer Zwei-Phasen-Messung
#include <anatomy/scannable_tier.hpp>    // INC-2a: IScannableTier (YCSB-E Range-Scan)
#if COMDARE_MEASUREMENT_ON
// NAHT-1: die CEB-Haelfte der Mess-Naht. Der Include steht BEWUSST im Gate -- der Header traegt
// einen #error und kann in einem OFF-Kompilat nicht existieren. Genau das ist die CEB-Seite der
// UND-Bedingung: eine funktional-only gebaute CacheEngineBuilder kann den Mess-Visitor nicht
// einmal BENENNEN, geschweige denn uebergeben.
#include "genus_mess_naht.hpp"
#endif

namespace comdare::cache_engine::builder::pruef_dock {

/// SearchAlgorithmDock -- Pruef-Dock der SearchAlgorithm-Gattung. Header-only (verkabelt inline-Funktionen).
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
        (void)opts;
        (void)out_csv;
        (void)out_json;
        return dock_status_mess_deaktiviert;
#else
        // Pfad-B-Probing (bewaehrtes Muster, vgl. IMeasurableWorkload): das gattungs-eigene Antriebs-
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
        // V5-I4: Konformitaets-Gate gegen std::map VOR der Messung -- eine nicht-konforme Huelle wird NICHT gemessen
        // (Reihenfolge import -> GATE -> messen). tier IS-A IDriveableTier (Split); das Gate leert den Tier am Ende
        // -> saubere Ausgangslage fuer die anschliessende Fuellstands-Messung.
        if (!run_conformance_gate(*tier).passed()) return dock_status_conformance_failed;
        // V5-I6/I7: memento_all-Sub-Interface derselben Tier-Instanz ziehen (nullbar). Vorhanden -> der Treiber
        // misst je Op ZWEI-PHASIG (save->warmup->rollback->measure, Mess-Architektur Par.4 Default); nullptr (altes
        // Modul / nicht-rollbackbares Organ) -> der Treiber faellt intern auf Einphasen-Kalt-Messung zurueck =
        // exakt das bisherige Verhalten. Eine RAII-untaugliche Slot-Aliasing-Gefahr besteht nicht (gleiche Instanz).
        auto*      rollback = dynamic_cast<anatomy::IRollbackableTier*>(base);
        auto const trace    = anatomy_cmds::drive_two_phase_tier_trace_abi(*tier, rollback, opts);
        out_csv             = anatomy_cmds::serialize_abi_tier_trace_csv(trace);
        out_json            = anatomy_cmds::serialize_abi_tier_trace_json(trace);
        return dock_status_ok;
#endif // COMDARE_MEASUREMENT_ON
    }
};

// K2-WANDERUNG 17.08.2026 (Owner-Entscheid 09.08.): SearchAlgorithmDrive und
// acquire_search_algorithm_drive liegen jetzt in anatomy_drive/search_algorithm_drive.hpp --
// stufen-neutral, damit die Hybrid-Stufe ihr Ziel auf DIESELBE Art anfasst wie dieses Dock
// (Owner: "technisch identisch bei Konfiguration"). Die Alternative waere eine zweite,
// eigene Antriebs-Beschaffung in der Hybrid-Stufe gewesen -- zwei Wege zum selben Ziel, die
// still auseinanderlaufen.
// Hier bleibt der Re-Export: Signatur, Semantik und Fehlerpfade sind UNVERAENDERT, und die
// drei Bestands-Aufrufer (pruef_only.hpp, cache_engine_builder_iterator.hpp und dieses Dock)
// sind nicht angefasst worden.
using ::comdare::cache_engine::anatomy_drive::acquire_search_algorithm_drive;
using ::comdare::cache_engine::anatomy_drive::SearchAlgorithmDrive;

} // namespace comdare::cache_engine::builder::pruef_dock
