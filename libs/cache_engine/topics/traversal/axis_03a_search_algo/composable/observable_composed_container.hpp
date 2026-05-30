#pragma once
// V41 Umstufung-B Phase 1 (#42) — ObservableComposedContainer<Container>: ObservableAxis-Huelle um ein Container-Organ.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier) @saeule 2 (Per-Achsen-Observer)
//
// **Zweck:** Macht JEDES uint64-Container-Organ (ComposedArtTrieSearch / ComposedHotPatriciaSearch /
// ComposedWormholeSearch / ComposedSurfMapSearch / ComposedStartTrieSearch / ComposedHashSearch /
// ComposedSkipListSearch / ComposedBTreeSearch — eigenstaendige Klassen-Templates <Traversal,Pool> mit
// dem EINHEITLICHEN std::map-Interface insert/lookup/erase/clear/occupied_count) zu einer ObservableAxis
// (observer_aggregate.hpp:41), damit ein Gattungs-Konfigurator (Composition, #42) das Organ STATT eines
// monolithischen Tier-Wrappers im search_algo-Slot fuehren kann, OHNE die Saeule-2-Messung
// (observe_all → SearchAlgoStatistics) zu verlieren (Doku 24 §5.2; Memory feedback_zwei_dimensionen_messmodell).
//
// **Abgrenzung zu ObservableComposedSearch<Traversal,Store>:** Jenes wrappt NUR die FLACHEN Organe
// (ComposedSearch<Traversal,Store> = LinearScan/SortedBinary/Interpolation ⊕ RawSlotStore mit STATISCHEN
// Traversal::insert_into<Store>). Die CONTAINER-Organe sind eigenstaendige Klassen-Templates mit Instanz-
// Methoden und passen NICHT in jene Huelle (observable_composed_search.hpp Z.29-30,97 haelt ComposedSearch
// by value). Dieser Adapter ist tier-agnostisch: EIN Template fuer ALLE Container-Organe (KEIN n-faches Copy).
//
// Statistik-Tracking exakt nach ObservableComposedSearch-Praezedenz (observable_composed_search.hpp:36-94):
// snapshot_t = SearchAlgoStatistics, MeasurableObserver, alles unter #ifdef COMDARE_CE_ENABLE_STATISTICS.
// Bei OFF: nackter Pass-Through (0 Footprint, ObservableAxis<...> = false → EmptyAxisSnapshot-Fallback).

#include "../concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"  // SearchAlgoStatistics
#include <measurement/measurable_concept.hpp>                                      // MeasurableObserver

#include <cstddef>
#include <optional>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable {

namespace ce_concepts = ::comdare::cache_engine::traversal::axis_03a_search_algo::concepts;

/// ObservableAxis-Huelle um EIN Container-Organ. Container = ComposedXxxSearch<Traversal,Pool> mit dem
/// einheitlichen std::map-Interface insert(k,v)/lookup(k)/erase(k)/clear()/occupied_count(). Tier-agnostisch:
/// EINE Schale fuer alle 8 Container-Organe. Default-konstruierbar (Pflicht: gehalten als axis_search_algo_-
/// Member in SearchAlgorithmAnatomy + direkt getrieben in abi_adapter::run_workload Z.149).
template <class Container>
class ObservableComposedContainer {
public:
    using key_type       = typename Container::key_type;     // == std::uint64_t (Organ-Invariante)
    using value_type     = typename Container::value_type;
    using container_type = Container;

    /// insert mit rekonstruiertem inserted-Flag (Container::insert ist void) — insert_or_assign-Semantik.
    bool insert(key_type k, value_type v) {
        bool const is_new = !container_.lookup(k).has_value();
        container_.insert(k, v);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (container_.occupied_count() > stats_.peak_occupancy)
            stats_.peak_occupancy = container_.occupied_count();
        observer_.notify(stats_);
#endif
        return is_new;
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        auto const r = container_.lookup(k);
        if (r) ++stats_.total_hit_count; else ++stats_.total_miss_count;
        observer_.notify(stats_);
        return r;
#else
        return container_.lookup(k);
#endif
    }

    bool erase(key_type k) {
        bool const ok = container_.erase(k);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if (ok) ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return ok;
    }

    // clear() leert NUR den Container, NICHT die Statistik (Memory-Regel: reset() = Statistik-Reset).
    void clear()                                     noexcept { container_.clear(); }
    [[nodiscard]] std::size_t occupied_count() const noexcept { return container_.occupied_count(); }

    [[nodiscard]] Container const& container() const noexcept { return container_; }
    [[nodiscard]] Container&       container()       noexcept { return container_; }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = ce_concepts::SearchAlgoStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; observer_.notify(stats_); }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer()       noexcept { return observer_; }
#endif

private:
    Container container_{};
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable snapshot_t stats_{};
    mutable observer_t observer_{};
#endif
};

}  // namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable
