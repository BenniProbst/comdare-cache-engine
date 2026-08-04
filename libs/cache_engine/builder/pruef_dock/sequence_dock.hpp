#pragma once
// L-76b (2026-06-03, Doc 24 §8.8) — Sequence-Gattungs-Prüf-Dock: die CacheEngineBuilder-Seite für die
// SEQUENCE-Gattung (Reptil, genus()==Sequence), analog AdapterDock/SearchAlgorithmDock.
//
// Doc 24 §8.8: per-Gattung Mess-Übergang — (a) Tier über die Gattungs-API treiben, (b) Observer messen, (c)
// persistieren. Hier in-process über die SequenceAnatomy (push_back/at-Workload → SequenceObserverSnapshot → CSV).
// Treibt REAL die axis_growth-Policy der Komposition (growth_events). Gattungs-Constraint: genus()==Sequence
// (Cross-Genus type-unmöglich, Doku 14 §32). Der DLL-Pfad (AnatomyModuleLoader) ist ein Folgeschritt. C++23, header-only.
//
// @related [[anatomie-gattungen]] [[gattungs-constraint-pruefling-merge]]

//
// E-24 C4 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C4)
// -- DER OBEN ANGEKUENDIGTE FOLGESCHRITT IST HIERMIT GEBAUT. Der Kopf-Kommentar darueber sagt seit
// 2026-06 "Der DLL-Pfad (AnatomyModuleLoader ... IPruefDock + AnatomyModuleHandle) ist ein Folgeschritt";
// genau der steht jetzt unten in dieser Datei.
//
// ZWEI SCHICHTEN, EINE DATEI (bewusst, nicht aus Verlegenheit):
//   * SequenceDock<SequenceAnatomyT> (oben, Bestand) ist der IN-PROCESS-Treiber ueber die ANATOMIE. Er kennt den
//     Kompositions-Typ und braucht keinen Loader.
//   * SequencePruefDock (unten, NEU) ist die IPruefDock-Impl ueber ein GELADENES MODUL. Er kennt nur
//     AnatomyModuleHandle + das ABI-Sub-Interface ISequenceTier (per dynamic_cast) und faehrt den
//     V5-Vertrag pruef_dock.hpp:74-79: import -> KONFORMITAETS-GATE -> messen.
// Sie liegen zusammen, weil sie DASSELBE Genus bedienen; sie heissen verschieden, weil sie verschiedene
// Ebenen sind.
//
// NAMENS-ABWEICHUNG VOM BAUPLAN, DEKLARIERT: Paragraf 3.1-C4 nennt die IPruefDock-Impl "SequenceDock". Dieser
// Name ist am Ist seit 2026-06 durch den Anatomie-Treiber oben belegt (gleicher Namensraum, real
// benutzt in tests/unit/test_genus_docks.cpp bzw. test_container_dock.cpp). Die Bauplan-Inventur
// (Paragraf 1.0) listet diese vier Dateien nicht -- die Namens-Annahme wurde also ohne Kenntnis der
// Belegung getroffen. Statt einen benutzten Produktions-Typ umzubenennen (Blast-Radius ausserhalb des
// C4-Auftrags) traegt die neue Klasse den praeziseren Namen "SequencePruefDock": sie IST die IPruefDock-Impl, und
// dock_name() gibt genau diesen Namen zurueck (grep-ehrlich, wie SearchAlgorithmDock). Die
// Zusammenfuehrung beider Schichten gehoert in den Abschluss-Aufraeumpass.

#include "anatomy/sequence_anatomy.hpp" // SequenceAnatomy / SequenceObserverSnapshot
#include "anatomy/anatomy_base.hpp"     // AnatomyGenus

#include "pruef_dock.hpp"             // IPruefDock + dock_status_* + V5-Vertrag (:74-79)
#include "genus_conformance_gate.hpp" // E-24 C4 (a/5): das Gattungs-Orakel
#include "pruef_dock_version.hpp"     // E-24 C4 (b/5): die ce-eigene Dock-Version (vX.Y.Zc)
#include "anatomy/sequence_tier.hpp"

#include <builder/anatomy_commands/genus_tier_observe_trace_abi.hpp> // E-24 C4 (c/5): der Mess-Treiber

#include <cstdint>
#include <string>

namespace comdare::cache_engine::builder::pruef_dock {

/// SequenceDock<SequenceAnatomyT> — treibt ein Sequence-Tier (V-indexed) über die Gattungs-API (push_back/at) +
/// misst den eingebauten Sequence-Observer (inkl. growth_events der axis_growth-Policy) + persistiert.
template <class SequenceAnatomyT>
class SequenceDock {
public:
    struct MeasureResult {
        ::comdare::cache_engine::anatomy::SequenceObserverSnapshot observer{};
        std::uint64_t                                              total_ops = 0;
    };

