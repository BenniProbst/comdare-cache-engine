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
// (der konsolidierte Observer-POD). Neue Gattung = neues Dock + neues Sub-Interface + neuer flacher POD — NIE eine
// Mutation von IAnatomyBase oder dem bestehenden Snapshot (vtable-/Layout-Bruch alter DLLs).
//
// PRÄZISIERUNG 17.08.2026 (K2, Owner-Entscheid 09.08.) — ADDITIV, der Satz oben bleibt wahr:
// "IPruefDock lebt nur im Builder-Binary" gilt unverändert für IPruefDock, die Docks, das
// Konformitäts-Gate, die Registry und den Sequencer. Was NICHT mehr hier lebt, sind die
// STATUS-CODES und das ANTRIEBS-BÜNDEL: sie sind nach libs/cache_engine/anatomy_drive/ gewandert,
// weil Owner 09.08. entschieden hat, "der Loader wandert in eine stufen-neutrale Bibliothek, weil
// das Pruefdock der CEB und das Pruefdock der Hybrid-Tier-Binary jeweils technisch identisch bei
// Konfiguration sein muessen."
// Die Trennlinie ist inhaltlich, nicht willkürlich: beschreibt ein Stück die ABI-KANTE zum
// geladenen Modul (Status, dynamic_cast auf Sub-Interfaces), gehört es in die neutrale Schicht;
// beschreibt es das VERHALTEN eines Docks (Gattungs-Match, Mess-Vertrag), bleibt es hier.
// Für Aufrufer ändert sich NICHTS — die Namen werden unten per using re-exportiert.
//
// @doku docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md §8.8
// @related [[anatomie-gattungen]] [[execution-engine-als-wurzel]] [[gattungs-constraint-pruefling-merge]]

#include <anatomy/anatomy_base.hpp>                                // AnatomyGenus, IAnatomyBase::genus()
#include <anatomy_drive/tier_drive_status.hpp>                    // K2: die Status-Codes, stufen-neutral
#include <builder/anatomy_module_loader/anatomy_module_loader.hpp> // AnatomyModuleHandle
#include <builder/anatomy_commands/tier_observe_trace_abi.hpp>     // AbiTierTraceConfig (Mess-Optionen-Reuse)

#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::pruef_dock {

namespace anatomy        = ::comdare::cache_engine::anatomy;
namespace anatomy_loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace anatomy_cmds   = ::comdare::cache_engine::builder::anatomy_commands;

/// Mess-Optionen eines Prüf-Docks — wiederverwendet die bestehende Treiber-Config (kein Neubau).
using PruefDockMeasureOptions = anatomy_cmds::AbiTierTraceConfig;

// errno-Stil Status-Codes (0 = ok), analog Loader + Hybrid-Search-Interface ([[hybrid-search-engine-interface]]).
//
// K2-WANDERUNG 17.08.2026: die Definitionen liegen jetzt in anatomy_drive/tier_drive_status.hpp --
// stufen-neutral, damit das Ebene-3-Dock der Hybrid-Binary dieselben Codes benutzt wie dieses
// Ebene-2-Dock (Owner 09.08.: "technisch identisch bei Konfiguration"). Hier stehen nur noch die
// Re-Exporte: Werte, Namen und Bedeutung sind UNVERAENDERT, und jeder Bestands-Aufrufer sieht
// weiterhin genau dieselben Bezeichner an derselben Stelle.
// KEINE ZWEITE WAHRHEIT: die Codes werden hier NICHT noch einmal definiert. Wer einen anhaengt,
// tut es in der neutralen Datei -- sonst haetten Ebene 2 und Ebene 3 verschiedene Zahlen fuer
// denselben Zustand, und das faellt erst am Messwert auf.
using ::comdare::cache_engine::anatomy_drive::dock_status_conformance_failed;
using ::comdare::cache_engine::anatomy_drive::dock_status_mess_deaktiviert;
using ::comdare::cache_engine::anatomy_drive::dock_status_mess_gate_mismatch;
using ::comdare::cache_engine::anatomy_drive::dock_status_name;
using ::comdare::cache_engine::anatomy_drive::dock_status_no_anatomy;
using ::comdare::cache_engine::anatomy_drive::dock_status_ok;
using ::comdare::cache_engine::anatomy_drive::dock_status_subinterface_missing;
using ::comdare::cache_engine::anatomy_drive::dock_status_wrong_genus;

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
    [[nodiscard]] virtual int measure(anatomy_loader::AnatomyModuleHandle& h, PruefDockMeasureOptions const& opts,
                                      std::string& out_csv, std::string& out_json) = 0;
};

} // namespace comdare::cache_engine::builder::pruef_dock
