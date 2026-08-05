#pragma once
// V41.F.6.1 R7.2 axis_03a_search_algo KArySearchAlgo S10 (2026-05-29)
//
// @topic traversal @achse 03a @family S10 KArySearchAlgo
// @subaxis SA2 sparse_access (sortierte Keys, Such-METHODE = k-ary statt binary)
//
// **Algorithmus:** k-ary search — Verallgemeinerung der Binärsuche, beschrieben in:
//   Benjamin Schlegel, Rainer Gemulla, Wolfgang Lehner: "k-ary search on modern
//   processors." DaMoN 2009 (5th Int. Workshop on Data Management on New Hardware),
//   Providence RI. DOI 10.1145/1565694.1565705.
//
// Binärsuche macht 1 Vergleich pro Iteration → Halbierung (log_2 n Iterationen).
// k-ary search macht K Vergleiche pro Iteration gegen K gleichverteilte Separatoren →
// Partition in K+1 Segmente (⌈log_(K+1) n⌉ Iterationen). Auf modernen Prozessoren
// amortisiert die parallele Vergleichsausführung (ILP/SIMD) die Mehrkosten pro Iteration,
// sodass die geringere Iterationszahl gewinnt — exakt das F15-Thema (Such-Methode im
// std::map-Innenleben → Performance). Die Arität K ist hier der iterable_aspect: die
// PermutationEngine misst K ∈ {2,4,8,16} (K=2 ≙ Binärsuche als Baseline).
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** Das Paper liefert die
// Algorithmen plattformunabhängig (Pseudocode) + Mess-Studie, KEINEN kanonischen
// permissiven Single-Repo-Code. Diese Implementierung ist eine originalgetreue
// C++23-Re-Implementierung der skalaren k-ary-Suche (sequential layout) →
// is_original = false (via AxisBase-Default). simd_lookup() markiert den
// data-level-parallel-Layout-Fast-Path (Paper §4); Pilot delegiert skalar.
//
// Erfuellt: SearchAlgoVariant, CacheEngineSearchAlgoPermutationStrategy,
//           DensityClassifiedStrategy, SimdCapableStrategy, IterableAspectSearchAlgoStrategy.
//
// Allocation: NUR ueber das Allokator-Achsen-Interface (axis_06) -- s. den ZWEI-EBENEN-SCHNITT unten.
// [[allocation-failure-exception]]: insert kann std::bad_alloc werfen (StdAllocatorAdapter, Posten 64).
//
// ===================================================================================================
// A8-S5 Familie 01c, Scheibe 2 (2026-08-05) -- ZWEI-EBENEN-SCHNITT (Pilot-Rezept linear_scan)
// ===================================================================================================
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4.
// Owner-KERN: LEDGER 04.08.2026 abend-11 ("Option B strikt"), Design-Entscheid abend-14 (D1).
// Die drei Ebenen (Core / namens-stabile Fassade / Rebound-Leaf) und ihre Begruendung stehen im Pilot
// (axis_03a_search_algo_linear_scan.hpp:22-58) und werden hier NICHT wiederholt.
//
// DIESE DATEI TRAEGT ZWEI FAMILIEN, und beide bekommen den Schnitt getrennt:
//   * KArySearchAlgo (S10, name "k_ary", ENABLED) -- die LAUFZEIT-Aritaets-Variante. Sie ist das
//     Registry-Organ, das in der committeten cache_engine_axis_registry.xml steht; ihre Fassade muss
//     deshalb nicht-Template bleiben (F30-Guard, axis_registry_gen main.cpp:161-162/:255-263).
//   * KArySearchAlgoK2/K4/K8/K16 (S18-S21, Default-OFF) -- die COMPILE-TIME-K-Familie, weiter unten.
// Vorwaerts-Deklaration `class KArySearchAlgo;` in composable/traversal_for_search_algo.hpp:37 -- eine
// Alias-Loesung haette sie sofort gebrochen; die Fassade haelt sie unveraendert.

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
class KArySearchAlgoRebound;

namespace detail {

/// DIE SUBSTANZ des S10-Organs: k-ary-Suche mit LAUFZEIT-Aritaet, Keys/Values ueber die Allokator-ACHSE.
///
/// @tparam Alloc  Die Allokator-Achsen-Strategie (axis_06). Default-Bindung der Fassade: ExgenAllocator.
/// @tparam Self   Der most-derived Typ (Fassade oder Rebound-Leaf) -- PFLICHT wegen der CRTP-Guards
///                der SearchAlgoBase (axis_03a_search_algo_base.hpp:47-59).
template <class Alloc, class Self>
class KArySearchAlgoCore : public SearchAlgoBase<Self> {
public:
    static constexpr bool enabled = flags::k_ary_enabled;
    // (E-Welle-A2 / Befund-2 / #188-4a-C5, 2026-06-29) k-ary ist Array-Familie (flacher SORTIERTER Store). Das TREUE
    // Traversal-Organ KAryTraversal<Arity> (axes/lookup/composable/k_ary_traversal_organ.hpp) ist compile-time +
    // std::map-konform (test_conformance_gate, k_ary<Arity=2/4/8/16> alle grün). AKTIVIERT (Weg-A): container_ führt
    // k-ary über DENSELBEN node/layout/allocator-LayoutAwareChunkedStore statt vor #188 entferntem SortedBinary-Spiegel
    // (Meta-Lehre #3 erfüllt). K-Variation = COMPILE-TIME-Permutation (User-Entscheid SE-13, KEIN Runtime-Kanal — der
    // wurde verworfen): der per-K-StaticAxisNode-Build (profile_to_tree) emittiert K∈{2,4,8,16} als EIGENE Binaries
    // (je KAryTraversal<K>). Default-Mapping = KAryTraversal<4u> (traversal_for_search_algo); per-K-Build = Folgestufe
    // (harness-gated #162). Greift in die Daten erst beim #215-320-DLL-Neubau.
    static constexpr bool axis_03a_store_traversable = true;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 10>; // S10

