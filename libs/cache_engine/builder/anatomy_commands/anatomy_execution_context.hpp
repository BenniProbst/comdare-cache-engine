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
#include <topics/axis_command_base.hpp>
#include <topics/traversal/axis_03a_search_algo/composable/observable_composed_search.hpp> // Saeule-2
#include <axes/lookup/composable/organ_for_search_algo.hpp>
#include <axes/lookup/composable/observable_composed_container.hpp>
#include <axes/lookup/composable/store_traversable_search_algo.hpp>
#include <axes/lookup/composable/traversal_for_search_algo.hpp>
#include <axes/node/axis_04_node_type_composed_store.hpp>

#include <cstdint>
#include <optional>
#include <type_traits>

namespace comdare::cache_engine::builder::anatomy_commands {

namespace ana     = ::comdare::cache_engine::anatomy;
namespace comp    = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable;
namespace lk_comp = ::comdare::cache_engine::lookup::composable;
namespace node    = ::comdare::cache_engine::node;
namespace topics  = ::comdare::cache_engine::topics;

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
    using search_algo_t        = typename Composition::search_algo;
    using container_algorithm_traversal_t =
        std::conditional_t<lk_comp::StoreTraversableSearchAlgo<search_algo_t>,
                           lk_comp::traversal_for_search_algo_t<search_algo_t>, comp::SortedBinaryTraversal>;
    // Flat-Fallback bewusst mit ComposedStore (NICHT LayoutAwareChunkedStore wie im Adapter): der Builder-
    // Pilot misst die Allocator-Achse ueber den Store-Vector-Growth REAL inkl. total_bytes_in_use
    // (Roadmap-1; Pilot-Test R5B asserted in_use > 0 — LayoutAware-Slab-Snapshot kennt kein in-use-Mass).
    using flat_container_algorithm_t = comp::ObservableComposedSearch<
        container_algorithm_traversal_t,
        node::ComposedStore<typename Composition::node_type, typename Composition::memory_layout,
                            typename Composition::allocator>>;
    static constexpr bool pool_family_ = !std::is_same_v<lk_comp::organ_for_search_algo_t<search_algo_t>, void>;
    static constexpr bool organ_hull_  = lk_comp::is_observable_organ_hull_v<search_algo_t>;
    using container_algorithm_t        = typename std::conditional_t<
        pool_family_,
        std::type_identity<lk_comp::ObservableComposedContainer<lk_comp::organ_for_search_algo_t<search_algo_t>>>,
        std::conditional_t<organ_hull_, std::type_identity<search_algo_t>,
                           std::type_identity<flat_container_algorithm_t>>>::type;

    // T6-Mess-Organ-Erkennung: bietet der konstitutive Speicher selbst eine Allocator-Statistik?
    static constexpr bool has_store_alloc_stats_ =
        requires(container_algorithm_t const& c) { c.store_allocator_statistics(); };

private:
    // Nicht-konstitutives T6-MESS-Organ (Adapter-Muster „container_algorithm_ + Mess-Organe"): nur fuer
    // Zweige OHNE eigene store_allocator_statistics (Organ-Huellen/Pools) — erhaelt das Roadmap-1-
    // Verhalten (ComposedStore-Vector-Growth treibt die Allocator-Achse REAL inkl. in_use; Pilot R5B).
    struct EmptyMeter {
        void insert(key_type, value_type) noexcept {}
        void erase(key_type) noexcept {}
        void clear() noexcept {}
    };
    using allocator_meter_t = std::conditional_t<has_store_alloc_stats_, EmptyMeter, flat_container_algorithm_t>;

