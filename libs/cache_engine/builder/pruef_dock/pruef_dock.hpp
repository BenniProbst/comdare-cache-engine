#pragma once
// V41.F.6.1.R6 — Prüf-Dock: der builder-seitige, per-Gattung ABI-stabile Mess-Übergang (Doku 24 §8.8).
//
// **KEIN NEUBAU — nur Benennung (User-Direktive 2026-05-30):** Ein Prüf-Dock benennt + organisiert die
// BEREITS EXISTIERENDE host-seitige ABI-Mess-Schnittstelle für zu prüfende ABI-stabile Algorithmus-Binaries.
// Pro Anatomie-Gattung (AnatomyGenus: SearchAlgorithm/Set/Sequence/Adapter/View — Lebewesen→Tiere; Viren =
// Graphen ohne Anatomie) gibt es genau EIN Prüf-Dock auf der CacheEngineBuilder-Seite. Die CacheEngineBuilder
// besitzt den vollen Lebenszyklus: Anatomie-Konfiguration + Compile (Binary-Tier-Modul) + Prüf-Dock-Messung.
//
// IPruefDock ist KEINE ABI-Grenze (lebt nur im Builder-Binary, eine vtable, NICHT im Hot-Path — der Hot-Path
// ist das compile-time-monomorphisierte Adapter-Tier IN der DLL). Die ABI-Grenze bleibt das gattungs-eigene
// Antriebs-Sub-Interface (für SearchAlgorithm: anatomy::IObservableTier) + der POD-Snapshot
// (ComdareTierObserverSnapshotV1). Neue Gattung = neues Dock + neues Sub-Interface + neuer V1-POD — NIE eine
// Mutation von IAnatomyBase oder dem bestehenden Snapshot (vtable-/Layout-Bruch alter DLLs).
//
// @doku docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md §8.8
// @related [[anatomie-gattungen]] [[execution-engine-als-wurzel]] [[gattungs-constraint-pruefling-merge]]

#include <anatomy/anatomy_base.hpp>                                  // AnatomyGenus, IAnatomyBase::genus()
#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>   // AnatomyModuleHandle
#include <builder/anatomy_commands/tier_observe_trace_abi.hpp>       // AbiTierTraceConfig (Mess-Optionen-Reuse)

#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::pruef_dock {

namespace anatomy        = ::comdare::cache_engine::anatomy;
namespace anatomy_loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace anatomy_cmds   = ::comdare::cache_engine::builder::anatomy_commands;

/// Mess-Optionen eines Prüf-Docks — wiederverwendet die bestehende Treiber-Config (kein Neubau).
using PruefDockMeasureOptions = anatomy_cmds::AbiTierTraceConfig;

// errno-Stil Status-Codes (0 = ok), analog Loader + Hybrid-Search-Interface ([[hybrid-search-engine-interface]]).
inline constexpr int dock_status_ok                   = 0;
inline constexpr int dock_status_no_anatomy           = 1;   // Handle ohne IAnatomyBase
inline constexpr int dock_status_wrong_genus          = 2;   // Modul-Gattung != Dock-Gattung
inline constexpr int dock_status_subinterface_missing = 3;   // gattungs-Antrieb (dynamic_cast) == nullptr (alte DLL)
inline constexpr int dock_status_conformance_failed   = 4;   // V5-I4: std::map-Konformitaets-Gate fehlgeschlagen (vor Messung)

[[nodiscard]] inline std::string_view dock_status_name(int s) noexcept {
    switch (s) {
        case dock_status_ok:                   return "ok";
        case dock_status_no_anatomy:           return "no_anatomy";
        case dock_status_wrong_genus:          return "wrong_genus";
        case dock_status_subinterface_missing: return "subinterface_missing";
        case dock_status_conformance_failed:   return "conformance_failed";
        default:                               return "unknown";
    }
}

/// IPruefDock — builder-seitiger per-Gattung Mess-Übergang (uniformer Vertrag über alle Gattungen).
/// Erweiterung um eine Gattung = eine neue konkrete IPruefDock-Implementierung (+ deren gattungs-eigenes
/// Antriebs-Sub-Interface + V1-POD); IPruefDock selbst bleibt stabil.
class IPruefDock {
public:
    virtual ~IPruefDock() = default;

    /// Die Gattung, die dieses Dock prüft (1:1 zu anatomy::AnatomyGenus).
    [[nodiscard]] virtual anatomy::AnatomyGenus dock_genus() const noexcept = 0;

    /// Anzeige-Name (z. B. "SearchAlgorithmDock").
    [[nodiscard]] virtual std::string_view dock_name() const noexcept = 0;

    /// Kann dieses Dock dieses geladene Modul treiben? Match über die IM MODUL deklarierte Gattung
    /// (anatomy()->genus()), NICHT über Dateinamen-Heuristik. Default-Kriterium: anatomy() vorhanden + Gattung passt.
    [[nodiscard]] virtual bool accepts(anatomy_loader::AnatomyModuleHandle const& h) const noexcept = 0;

    /// Misst EIN geladenes Modul der eigenen Gattung: zieht das gattungs-eigene Antriebs-Sub-Interface,
    /// fährt den gattungs-eigenen Mess-Treiber, schreibt das serialisierte Ergebnis (CSV/JSON) nach out_*.
    /// errno-style int (0 = ok; sonst dock_status_*). noexcept-frei: die Treiber sind selbst exception-arm.
    /// **VERTRAG (V5 Konformitäts-Gate):** Jede measure()-Implementierung MUSS das Modul VOR der Messung gegen
    /// die std::map-Hüllen-Konformität prüfen (run_conformance_gate, Reihenfolge import → GATE → messen) und bei
    /// Fehlschlag dock_status_conformance_failed liefern, ohne zu messen. Gleiches gilt für JEDEN produktiven
    /// Mess-Eintrittspunkt außerhalb der Docks (z.B. die f15_compare-Pfade A/Observe/measurement-plan).
    [[nodiscard]] virtual int measure(anatomy_loader::AnatomyModuleHandle& h,
                                      PruefDockMeasureOptions const& opts,
                                      std::string& out_csv, std::string& out_json) = 0;
};

}  // namespace comdare::cache_engine::builder::pruef_dock