    /// A8-S5 SCHNITT-FORM (B): Keys UND Values haengen an der Allokator-ACHSE. Diese Zeile IST der
    /// Ausweis, den die Familien-Konformitaets-Wache liest (tests/unit/s5_family_alloc_conformance.hpp);
    /// anders als in 01a/01d ist sie NICHT fest, sondern der Kompositions-Parameter.
    using allocator_type = Alloc;
    static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<allocator_type>,
                  "A8-S5: der gebundene Allokator erfuellt das axis_06-Achsen-Concept nicht mehr -- dann liefen "
                  "Keys/Values wieder an der Allokator-Achse vorbei (Schnitt-Regel Dossier 3.4).");

    /// iterable_aspect_t = Arität K (Such-METHODE). K-Variation = COMPILE-TIME-Permutation (SE-13): der
    /// per-K-StaticAxisNode-Build (profile_to_tree) emittiert je K aus kIterableArities eine EIGENE Tier-Binary
    /// (je KAryTraversal<K>) — KEIN Runtime-Loop (der wurde verworfen). K=2 = Binärsuch-Baseline; K∈{4,8,16} = echte k-ary-Varianten.
    using iterable_aspect_t = unsigned;
    static constexpr std::array<unsigned, 4>                 kIterableArities{2u, 4u, 8u, 16u};
    [[nodiscard]] static constexpr std::span<unsigned const> iterable_values() noexcept {
        return std::span<unsigned const>{kIterableArities.data(), kIterableArities.size()};
    }

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_fanout() noexcept { return 65536; } // u16 Keyraum
    /// SERIALISIERUNGS-SCHLUESSEL -- bewusst OHNE jeden Allokator-Bezug (s. name()-Invarianz-Wache
    /// unter der Fassade): die T6-Wahl darf nie in den serialize-/binary_id-Schluessel lecken.
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "k_ary"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "KArySearchAlgo (k-ary search — Schlegel/Gemulla/Lehner DaMoN 2009)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "K_ARY"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0c";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return true; } // data-parallel Layout (Paper §4)
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // sortiert
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }           // sparse sortiert
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

    static constexpr unsigned kDefaultArity = 4; // 5-Wege-Partition (Paper-Beispiel)

private:
    using key_alloc    = typename Alloc::template StdAllocatorAdapter<key_type>;
    using value_alloc  = typename Alloc::template StdAllocatorAdapter<value_type>;
    using key_vector   = std::vector<key_type, key_alloc>;
    using value_vector = std::vector<value_type, value_alloc>;

public:
    /// Beide Vektoren werden an das EIGENE allocator_ gebunden (der Adapter haelt &allocator_).
    KArySearchAlgoCore()
        : arity_(kDefaultArity), keys_(allocator_.template as_std_allocator<key_type>()),
          values_(allocator_.template as_std_allocator<value_type>()) {}

    /// Laufzeit-Aritaet vorgeben (bestehender SE-13-Kanal, unveraendert).
    explicit KArySearchAlgoCore(unsigned arity)
        : arity_(arity < 2u ? 2u : arity), keys_(allocator_.template as_std_allocator<key_type>()),
          values_(allocator_.template as_std_allocator<value_type>()) {}

    /// KF-6-NAHT (Posten 62, LEDGER 04.08. abend-12): eine vor-parametrierte Strategie-Instanz
    /// uebernehmen, statt sie default zu konstruieren. Heute nirgends benutzt und bewusst `explicit`.
    explicit KArySearchAlgoCore(allocator_type a)
        : allocator_(std::move(a)), arity_(kDefaultArity),
          keys_(allocator_.template as_std_allocator<key_type>()),
          values_(allocator_.template as_std_allocator<value_type>()) {}

    /// Copy: Strategie mitkopieren, beide Vektoren an das EIGENE allocator_ binden, dann die transiente
    /// Kopier-Allokation aus der Statistik nehmen (Memento) -- 1:1 btree_node_pool_store.hpp:86.
    /// MOVE bewusst NICHT deklariert (der user-definierte Copy unterdrueckt ihn implizit).
    KArySearchAlgoCore(KArySearchAlgoCore const& o)
        : allocator_(o.allocator_), arity_(o.arity_),
          keys_(o.keys_, allocator_.template as_std_allocator<key_type>()),
          values_(o.values_, allocator_.template as_std_allocator<value_type>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_ = o.stats_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }

    KArySearchAlgoCore& operator=(KArySearchAlgoCore const& o) {
        if (this != &o) {
            // POCCA=false (der Adapter fuehrt keine propagate_-Typedefs) -> die Vektoren behalten ihr an
            // this-allocator_ gebundenes Adapter-Objekt; die Zuweisung re-alloziert ueber this-allocator_.
            arity_  = o.arity_;
            keys_   = o.keys_;
            values_ = o.values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_ = o.stats_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }

    ~KArySearchAlgoCore() = default;

    [[nodiscard]] bool operator==(KArySearchAlgoCore const& other) const noexcept {
        return keys_.size() == other.keys_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: insert kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        auto        it  = std::lower_bound(keys_.begin(), keys_.end(), k);
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        if (it != keys_.end() && *it == k) {
            values_[idx] = v; // update
        } else {
            keys_.insert(it, k);
            values_.insert(values_.begin() + static_cast<std::ptrdiff_t>(idx), v);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (keys_.size() > stats_.peak_occupancy) stats_.peak_occupancy = keys_.size();
        observer_.notify(stats_);
#endif
    }

    /// k-ary search (Schlegel DaMoN 2009): pro Iteration werden bis zu K gleichverteilte
    /// Separatoren verglichen → Partition in K+1 Segmente. Bei Restbreite <= K linearer Scan.
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        std::size_t const         n      = keys_.size();
        std::size_t               lo = 0, hi = n; // Halb-offenes Intervall [lo, hi)
        std::size_t const         K = arity_;
        while (hi - lo > K) {
            std::size_t const width  = hi - lo;
            std::size_t       new_lo = lo, new_hi = hi;
            bool              narrowed = false;
            for (std::size_t j = 1; j <= K; ++j) {
                std::size_t const pos = lo + (width * j) / (K + 1); // Separator-Position in [lo, hi)
                key_type const    sep = keys_[pos];
                if (sep == k) { // Direkt-Treffer auf Separator
#ifdef COMDARE_CE_ENABLE_STATISTICS
                    ++stats_.total_lookup_count;
                    ++stats_.total_hit_count;
                    observer_.notify(stats_);
#endif
                    return values_[pos];
                }
                if (k < sep) {
                    new_hi   = pos;
                    narrowed = true;
                    break;
                } // Ziel im Segment vor pos
                new_lo = pos + 1; // Ziel hinter diesem Separator
            }
            lo = new_lo;
            if (narrowed) hi = new_hi; // sonst: k > alle Separatoren → [letzter_sep+1, hi)
        }
        // Rest-Segment (Breite <= K): linearer Scan
        for (std::size_t i = lo; i < hi; ++i) {
            if (keys_[i] == k) {
                result = values_[i];
                break;
            }
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        if (result)
            ++stats_.total_hit_count;
        else
            ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        return result;
    }

    /// SIMD-Fast-Path ([[simd-capable-strategy]]): data-level-parallel Layout (Paper §4) vergleicht
    /// die K Separatoren in einem SIMD-Register. Pilot delegiert skalar an lookup().
    [[nodiscard]] std::optional<value_type> simd_lookup(key_type k) const { return lookup(k); }

    bool erase(key_type k) {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        if (it == keys_.end() || *it != k) return false;
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        keys_.erase(it);
        values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(idx));
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return keys_.size(); }
    [[nodiscard]] double    density_percent() const noexcept {
        return 100.0 * static_cast<double>(keys_.size()) / 65536.0;
    }
    void clear() noexcept {
        keys_.clear();
        values_.clear();
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]]: Belegungs-basierte Klassifikation
    /// (k-ary search lohnt erst ab groesseren sortierten Regionen).
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        std::size_t const n = keys_.size();
        if (n > 1024) return concepts::DensityClass::Dense;
        if (n > 64) return concepts::DensityClass::Balanced;
        return concepts::DensityClass::Sparse;
    }

    /// IterableAspectSearchAlgoStrategy [[iterable-aspect-strategy]]: Setter fuer die Arität K
    /// (konsolidierte Laufzeit-Permutation analog 03a-Schablone).
    void set_iterable_aspect(unsigned new_arity) noexcept { arity_ = (new_arity < 2u ? 2u : new_arity); }

    /// Accessor (Diagnostik): aktuelle Arität K.
    [[nodiscard]] unsigned arity() const noexcept { return arity_; }

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

    /// EINSAMMEL-NAHT der T6-Durchbindung (Owner-KERN abend-11, Pflicht (a)) -- NUR die Naht.
    /// BEWUSST EIN VIERTER NAME neben store_/traversal_/mapping_/slot_allocator_statistics: jede
    /// Organ-Instanz haelt ihre EIGENE Strategie-Instanz, die Snapshots sind also DISJUNKT zum
    /// konstitutiven Store-Snapshot. Die Einsammlung + die Doppelzaehlungs-Regel sind der EXPLIZITE
    /// Schritt des Mess-Schnitt-Fensters VOR Messbeginn.
    using allocator_snapshot_t = typename allocator_type::snapshot_t;
    [[nodiscard]] allocator_snapshot_t search_allocator_statistics() const noexcept {
        return allocator_.statistics();
    }
