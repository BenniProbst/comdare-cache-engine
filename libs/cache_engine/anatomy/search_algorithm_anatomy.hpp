#pragma once
// V41.F.6.1.R3 — SearchAlgorithmAnatomy<Composition> Skelett
//
// Saeugetier-Anatomie: EINE generische Such-Algorithmus-Klasse, die durch
// Template-Spezialisierung mit jeder Composition zu einem konkreten Algorithmus
// wird. Phase R3 ist Pilot-Skelett mit std::map als Container; echte
// Composition-driven Implementation kommt in R4+R5.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §11.3+§12+§14
// @task V41.F.6.1.R3

#include "anatomy_base.hpp"
#include "composition_concept.hpp"
#include "observer_aggregate.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// SearchAlgorithmAnatomy — zentrale Anatomie-Klasse fuer ALLE Suchalgorithmen.
///
/// Template-Parameter Composition liefert die 17 Achsen-Auspraegungen. Konkrete
/// Algorithmen (ART/HOT/Wormhole/SuRF/Masstree/START) sind reine Template-
/// Instantiationen — siehe `anatomy::Art`, `anatomy::Hot` etc. unten.
///
/// Phase R3 (Pilot): interner std::map<uint64_t,uint64_t> als Container.
/// Phase R4+: Container wird durch Composition::node_type + Composition::allocator
/// + Composition::concurrency-getriebene Implementation ersetzt.
template <IsComposition Composition>
class SearchAlgorithmAnatomy {
public:
    using composition_t        = Composition;
    using key_type             = std::uint64_t;
    using value_type           = std::uint64_t;
    using observer_aggregate_t = ObserverAggregate<Composition>;

    // Composition-Inspection (statisch — Pflicht fuer Mess-Treiber)
    static constexpr std::string_view composition_name() noexcept { return Composition::name; }
    static constexpr std::string_view paper_id() noexcept { return Composition::paper_id; }
    static constexpr std::size_t organ_count() noexcept { return composition_organ_count<Composition>::value; }

    // R5.C.A Gattungs-Marker (User-Direktive Doku 14 Teil 4 §27.2)
    // SearchAlgorithmAnatomy gehoert zur Mammal-Gattung (vollstaendige 17-Achsen-Anatomie)
    static constexpr AnatomyGenus genus() noexcept { return AnatomyGenus::SearchAlgorithm; }

    // ─────────────────────────────────────────────────────────────────────
    // R5.A Observer-Aggregate: ABI-stabiler 17-Achsen-Snapshot
    // (User-Direktive 2026-05-26 spaet, Doku 14 Teil 3 §17.2+§20)
    // ─────────────────────────────────────────────────────────────────────

    /// observe_all() — sammelt Snapshots aller 17 Achsen zu einem POD-Struct.
    /// Achsen ohne statistics() liefern EmptyAxisSnapshot.
    /// Pflicht-API fuer CacheEngineBuilder Mess-Treiber + ABI-Loader.
    ///
    /// **R5.A Pilot:** Default-Aggregate (alle Achsen Empty-Snapshot).
    /// **R5.B+ Ziel:** echte Achsen-Members + statistics()-Aufrufe.
    /// Aktueller Block: Achsen-Wrappers haben protected CRTP-Constructor —
    /// direktes Halten als Anatomie-Member blockiert. Wird durch
    /// public-Constructor-Fix oder Tuple-basierte Komposition spaeter geloest.
    [[nodiscard]] observer_aggregate_t observe_all() const noexcept {
        observer_aggregate_t agg{};
        // Saeule-2-Korrektur (Doku 24 §2.2/§3): ECHTE Per-Achsen-statistics() statt EmptyAxisSnapshot-
        // Stub. Das search_algo-Organ wird real gehalten + (vom Builder) getrieben → sein Observer
        // liefert echte Werte (insert/lookup/hit/miss/peak_occupancy). Weitere Achsen: Slot bleibt
        // Default, bis sie als Organe getrieben werden (Saeule 1) — Mechanismus identisch via ObservableAxis.
        if constexpr (ObservableAxis<typename Composition::search_algo>) {
            agg.search_algo = axis_search_algo_.statistics();
        }
        // V42 L-74c Composition-Driver (Doc 29 §3c): telemetry-Achse als 2. real gehaltenes Organ. Die
        // Composition traegt jetzt die ObservableTelemetry-Huelle (art_reference.hpp), die — anders als der
        // nackte Strategie-Marker (test_d_v42_probe2: ObservableAxis=0, kein statistics()) — eine echte
        // Mess-Mechanik bietet. Greift nur im STATISTICS-Build; sonst EmptyAxisSnapshot (Release-Pfad).
        if constexpr (ObservableAxis<typename Composition::telemetry>) {
            agg.telemetry = axis_telemetry_.statistics();
        }
        // V42 L-74c (Doc 29 §3e): memory_layout-Achse als 3. real gehaltenes Organ. Die Composition traegt
        // die ObservableMemoryLayout-Huelle (static scan_field_sum bleibt fuer ComposedStore/abi_adapter; der
        // Observer-Treiber ruft observe_scan() → statistics()). Greift nur im STATISTICS-Build.
        if constexpr (ObservableAxis<typename Composition::memory_layout>) {
            agg.memory_layout = axis_memory_layout_.statistics();
        }
        return agg;
    }

