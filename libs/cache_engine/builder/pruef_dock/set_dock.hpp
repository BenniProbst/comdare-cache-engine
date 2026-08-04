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

//
// E-24 C4 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C4)
// -- DER OBEN ANGEKUENDIGTE FOLGESCHRITT IST HIERMIT GEBAUT. Der Kopf-Kommentar darueber sagt seit
// 2026-06 "Der DLL-Pfad (AnatomyModuleLoader ... IPruefDock + AnatomyModuleHandle) ist ein Folgeschritt";
// genau der steht jetzt unten in dieser Datei.
//
// ZWEI SCHICHTEN, EINE DATEI (bewusst, nicht aus Verlegenheit):
//   * SetDock<SetAnatomyT> (oben, Bestand) ist der IN-PROCESS-Treiber ueber die ANATOMIE. Er kennt den
//     Kompositions-Typ und braucht keinen Loader.
//   * SetPruefDock (unten, NEU) ist die IPruefDock-Impl ueber ein GELADENES MODUL. Er kennt nur
//     AnatomyModuleHandle + das ABI-Sub-Interface ISetTier (per dynamic_cast) und faehrt den
//     V5-Vertrag pruef_dock.hpp:74-79: import -> KONFORMITAETS-GATE -> messen.
// Sie liegen zusammen, weil sie DASSELBE Genus bedienen; sie heissen verschieden, weil sie verschiedene
// Ebenen sind.
//
// NAMENS-ABWEICHUNG VOM BAUPLAN, DEKLARIERT: Paragraf 3.1-C4 nennt die IPruefDock-Impl "SetDock". Dieser
// Name ist am Ist seit 2026-06 durch den Anatomie-Treiber oben belegt (gleicher Namensraum, real
// benutzt in tests/unit/test_genus_docks.cpp bzw. test_container_dock.cpp). Die Bauplan-Inventur
// (Paragraf 1.0) listet diese vier Dateien nicht -- die Namens-Annahme wurde also ohne Kenntnis der
// Belegung getroffen. Statt einen benutzten Produktions-Typ umzubenennen (Blast-Radius ausserhalb des
// C4-Auftrags) traegt die neue Klasse den praeziseren Namen "SetPruefDock": sie IST die IPruefDock-Impl, und
// dock_name() gibt genau diesen Namen zurueck (grep-ehrlich, wie SearchAlgorithmDock). Die
// Zusammenfuehrung beider Schichten gehoert in den Abschluss-Aufraeumpass.

#include "anatomy/set_anatomy.hpp"  // SetAnatomy / SetObserverSnapshot
#include "anatomy/anatomy_base.hpp" // AnatomyGenus

#include "pruef_dock.hpp"             // IPruefDock + dock_status_* + V5-Vertrag (:74-79)
#include "genus_conformance_gate.hpp" // E-24 C4 (a/5): das Gattungs-Orakel
#include "pruef_dock_version.hpp"     // E-24 C4 (b/5): die ce-eigene Dock-Version (vX.Y.Zc)
#include "anatomy/set_tier.hpp"

#include <builder/anatomy_commands/genus_tier_observe_trace_abi.hpp> // E-24 C4 (c/5): der Mess-Treiber

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

/// SetPruefDock -- die IPruefDock-Impl der Set-Gattung ueber ein GELADENES Modul (V5-Vertrag
/// pruef_dock.hpp:74-79). Header-only, verkabelt die bereits vorhandenen Zahnraeder uniform:
///   (1) anatomy::ISetTier                      -- das ABI-stabile Gattungs-Antriebs-Sub-Interface,
///   (2) anatomy_loader::AnatomyModuleHandle    -- das geladene Tier-Modul (Loader),
///   (3) run_set_conformance_gate   -- das Gattungs-Orakel (E-24 C4 a/5),
///   (4) drive_set_tier_trace_abi  -- der Mess-Treiber (E-24 C4 c/5).
/// Der compile-time-Gattungs-Constraint bleibt unangetastet im SetAbiAdapter IN der DLL
/// (static_assert genus()==Set); dieses Dock validiert runtime-seitig per genus()-Abfrage und zieht
/// den Antrieb per dynamic_cast (bewaehrtes Pfad-B-Probing).
class SetPruefDock final : public IPruefDock {
public:
    /// Die ce-eigene Selbst-Version dieses Docks (Q3-Grammatik vX.Y.Zc, ENFORCE-bewacht in
    /// pruef_dock_version.hpp). BEWUSST nicht virtuell: der IPruefDock-Vertrag steht unter dem
    /// HY-D2-Freeze und wird von C4 nicht erweitert.
    static constexpr std::string_view dock_version() noexcept { return kSetDockVersion; }

    [[nodiscard]] anatomy::AnatomyGenus dock_genus() const noexcept override { return anatomy::AnatomyGenus::Set; }

    [[nodiscard]] std::string_view dock_name() const noexcept override { return "SetPruefDock"; }

    [[nodiscard]] bool accepts(anatomy_loader::AnatomyModuleHandle const& h) const noexcept override {
        return h.anatomy() != nullptr && h.anatomy()->genus() == dock_genus();
    }

    [[nodiscard]] int measure(anatomy_loader::AnatomyModuleHandle& h, PruefDockMeasureOptions const& opts,
                              std::string& out_csv, std::string& out_json) override {
        auto* base = h.anatomy();
        if (base == nullptr) return dock_status_no_anatomy;
        if (base->genus() != dock_genus()) return dock_status_wrong_genus;
        // Pfad-B-Probing: das gattungs-eigene Antriebs-Sub-Interface aus IAnatomyBase ziehen.
        // nullptr = altes Modul ohne diesen Pfad -> sauber degradieren, NICHT messen.
        auto* tier = dynamic_cast<anatomy::ISetTier*>(base);
        if (tier == nullptr) return dock_status_subinterface_missing;
        // V5-I4-AEQUIVALENT der Gattung: das Modul MUSS vor der Messung sein Gattungs-Orakel bestehen
        // (Reihenfolge import -> GATE -> messen). Eine nicht-konforme Huelle wird NICHT gemessen.
        if (!run_set_conformance_gate(*tier).passed()) return dock_status_conformance_failed;
        auto const trace = ::comdare::cache_engine::builder::anatomy_commands::drive_set_tier_trace_abi(*tier, opts);
        out_csv          = ::comdare::cache_engine::builder::anatomy_commands::serialize_set_tier_trace_csv(trace);
        out_json         = ::comdare::cache_engine::builder::anatomy_commands::serialize_set_tier_trace_json(trace);
        return dock_status_ok;
    }
};

} // namespace comdare::cache_engine::builder::pruef_dock