#endif

private:
    // allocator_ MUSS VOR keys_/values_ stehen: der Adapter haelt &allocator_, und die Member-
    // Initialisierungs-/Zerstoerungsreihenfolge ist die Deklarationsreihenfolge (die Vektoren muessen
    // VOR der Strategie sterben). Dieselbe Reihenfolge wie in den Pool-Stores (01a), 01d und im Pilot.
    allocator_type allocator_{};
    unsigned       arity_;
    key_vector     keys_;
    value_vector   values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace detail

/// DIE IDENTITAET -- das Registry-Organ S10. Nicht-Template, exakt der historische Typ-Name.
class KArySearchAlgo final
    : public detail::KArySearchAlgoCore<::comdare::cache_engine::alloc::ExgenAllocator, KArySearchAlgo> {
public:
    /// Die Default-Bindung der Identitaets-Ebene: der BENANNTE Achsen-Default, nie std::allocator.
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;

    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::KArySearchAlgo",
                                  "axes/lookup/axis_03a_search_algo_k_ary.hpp");

    /// Der Migrations-Ausweis (composable::AllocatorRebindableSearchAlgo).
    template <class A2>
    using rebind_allocator = KArySearchAlgoRebound<A2>;

    using detail::KArySearchAlgoCore<default_allocator_type, KArySearchAlgo>::KArySearchAlgoCore;
};

/// DIE GEBUNDENE FORM -- traegt BEWUSST KEIN COMDARE_DEFINE_ORGAN_LOCATION.
template <class A2>
class KArySearchAlgoRebound final : public detail::KArySearchAlgoCore<A2, KArySearchAlgoRebound<A2>> {
public:
    /// Der EBENEN-AUSWEIS (s. composable::IsReboundSearchAlgoLeaf).
    using axis03a_rebound_tag = void;

    using detail::KArySearchAlgoCore<A2, KArySearchAlgoRebound<A2>>::KArySearchAlgoCore;
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<KArySearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<KArySearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<KArySearchAlgo>);
static_assert(concepts::SimdCapableStrategy<KArySearchAlgo>);
static_assert(concepts::IterableAspectSearchAlgoStrategy<KArySearchAlgo>);

// ---------------------------------------------------------------------------------------------
// Der Zwei-Ebenen-Vertrag, self-proving an der Datei, die ihn eingeht (Pilot-Rezept, linear_scan:352).
// ---------------------------------------------------------------------------------------------

/// LEVEL 0 (der golden-Pfad): die Kompositions-Naht liefert am Achsen-Default die FASSADE SELBST.
static_assert(std::is_same_v<composable::search_algo_for_composition_t<
                                 KArySearchAlgo, ::comdare::cache_engine::alloc::ExgenAllocator>,
                             KArySearchAlgo>,
              "01c Level-0-IDENTITAET verletzt: die Kompositions-Naht liefert am ACHSEN-DEFAULT nicht mehr die "
              "Fassade selbst. Damit laege ein anderer Typ auf dem golden-Pfad -- Typ-Neutralitaet weg.");

static_assert(composable::AllocatorRebindableSearchAlgo<KArySearchAlgo,
                                                        ::comdare::cache_engine::alloc::ExgenAllocator>,
              "01c: KArySearchAlgo traegt keinen rebind_allocator mehr -- nicht migriert.");