    /// Zugriff auf das search_algo-Organ (Driver-Organ). Der Builder/Mess-Treiber treibt es
    /// (insert/lookup/erase) → seine statistics() fliessen via observe_all() in den Per-Achsen-Trace.
    [[nodiscard]] typename Composition::search_algo&       search_algo_organ()       noexcept { return axis_search_algo_; }
    [[nodiscard]] typename Composition::search_algo const& search_algo_organ() const noexcept { return axis_search_algo_; }

    /// V42 L-74c: Zugriff auf das telemetry-Organ (2. getriebenes Achsen-Organ). Der Builder/Mess-Treiber
    /// koppelt es an die Tier-Op (record_node_touch beim insert/lookup) → statistics() fliesst via observe_all().
    [[nodiscard]] typename Composition::telemetry&       telemetry_organ()       noexcept { return axis_telemetry_; }
    [[nodiscard]] typename Composition::telemetry const& telemetry_organ() const noexcept { return axis_telemetry_; }

    /// V42 L-74c: Zugriff auf das memory_layout-Organ (3. getriebenes Achsen-Organ). Der Treiber ruft
    /// observe_scan(buf,n,record_size) → die Layout-Scan-Statistik fliesst via observe_all().
    [[nodiscard]] typename Composition::memory_layout&       memory_layout_organ()       noexcept { return axis_memory_layout_; }
    [[nodiscard]] typename Composition::memory_layout const& memory_layout_organ() const noexcept { return axis_memory_layout_; }

    /// Diagnose: wie viele Achsen liefern echte Snapshots? (Rest = EmptyAxisSnapshot)
    [[nodiscard]] static constexpr std::size_t observable_axis_count() noexcept {
        return observer_aggregate_t::observable_count();
    }

    // ─────────────────────────────────────────────────────────────────────
    // R5.B Container-API ENTFERNT — siehe builder::anatomy_commands::
    //      AnatomyExecutionContext<Composition> fuer insert/lookup/erase/clear/size/empty.
    //
    // User-Direktive 2026-05-26 sehr spaet (Doku 14 Teil 3 §17.2+§24):
    //   "Anatomie enthaelt nur Achsen + Statistik-Observer. Alle anderen Methoden
    //   und tools gehoeren in CacheEngineBuilder."
    //
    // Migration-Pfad fuer existing Code:
    //   ANATOMY-OLD: SearchAlgorithmAnatomy<C> a; a.insert(k,v);
    //   BUILDER-NEU: AnatomyExecutionContext<C> ctx; ctx.insert(k,v);
    //              (Anatomie ist intern Bestandteil des ctx)
    // ─────────────────────────────────────────────────────────────────────

private:
    // Saeule-2 (Doku 24): das search_algo-Organ wird real gehalten. Default-konstruiert; vom Builder
    // ueber search_algo_organ() getrieben. Sein statistics()-Observer liefert echte Per-Achsen-Daten.
    typename Composition::search_algo axis_search_algo_{};

    // V42 L-74c (Doc 29 §3c): telemetry als 2. real gehaltenes Organ. OHNE `{}` (default-init) — sowohl die
    // ObservableTelemetry-Huelle ALS AUCH eine nackte Aggregat-Strategie sind default-init-fähig, aber
    // Aggregat + `{}` waere ill-formed (test_d_v42_probe2: is_aggregate=1, brace_ok{T{}}=0). Vom Builder via
    // telemetry_organ() getrieben; statistics() fliesst via observe_all() (nur im STATISTICS-Build).
    typename Composition::telemetry axis_telemetry_;

    // V42 L-74c (Doc 29 §3e): memory_layout-Huelle als 3. Organ. Kein Aggregat (Decorator) → default-init.
    typename Composition::memory_layout axis_memory_layout_;
};

}  // namespace comdare::cache_engine::anatomy
