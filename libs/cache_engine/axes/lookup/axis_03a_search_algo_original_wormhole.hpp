#pragma once
// V41.F.6.1.P2.D.tr.s3 OriginalWormholeSearchAlgo S07 (2026-05-26)
//
// @paper P07 Wormhole (Wu/Ni/Jiang, EuroSys 2019)
// @source ext/traversal/P07-Wormhole/wormhole/wh.c
//
// 3/4 originall (wh_put/wh_get/wh_del), 1 Lücke (clear — Re-Impl via Drain-Loop).
//
// ===================================================================================================
// A8-S5 Familie 01c, Scheibe 3 (2026-08-05) -- ZWEI-EBENEN-SCHNITT (Pilot-Rezept linear_scan)
// ===================================================================================================
// KLASSEN-ENTSCHEID: **VOLL-REZEPT** -- die beiden Staging-Vektoren keys_/values_ des WRAPPERS haengen
// ab jetzt an der Allokator-ACHSE (2 der 39 Rest-Zeilen der 01b-Schlussbilanz).
//
// **EV-4-GRENZE, HIER AUSDRUECKLICH DEKLARIERT (Design-Entscheid LEDGER 04.08. abend-14, Punkt (iv);
// dieselbe Form, in der 02b seine Papertreue-Grenze im Header deklariert hat):**
//   DIESSEITS der Grenze (Gegenstand dieser Scheibe): die wrapper-EIGENEN Staging-Vektoren keys_ und
//     values_. Sie sind Cache-Engine-Code -- die Standalone-Re-Impl des Wrapper-Bodys, nicht Vendor-Code.
//     Sie gehoeren an die Achse wie jeder andere Organ-Puffer.
//   JENSEITS der Grenze (hier NICHT angefasst, und das ist eine Zusage, kein Versaeumnis): die
//     is_original-CODEGEN-Pfade und jede Allokation, die im Vendor-Code des Papers selbst passiert.
//     Sie bleiben T6-BLIND. Faithful heisst: der Paper-Code wird nicht umgeschrieben, auch nicht "nur"
//     an einer Allokations-Stelle -- sonst waere die Habich-Compliance-Aussage dieses Wrappers
//     (is_original_*) nicht mehr wahr. Die Konsequenz ist ebenso deklariert wie die Grenze: bei
//     eingeschaltetem Codegen-Gate misst die T6-Spalte fuer dieses Organ den STAGING-Anteil, nicht den
//     Vendor-Anteil. Wer das aendern will, aendert die Papertreue -- Owner-Entscheid, nicht Bau.
// Die Grenze ist nicht nur Prosa: der gesamte Schnitt liegt in Membern und Konstruktoren des Wrapper-
// Bodys; unter `#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)` steht KEINE geaenderte Zeile.
//
// Konstruktion/Begruendung im Uebrigen zeilengleich zum Pilot (axis_03a_search_algo_linear_scan.hpp:22-58).
// ORGAN_LOCATION NEU (Default-OFF -> Byte-Effekt NULL, s. XML-ABWESENHEITS-Probe (8b) der Familien-Wache).
#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
#include "concepts/axis_03a_search_algo_original_code_mixin.hpp"
#include <topics/traversal/axis_03a_search_algo/legacy_code/paper_p07_wormhole_is_original.hpp>
#endif

#include <axes/alloc/axis_06_allocator_exgen.hpp>
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <axes/lookup/composable/search_algo_rebind.hpp>
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

namespace comdare::cache_engine::lookup {

// Vorwaerts-Deklaration: die Fassade nennt ihren eigenen Rebound-Leaf als Member-Alias, und der
// Rebound-Leaf erbt vom selben Core -- beide brauchen den Namen, bevor der andere vollstaendig ist.
template <class A2>
class OriginalWormholeSearchAlgoRebound;

namespace detail {

/// DIE SUBSTANZ des S07-Organs (Wormhole (Wu/Ni/Jiang ATC 2019)): die wrapper-EIGENEN Staging-Vektoren ueber die
/// Allokator-ACHSE. Der Vendor-/Codegen-Pfad bleibt unberuehrt (EV-4-Grenze im Datei-Kopf).
///
/// @tparam Alloc  Die Allokator-Achsen-Strategie (axis_06). Default-Bindung der Fassade: ExgenAllocator.
/// @tparam Self   Der most-derived Typ -- PFLICHT wegen der CRTP-Guards der SearchAlgoBase.
template <class Alloc, class Self>
class OriginalWormholeSearchAlgoCore : public SearchAlgoBase<Self>
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
    ,
                                       public generated::p07_wormhole::OriginalCodeMixin
#endif
{
public:
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
    using generated::p07_wormhole::OriginalCodeMixin::get_compiler;
    using generated::p07_wormhole::OriginalCodeMixin::is_original_clear;
    using generated::p07_wormhole::OriginalCodeMixin::is_original_erase;
    using generated::p07_wormhole::OriginalCodeMixin::is_original_insert;
    using generated::p07_wormhole::OriginalCodeMixin::is_original_lookup;
    using generated::p07_wormhole::OriginalCodeMixin::is_original_module;
#endif

    static constexpr bool enabled = flags::original_wormhole_enabled;

    using key_type   = std::uint8_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 7>; // S07

    /// A8-S5 SCHNITT-FORM (B): die wrapper-eigenen Staging-Vektoren haengen an der Allokator-ACHSE.
    /// Diese Zeile IST der Ausweis, den die Familien-Konformitaets-Wache liest
    /// (tests/unit/s5_family_alloc_conformance.hpp). Sie sagt NICHTS ueber den Vendor-Pfad -- s. EV-4.
    using allocator_type = Alloc;
    static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<allocator_type>,
                  "A8-S5: der gebundene Allokator erfuellt das axis_06-Achsen-Concept nicht mehr -- dann liefen "
                  "die Staging-Vektoren wieder an der Allokator-Achse vorbei (Schnitt-Regel Dossier 3.4).");

    [[nodiscard]] static constexpr bool is_thread_safe() noexcept { return true; } // Wormhole hat Wormref-Concurrency
    [[nodiscard]] static constexpr std::size_t      max_fanout() noexcept { return 256; }
    [[nodiscard]] static constexpr std::string_view name() noexcept {
        return enabled ? std::string_view{"original_wormhole"} : std::string_view{"original_wormhole(disabled)"};
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "OriginalWormholeSearchAlgo (Wormhole Wu/Ni/Jiang EuroSys 2019, 3/4 originall)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "ORIGINAL_WORMHOLE"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return true; } // Wormhole nutzt AVX2-Comparisons
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; }
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

private:
    using key_alloc    = typename Alloc::template StdAllocatorAdapter<key_type>;
    using value_alloc  = typename Alloc::template StdAllocatorAdapter<value_type>;
    using key_vector   = std::vector<key_type, key_alloc>;
    using value_vector = std::vector<value_type, value_alloc>;

public:
    /// Beide Staging-Vektoren werden an das EIGENE allocator_ gebunden (der Adapter haelt &allocator_).
    /// NICHT MEHR noexcept: die Vektor-Konstruktion laeuft ueber die Strategie -- die Zusicherung waere
    /// ab hier eine Behauptung. Der Konstruktor alloziert NICHT (leere Vektoren), er bindet nur.
    OriginalWormholeSearchAlgoCore()
        : keys_(allocator_.template as_std_allocator<key_type>()),
          values_(allocator_.template as_std_allocator<value_type>()) {}

    /// KF-6-NAHT (Posten 62, LEDGER 04.08. abend-12): eine vor-parametrierte Strategie-Instanz
    /// uebernehmen, statt sie default zu konstruieren. Heute nirgends benutzt und bewusst `explicit`.
    explicit OriginalWormholeSearchAlgoCore(allocator_type a)
        : allocator_(std::move(a)), keys_(allocator_.template as_std_allocator<key_type>()),
          values_(allocator_.template as_std_allocator<value_type>()) {}

    /// Copy: Strategie mitkopieren, beide Vektoren an das EIGENE allocator_ binden, dann die transiente
    /// Kopier-Allokation aus der Statistik nehmen (Memento) -- 1:1 btree_node_pool_store.hpp:86.
    OriginalWormholeSearchAlgoCore(OriginalWormholeSearchAlgoCore const& o)
        : allocator_(o.allocator_), keys_(o.keys_, allocator_.template as_std_allocator<key_type>()),
          values_(o.values_, allocator_.template as_std_allocator<value_type>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_ = o.stats_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }

    OriginalWormholeSearchAlgoCore& operator=(OriginalWormholeSearchAlgoCore const& o) {
        if (this != &o) {
            keys_   = o.keys_;
            values_ = o.values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_ = o.stats_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }

    ~OriginalWormholeSearchAlgoCore() = default;
    [[nodiscard]] bool operator==(OriginalWormholeSearchAlgoCore const& other) const noexcept {
        return keys_.size() == other.keys_.size();
    }

    void insert(key_type k, value_type v) {
        if constexpr (enabled) {
            auto        it  = std::lower_bound(keys_.begin(), keys_.end(), k);
            std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
            if (it != keys_.end() && *it == k)
                values_[idx] = v;
            else {
                keys_.insert(it, k);
                values_.insert(values_.begin() + idx, v);
            }
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (keys_.size() > stats_.peak_occupancy) stats_.peak_occupancy = keys_.size();
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        if (it != keys_.end() && *it == k)
            ++stats_.total_hit_count;
        else
            ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        if (it == keys_.end() || *it != k) return std::nullopt;
        return values_[static_cast<std::size_t>(it - keys_.begin())];
    }

    bool erase(key_type k) {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        if (it == keys_.end() || *it != k) return false;
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        keys_.erase(it);
        values_.erase(values_.begin() + idx);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return keys_.size(); }
    [[nodiscard]] double density_percent() const noexcept { return 100.0 * static_cast<double>(keys_.size()) / 256.0; }
    /// LUECKE: kein Bulk-Clear im Wormhole-Paper — Re-Impl Drain.
    void clear() noexcept {
        keys_.clear();
        values_.clear();
    }

    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        double dp = density_percent();
        if (dp > 60.0) return concepts::DensityClass::Dense;
        if (dp > 30.0) return concepts::DensityClass::Balanced;
        return concepts::DensityClass::Sparse;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::SearchAlgoStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    }
    // CoW-Memento (#142/Audit-K3): Stat-POD-Restore -> organ_cow_capable_v aktiv (spiegelt Observable-Huelle).
    void restore_statistics(snapshot_t const& s) noexcept {
        stats_ = s;
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }

    /// EINSAMMEL-NAHT der T6-Durchbindung (Owner-KERN abend-11, Pflicht (a)) -- NUR die Naht,
    /// BEWUSST unter einem VIERTEN Namen (Doppelzaehlungs-Absicherung, Pilot-Begruendung).
    /// Sie meldet den STAGING-Anteil dieses Wrappers, nicht den Vendor-Anteil (EV-4).
    using allocator_snapshot_t = typename allocator_type::snapshot_t;
    [[nodiscard]] allocator_snapshot_t search_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    // allocator_ MUSS VOR keys_/values_ stehen: der Adapter haelt &allocator_, und die Member-
    // Initialisierungs-/Zerstoerungsreihenfolge ist die Deklarationsreihenfolge.
    allocator_type allocator_{};
    key_vector     keys_;
    value_vector   values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace detail

/// DIE IDENTITAET -- das Registry-Organ S07. Nicht-Template, exakt der historische Typ-Name.
class OriginalWormholeSearchAlgo final
    : public detail::OriginalWormholeSearchAlgoCore<::comdare::cache_engine::alloc::ExgenAllocator,
                                                    OriginalWormholeSearchAlgo> {
public:
    /// Die Default-Bindung der Identitaets-Ebene: der BENANNTE Achsen-Default, nie std::allocator.
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;

    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::OriginalWormholeSearchAlgo",
                                  "axes/lookup/axis_03a_search_algo_original_wormhole.hpp");

    /// Der Migrations-Ausweis (composable::AllocatorRebindableSearchAlgo).
    template <class A2>
    using rebind_allocator = OriginalWormholeSearchAlgoRebound<A2>;

    using detail::OriginalWormholeSearchAlgoCore<default_allocator_type,
                                                 OriginalWormholeSearchAlgo>::OriginalWormholeSearchAlgoCore;
};

/// DIE GEBUNDENE FORM -- traegt BEWUSST KEIN COMDARE_DEFINE_ORGAN_LOCATION.
template <class A2>
class OriginalWormholeSearchAlgoRebound final
    : public detail::OriginalWormholeSearchAlgoCore<A2, OriginalWormholeSearchAlgoRebound<A2>> {
public:
    /// Der EBENEN-AUSWEIS (s. composable::IsReboundSearchAlgoLeaf).
    using axis03a_rebound_tag = void;

    using detail::OriginalWormholeSearchAlgoCore<A2,
                                                 OriginalWormholeSearchAlgoRebound<A2>>::OriginalWormholeSearchAlgoCore;
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<OriginalWormholeSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<OriginalWormholeSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<OriginalWormholeSearchAlgo>);
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
static_assert(OriginalWormholeSearchAlgo::is_original_insert(),
              "OriginalWormholeSearchAlgo: insert MUSS via wh_put Paper-Bindung originall sein");
static_assert(OriginalWormholeSearchAlgo::is_original_lookup(),
              "OriginalWormholeSearchAlgo: lookup MUSS via wh_get Paper-Bindung originall sein");
static_assert(OriginalWormholeSearchAlgo::is_original_erase(),
              "OriginalWormholeSearchAlgo: erase MUSS via wh_del Paper-Bindung originall sein");
static_assert(!OriginalWormholeSearchAlgo::is_original_clear(),
              "OriginalWormholeSearchAlgo: clear ist Re-Impl Luecke (kein Bulk-Clear in Wormhole) — MUSS false sein");
static_assert(!OriginalWormholeSearchAlgo::is_original_module(),
              "OriginalWormholeSearchAlgo: is_original_module MUSS false sein (1/4 Luecke)");
#else
static_assert(!OriginalWormholeSearchAlgo::is_original_module(),
              "OriginalWormholeSearchAlgo: is_original_module()=false wenn is_original-Codegen-Gate AUS ist");
#endif

// ---------------------------------------------------------------------------------------------
// Der Zwei-Ebenen-Vertrag, self-proving an der Datei, die ihn eingeht (Pilot-Rezept, linear_scan:352).
// ---------------------------------------------------------------------------------------------
static_assert(std::is_same_v<composable::search_algo_for_composition_t<OriginalWormholeSearchAlgo,
                                                                       ::comdare::cache_engine::alloc::ExgenAllocator>,
                             OriginalWormholeSearchAlgo>,
              "01c Level-0-IDENTITAET verletzt: die Kompositions-Naht liefert am ACHSEN-DEFAULT nicht mehr die "
              "Fassade selbst. Damit laege ein anderer Typ auf dem golden-Pfad -- Typ-Neutralitaet weg.");
static_assert(composable::AllocatorRebindableSearchAlgo<OriginalWormholeSearchAlgo,
                                                        ::comdare::cache_engine::alloc::ExgenAllocator>,
              "01c: OriginalWormholeSearchAlgo traegt keinen rebind_allocator mehr -- nicht migriert.");
static_assert(!composable::IsReboundSearchAlgoLeaf<OriginalWormholeSearchAlgo>,
              "01c EBENEN-VERMISCHUNG: die Fassade traegt den Rebound-Tag -- Emitter-type_name-Reise und "
              "Registry-Reflektion zeigten dann auf die Substanz-Ebene.");
static_assert(composable::IsReboundSearchAlgoLeaf<
                  OriginalWormholeSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>,
              "01c: der Rebound-Leaf traegt seinen Ausweis nicht -- der Identitaets-Pin kann nicht mehr greifen.");
static_assert(composable::search_algo_name_is_allocator_invariant_v<OriginalWormholeSearchAlgo,
                                                                    ::comdare::cache_engine::alloc::ExgenAllocator>,
              "01c name()-INVARIANZ (Level 0) verletzt.");
static_assert(OriginalWormholeSearchAlgo::name() ==
                  OriginalWormholeSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>::name(),
              "01c name()-INVARIANZ verletzt: der Rebound-Leaf traegt einen anderen Organ-Namen als die Fassade -- "
              "die T6-Wahl leckte in den serialize-/binary_id-Schluessel.");
static_assert(
    concepts::SearchAlgoVariant<OriginalWormholeSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<
              OriginalWormholeSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::DensityClassifiedStrategy<
              OriginalWormholeSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
} // namespace comdare::cache_engine::lookup