/// EBENEN-TRENNUNG.
static_assert(!composable::IsReboundSearchAlgoLeaf<KArySearchAlgo>,
              "01c EBENEN-VERMISCHUNG: die Fassade traegt den Rebound-Tag.");
static_assert(
    composable::IsReboundSearchAlgoLeaf<KArySearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>,
    "01c: der Rebound-Leaf traegt seinen Ausweis nicht.");

/// name()-ALLOKATOR-INVARIANZ.
static_assert(composable::search_algo_name_is_allocator_invariant_v<
                  KArySearchAlgo, ::comdare::cache_engine::alloc::ExgenAllocator>,
              "01c name()-INVARIANZ (Level 0) verletzt.");
static_assert(KArySearchAlgo::name() ==
                  KArySearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>::name(),
              "01c name()-INVARIANZ verletzt: der Rebound-Leaf traegt einen anderen Organ-Namen als die Fassade.");

/// Der Rebound-Leaf ist ein VOLLWERTIGES Organ -- inklusive der beiden ZUSATZ-Concepts dieser Familie
/// (SIMD-Fast-Path + der Laufzeit-Aritaets-Kanal), die nur k_ary traegt.
static_assert(concepts::SearchAlgoVariant<KArySearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<
              KArySearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(
    concepts::DensityClassifiedStrategy<KArySearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::SimdCapableStrategy<KArySearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>,
              "01c: der Rebound-Leaf verlor den SIMD-Fast-Path -- die Ebenen truegen verschiedene Faehigkeiten.");
static_assert(
    concepts::IterableAspectSearchAlgoStrategy<KArySearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>,
    "01c: der Rebound-Leaf verlor den Laufzeit-Aritaets-Kanal (iterable_aspect) der S10-Familie.");
} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {

// =====================================================================================================
// #188 per-K-Build Increment 1 (2026-07-01) — compile-time-K k-ary Wrapper-Familie (Weg-A).
// =====================================================================================================
// Die per-K-Permutation der k-ary-Such-Achse als EIGENER, compile-time fixierter Typ je K
// (StaticAxisNode-Kandidat). Sie ERSETZT den verworfenen SE-13-Runtime-Kanal (iterable_aspect_t/arity_/
// set_iterable_aspect): K ist hier ein NON-TYPE-TEMPLATE-PARAMETER, kein Laufzeit-Setter — reale k-ary-Impls
// waehlen K ebenfalls statisch (SIMD-Breite, Paper Abschn. 4). Jede Variante ist ein DISTINKTER Typ mit DISTINKTEM
// name() ("k_ary_k2".."k_ary_k16") -> in Increment 2 eine EIGENE Tier-Binary je K (kein binary_id-Kollaps/
// Dedup, Risk#3). Marker axis_03a_store_traversable=true -> container_ (abi_adapter:1890 container_traversal_t)
// fuehrt sie ueber KAryTraversal<K> (traversal_for_search_algo, Header 2), NICHT SortedBinary (Weg-A, Risk#2).
//
// Increment 1 war rein ADDITIV, KEINE Registry-Aenderung; Increment 2 registrierte die vier in AllStrategies
// mit je EIGENEM Enable-Flag (Default OFF). Die Legacy KArySearchAlgo (Runtime-Aritaet, name "k_ary", S10)
// bleibt bis zur Increment-2-Registry-Umschaltung UNANGETASTET (Profile/adhoc_emitter/Fixtures haengen am
// Literal "k_ary"; ihre Identitaet darf hier nicht brechen).
//
// lookup = bit-identisch zur Legacy KArySearchAlgo::lookup, nur K = kArity (compile-time-Konstante) statt des
// Laufzeit-arity_. Die std::map-Konformitaet je K ist am treuen Organ KAryTraversal<K> bereits festgenagelt
// (test_conformance_gate.cpp run_kary_arity_gate<2/4/8/16>); der Wrapper-lookup traegt denselben Separator-Pfad.
// (Bewusste, TRANSIENTE Duplikation der Legacy-Schleife: die Legacy wird in Increment 2 mit der Registry-
//  Umschaltung entfernt; ein Refactoring des verifizierten Legacy-Hot-Pfads waere ein nicht notwendiges
//  Regressions-Risiko -> additiv + Legacy unberuehrt ist der sauberere, sichere Weg.)
//
// =====================================================================================================
// A8-S5 01c, Scheibe 2 (2026-08-05) -- PER-K-LEAF-HEBUNG (Owner-Punkt (ii), LEDGER 04.08. abend-14).
// =====================================================================================================
// Bis hierher waren KArySearchAlgoK2..K16 ALIASE auf eine Template-Id (`using KArySearchAlgoK2 =
// KArySearchAlgoT<2u>;`). Das trug eine LATENTE Registry-XML-Kante, die nur deshalb nie zuschlug, weil die
// vier Default-OFF sind: der Generator reflektiert `type_name<W>()` (axis_registry_gen main.cpp:161-162);
// bei einer Template-Id waere `type=` "...::KArySearchAlgoT<2>" und `wrapper=` entsprechend -- und der
// F30-Guard (:255-263, `type=` muss mit dem ORGAN_LOCATION-Literal BEGINNEN) haette gar keine Eingabe
// gehabt, weil KArySearchAlgoT nie ein COMDARE_DEFINE_ORGAN_LOCATION trug. Der Tag, an dem jemand ein
// per-K-Flag anschaltet, waere der Tag des XML-Byte-Ereignisses gewesen -- ohne Vorwarnung.
//
// Diese Scheibe schliesst die Kante KONSTRUKTIV statt sie stehenzulassen: die Substanz wandert nach
// detail::KAryPerKCore<K, Alloc, Self> (dieselbe Zwei-Ebenen-Konstruktion wie bei den vier enabled
// Organen), und die vier per-K-Namen werden zu ECHTEN LEAF-KLASSEN -- nicht-Template, mit eigenem
// COMDARE_DEFINE_ORGAN_LOCATION, damit der F30-Guard sie ab sofort ueberhaupt PRUEFEN kann. Ihr
// type_name ist damit argument-frei, und ein spaeteres Einschalten ist ein reiner, vorhersagbarer
// Registry-Zuwachs statt eines Formwechsels.
// AM IST BEWEGT SICH NICHTS: alle vier bleiben Default-OFF, der Generator reflektiert nur Enabled*,
// also traegt die committete cache_engine_axis_registry.xml sie weiterhin NICHT -- belegt per
// git-status + Generator-cmp + der Abwesenheits-Probe in tests/unit/test_s5_01c_fassaden_conformance.
//
// KArySearchAlgoT<K> ist damit ERSETZT, nicht danebengelegt (Owner-KERN Aufraeumpass): der einzige
// Konsument war das Traversal-Mapping, das jetzt auf die vier Leaf-Klassen + den Rebound-Leaf keyed
// (composable/traversal_for_search_algo.hpp).
namespace detail {
/// #188 per-K Increment 2: compile-time K -> SEIN eigener Enable-Flag (Default OFF; opt-in wie OriginalXxx). Nicht-
/// kanonisches K ist nie enabled (nur K in {2,4,8,16} werden registriert/gebaut). Saubere Trennung statt Familien-Flag.
template <unsigned K>
[[nodiscard]] constexpr bool k_ary_per_k_enabled() noexcept {
    if constexpr (K == 2u)
        return flags::k_ary_k2_enabled;
    else if constexpr (K == 4u)
        return flags::k_ary_k4_enabled;
    else if constexpr (K == 8u)
        return flags::k_ary_k8_enabled;
    else if constexpr (K == 16u)
        return flags::k_ary_k16_enabled;
    else
        return false;
}

/// DIE SUBSTANZ der per-K-Familie (S18-S21): k-ary-Suche mit COMPILE-TIME-Aritaet, Keys/Values ueber die
/// Allokator-ACHSE. Drei Parameter statt zwei, weil zur Strategie/Self-Achse die Aritaet als NTTP tritt.
///
/// @tparam K      Die compile-time Aritaet (>= 2).
/// @tparam Alloc  Die Allokator-Achsen-Strategie (axis_06). Default-Bindung der Leaf-Klassen: ExgenAllocator.
/// @tparam Self   Der most-derived Typ (Leaf-Klasse oder Rebound-Leaf) -- PFLICHT wegen der CRTP-Guards.
template <unsigned K, class Alloc, class Self>
class KAryPerKCore : public SearchAlgoBase<Self> {
    static_assert(K >= 2u, "#188 per-K: Aritaet K muss >= 2 sein (K=2 = Binaersuch-Baseline; K<2 waere linearer Scan)");

public:
    // Je-K EIGENER Enable-Flag (Default OFF, opt-in wie der OriginalXxx-Praezedenzfall) -> die Registrierung in
    // AllStrategies ist nicht-disruptiv (EnabledStrategies waechst NICHT); die Aktivierung erfolgt gezielt fuer den
    // per-K-Mess-Lauf (Increment 2b), ohne golden-320/EnabledStrategies-Tests pauschal zu stoeren.
    static constexpr bool enabled = detail::k_ary_per_k_enabled<K>();
    // Weg-A-Marker (Risk#2: fehlt er, faellt container_traversal_t auf SortedBinaryTraversal = falsches Organ zurueck).
    static constexpr bool axis_03a_store_traversable = true;
    /// Compile-time-Aritaet K (Risk#5: KEIN Runtime-arity_ -> Pfad-A run_workload misst K nicht als K=4-Phantom).
    static constexpr unsigned kArity = K;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 10>; // S10 (dieselbe k-ary-Familie wie Legacy)

    /// A8-S5 SCHNITT-FORM (B): Keys UND Values haengen an der Allokator-ACHSE -- wie bei der Legacy-Fassade,
    /// nur dass die Aritaet hier compile-time ist. Die per-K-Familie ist Default-OFF, aber der Schnitt gilt
    /// ihr trotzdem: ein spaeter eingeschaltetes Organ soll nicht erst nachgezogen werden muessen.
    using allocator_type = Alloc;
    static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<allocator_type>,
                  "A8-S5: der gebundene Allokator erfuellt das axis_06-Achsen-Concept nicht mehr -- dann liefen "
                  "Keys/Values wieder an der Allokator-Achse vorbei (Schnitt-Regel Dossier 3.4).");

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_fanout() noexcept { return 65536; } // u16 Keyraum
    /// DISTINKTE name() je K (Risk#3: binary_id/Hash wird aus name() abgeleitet -> gleiche Namen kollidieren ->
    /// Dedup -> nur 1 Binary statt 4). Die 4 kanonischen Aritaeten sind paarweise + vs Legacy-"k_ary" distinkt.
    [[nodiscard]] static constexpr std::string_view name() noexcept {
        if constexpr (K == 2u)
            return "k_ary_k2";
        else if constexpr (K == 4u)
            return "k_ary_k4";
        else if constexpr (K == 8u)
            return "k_ary_k8";
        else if constexpr (K == 16u)
            return "k_ary_k16";
        else
            return "k_ary_kN"; // Fallback (nicht-kanonisches K; nur die 4 Standard-Aritaeten werden registriert)
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "KArySearchAlgoK<K> (k-ary search compile-time per-K — Schlegel/Gemulla/Lehner DaMoN 2009)";
    }
    /// Distinkter flag_suffix je K (korrespondiert 1:1 mit COMDARE_AXIS_03A_ENABLE_K_ARY_K<N> — je eigenes Flag).
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept {
        if constexpr (K == 2u)
            return "K_ARY_K2";
        else if constexpr (K == 4u)
            return "K_ARY_K4";
        else if constexpr (K == 8u)
            return "K_ARY_K8";
        else if constexpr (K == 16u)
            return "K_ARY_K16";
        else
            return "K_ARY_KN";
    }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0c";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return true; } // data-parallel Layout (Paper §4)
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // sortiert
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }           // sparse sortiert
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

private:
    using key_alloc    = typename Alloc::template StdAllocatorAdapter<key_type>;
    using value_alloc  = typename Alloc::template StdAllocatorAdapter<value_type>;
    using key_vector   = std::vector<key_type, key_alloc>;
    using value_vector = std::vector<value_type, value_alloc>;

public:
    /// KEIN arity_-Ctor (K ist compile-time). Beide Vektoren an das EIGENE allocator_ gebunden.
    KAryPerKCore()
        : keys_(allocator_.template as_std_allocator<key_type>()),
          values_(allocator_.template as_std_allocator<value_type>()) {}

    /// KF-6-NAHT (Posten 62): eine vor-parametrierte Strategie-Instanz uebernehmen, bewusst `explicit`.
    explicit KAryPerKCore(allocator_type a)
        : allocator_(std::move(a)), keys_(allocator_.template as_std_allocator<key_type>()),
          values_(allocator_.template as_std_allocator<value_type>()) {}

    /// Copy: Strategie mitkopieren, beide Vektoren an das EIGENE allocator_ binden, dann die transiente
    /// Kopier-Allokation aus der Statistik nehmen (Memento). MOVE bewusst NICHT deklariert.
    KAryPerKCore(KAryPerKCore const& o)
        : allocator_(o.allocator_), keys_(o.keys_, allocator_.template as_std_allocator<key_type>()),
          values_(o.values_, allocator_.template as_std_allocator<value_type>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_ = o.stats_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }

    KAryPerKCore& operator=(KAryPerKCore const& o) {
        if (this != &o) {
            // POCCA=false -> die Vektoren behalten ihr an this-allocator_ gebundenes Adapter-Objekt.
            keys_   = o.keys_;
            values_ = o.values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_ = o.stats_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }

    ~KAryPerKCore() = default;

    [[nodiscard]] bool operator==(KAryPerKCore const& other) const noexcept {
        return keys_.size() == other.keys_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: insert kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        auto        it  = std::lower_bound(keys_.begin(), keys_.end(), k);
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        if (it != keys_.end() && *it == k) {
            values_[idx] = v; // update
        } else {
            keys_.insert(it, k);
            values_.insert(values_.begin() + static_cast<std::ptrdiff_t>(idx), v);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (keys_.size() > stats_.peak_occupancy) stats_.peak_occupancy = keys_.size();
        observer_.notify(stats_);
#endif
    }

    /// k-ary search (Schlegel DaMoN 2009), K = kArity compile-time fixiert: pro Iteration bis zu K gleichverteilte
    /// Separatoren -> Partition in K+1 Segmente; Restbreite <= K -> linearer Scan. Bit-identisch zur Legacy
    /// KArySearchAlgo::lookup (nur K statt arity_) -> selbe lower_bound-Semantik (Wert iff Key vorhanden, sonst nullopt).
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        std::size_t const         n      = keys_.size();
        std::size_t               lo = 0, hi = n; // Halb-offenes Intervall [lo, hi)
        constexpr std::size_t     Kc = kArity;    // compile-time Aritaet
        while (hi - lo > Kc) {
            std::size_t const width  = hi - lo;
            std::size_t       new_lo = lo, new_hi = hi;
            bool              narrowed = false;
            for (std::size_t j = 1; j <= Kc; ++j) {
                std::size_t const pos = lo + (width * j) / (Kc + 1); // Separator-Position in [lo, hi)
                key_type const    sep = keys_[pos];
                if (sep == k) { // Direkt-Treffer auf Separator
#ifdef COMDARE_CE_ENABLE_STATISTICS
                    ++stats_.total_lookup_count;
                    ++stats_.total_hit_count;
                    observer_.notify(stats_);
#endif
                    return values_[pos];
                }
                if (k < sep) {
                    new_hi   = pos;
                    narrowed = true;
                    break;
                } // Ziel im Segment vor pos
                new_lo = pos + 1; // Ziel hinter diesem Separator
            }
            lo = new_lo;
            if (narrowed) hi = new_hi; // sonst: k > alle Separatoren -> [letzter_sep+1, hi)
        }
        for (std::size_t i = lo; i < hi; ++i) { // Rest-Segment (Breite <= K): linearer Scan
            if (keys_[i] == k) {
                result = values_[i];
                break;
            }
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        if (result)
            ++stats_.total_hit_count;
        else
            ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        return result;
    }

    /// SIMD-Fast-Path ([[simd-capable-strategy]]): data-level-parallel Layout (Paper §4); Pilot delegiert skalar.
    [[nodiscard]] std::optional<value_type> simd_lookup(key_type k) const { return lookup(k); }

    bool erase(key_type k) {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        if (it == keys_.end() || *it != k) return false;
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        keys_.erase(it);
        values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(idx));
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return keys_.size(); }
    [[nodiscard]] double    density_percent() const noexcept {
        return 100.0 * static_cast<double>(keys_.size()) / 65536.0;
    }
    void clear() noexcept {
        keys_.clear();
        values_.clear();
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]]: Belegungs-basierte Klassifikation.
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        std::size_t const n = keys_.size();
        if (n > 1024) return concepts::DensityClass::Dense;
        if (n > 64) return concepts::DensityClass::Balanced;
        return concepts::DensityClass::Sparse;
    }

    // KEIN set_iterable_aspect / arity() / iterable_values (verworfener SE-13-Runtime-Kanal; K ist compile-time).

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

    /// EINSAMMEL-NAHT der T6-Durchbindung (Owner-KERN abend-11, Pflicht (a)) -- NUR die Naht.
    using allocator_snapshot_t = typename allocator_type::snapshot_t;
    [[nodiscard]] allocator_snapshot_t search_allocator_statistics() const noexcept {
        return allocator_.statistics();
    }
