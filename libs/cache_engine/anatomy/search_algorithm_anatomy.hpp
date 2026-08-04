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
#include "organ_concept.hpp" // E-24 C1: OrganGuard (CRTP-Wache)

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// SearchAlgorithmAnatomy — zentrale Anatomie-Klasse fuer ALLE Suchalgorithmen.
///
/// Template-Parameter Composition liefert die 17 Achsen-Auspraegungen (15 Such-Achsen
/// + queuing q1/q2, Doc 30 §8.0; INC-2c/2d: telemetry+isa sind System-Achsen). Konkrete Algorithmen (ART/HOT/Wormhole/SuRF/Masstree/
/// START) sind reine Template-Instantiationen — siehe `anatomy::Art`, `anatomy::Hot` etc. unten.
///
/// Phase R3 (Pilot): interner std::map<uint64_t,uint64_t> als Container.
/// Phase R4+: Container wird durch Composition::node_type + Composition::allocator
/// + Composition::concurrency-getriebene Implementation ersetzt.
/// E-24 C1 (Luecke L5): erbt die CRTP-Wache OrganGuard (Goldstandard axis_io_strategy_base.hpp:15-21) --
/// die erste Konstruktion prueft compile-hart gegen OrganConcept. Zero-cost, kein ABI-Ereignis.
template <IsComposition Composition>
class SearchAlgorithmAnatomy : public OrganGuard<SearchAlgorithmAnatomy<Composition>> {
public:
    using composition_t        = Composition;
    using key_type             = std::uint64_t;
    using value_type           = std::uint64_t;
    using observer_aggregate_t = ObserverAggregate<Composition>;

    // Composition-Inspection (statisch — Pflicht fuer Mess-Treiber)
    static constexpr std::string_view composition_name() noexcept { return Composition::name; }
    static constexpr std::string_view paper_id() noexcept { return Composition::paper_id; }
    static constexpr std::size_t      organ_count() noexcept { return composition_organ_count<Composition>::value; }

    // R5.C.A Gattungs-Marker (User-Direktive Doku 14 Teil 4 §27.2)
    // SearchAlgorithmAnatomy gehoert zur Mammal-Gattung (vollstaendige 17-Achsen-Anatomie)
    static constexpr AnatomyGenus genus() noexcept { return AnatomyGenus::SearchAlgorithm; }

    // ─────────────────────────────────────────────────────────────────────
    // R5.A Observer-Aggregate: ABI-stabiler 17-Achsen-Snapshot
    // (User-Direktive 2026-05-26 spaet, Doku 14 Teil 3 §17.2+§20)
    // ─────────────────────────────────────────────────────────────────────

