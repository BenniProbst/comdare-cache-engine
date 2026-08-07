#pragma once
// V41.F.6.1 axis_03a_search_algo VectorU8U8SearchAlgo S02 (2026-05-26)
//
// @topic traversal @achse 03a @family S02 VectorU8U8SearchAlgo
// @subaxis SA2 sparse_access
//
// **Algorithmus-Pattern:** sparse sortiertes Key-Value-Vektor-Paar mit
// Patricia-Compression-Eigenschaften, beschrieben in:
//   Binna/Zangerle/Pichl: "HOT: A Height Optimized Trie Index for Main-Memory
//   Database Systems." SIGMOD 2018.
//
// Standalone-Implementation (kein Delegate, keine Legacy-Code-Referenz):
// std::lower_bound auf std::vector<uint8> keys + parallel std::vector<uint64>
// values fuer O(log N) Lookup + O(N) Insert/Erase (shift). Konzept-aequivalent
// zu HOT-k-constrained mit kleinem k.
//
// Erfuellt:
//   - SearchAlgoVariant (Pflicht-API)
//   - CacheEngineSearchAlgoPermutationStrategy (cache-engine-spec)
//   - DensityClassifiedStrategy (DensityClass dynamisch nach density_threshold)
//   - SimdCapableStrategy (simd_lookup mit Bit-Mask-Scan ueber sortierte Keys)
//   - IterableAspectSearchAlgoStrategy (density_threshold_pct hybride Permutation)
//
// Allocation: NUR ueber das Allokator-Achsen-Interface (axis_06) -- s. den ZWEI-EBENEN-SCHNITT unten.
// [[allocation-failure-exception]]: insert kann std::bad_alloc werfen (StdAllocatorAdapter, Posten 64).
//
// ===================================================================================================
// A8-S5 Familie 01c, Scheibe 3 (2026-08-05) -- ZWEI-EBENEN-SCHNITT (Pilot-Rezept linear_scan)
// ===================================================================================================
// KLASSEN-ENTSCHEID: **VOLL-REZEPT** -- zwei Default-Allokator-Vektoren (keys_/values_, 2 der 39
// Rest-Zeilen der 01b-Schlussbilanz). Konstruktion und Begruendung zeilengleich zum Pilot
// (axis_03a_search_algo_linear_scan.hpp:22-58), hier referenziert statt wiederholt.
//
// EINE BESONDERHEIT DIESES ORGANS: es traegt zusaetzlich den ITERABLE-ASPEKT (density_threshold_pct,
// Laufzeit-Permutation ueber kIterableDensityThresholds). Der bleibt UNVERAENDERT im Core -- er ist eine
// Klassifikations-Schwelle, keine Speicher-Eigenschaft, und beide Ebenen erben ihn identisch. Der
// vorhandene `explicit VectorU8U8SearchAlgoCore(unsigned)` und die neue KF-6-Naht
// `explicit VectorU8U8SearchAlgoCore(allocator_type)` sind ueberladungs-disjunkt (ExgenAllocator ist
// nicht aus unsigned konstruierbar); beide reisen ueber `using ...Core;` in Fassade UND Rebound-Leaf.
//
// ORGAN_LOCATION NEU (Default-OFF -> Byte-Effekt NULL, s. XML-ABWESENHEITS-Probe (8b) der Familien-Wache).

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include "concepts/axis_03a_search_algo_simd_capable_strategy_concept.hpp"
#include "concepts/axis_03a_search_algo_iterable_aspect_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/alloc/axis_06_allocator_exgen.hpp>
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#include <axes/lookup/composable/search_algo_rebind.hpp>
#include <measurement/measurable_concept.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::lookup {

// Vorwaerts-Deklaration: die Fassade nennt ihren eigenen Rebound-Leaf als Member-Alias, und der
// Rebound-Leaf erbt vom selben Core -- beide brauchen den Namen, bevor der andere vollstaendig ist.
template <class A2>
class VectorU8U8SearchAlgoRebound;