#endif

private:
    // allocator_ MUSS VOR keys_/values_ stehen (der Adapter haelt &allocator_).
    allocator_type allocator_{};
    key_vector     keys_;
    value_vector   values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace detail

/// DIE GEBUNDENE FORM der per-K-Familie -- EIN Template fuer alle vier Aritaeten (der Rebound-Leaf ist
/// kein Registry-Organ, seine Template-Id geht nirgends auf die Reise). Traegt BEWUSST KEIN
/// COMDARE_DEFINE_ORGAN_LOCATION.
template <unsigned K, class A2>
class KArySearchAlgoKRebound final : public detail::KAryPerKCore<K, A2, KArySearchAlgoKRebound<K, A2>> {
public:
    /// Der EBENEN-AUSWEIS (s. composable::IsReboundSearchAlgoLeaf).
    using axis03a_rebound_tag = void;

    using detail::KAryPerKCore<K, A2, KArySearchAlgoKRebound<K, A2>>::KAryPerKCore;
};

// -------------------------------------------------------------------------------------------------
// DIE VIER IDENTITAETEN (S18-S21) -- seit der 01c-Hebung ECHTE Leaf-KLASSEN statt Template-Id-Aliase
// (K in {2,4,8,16}; K=2 = Binaersuch-Baseline, Paper-Mess-Set DaMoN 2009).
//
// BEWUSST VIERMAL AUSGESCHRIEBEN statt per Makro erzeugt: das ORGAN_LOCATION-Argument ist ein LITERAL,
// und der F30-Guard des Generators lebt genau davon, dass dieses Literal im Quelltext STEHT und
// auffindbar ist (er faengt die Drift zwischen Makro-Deklaration und realem Typ). Ein
// stringifizierendes Erzeuger-Makro machte die Drift zwar unmoeglich, naehme dem Auditor aber die
// Greppability -- und "der Wrapper-Name steht nirgends im Quelltext" ist genau die Sorte Unsichtbarkeit,
// die diese Achse nicht will. Vier kurze Klassen sind der ehrlichere Preis.
// -------------------------------------------------------------------------------------------------

