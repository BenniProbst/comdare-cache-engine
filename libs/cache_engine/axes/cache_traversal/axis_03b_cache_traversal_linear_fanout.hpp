#pragma once
// V41.F.6.1 axis_03b_cache_traversal LinearFanout CT01 (2026-05-26)
//
// @topic traversal @achse 03b @family CT01 LinearFanout
// @subaxis CT1 linear_access
//
// **Algorithmus-Pattern:** Linear-scan auf kleinen Fanout-Arrays. Klassischer
// B+ Tree Inner-Node-Search (Bayer/McCreight, Acta Informatica 1972) ohne
// Hash-Overhead. Cache-freundlich fuer kleine N (<32), CPU-Branch-Predictor-
// friendly bei sortierten Eintraegen.
//
// Standalone-Implementation: sequenzieller Eintrags-Vektor + std::find_if.
//
// ALLOCATION (A8-S5, Familie 01d, 2026-08-04): der Eintrags-Speicher laeuft ueber die ALLOKATOR-ACHSE
// (axis_06, StdAllocatorAdapter) statt ueber den Default-Allokator -- Schnitt-Regel A8/F2-Dossier 3.4
// ("in Achsen-Algorithmen keine generischen OS-Calls: Speicher NUR ueber das Allokator-Achsen-Interface").
// [[allocation-failure-exception]] ENTFAELLT an DIESER Achse: die axis_06-Strategie meldet einen
// Fehlschlag als nullptr (ExgenAllocator::allocate) und wirft kein std::bad_alloc mehr. Der OOM-Fall
// gehoert damit dem Fehlerraum der ALLOKATOR-Achse (FK-5-Boden, axis_06_allocator_strategy_base.hpp
// error_classes) und nicht mehr dem der Traversal-Achse -- KEINE neue Fehlerklasse (Auflage 11,
// Pilot-Praezedenz A8-S5/04: ein entfallender bad_alloc-Pfad begruendet keine neue Klasse).