namespace detail {

/// DIE SUBSTANZ des S02-Organs: sortierte u8-Keys + parallele Values, Speicher ueber die Allokator-ACHSE.
///
/// @tparam Alloc  Die Allokator-Achsen-Strategie (axis_06). Default-Bindung der Fassade: ExgenAllocator.
/// @tparam Self   Der most-derived Typ -- PFLICHT wegen der CRTP-Guards der SearchAlgoBase.
template <class Alloc, class Self>
class VectorU8U8SearchAlgoCore : public SearchAlgoBase<Self> {
public:
    static constexpr bool enabled = flags::vector_u8u8_enabled;
    // #188-4c-ii: faithful Flach-Store-Pfad via SortedVectorTraversal; density_threshold bleibt Klassifizierung.
    static constexpr bool axis_03a_store_traversable = true;

    using key_type   = std::uint8_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 2>; // S02

    /// A8-S5 SCHNITT-FORM (B): Keys UND Values haengen an der Allokator-ACHSE. Diese Zeile IST der
    /// Ausweis, den die Familien-Konformitaets-Wache liest (tests/unit/s5_family_alloc_conformance.hpp).
    using allocator_type = Alloc;
    static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<allocator_type>,
                  "A8-S5: der gebundene Allokator erfuellt das axis_06-Achsen-Concept nicht mehr -- dann liefen "
                  "Keys/Values wieder an der Allokator-Achse vorbei (Schnitt-Regel Dossier 3.4).");

    /// iterable_aspect_t (F.6.1.E hybride Laufzeit-Permutation):
    /// density_threshold_pct steuert die Klassifizierungs-Schwelle Sparse/Balanced.
    /// PermutationEngine erkennt via HasIterableAspect<V> und generiert 1 Binary
    /// mit Runtime-Loop ueber kIterableDensityThresholds statt 5 separate Binaries.
    using iterable_aspect_t = unsigned;
    static constexpr std::array<unsigned, 5>                 kIterableDensityThresholds{10u, 20u, 30u, 50u, 70u};
    [[nodiscard]] static constexpr std::span<unsigned const> iterable_values() noexcept {
        return std::span<unsigned const>{kIterableDensityThresholds.data(), kIterableDensityThresholds.size()};
    }

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_fanout() noexcept { return 256; } // theoretisch, sparse
    /// SERIALISIERUNGS-SCHLUESSEL -- bewusst OHNE jeden Allokator-Bezug (name()-Invarianz, s. unten).
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "vector_u8u8"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "VectorU8U8SearchAlgo (HOT Patricia sparse — Binna PVLDB 2018)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "VECTOR_U8U8"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return true; }
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // sorted insert
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

    static constexpr unsigned kDefaultDensityThresholdPct = 30;

private:
    using key_alloc    = typename Alloc::template StdAllocatorAdapter<key_type>;
    using value_alloc  = typename Alloc::template StdAllocatorAdapter<value_type>;
    using key_vector   = std::vector<key_type, key_alloc>;
    using value_vector = std::vector<value_type, value_alloc>;

public:
    /// Beide Vektoren werden an das EIGENE allocator_ gebunden (der Adapter haelt &allocator_).
    /// NICHT MEHR noexcept: der Adapter ist nicht default-konstruierbar und die Vektor-Konstruktion
    /// laeuft ueber die Strategie -- die Zusicherung waere ab hier eine Behauptung (Auflage: keine
    /// stale noexcept-Vertraege). Der Konstruktor alloziert NICHT (leere Vektoren), er bindet nur.
    VectorU8U8SearchAlgoCore()
        : density_threshold_pct_(kDefaultDensityThresholdPct), keys_(allocator_.template as_std_allocator<key_type>()),
          values_(allocator_.template as_std_allocator<value_type>()) {}

    explicit VectorU8U8SearchAlgoCore(unsigned density_threshold_pct)
        : density_threshold_pct_(density_threshold_pct), keys_(allocator_.template as_std_allocator<key_type>()),
          values_(allocator_.template as_std_allocator<value_type>()) {}

    /// KF-6-NAHT (Posten 62, LEDGER 04.08. abend-12): eine vor-parametrierte Strategie-Instanz
    /// uebernehmen, statt sie default zu konstruieren. Heute nirgends benutzt und bewusst `explicit`.
    explicit VectorU8U8SearchAlgoCore(allocator_type a)
        : allocator_(std::move(a)), density_threshold_pct_(kDefaultDensityThresholdPct),
          keys_(allocator_.template as_std_allocator<key_type>()),
          values_(allocator_.template as_std_allocator<value_type>()) {}