class KArySearchAlgoK2 final
    : public detail::KAryPerKCore<2u, ::comdare::cache_engine::alloc::ExgenAllocator, KArySearchAlgoK2> {
public:
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::KArySearchAlgoK2",
                                  "axes/lookup/axis_03a_search_algo_k_ary.hpp");
    template <class A2>
    using rebind_allocator = KArySearchAlgoKRebound<2u, A2>;
    using detail::KAryPerKCore<2u, ::comdare::cache_engine::alloc::ExgenAllocator, KArySearchAlgoK2>::KAryPerKCore;
};

class KArySearchAlgoK4 final
    : public detail::KAryPerKCore<4u, ::comdare::cache_engine::alloc::ExgenAllocator, KArySearchAlgoK4> {
public:
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::KArySearchAlgoK4",
                                  "axes/lookup/axis_03a_search_algo_k_ary.hpp");
    template <class A2>
    using rebind_allocator = KArySearchAlgoKRebound<4u, A2>;
    using detail::KAryPerKCore<4u, ::comdare::cache_engine::alloc::ExgenAllocator, KArySearchAlgoK4>::KAryPerKCore;
};

class KArySearchAlgoK8 final
    : public detail::KAryPerKCore<8u, ::comdare::cache_engine::alloc::ExgenAllocator, KArySearchAlgoK8> {
public:
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::KArySearchAlgoK8",
                                  "axes/lookup/axis_03a_search_algo_k_ary.hpp");
    template <class A2>
    using rebind_allocator = KArySearchAlgoKRebound<8u, A2>;
    using detail::KAryPerKCore<8u, ::comdare::cache_engine::alloc::ExgenAllocator, KArySearchAlgoK8>::KAryPerKCore;
};