    template <class Target, class Source>
    static void assign_allocator_snapshot_(Target& target, Source const& source) noexcept {
        if constexpr (std::is_assignable_v<Target&, Source const&>) {
            target = source;
        } else if constexpr (requires {
                                 source.total_bytes_allocated;
                                 source.total_bytes_in_use;
                                 source.allocation_count;
                                 source.deallocation_count;
                                 source.failure_count;
                             }) {
            target.total_bytes_allocated = static_cast<std::uint64_t>(source.total_bytes_allocated);
            target.total_bytes_in_use    = static_cast<std::uint64_t>(source.total_bytes_in_use);
            target.allocation_count      = static_cast<std::uint64_t>(source.allocation_count);
            target.deallocation_count    = static_cast<std::uint64_t>(source.deallocation_count);
            target.failure_count         = static_cast<std::uint64_t>(source.failure_count);
        } else if constexpr (requires {
                                 source.bytes_allocated;
                                 source.alloc_calls;
                             }) {
            // Adapter-Spiegel-Konvention (abi_adapter tier_get_allocator, Zweig 2): native Slab-/Pool-
            // Formate ohne free-Pfad haben allocated == in_use per Konstruktion — KEINE Fabrikation.
            target.total_bytes_allocated = static_cast<std::uint64_t>(source.bytes_allocated);
            target.total_bytes_in_use    = static_cast<std::uint64_t>(source.bytes_allocated);
            target.allocation_count      = static_cast<std::uint64_t>(source.alloc_calls);
        }
    }

    struct MeasurementVisitor {
        observer_aggregate_t&        agg;
        container_algorithm_t const& container_algorithm;
        allocator_meter_t const&     allocator_meter;

        template <class Axis>
        void visit_observable() const noexcept {
            if constexpr (std::is_same_v<Axis, typename Composition::search_algo>) {
                agg.search_algo = container_algorithm.statistics();
            } else if constexpr (std::is_same_v<Axis, typename Composition::allocator>) {
                if constexpr (has_store_alloc_stats_) {
                    assign_allocator_snapshot_(agg.allocator, container_algorithm.store_allocator_statistics());
                } else {
                    assign_allocator_snapshot_(agg.allocator, allocator_meter.store_allocator_statistics());
                }
            }
        }
    };

public:
    /// Default-konstruiert — Anatomie + leeren Container
    AnatomyExecutionContext() = default;

    /// Anatomie-Referenz (read-only) fuer Observer-Aggregation
    [[nodiscard]] anatomy_t const& anatomy() const noexcept { return anatomy_; }

    // ─────────────────────────────────────────────────────────────────────
    // Container-Operationen (R5.B: gehoeren in Builder, nicht Anatomie)
    // ─────────────────────────────────────────────────────────────────────

    bool insert(key_type k, value_type v) {
        if constexpr (!has_store_alloc_stats_) { allocator_meter_.insert(k, v); }
        return container_algorithm_.insert(k, v);
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const { return container_algorithm_.lookup(k); }

    bool erase(key_type k) {
        if constexpr (!has_store_alloc_stats_) { allocator_meter_.erase(k); }
        return container_algorithm_.erase(k);
    }

    void clear() noexcept {
        if constexpr (!has_store_alloc_stats_) { allocator_meter_.clear(); }
        container_algorithm_.clear();
    }

    [[nodiscard]] std::size_t size() const noexcept { return container_algorithm_.occupied_count(); }
    [[nodiscard]] bool        empty() const noexcept { return container_algorithm_.occupied_count() == 0; }

    /// Snapshot-Abruf (R5.A observe_all) — Saeule-2 (Doku 24 §5.2/§5.3): die 16 nicht-getriebenen Achsen
    /// kommen als Default aus der Anatomie; die GETRIEBENEN search_algo-/allocator-Slots bekommen die echten
    /// Zaehler aus dem einen container_algorithm_-Zustand.
    [[nodiscard]] observer_aggregate_t observe_all() const noexcept {
        observer_aggregate_t agg = anatomy_.observe_all();
#ifdef COMDARE_CE_ENABLE_STATISTICS
        MeasurementVisitor visitor{agg, container_algorithm_, allocator_meter_};
        topics::axis_accept_measurement<typename Composition::search_algo>(visitor);
        topics::axis_accept_measurement<typename Composition::allocator>(visitor);
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
    // CMD-1 (#251): ein konstitutiver Speicher analog abi_adapter::container_algorithm_. Der Typ ist entweder die
    // native observable SearchAlgo-Huelle der Composition, ein organ_for<>-Wrapper oder der flache LayoutAware-Fallback.
    container_algorithm_t container_algorithm_{};
    allocator_meter_t     allocator_meter_{};
};

} // namespace comdare::cache_engine::builder::anatomy_commands
