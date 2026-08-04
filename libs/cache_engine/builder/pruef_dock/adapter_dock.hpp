#pragma once
// Container-Gattungs-Prüf-Dock (2026-06-02, Doc 24 §8.8, User-Option-B Schritt 3) — die CacheEngineBuilder-Seite
// für die CONTAINER-Gattung (Adapter), analog SearchAlgorithmDock für die SearchAlgorithm-Gattung.
//
// Doc 24 §8.8: ein Prüf-Dock ist der per-Gattung ABI-stabile Mess-Übergang — es (a) hält/treibt ein Tier der
// Gattung über die Gattungs-API, (b) misst dessen eingebaute Observer, (c) persistiert. Hier in-process über die
// AdapterAnatomy (put/get-Workload → AdapterObserverSnapshot → CSV). Gattungs-Constraint: genus()==Adapter
// (Cross-Genus type-unmöglich, Doku 14 §32). Der DLL-Pfad (AnatomyModuleLoader) ist analog BR-4 ein Folgeschritt.
// C++23, header-only.

//
// E-24 C4 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C4)
// -- DER OBEN ANGEKUENDIGTE FOLGESCHRITT IST HIERMIT GEBAUT. Der Kopf-Kommentar darueber sagt seit
// 2026-06 "Der DLL-Pfad (AnatomyModuleLoader ... IPruefDock + AnatomyModuleHandle) ist ein Folgeschritt";
// genau der steht jetzt unten in dieser Datei.
//
// ZWEI SCHICHTEN, EINE DATEI (bewusst, nicht aus Verlegenheit):
//   * AdapterDock<AdapterAnatomyT> (oben, Bestand) ist der IN-PROCESS-Treiber ueber die ANATOMIE. Er kennt den
//     Kompositions-Typ und braucht keinen Loader.
//   * AdapterPruefDock (unten, NEU) ist die IPruefDock-Impl ueber ein GELADENES MODUL. Er kennt nur
//     AnatomyModuleHandle + das ABI-Sub-Interface IAdapterTier (per dynamic_cast) und faehrt den
//     V5-Vertrag pruef_dock.hpp:74-79: import -> KONFORMITAETS-GATE -> messen.
// Sie liegen zusammen, weil sie DASSELBE Genus bedienen; sie heissen verschieden, weil sie verschiedene
// Ebenen sind.
//
// NAMENS-ABWEICHUNG VOM BAUPLAN, DEKLARIERT: Paragraf 3.1-C4 nennt die IPruefDock-Impl "AdapterDock". Dieser
// Name ist am Ist seit 2026-06 durch den Anatomie-Treiber oben belegt (gleicher Namensraum, real
// benutzt in tests/unit/test_genus_docks.cpp bzw. test_container_dock.cpp). Die Bauplan-Inventur
// (Paragraf 1.0) listet diese vier Dateien nicht -- die Namens-Annahme wurde also ohne Kenntnis der
// Belegung getroffen. Statt einen benutzten Produktions-Typ umzubenennen (Blast-Radius ausserhalb des
// C4-Auftrags) traegt die neue Klasse den praeziseren Namen "AdapterPruefDock": sie IST die IPruefDock-Impl, und
// dock_name() gibt genau diesen Namen zurueck (grep-ehrlich, wie SearchAlgorithmDock). Die
// Zusammenfuehrung beider Schichten gehoert in den Abschluss-Aufraeumpass.

#include "anatomy/adapter_anatomy.hpp" // AdapterAnatomy / AdapterObserverSnapshot
#include "anatomy/anatomy_base.hpp"    // AnatomyGenus

#include "pruef_dock.hpp"             // IPruefDock + dock_status_* + V5-Vertrag (:74-79)
#include "genus_conformance_gate.hpp" // E-24 C4 (a/5): das Gattungs-Orakel
#include "pruef_dock_version.hpp"     // E-24 C4 (b/5): die ce-eigene Dock-Version (vX.Y.Zc)
#include "anatomy/adapter_tier.hpp"

#include <builder/anatomy_commands/genus_tier_observe_trace_abi.hpp> // E-24 C4 (c/5): der Mess-Treiber

#include <cstdint>
#include <string>

namespace comdare::cache_engine::builder::pruef_dock {

/// AdapterDock<AdapterAnatomyT> — treibt ein Container-Tier (Queue) über die Gattungs-API + misst den
/// eingebauten Container-Observer + persistiert. Per-Gattung gebunden (genus()==Adapter).
template <class AdapterAnatomyT>
class AdapterDock {
public:
    struct MeasureResult {
        ::comdare::cache_engine::anatomy::AdapterObserverSnapshot observer{};
        std::uint64_t                                             total_ops = 0;
    };

    /// Diese Dock-Seite bedient die Container/Adapter-Gattung (Doc 24 §8.8 Gattungs-Bindung).
    [[nodiscard]] static constexpr ::comdare::cache_engine::anatomy::AnatomyGenus genus() noexcept {
        return ::comdare::cache_engine::anatomy::AnatomyGenus::Adapter;
    }

    /// Treibt das Container-Tier mit n_puts put + n_gets get (Zustands-Manipulation, Doku 24 §8.7b) und zieht
    /// den eingebauten Container-Observer. Reines in-process-Treiben der spezifischen §28-Achse inner_container
    /// (push + get/pop_front). #87+#90: capacity wird für Aufruf-Kompatibilität akzeptiert, aber ignoriert (unbeschränkter Adapter).
    [[nodiscard]] MeasureResult measure(std::uint64_t n_puts, std::uint64_t n_gets, std::size_t capacity = 0) const {
        AdapterAnatomyT tier(capacity);
        for (std::uint64_t i = 0; i < n_puts; ++i) tier.put(static_cast<typename AdapterAnatomyT::element_type>(i));
        for (std::uint64_t i = 0; i < n_gets; ++i) (void)tier.get();
        return MeasureResult{tier.observe_all(), n_puts + n_gets};
    }

    /// Persistierung (Doc 24 §8.8 Schritt c): eine CSV-Zeile mit den korrelierten Container-Observer-Werten der
    /// spezifischen §28-Achse inner_container (push/pop + Enden-Zugriffe + Belegung).
    [[nodiscard]] static std::string serialize_csv(MeasureResult const& r) {
        std::string s = "genus,total_ops,push_count,pop_count,front_reads,back_reads,"
                        "peak_occupancy,current_occupancy\n";
        s += "Container," + std::to_string(r.total_ops) + "," + std::to_string(r.observer.push_count) + "," +
             std::to_string(r.observer.pop_count) + "," + std::to_string(r.observer.front_reads) + "," +
             std::to_string(r.observer.back_reads) + "," + std::to_string(r.observer.peak_occupancy) + "," +
             std::to_string(r.observer.current_occupancy) + "\n";
        return s;
    }
};

/// AdapterPruefDock -- die IPruefDock-Impl der Adapter-Gattung ueber ein GELADENES Modul (V5-Vertrag
/// pruef_dock.hpp:74-79). Header-only, verkabelt die bereits vorhandenen Zahnraeder uniform:
///   (1) anatomy::IAdapterTier                      -- das ABI-stabile Gattungs-Antriebs-Sub-Interface,
///   (2) anatomy_loader::AnatomyModuleHandle    -- das geladene Tier-Modul (Loader),
///   (3) run_adapter_conformance_gate   -- das Gattungs-Orakel (E-24 C4 a/5),
///   (4) drive_adapter_tier_trace_abi  -- der Mess-Treiber (E-24 C4 c/5).
/// Der compile-time-Gattungs-Constraint bleibt unangetastet im AdapterAbiAdapter IN der DLL
/// (static_assert genus()==Adapter); dieses Dock validiert runtime-seitig per genus()-Abfrage und zieht
/// den Antrieb per dynamic_cast (bewaehrtes Pfad-B-Probing).
class AdapterPruefDock final : public IPruefDock {
public:
    /// Die ce-eigene Selbst-Version dieses Docks (Q3-Grammatik vX.Y.Zc, ENFORCE-bewacht in
    /// pruef_dock_version.hpp). BEWUSST nicht virtuell: der IPruefDock-Vertrag steht unter dem
    /// HY-D2-Freeze und wird von C4 nicht erweitert.
    static constexpr std::string_view dock_version() noexcept { return kAdapterDockVersion; }

    [[nodiscard]] anatomy::AnatomyGenus dock_genus() const noexcept override { return anatomy::AnatomyGenus::Adapter; }

    [[nodiscard]] std::string_view dock_name() const noexcept override { return "AdapterPruefDock"; }

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
        auto* tier = dynamic_cast<anatomy::IAdapterTier*>(base);
        if (tier == nullptr) return dock_status_subinterface_missing;
        // V5-I4-AEQUIVALENT der Gattung: das Modul MUSS vor der Messung sein Gattungs-Orakel bestehen
        // (Reihenfolge import -> GATE -> messen). Die erkannte Entnahme-Disziplin wird mitgefuehrt, weil
        // sie bestimmt, WELCHES der drei Orakel gegolten hat (FIFO/LIFO/PRIORITY, siehe
        // genus_conformance_gate.hpp) -- ohne sie waere das Gate-Ergebnis nicht nachvollziehbar.
        auto discipline = AdapterDiscipline::unknown;
        if (!run_adapter_conformance_gate(*tier, /*seed=*/42, /*n_random=*/512, &discipline).passed()) {
            return dock_status_conformance_failed;
        }
        last_discipline_ = discipline;
        auto const trace =
            ::comdare::cache_engine::builder::anatomy_commands::drive_adapter_tier_trace_abi(*tier, opts);
        out_csv  = ::comdare::cache_engine::builder::anatomy_commands::serialize_adapter_tier_trace_csv(trace);
        out_json = ::comdare::cache_engine::builder::anatomy_commands::serialize_adapter_tier_trace_json(trace);
        return dock_status_ok;
    }

    /// Die Entnahme-Disziplin, die das ZULETZT gemessene Modul gezeigt hat (unknown vor der ersten
    /// Messung). Kein Teil des IPruefDock-Vertrags -- eine Auskunft des Adapter-Docks fuer den Aufrufer,
    /// der die Mess-Zeile einordnen will.
    [[nodiscard]] AdapterDiscipline last_discipline() const noexcept { return last_discipline_; }

private:
    AdapterDiscipline last_discipline_ = AdapterDiscipline::unknown;
};

} // namespace comdare::cache_engine::builder::pruef_dock