class KArySearchAlgoK16 final
    : public detail::KAryPerKCore<16u, ::comdare::cache_engine::alloc::ExgenAllocator, KArySearchAlgoK16> {
public:
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::KArySearchAlgoK16",
                                  "axes/lookup/axis_03a_search_algo_k_ary.hpp");
    template <class A2>
    using rebind_allocator = KArySearchAlgoKRebound<16u, A2>;
    using detail::KAryPerKCore<16u, ::comdare::cache_engine::alloc::ExgenAllocator, KArySearchAlgoK16>::KAryPerKCore;
};

// ── Self-proving static_asserts (Increment-1-Verifikation, Codebase-Idiom — vgl. Registry/Organ-Selbstbeweis).
// (a) Jeder per-K-Wrapper erfuellt die Pflicht-Concepts (in Increment 2 registrierbar; wie Legacy MINUS dem
//     verworfenen IterableAspect-Runtime-Kanal). Boundary-Aritaeten K=2 + K=16 stellvertretend geprueft.
static_assert(concepts::SearchAlgoVariant<KArySearchAlgoK2> && concepts::SearchAlgoVariant<KArySearchAlgoK16>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<KArySearchAlgoK2> &&
              concepts::CacheEngineSearchAlgoPermutationStrategy<KArySearchAlgoK16>);
static_assert(concepts::DensityClassifiedStrategy<KArySearchAlgoK4> &&
              concepts::DensityClassifiedStrategy<KArySearchAlgoK8>);
static_assert(concepts::SimdCapableStrategy<KArySearchAlgoK4> && concepts::SimdCapableStrategy<KArySearchAlgoK16>);
// (b) Risk#5 — KEIN Runtime-Kanal: die per-K-Wrapper erfuellen IterableAspect NICHT (kein iterable_aspect_t/
//     set_iterable_aspect). K ist compile-time. (Legacy KArySearchAlgo erfuellt es noch — Runtime, retiring in Inc 2.)
static_assert(!concepts::IterableAspectSearchAlgoStrategy<KArySearchAlgoK4>,
              "#188 per-K: K ist COMPILE-TIME (kein Runtime-iterable_aspect) — sonst Phantom-Messung (Risk#5)");
// (c) Risk#2 — Weg-A-Marker vorhanden (sonst SortedBinary-Fallback in container_traversal_t).
static_assert(KArySearchAlgoK2::axis_03a_store_traversable && KArySearchAlgoK4::axis_03a_store_traversable &&
                  KArySearchAlgoK8::axis_03a_store_traversable && KArySearchAlgoK16::axis_03a_store_traversable,
              "#188 per-K: store_traversable-Marker je Wrapper (Weg-A; Risk#2)");
// (d) Risk#3 — DISTINKTE name() je K + verschieden vom Legacy-"k_ary" (binary_id-Trennung).
static_assert(KArySearchAlgoK2::name() == "k_ary_k2" && KArySearchAlgoK4::name() == "k_ary_k4" &&
              KArySearchAlgoK8::name() == "k_ary_k8" && KArySearchAlgoK16::name() == "k_ary_k16");
static_assert(KArySearchAlgoK2::name() != KArySearchAlgo::name() &&
                  KArySearchAlgoK4::name() != KArySearchAlgo::name() &&
                  KArySearchAlgoK8::name() != KArySearchAlgo::name() &&
                  KArySearchAlgoK16::name() != KArySearchAlgo::name(),
              "#188 per-K: name() distinkt vom Legacy-\"k_ary\" (binary_id-Trennung, Risk#3)");
// (e) compile-time-Aritaet korrekt propagiert (Grundlage der per-K-Organ-Wahl in Header 2).
static_assert(KArySearchAlgoK2::kArity == 2u && KArySearchAlgoK4::kArity == 4u && KArySearchAlgoK8::kArity == 8u &&
              KArySearchAlgoK16::kArity == 16u);