    /// observe_all() -- sammelt Snapshots ALLER 18 Achsen (kV3AxisCount) zu einem POD-Struct.
    /// Achsen ohne statistics() liefern EmptyAxisSnapshot.
    /// Pflicht-API fuer CacheEngineBuilder Mess-Treiber + ABI-Loader.
    ///
    /// **R5.A Pilot:** Default-Aggregate (alle Achsen Empty-Snapshot).
    /// **R5.B+ Ziel:** echte Achsen-Members + statistics()-Aufrufe.
    /// **A8-S3 (2026-08-04): ERREICHT.** Alle 18 Slots werden als reale Member gehalten und hier
    /// eingesammelt (vorher 9 -- die uebrigen 9 blieben default-konstruiert, also eine stille 0).
    /// Der frueher hier notierte Block ("Achsen-Wrappers haben protected CRTP-Constructor") gilt nicht
    /// mehr: die Bestands-Slots sind default-konstruierbar, was die Bau-Kadenz dieser Scheibe belegt.
    /// Die EHRLICHKEITS-Grenze bleibt und ist gewollt: ein Slot, dessen Registry-Variante kein
    /// statistics() traegt, liefert EmptyAxisSnapshot -- keine erfundene Zahl.
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
        // V42 L-74c (Doc 29 §3e): memory_layout-Achse als 3. real gehaltenes Organ. Die Composition traegt
        // die ObservableMemoryLayout-Huelle (static scan_field_sum bleibt fuer ComposedStore/abi_adapter; der
        // Observer-Treiber ruft observe_scan() → statistics()). Greift nur im STATISTICS-Build.
        if constexpr (ObservableAxis<typename Composition::memory_layout>) {
            agg.memory_layout = axis_memory_layout_.statistics();
        }
        // V42 L-74c: serialization-Achse als 4. real gehaltenes Organ (static serialize_scan bleibt fuer
        // abi_adapter:218; Observer-Treiber ruft observe_serialize()). Greift nur im STATISTICS-Build.
        if constexpr (ObservableAxis<typename Composition::serialization>) {
            agg.serialization = axis_serialization_.statistics();
        }
        // V42 L-74c: node_type-Achse als 5. real gehaltenes Organ (static node_find_scan bleibt fuer N-Einsatz
        // in ComposedStore; Observer-Treiber ruft observe_node_find()). Greift nur im STATISTICS-Build.
        if constexpr (ObservableAxis<typename Composition::node_type>) { agg.node_type = axis_node_type_.statistics(); }
        // Per-Achsen-Vervollständigung Phase A (2026-06-04): T1 cache_traversal + T2 mapping + T17 queuing_q1 +
        // T18 queuing_q2 als real gehaltene Organe (statistics() existierte je Wrapper schon — nur fehlte der
        // observe_all-Slot). Identischer Mechanismus wie oben (ObservableAxis-Concept-Guard, STATISTICS-Build).
        // Der Builder/abi_adapter koppelt sie an die Tier-Op (resolve/register bzw. put/should_flush) → ihre
        // statistics() fliessen via observe_all() in den Per-Achsen-Trace (Pfad B).
        if constexpr (ObservableAxis<typename Composition::cache_traversal>) {
            agg.cache_traversal = axis_cache_traversal_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::mapping>) { agg.mapping = axis_mapping_.statistics(); }
        if constexpr (ObservableAxis<typename Composition::queuing_q1>) {
            agg.queuing_q1 = axis_queuing_q1_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::queuing_q2>) {
            agg.queuing_q2 = axis_queuing_q2_.statistics();
        }
        // STRUKT-R ORG-18: persistence_target-Slot. Wie io_dispatch/migration_policy wird das Organ NICHT hier
        // gehalten, sondern im abi_adapter (pt_organ_) als Pfad-B-Scan getrieben; der Guard greift daher nur,
        // wenn die Composition eine observable Huelle traegt (ObservablePersistenceTarget). Sonst bleibt der
        // Slot EmptyAxisSnapshot -- korrekt, kein Sonderfall.
        if constexpr (ObservableAxis<typename Composition::persistence_target>) {
            agg.persistence_target = axis_persistence_target_.statistics();
        }
        // A8-S3 (2026-08-04) -- DIE NEUN NACHGERUESTETEN SLOTS. Bis hierher sammelte observe_all() nur 9 der
        // 18 Achsen ein; die uebrigen 9 blieben default-konstruierte Aggregat-Felder, also eine STILLE 0 statt
        // einer Aussage (Katalog-Arbeitsliste P1, LEDGER:3812 Punkt 1). Der Guard ist derselbe wie oben:
        // ObservableAxis<Slot-Typ>. Traegt die Registry-Variante des Slots KEIN statistics() (die nackten
        // Strategien der Bestands-Achsen), bleibt der Slot EmptyAxisSnapshot -- das ist die EHRLICHE Antwort
        // "diese Auspraegung hat nichts zu berichten", keine erfundene Zahl. Traegt der Slot dagegen eine
        // observable Huelle oder ein Cross-Genus-Sub-Organ (ObservableOrgan, E-24 C1/C3), liefert er ab jetzt
        // ECHTE Werte -- ohne Wire-Aenderung, denn ObserverAggregate hat alle 18 Slots seit jeher.
        // index_organization steht bewusst zuerst: an ihm haengt die Cross-Genus-SA-Naht (Set-Organ als
        // Index-Organisation, cross_genus_organ.hpp), die ohne diesen Member stumm blieb.
        if constexpr (ObservableAxis<typename Composition::index_organization>) {
            agg.index_organization = axis_index_organization_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::value_handle>) {
            agg.value_handle = axis_value_handle_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::filter>) { agg.filter = axis_filter_.statistics(); }
        if constexpr (ObservableAxis<typename Composition::allocator>) { agg.allocator = axis_allocator_.statistics(); }
        if constexpr (ObservableAxis<typename Composition::path_compression>) {
            agg.path_compression = axis_path_compression_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::prefetch>) { agg.prefetch = axis_prefetch_.statistics(); }
        if constexpr (ObservableAxis<typename Composition::concurrency>) {
            agg.concurrency = axis_concurrency_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::io_dispatch>) {
            agg.io_dispatch = axis_io_dispatch_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::migration_policy>) {
            agg.migration_policy = axis_migration_policy_.statistics();
        }
        return agg;
    }

    /// Zugriff auf das search_algo-Organ (Driver-Organ). Der Builder/Mess-Treiber treibt es
    /// (insert/lookup/erase) → seine statistics() fliessen via observe_all() in den Per-Achsen-Trace.
    [[nodiscard]] typename Composition::search_algo&       search_algo_organ() noexcept { return axis_search_algo_; }
    [[nodiscard]] typename Composition::search_algo const& search_algo_organ() const noexcept {
        return axis_search_algo_;
    }

    /// V42 L-74c: Zugriff auf das telemetry-Organ (2. getriebenes Achsen-Organ). Der Builder/Mess-Treiber
    /// koppelt es an die Tier-Op (record_node_touch beim insert/lookup) → statistics() fliesst via observe_all().

    /// V42 L-74c: Zugriff auf das memory_layout-Organ (3. getriebenes Achsen-Organ). Der Treiber ruft
    /// observe_scan(buf,n,record_size) → die Layout-Scan-Statistik fliesst via observe_all().
    [[nodiscard]] typename Composition::memory_layout& memory_layout_organ() noexcept { return axis_memory_layout_; }
    [[nodiscard]] typename Composition::memory_layout const& memory_layout_organ() const noexcept {
        return axis_memory_layout_;
    }

    /// V42 L-74c: Zugriff auf das serialization-Organ (4. getriebenes Achsen-Organ). Treiber ruft
    /// observe_serialize(buf,n,record_size) → die Serialisierungs-Statistik fliesst via observe_all().
    [[nodiscard]] typename Composition::serialization& serialization_organ() noexcept { return axis_serialization_; }
    [[nodiscard]] typename Composition::serialization const& serialization_organ() const noexcept {
        return axis_serialization_;
    }

    /// V42 L-74c: Zugriff auf das node_type-Organ (5. getriebenes Achsen-Organ). Treiber ruft
    /// observe_node_find(stored,n,queries,q) → die Node-Lookup-Statistik fliesst via observe_all().
    [[nodiscard]] typename Composition::node_type&       node_type_organ() noexcept { return axis_node_type_; }
    [[nodiscard]] typename Composition::node_type const& node_type_organ() const noexcept { return axis_node_type_; }

    /// Phase A (2026-06-04): Zugriff auf die 4 neu verdrahteten Achsen-Organe (T1/T2/T15/T16 seit INC-2d;
    /// historisch T1/T2/T17/T18). Der Builder/
    /// abi_adapter koppelt sie an die Tier-Op (register_entry/resolve, register_slot/resolve_offset, put/get,
    /// should_flush/on_flush_complete) → ihre statistics() fliessen via observe_all() in den Per-Achsen-Trace.
    [[nodiscard]] typename Composition::cache_traversal& cache_traversal_organ() noexcept {
        return axis_cache_traversal_;
    }
    [[nodiscard]] typename Composition::cache_traversal const& cache_traversal_organ() const noexcept {
        return axis_cache_traversal_;
    }
    [[nodiscard]] typename Composition::mapping&          mapping_organ() noexcept { return axis_mapping_; }
    [[nodiscard]] typename Composition::mapping const&    mapping_organ() const noexcept { return axis_mapping_; }
    [[nodiscard]] typename Composition::queuing_q1&       queuing_q1_organ() noexcept { return axis_queuing_q1_; }
    [[nodiscard]] typename Composition::queuing_q1 const& queuing_q1_organ() const noexcept { return axis_queuing_q1_; }
    [[nodiscard]] typename Composition::queuing_q2&       queuing_q2_organ() noexcept { return axis_queuing_q2_; }
    [[nodiscard]] typename Composition::queuing_q2 const& queuing_q2_organ() const noexcept { return axis_queuing_q2_; }

    /// STRUKT-R ORG-18: Zugriff auf das persistence_target-Organ (18. Slot). Getrieben wird es im abi_adapter
    /// (pt_organ_, Pfad-B-Scan); dieser Accessor haelt die Anatomie-Oberflaeche vollstaendig.
    [[nodiscard]] typename Composition::persistence_target& persistence_target_organ() noexcept {
        return axis_persistence_target_;
    }
    [[nodiscard]] typename Composition::persistence_target const& persistence_target_organ() const noexcept {
        return axis_persistence_target_;
    }

    /// A8-S3: die Accessoren der neun nachgeruesteten Slots. Sie halten die Anatomie-Oberflaeche
    /// VOLLSTAENDIG (Owner-KERN "observe_axes ueber ALLE Achsen"): jeder der 18 Achsen-Slots ist jetzt als
    /// realer Member greifbar und wird von observe_all() eingesammelt. Treiben tut sie, wer den Slot besetzt
    /// -- ein Cross-Genus-Sub-Organ treibt sich selbst, eine nackte Bestands-Strategie bleibt ehrlich stumm.
    [[nodiscard]] typename Composition::index_organization& index_organization_organ() noexcept {
        return axis_index_organization_;
    }
    [[nodiscard]] typename Composition::index_organization const& index_organization_organ() const noexcept {
        return axis_index_organization_;
    }
    [[nodiscard]] typename Composition::value_handle&       value_handle_organ() noexcept { return axis_value_handle_; }
    [[nodiscard]] typename Composition::value_handle const& value_handle_organ() const noexcept {
        return axis_value_handle_;
    }
    [[nodiscard]] typename Composition::filter&           filter_organ() noexcept { return axis_filter_; }
    [[nodiscard]] typename Composition::filter const&     filter_organ() const noexcept { return axis_filter_; }
    [[nodiscard]] typename Composition::allocator&        allocator_organ() noexcept { return axis_allocator_; }
    [[nodiscard]] typename Composition::allocator const&  allocator_organ() const noexcept { return axis_allocator_; }
    [[nodiscard]] typename Composition::path_compression& path_compression_organ() noexcept {
        return axis_path_compression_;
    }
    [[nodiscard]] typename Composition::path_compression const& path_compression_organ() const noexcept {
        return axis_path_compression_;
    }
    [[nodiscard]] typename Composition::prefetch&          prefetch_organ() noexcept { return axis_prefetch_; }
    [[nodiscard]] typename Composition::prefetch const&    prefetch_organ() const noexcept { return axis_prefetch_; }
    [[nodiscard]] typename Composition::concurrency&       concurrency_organ() noexcept { return axis_concurrency_; }
    [[nodiscard]] typename Composition::concurrency const& concurrency_organ() const noexcept {
        return axis_concurrency_;
    }
    [[nodiscard]] typename Composition::io_dispatch&       io_dispatch_organ() noexcept { return axis_io_dispatch_; }
    [[nodiscard]] typename Composition::io_dispatch const& io_dispatch_organ() const noexcept {
        return axis_io_dispatch_;
    }
    [[nodiscard]] typename Composition::migration_policy& migration_policy_organ() noexcept {
        return axis_migration_policy_;
    }
    [[nodiscard]] typename Composition::migration_policy const& migration_policy_organ() const noexcept {
        return axis_migration_policy_;
    }

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

    // V42 L-74c (Doc 29 §3e): memory_layout-Huelle als 3. Organ. Kein Aggregat (Decorator) → default-init.
    typename Composition::memory_layout axis_memory_layout_;

    // V42 L-74c: serialization-Huelle als 4. Organ.
    typename Composition::serialization axis_serialization_;

    // V42 L-74c: node_type-Huelle als 5. Organ.
    typename Composition::node_type axis_node_type_;

    // Phase A (2026-06-04): die 4 neu verdrahteten Achsen-Organe (T1/T2/T15/T16 seit INC-2d; historisch
    // T1/T2/T17/T18). Default-konstruiert; vom
    // Builder/abi_adapter ueber die Organ-Accessoren getrieben. statistics() fliesst via observe_all() (nur
    // STATISTICS-Build). KEIN `{}` (wie axis_telemetry_): die nackte Strategie kann ein Aggregat sein.
    typename Composition::cache_traversal axis_cache_traversal_;
    typename Composition::mapping         axis_mapping_;
    typename Composition::queuing_q1      axis_queuing_q1_;
    typename Composition::queuing_q2      axis_queuing_q2_;

    // STRUKT-R ORG-18: persistence_target als 18. Organ-Slot. KEIN `{}` (wie die vier oben): der Slot kann eine
    // nackte Aggregat-Strategie sein, fuer die Aggregat + `{}` ill-formed waere (test_d_v42_probe2).
    typename Composition::persistence_target axis_persistence_target_;

    // A8-S3 (2026-08-04): die NEUN nachgeruesteten Organ-Slots -- damit haelt die Anatomie ALLE 18 Achsen als
    // reale Member (vorher 9). Reihenfolge wie in observe_all(): index_organization zuerst (Cross-Genus-Naht),
    // dann nach Katalog-Prioritaet. KEIN `{}` -- exakt aus demselben Grund wie oben: ein Slot kann eine nackte
    // Aggregat-Strategie sein, fuer die Aggregat + `{}` ill-formed waere (test_d_v42_probe2).
    // KEIN Wire-Ereignis: ObserverAggregate<Composition> traegt diese Slots seit STRUKT-R ORG-18; hier entsteht
    // nur der HALTER, der sie fuellen kann. Wer den Slot treibt, entscheidet die Slot-Belegung.
    typename Composition::index_organization axis_index_organization_;
    typename Composition::value_handle       axis_value_handle_;
    typename Composition::filter             axis_filter_;
    typename Composition::allocator          axis_allocator_;
    typename Composition::path_compression   axis_path_compression_;
    typename Composition::prefetch           axis_prefetch_;
    typename Composition::concurrency        axis_concurrency_;
    typename Composition::io_dispatch        axis_io_dispatch_;
    typename Composition::migration_policy   axis_migration_policy_;
};

} // namespace comdare::cache_engine::anatomy