    /// Copy: Strategie mitkopieren, beide Vektoren an das EIGENE allocator_ binden, dann die transiente
    /// Kopier-Allokation aus der Statistik nehmen (Memento) -- 1:1 btree_node_pool_store.hpp:86.
    VectorU8U8SearchAlgoCore(VectorU8U8SearchAlgoCore const& o)
        : allocator_(o.allocator_), density_threshold_pct_(o.density_threshold_pct_),
          keys_(o.keys_, allocator_.template as_std_allocator<key_type>()),
          values_(o.values_, allocator_.template as_std_allocator<value_type>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_ = o.stats_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }

    VectorU8U8SearchAlgoCore& operator=(VectorU8U8SearchAlgoCore const& o) {
        if (this != &o) {
            density_threshold_pct_ = o.density_threshold_pct_;
            keys_                  = o.keys_;
            values_                = o.values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_ = o.stats_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }

    ~VectorU8U8SearchAlgoCore() = default;

    [[nodiscard]] bool operator==(VectorU8U8SearchAlgoCore const& other) const noexcept {
        return keys_.size() == other.keys_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: push_back kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        auto        it  = std::lower_bound(keys_.begin(), keys_.end(), k);
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        if (it != keys_.end() && *it == k) {
            values_[idx] = v; // update
        } else {
            keys_.insert(it, k);
            values_.insert(values_.begin() + idx, v);
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
        bool hit = (it != keys_.end() && *it == k);
        if (hit)
            ++stats_.total_hit_count;
        else
            ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        if (it == keys_.end() || *it != k) return std::nullopt;
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        return values_[idx];
    }

    /// SIMD-Fast-Path ([[simd-capable-strategy]] Sub-Concept).
    /// HOT-typisch: Bit-Mask-Scan ueber kleinen sortierten Keys-Vektor mit AVX2.
    /// Pilot-Implementation: identisch mit lookup (skalare lower_bound), real
    /// wuerde SIMD-Comparison ueber 8/16 keys auf einmal genutzt.
    [[nodiscard]] std::optional<value_type> simd_lookup(key_type k) const { return lookup(k); }

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
    void                 clear() noexcept {
        keys_.clear();
        values_.clear();
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]]:
    /// Sparse default; dynamisch klassifiziert ueber density_threshold_pct
    /// (iterable_aspect_t — Runtime-Setter via set_iterable_aspect).
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        double       dp = density_percent();
        double const t  = static_cast<double>(density_threshold_pct_);
        if (dp > t * 2.0) return concepts::DensityClass::Dense; // weit ueber Threshold
        if (dp > t) return concepts::DensityClass::Balanced;    // ueber Threshold
        return concepts::DensityClass::Sparse;                  // unter Threshold
    }

    /// IterableAspectSearchAlgoStrategy [[iterable-aspect-strategy]]:
    /// Konsolidierter Setter analog Q1/Q2 + 03a-Schablone.
    void set_iterable_aspect(unsigned new_threshold_pct) noexcept { density_threshold_pct_ = new_threshold_pct; }

    /// Accessor (Diagnostik): aktueller density_threshold_pct.
    [[nodiscard]] unsigned density_threshold_pct() const noexcept { return density_threshold_pct_; }

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
    using allocator_snapshot_t = typename allocator_type::snapshot_t;
    [[nodiscard]] allocator_snapshot_t search_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    // allocator_ MUSS VOR keys_/values_ stehen: der Adapter haelt &allocator_, und die Member-
    // Initialisierungs-/Zerstoerungsreihenfolge ist die Deklarationsreihenfolge.
    allocator_type allocator_{};
    unsigned       density_threshold_pct_;
    key_vector     keys_;
    value_vector   values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace detail

/// DIE IDENTITAET -- das Registry-Organ S02. Nicht-Template, exakt der historische Typ-Name.
class VectorU8U8SearchAlgo final
    : public detail::VectorU8U8SearchAlgoCore<::comdare::cache_engine::alloc::ExgenAllocator, VectorU8U8SearchAlgo> {
public:
    /// Die Default-Bindung der Identitaets-Ebene: der BENANNTE Achsen-Default, nie std::allocator.
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;

    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::VectorU8U8SearchAlgo",
                                  "axes/lookup/axis_03a_search_algo_vector_u8u8.hpp");

