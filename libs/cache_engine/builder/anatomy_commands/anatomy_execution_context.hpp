#pragma once
// V41.F.6.1.R5.B — AnatomyExecutionContext<Composition>
//
// Wraps SearchAlgorithmAnatomy<C> + Container fuer Mess-Treiber-Operationen.
// Container (std::map als Pilot R3) wandert hierhin nach User-Direktive:
// "Anatomie enthaelt nur Achsen + Observer. Alle anderen Methoden/Tools
// gehoeren in CacheEngineBuilder."
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §17.3+§24
// @task #698 V41.F.6.1.R5.B
// @related [[anatomie-nur-achsen-und-observer]]

#include <anatomy/search_algorithm_anatomy.hpp>
#include <anatomy/composition_concept.hpp>
#include <anatomy/observer_aggregate.hpp>
#include <topics/traversal/axis_03a_search_algo/composable/observable_composed_search.hpp> // Saeule-2
#include <topics/nodes/axis_04_node_type/axis_04_node_type_composed_store.hpp>             // Roadmap-1: allocator real

#include <cstdint>
#include <optional>

namespace comdare::cache_engine::builder::anatomy_commands {

namespace ana   = ::comdare::cache_engine::anatomy;
namespace comp  = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable;
namespace nodes = ::comdare::cache_engine::nodes::axis_04_node_type;

/// AnatomyExecutionContext — Builder-Side Container + Anatomie-Holder.
///
/// Trennung: Anatomie haelt nur Achsen + Observer (R5.A). Container (std::map
/// als R5.B Pilot, spaeter Composition::node_type/allocator-getrieben) und
/// Container-Operationen sind Builder-Verantwortung.
template <ana::IsComposition Composition>
class AnatomyExecutionContext {
public:
    using anatomy_t            = ana::SearchAlgorithmAnatomy<Composition>;
    using observer_aggregate_t = typename anatomy_t::observer_aggregate_t;
    using key_type             = std::uint64_t;
    using value_type           = std::uint64_t;
    // Saeule-2 (Doku 24 §5.3/§5.5) + Roadmap-1: GETRIEBENER uint64-Container statt losgelostem std::map.
    // Inneres Storage-Organ ist jetzt ComposedStore<N,L,A> mit der Composition-Allocator-Achse → dessen
    // Vector-Growth treibt die Allocator-Statistik REAL (2. observierbare Achse). ObservableComposedSearch
    // haelt den Store by value (nie kopiert) → ComposedStore Copy/Move=delete (Derived*-Lifetime) vertraeglich.
    using container_t = comp::ObservableComposedSearch<
        comp::SortedBinaryTraversal,
        nodes::ComposedStore<typename Composition::node_type, typename Composition::memory_layout,
                             typename Composition::allocator>>;

    /// Default-konstruiert — Anatomie + leeren Container
    AnatomyExecutionContext() = default;

    /// Anatomie-Referenz (read-only) fuer Observer-Aggregation
    [[nodiscard]] anatomy_t const& anatomy() const noexcept { return anatomy_; }

    // ─────────────────────────────────────────────────────────────────────
    // Container-Operationen (R5.B: gehoeren in Builder, nicht Anatomie)
    // ─────────────────────────────────────────────────────────────────────

    bool insert(key_type k, value_type v) {
        search_organ_.insert(
            k,
            v); // treibt + MISST die search_algo-Achse (echtes Composition-Organ; Rueckgabe egal — manche Organe insert()->void)
        return container_.insert(k, v); // inserted-Flag + ALLOCATOR-Achse (container_ ist immer bool)
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        return search_organ_.lookup(k);
    } // echtes Organ (Lookup-Stats)

    bool erase(key_type k) {
        search_organ_.erase(k);
        return container_.erase(k);
    }

    void clear() noexcept {
        container_.clear();
        search_organ_.clear();
    }

    [[nodiscard]] std::size_t size() const noexcept { return search_organ_.occupied_count(); }
    [[nodiscard]] bool        empty() const noexcept { return search_organ_.occupied_count() == 0; }

    /// Snapshot-Abruf (R5.A observe_all) — Saeule-2 (Doku 24 §5.2/§5.3): die 16 nicht-getriebenen Achsen
    /// kommen als Default aus der Anatomie; der GETRIEBENE search_algo-Slot bekommt die ECHTEN Zaehler
    /// aus dem Container. Typkompatibel: container_t::snapshot_t == SearchAlgoStatistics ==
    /// snapshot_of_t<Composition::search_algo> fuer alle Compositions (Praezedenz Array256SearchAlgo:124).
    [[nodiscard]] observer_aggregate_t observe_all() const noexcept {
        observer_aggregate_t agg = anatomy_.observe_all();
#ifdef COMDARE_CE_ENABLE_STATISTICS
        // Achse 1 (search_algo) — echte Zaehler aus dem GETRIEBENEN ECHTEN Composition-Organ (#42-Folge:
        // jede Composition misst ihr EIGENES seziertes Organ, nicht mehr generisch SortedBinary).
        if constexpr (ana::ObservableAxis<typename Composition::search_algo>) {
            agg.search_algo = search_organ_.statistics(); // Doku 24 §5.2-Luecke geschlossen + mess-treu
        }
        // Achse 2 (allocator, Roadmap-1) — der innere ComposedStore-Vector treibt die Allocator-Achse REAL.
        // Doppeltes Gate: Composition::allocator observable UND der Store bietet allocator_statistics().
        if constexpr (ana::ObservableAxis<typename Composition::allocator> &&
                      container_t::template store_has_allocator_stats<typename container_t::store_type>) {
            agg.allocator = container_.store_allocator_statistics();
        }
#endif
        return agg;
    }

    // ─────────────────────────────────────────────────────────────────────
    // Composition-Inspection (durchgereicht von Anatomie)
    // ─────────────────────────────────────────────────────────────────────

    static constexpr std::string_view composition_name() noexcept { return anatomy_t::composition_name(); }
    static constexpr std::string_view paper_id() noexcept { return anatomy_t::paper_id(); }
    static constexpr std::size_t      organ_count() noexcept { return anatomy_t::organ_count(); }

private:
    anatomy_t anatomy_{};
    // Builder-Pilot-Eigenpaar: bewusst NICHT im 4c-iv-Rename (nur SearchAlgorithmAbiAdapter).
    // Konsolidierung dieses search_organ_/container_-Paars folgt mit CMD-1 (#251).
    container_t
        container_{}; // misst die ALLOCATOR-Achse (ComposedStore<N,L,A>-Vector-Growth treibt allocator_statistics)
    // Saeule-2-Mess-Treue (#42-Folge): das ECHTE sezierte Composition-Organ (ART/Masstree/Wormhole/SuRF/...)
    // im search_algo-Slot misst jetzt die search_algo-Achse — statt fuer ALLE Compositions generisch SortedBinary.
    // Default-konstruierbar + ObservableAxis (durch #42 garantiert: ObservableComposedContainer-Huelle).
    typename Composition::search_algo search_organ_{};
};

} // namespace comdare::cache_engine::builder::anatomy_commands