// ---------------------------------------------------------------------------------------------
// A8-S5 01c: die HEBUNG, self-proving. Die vier per-K-Namen sind jetzt eigenstaendige Klassen und
// keine Template-Ids mehr -- das ist die ganze Aussage des Owner-Punkts (ii).
// ---------------------------------------------------------------------------------------------

/// (f) DIE KANTE IST ZU: jede der vier ist ein eigener, PAARWEISE VERSCHIEDENER Typ und traegt ihre
/// EIGENE Organ-Lokation. Vor der Hebung war KArySearchAlgoK2 nur ein anderer Name fuer eine
/// Template-Id -- ihr type_name haette beim Einschalten Template-Argumente in `type=` gebracht, und der
/// F30-Guard haette mangels Literal gar nicht gegriffen.
static_assert(!std::is_same_v<KArySearchAlgoK2, KArySearchAlgoK4> &&
                  !std::is_same_v<KArySearchAlgoK4, KArySearchAlgoK8> &&
                  !std::is_same_v<KArySearchAlgoK8, KArySearchAlgoK16>,
              "01c per-K-Hebung: die vier per-K-Organe sind nicht mehr paarweise distinkt.");
static_assert(::comdare::cache_engine::anatomy::HasOrganLocation<KArySearchAlgoK2> &&
                  ::comdare::cache_engine::anatomy::HasOrganLocation<KArySearchAlgoK4> &&
                  ::comdare::cache_engine::anatomy::HasOrganLocation<KArySearchAlgoK8> &&
                  ::comdare::cache_engine::anatomy::HasOrganLocation<KArySearchAlgoK16>,
              "01c per-K-Hebung: eine der vier Leaf-Klassen traegt keine Organ-Lokation -- dann haette der "
              "F30-Guard beim Einschalten wieder keine Eingabe und die Template-Id-Kante waere offen.");

/// (g) DER XML-NEUTRALITAETS-GRUND, am Typ statt in Prosa: alle vier sind Default-OFF. Der Generator
/// reflektiert ausschliesslich Enabled*-Listen -- deshalb bewegt die Hebung die committete Registry-XML
/// um NULL Byte. Faellt diese Zeile (jemand schaltet ein Flag an), IST das ein Registry-Ereignis und
/// gehoert vor den Owner, nicht in einen Bau-Commit.
static_assert(!KArySearchAlgoK2::enabled && !KArySearchAlgoK4::enabled && !KArySearchAlgoK8::enabled &&
                  !KArySearchAlgoK16::enabled,
              "01c per-K-Hebung: ein per-K-Organ ist ENABLED. Dann waechst die committete "
              "cache_engine_axis_registry.xml um einen Baustein -- ein XML-BYTE-EREIGNIS, das ein "
              "Owner-Entscheid ist (nacht-2: XML-Aenderungen sind ein Rueckfrage-Gate) und eine "
              "Registry-Regeneration braucht, nicht bloss einen gruenen Bau.");

/// (h) DER ZWEI-EBENEN-VERTRAG je per-K-Organ, stellvertretend an den Rand-Aritaeten K=2 und K=16
/// (die vier laufen durch DENSELBEN Core -- was fuer die Raender gilt, gilt fuer 4 und 8 mit).
/// Die vollstaendige Pruefung ALLER vier laeuft ohnehin ueber die abgeleitete Population der
/// Familien-Wache (tests/unit/test_s5_01c_fassaden_conformance.cpp).
static_assert(std::is_same_v<composable::search_algo_for_composition_t<
                                 KArySearchAlgoK2, ::comdare::cache_engine::alloc::ExgenAllocator>,
                             KArySearchAlgoK2>,
              "01c Level-0-IDENTITAET verletzt (per-K K=2).");
static_assert(std::is_same_v<composable::search_algo_for_composition_t<
                                 KArySearchAlgoK16, ::comdare::cache_engine::alloc::ExgenAllocator>,
                             KArySearchAlgoK16>,
              "01c Level-0-IDENTITAET verletzt (per-K K=16).");
static_assert(!composable::IsReboundSearchAlgoLeaf<KArySearchAlgoK2> &&
                  composable::IsReboundSearchAlgoLeaf<
                      KArySearchAlgoKRebound<2u, ::comdare::cache_engine::alloc::ExgenAllocator>>,
              "01c EBENEN-TRENNUNG (per-K): Identitaets- und Substanz-Ebene sind vermischt.");
static_assert(!::comdare::cache_engine::anatomy::HasOrganLocation<
                  KArySearchAlgoKRebound<2u, ::comdare::cache_engine::alloc::ExgenAllocator>>,
              "01c: der per-K-Rebound-Leaf traegt eine Organ-Lokation -- die Substanz-Ebene wuerde "
              "reflektierbar.");
/// Die Aritaet ueberlebt den Rebind -- sonst maesse eine mimalloc-Komposition still ein anderes K.
static_assert(KArySearchAlgoKRebound<8u, ::comdare::cache_engine::alloc::ExgenAllocator>::kArity == 8u &&
                  KArySearchAlgoK8::name() ==
                      KArySearchAlgoKRebound<8u, ::comdare::cache_engine::alloc::ExgenAllocator>::name(),
              "01c per-K: der Rebound-Leaf verliert die Aritaet oder den Organ-Namen -- die T6-Wahl leckte "
              "damit in die K-Identitaet.");
/// Der per-K-Rebound-Leaf bleibt ein VOLLWERTIGES Organ (CRTP-Guard-Kette auf beiden Leaves) und
/// behaelt die Risk#5-Eigenschaft: KEIN Laufzeit-Aritaets-Kanal.
static_assert(concepts::SearchAlgoVariant<KArySearchAlgoKRebound<2u, ::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::SimdCapableStrategy<
              KArySearchAlgoKRebound<16u, ::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(!concepts::IterableAspectSearchAlgoStrategy<
                  KArySearchAlgoKRebound<4u, ::comdare::cache_engine::alloc::ExgenAllocator>>,
              "01c per-K: K bleibt COMPILE-TIME auch am Rebound-Leaf (Risk#5, sonst Phantom-Messung).");

} // namespace comdare::cache_engine::lookup