#include "axis_03b_cache_traversal_base.hpp"
#include "axis_03b_cache_traversal_subaxes_ct1_to_ct2.hpp"
#include "concepts/axis_03b_cache_traversal_concept.hpp"
#include "concepts/axis_03b_cache_traversal_cache_engine_permutation_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/alloc/axis_06_allocator_exgen.hpp> // A8-S5: Speicher ueber die Allokator-ACHSE
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <axes/cache_traversal/axis_03b_cache_traversal_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::cache_traversal {

class LinearFanout : public CacheTraversalBase<LinearFanout> {
public:
    static constexpr bool enabled = flags::linear_fanout_enabled;

    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::linear_access_tag;
    using family_id  = std::integral_constant<int, 1>; // CT01

    /// A8-S5 SCHNITT-FORM (B) -- der Eintrags-Speicher haengt an der Allokator-ACHSE. Diese Zeile IST
    /// der Ausweis, den die Familien-Konformitaets-Wache liest (tests/unit/s5_family_alloc_conformance.hpp).
    ///
    /// WARUM FEST GEBUNDEN statt Template-Kopf + Namens-Alias (die Mechanik der Pool-Stores,
    /// btree_node_pool_store.hpp): dieses Organ ist ein REGISTRY-Organ. tools/axis_registry_gen
    /// reflektiert `type_name<W>()` in die committete cache_engine_axis_registry.xml (Feld type=/wrapper=)
    /// und der F30-GUARD verlangt, dass type= mit dem COMDARE_DEFINE_ORGAN_LOCATION-Literal beginnt. Ein
    /// Template-Kopf machte aus `LinearFanout` den Alias von `LinearFanoutT<ExgenAllocator>` und drehte
    /// damit zwei XML-Felder + das Makro-Literal -- gegen Auflage 10 ("KEINE Registry-XML-Aenderung") und
    /// gegen das Byte-diff-Gate test_axis_registry_roundtrip. Die Pool-Stores duerfen den Template-Kopf
    /// tragen, weil sie in composable/ leben und NICHT in der Achsen-Registry stehen.
    using allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;
    static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<allocator_type>,
                  "A8-S5: der gebundene Allokator erfuellt das axis_06-Achsen-Concept nicht mehr -- dann liefe "
                  "der Eintrags-Speicher wieder an der Allokator-Achse vorbei (Schnitt-Regel Dossier 3.4).");

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "linear_fanout"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::cache_traversal::LinearFanout",
                                  "axes/cache_traversal/axis_03b_cache_traversal_linear_fanout.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "LinearFanout (prt-art IFanout::lookup_page linear-scan-Fallback)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "LINEAR_FANOUT"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0c";

    [[nodiscard]] static constexpr bool is_hashed() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_collision_chains() noexcept { return false; }
    [[nodiscard]] static constexpr bool amortized_o1() noexcept { return false; } // O(N) linear

    LinearFanout() : entries_(allocator_.as_std_allocator<entry_type>()) {}

    /// COW-SICHERHEIT (Memento-Muster, Praezedenz btree_node_pool_store.hpp:19/:86): der
    /// StdAllocatorAdapter haelt einen Zeiger auf die Strategie-INSTANZ. Eine implizite Kopie wuerde
    /// den Adapter der QUELLE mitkopieren -- die Kopie allozierte/deallozierte dann ueber fremden,
    /// potentiell schon zerstoerten Speicher. Copy-Ctor/Assign rebinden deshalb an das EIGENE
    /// allocator_ und verwerfen die durch die Vollkopie entstandene transiente Allokations-Pollution
    /// per restore_statistics. Move ist BEWUSST nicht deklariert (degradiert zu Copy) -- ein Move
    /// zoege den Adapter mitsamt Fremd-Zeiger in das Ziel.
    LinearFanout(LinearFanout const& o) : entries_(o.entries_, allocator_.as_std_allocator<entry_type>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_    = o.stats_;
        observer_ = o.observer_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    LinearFanout& operator=(LinearFanout const& o) {
        if (this != &o) {
            // propagate_on_container_copy_assignment ist false -> entries_ BEHAELT seinen eigenen
            // Adapter (auf unser allocator_); genau das ist hier gewollt.
            entries_ = o.entries_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_    = o.stats_;
            observer_ = o.observer_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~LinearFanout() = default;

    [[nodiscard]] bool operator==(LinearFanout const& other) const noexcept {
        return entries_.size() == other.entries_.size();
    }

    /// Wachstum allokiert ueber die Allokator-Achse (kein Default-Allokator, kein bad_alloc-Pfad --
    /// s. Kopf-Doku ALLOCATION).
    void register_entry(key_type k, value_type v) {
        auto it = std::find_if(entries_.begin(), entries_.end(), [k](auto const& e) { return e.first == k; });
        if (it != entries_.end()) {
            it->second = v;
        } else {
            entries_.emplace_back(k, v);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_register_count;
        if (entries_.size() > stats_.peak_tracked) stats_.peak_tracked = entries_.size();
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<value_type> resolve(key_type k) const {
        auto it = std::find_if(entries_.begin(), entries_.end(), [k](auto const& e) { return e.first == k; });
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_resolve_count;
        if (it != entries_.end())
            ++stats_.total_resolve_hit_count;
        else
            ++stats_.total_resolve_miss_count;
        observer_.notify(stats_);
#endif
        if (it == entries_.end()) return std::nullopt;
        return it->second;
    }

    bool unregister(key_type k) {
        auto it = std::find_if(entries_.begin(), entries_.end(), [k](auto const& e) { return e.first == k; });
        if (it == entries_.end()) return false;
        entries_.erase(it);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_unregister_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type tracked_count() const noexcept { return entries_.size(); }
    void                    clear() noexcept { entries_.clear(); }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::CacheTraversalStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }
#endif

private:
    using entry_type   = std::pair<key_type, value_type>;
    using entry_alloc  = allocator_type::StdAllocatorAdapter<entry_type>;
    using entry_vector = std::vector<entry_type, entry_alloc>;

    // allocator_ MUSS VOR entries_ stehen: der Adapter haelt &allocator_, und die Member-
    // Initialisierungsreihenfolge ist die Deklarationsreihenfolge (Praezedenz btree/tree-Pool-Store).
    allocator_type allocator_{};
    entry_vector   entries_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::CacheTraversalStatistics stats_{};
    mutable observer_t                         observer_{};
#endif
};

} // namespace comdare::cache_engine::cache_traversal

namespace comdare::cache_engine::cache_traversal {
static_assert(concepts::CacheTraversalVariant<LinearFanout>);
static_assert(concepts::CacheEngineCacheTraversalPermutationStrategy<LinearFanout>);
} // namespace comdare::cache_engine::cache_traversal