    /// Diese Dock-Seite bedient die Sequence-Gattung (Doc 24 §8.8 Gattungs-Bindung).
    [[nodiscard]] static constexpr ::comdare::cache_engine::anatomy::AnatomyGenus genus() noexcept {
        return ::comdare::cache_engine::anatomy::AnatomyGenus::Sequence;
    }

    /// Treibt das Sequence-Tier: erst n_pushes push_back(0..n_pushes-1) (treibt die axis_growth-Policy → reserve →
    /// growth_events), dann n_reads at(0..n_reads-1) (i<n_pushes = in-bounds, sonst OOB). Zieht den Sequence-Observer.
    [[nodiscard]] MeasureResult measure(std::uint64_t n_pushes, std::uint64_t n_reads) const {
        SequenceAnatomyT tier;
        using elem_t = typename SequenceAnatomyT::element_type;
        for (std::uint64_t i = 0; i < n_pushes; ++i) tier.push_back(static_cast<elem_t>(i));
        for (std::uint64_t i = 0; i < n_reads; ++i) (void)tier.at(i);
        return MeasureResult{tier.observe_all(), n_pushes + n_reads};
    }

    /// Persistierung (Doc 24 §8.8 Schritt c): eine CSV-Zeile mit den korrelierten Sequence-Observer-Werten.
    [[nodiscard]] static std::string serialize_csv(MeasureResult const& r) {
        std::string s = "genus,total_ops,push_count,at_count,at_oob,current_size,peak_size,growth_events\n";
        s += "Sequence," + std::to_string(r.total_ops) + "," + std::to_string(r.observer.push_count) + "," +
             std::to_string(r.observer.at_count) + "," + std::to_string(r.observer.at_oob_count) + "," +
             std::to_string(r.observer.current_size) + "," + std::to_string(r.observer.peak_size) + "," +
             std::to_string(r.observer.growth_events) + "\n";
        return s;
    }
};

/// SequencePruefDock -- die IPruefDock-Impl der Sequence-Gattung ueber ein GELADENES Modul (V5-Vertrag
/// pruef_dock.hpp:74-79). Header-only, verkabelt die bereits vorhandenen Zahnraeder uniform:
///   (1) anatomy::ISequenceTier                      -- das ABI-stabile Gattungs-Antriebs-Sub-Interface,
///   (2) anatomy_loader::AnatomyModuleHandle    -- das geladene Tier-Modul (Loader),
///   (3) run_sequence_conformance_gate   -- das Gattungs-Orakel (E-24 C4 a/5),
///   (4) drive_sequence_tier_trace_abi  -- der Mess-Treiber (E-24 C4 c/5).
/// Der compile-time-Gattungs-Constraint bleibt unangetastet im SequenceAbiAdapter IN der DLL
/// (static_assert genus()==Sequence); dieses Dock validiert runtime-seitig per genus()-Abfrage und zieht
/// den Antrieb per dynamic_cast (bewaehrtes Pfad-B-Probing).
class SequencePruefDock final : public IPruefDock {
public:
    /// Die ce-eigene Selbst-Version dieses Docks (Q3-Grammatik vX.Y.Zc, ENFORCE-bewacht in
    /// pruef_dock_version.hpp). BEWUSST nicht virtuell: der IPruefDock-Vertrag steht unter dem
    /// HY-D2-Freeze und wird von C4 nicht erweitert.
    static constexpr std::string_view dock_version() noexcept { return kSequenceDockVersion; }

    [[nodiscard]] anatomy::AnatomyGenus dock_genus() const noexcept override { return anatomy::AnatomyGenus::Sequence; }

    [[nodiscard]] std::string_view dock_name() const noexcept override { return "SequencePruefDock"; }

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
        auto* tier = dynamic_cast<anatomy::ISequenceTier*>(base);
        if (tier == nullptr) return dock_status_subinterface_missing;
        // V5-I4-AEQUIVALENT der Gattung: das Modul MUSS vor der Messung sein Gattungs-Orakel bestehen
        // (Reihenfolge import -> GATE -> messen). Eine nicht-konforme Huelle wird NICHT gemessen.
        if (!run_sequence_conformance_gate(*tier).passed()) return dock_status_conformance_failed;
        auto const trace =
            ::comdare::cache_engine::builder::anatomy_commands::drive_sequence_tier_trace_abi(*tier, opts);
        out_csv  = ::comdare::cache_engine::builder::anatomy_commands::serialize_sequence_tier_trace_csv(trace);
        out_json = ::comdare::cache_engine::builder::anatomy_commands::serialize_sequence_tier_trace_json(trace);
        return dock_status_ok;
    }
};

} // namespace comdare::cache_engine::builder::pruef_dock