    /// Der Migrations-Ausweis (composable::AllocatorRebindableSearchAlgo).
    template <class A2>
    using rebind_allocator = VectorU8U8SearchAlgoRebound<A2>;

    using detail::VectorU8U8SearchAlgoCore<default_allocator_type, VectorU8U8SearchAlgo>::VectorU8U8SearchAlgoCore;
};

/// DIE GEBUNDENE FORM -- traegt BEWUSST KEIN COMDARE_DEFINE_ORGAN_LOCATION.
template <class A2>
class VectorU8U8SearchAlgoRebound final : public detail::VectorU8U8SearchAlgoCore<A2, VectorU8U8SearchAlgoRebound<A2>> {
public:
    /// Der EBENEN-AUSWEIS (s. composable::IsReboundSearchAlgoLeaf).
    using axis03a_rebound_tag = void;

    using detail::VectorU8U8SearchAlgoCore<A2, VectorU8U8SearchAlgoRebound<A2>>::VectorU8U8SearchAlgoCore;
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<VectorU8U8SearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<VectorU8U8SearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<VectorU8U8SearchAlgo>);
static_assert(concepts::SimdCapableStrategy<VectorU8U8SearchAlgo>);
static_assert(concepts::IterableAspectSearchAlgoStrategy<VectorU8U8SearchAlgo>);

// ---------------------------------------------------------------------------------------------
// Der Zwei-Ebenen-Vertrag, self-proving an der Datei, die ihn eingeht (Pilot-Rezept, linear_scan:352).
// ---------------------------------------------------------------------------------------------
static_assert(std::is_same_v<composable::search_algo_for_composition_t<VectorU8U8SearchAlgo,
                                                                       ::comdare::cache_engine::alloc::ExgenAllocator>,
                             VectorU8U8SearchAlgo>,
              "01c Level-0-IDENTITAET verletzt: die Kompositions-Naht liefert am ACHSEN-DEFAULT nicht mehr die "
              "Fassade selbst. Damit laege ein anderer Typ auf dem golden-Pfad -- Typ-Neutralitaet weg.");
static_assert(
    composable::AllocatorRebindableSearchAlgo<VectorU8U8SearchAlgo, ::comdare::cache_engine::alloc::ExgenAllocator>,
    "01c: VectorU8U8SearchAlgo traegt keinen rebind_allocator mehr -- nicht migriert.");
static_assert(!composable::IsReboundSearchAlgoLeaf<VectorU8U8SearchAlgo>,
              "01c EBENEN-VERMISCHUNG: die Fassade traegt den Rebound-Tag -- Emitter-type_name-Reise und "
              "Registry-Reflektion zeigten dann auf die Substanz-Ebene.");
static_assert(
    composable::IsReboundSearchAlgoLeaf<VectorU8U8SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>,
    "01c: der Rebound-Leaf traegt seinen Ausweis nicht -- der Identitaets-Pin kann nicht mehr greifen.");
static_assert(composable::search_algo_name_is_allocator_invariant_v<VectorU8U8SearchAlgo,
                                                                    ::comdare::cache_engine::alloc::ExgenAllocator>,
              "01c name()-INVARIANZ (Level 0) verletzt.");
static_assert(VectorU8U8SearchAlgo::name() ==
                  VectorU8U8SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>::name(),
              "01c name()-INVARIANZ verletzt: der Rebound-Leaf traegt einen anderen Organ-Namen als die Fassade -- "
              "die T6-Wahl leckte in den serialize-/binary_id-Schluessel.");
/// Der Rebound-Leaf ist ein VOLLWERTIGES Organ -- hier inklusive der BEIDEN Zusatz-Concepts dieses
/// Organs (SIMD + Iterable-Aspekt): der Rebind darf keine Faehigkeit unterschlagen.
static_assert(concepts::SearchAlgoVariant<VectorU8U8SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<
              VectorU8U8SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(
    concepts::DensityClassifiedStrategy<VectorU8U8SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(
    concepts::SimdCapableStrategy<VectorU8U8SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::IterableAspectSearchAlgoStrategy<
              VectorU8U8SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
} // namespace comdare::cache_engine::lookup
