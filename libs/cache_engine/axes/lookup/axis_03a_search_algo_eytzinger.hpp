#pragma once
// V41.F.6.1 R7.2 axis_03a_search_algo EytzingerSearchAlgo S12 (2026-05-29)
//
// @topic traversal @achse 03a @family S12 EytzingerSearchAlgo
// @subaxis SA2 sparse_access (sortierte Keys, Such-METHODE = cache-conscious Layout + branch-free)
//
// **Algorithmus:** Eytzinger- (BFS-/Heap-) Layout-Suche, beschrieben in:
//   Paul-Virak Khuong, Pat Morin: "Array Layouts for Comparison-Based Searching."
//   Journal of Experimental Algorithmics (JEA) 22, 2017 (arXiv:1509.05053).
//
// Statt sortierter Reihenfolge werden die Keys in Breadth-First-Order des impliziten
// Suchbaums abgelegt (Wurzel an Index 1, Kinder von i an 2i/2i+1). Die Suche ist branch-free:
//   k = 1;  while (k <= n)  k = 2*k + (eyt[k] < x);
// danach liefert  idx = k >> (countr_one(k) + 1)  den Eytzinger-Index des lower_bound (0 = keiner).
// Vorteil (Paper): die ersten Ebenen liegen dicht beieinander → cache-/prefetch-freundlich, der
// branch-free Kern vermeidet Branch-Mispredictions. Fuer grosse Arrays laut Paper das schnellste
// Allzweck-Layout. Distinkt von KArySearchAlgo (SIMD-Partition), InterpolationSearchAlgo
// (Verteilung) und den lower_bound-Wrappern (sortiert/halbierend): CACHE-LAYOUT-Such-METHODE —
// trifft direkt das cache-engine-/F15-Thema (Speicher-Layout der std::map-Innenstruktur → Performance).
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** Referenz-Code (github.com/patmorin/
// arraylayout) ist ein Experiment-Harness ohne deklarierte Standard-OSS-Lizenz; diese Implementierung
// ist eine originalgetreue C++23-Re-Implementierung des Eytzinger-Layouts + der branch-free Suche
// → is_original = false (via AxisBase-Default).
//
// Erfuellt: SearchAlgoVariant, CacheEngineSearchAlgoPermutationStrategy, DensityClassifiedStrategy.
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
// **DER EYTZINGER-SONDERFALL -- EIN Versorger, nicht zwei.** Alle anderen Organe dieser Familie halten
// ihre Vektoren selbst. Dieses Organ nicht: sein Substrat (sortierte Basis + lazy BFS-Puffer) EXISTIERT
// bereits als composable::EytzingerLayoutStore, und der hat in Scheibe 01a (LEDGER 04.08. abend-13)
// seinen Template-Kopf ueber genau dieselbe Allokator-Achse BEKOMMEN. Der Core konsumiert ihn deshalb
// ueber DENSELBEN Alloc, statt die vier Vektoren ein zweites Mal selbst zu fuehren:
//   * KOMPOSITIONS-KONSISTENZ: es gibt pro Organ-Instanz GENAU EINE Strategie-Instanz (die im Store).
//     Ein eigener allocator_ im Core NEBEN dem Store-Alloc waere exakt der Fehler, den 01c abstellt --
//     eine stille zweite Strategie im selben Organ (Owner-Definition abend-11).
//   * search_allocator_statistics() ist deshalb hier eine WEITERLEITUNG auf den Store-Snapshot, kein
//     eigener Zaehler. Die Naht bleibt namentlich getrennt (Doppelzaehlungs-Regel, Mess-Schnitt-Fenster).
//   * Der Store fuehrt seine Keys als uint64 (er ist das gemeinsame Substrat mit dem Organ-Pfad). Der
//     u16-Keyraum dieses Wrappers ist eine echte Teilmenge -- Verhalten identisch, s. den Aequivalenz-Pin
//     tests/unit/test_v41_axis_03a_tier_organ_equivalence.cpp:218/237 (gegen std::map UND SortedBinary,
//     also NICHT gegen den Store selbst -- die Aequivalenz-Aussage bleibt unabhaengig).
// Die vormals dateiprivate rebuild_eytzinger/fill_eytzinger-Duplikation der Store-Logik entfaellt damit
// (Owner-KERN Aufraeumpass: toten/doppelten Code entfernen, nicht danebenlegen).

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/alloc/axis_06_allocator_exgen.hpp>
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#include <axes/lookup/composable/eytzinger_layout_store.hpp>
#include <axes/lookup/composable/search_algo_rebind.hpp>
#include <measurement/measurable_concept.hpp>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::lookup {

// Vorwaerts-Deklaration: die Fassade nennt ihren eigenen Rebound-Leaf als Member-Alias, und der
// Rebound-Leaf erbt vom selben Core -- beide brauchen den Namen, bevor der andere vollstaendig ist.
template <class A2>
class EytzingerSearchAlgoRebound;

namespace detail {

/// DIE SUBSTANZ des S12-Organs: branch-free Eytzinger-Suche ueber das GETEILTE Layout-Substrat.
///
/// @tparam Alloc  Die Allokator-Achsen-Strategie (axis_06). Default-Bindung der Fassade: ExgenAllocator.
/// @tparam Self   Der most-derived Typ (Fassade oder Rebound-Leaf) -- PFLICHT wegen der CRTP-Guards
///                der SearchAlgoBase (axis_03a_search_algo_base.hpp:47-59).
template <class Alloc, class Self>
class EytzingerSearchAlgoCore : public SearchAlgoBase<Self> {
public:
    static constexpr bool enabled = flags::eytzinger_enabled;
    // #188-4a (2026-07-02, Option b): store-traversierbar via organ_for_search_algo -> EytzingerOrgan
    // (EytzingerLayoutStore: sortierter Primaerzustand + lazy BFS-Puffer). BEWUSST weiterhin KEIN
    // axis_03a_store_traversable-Marker (der gilt dem FLAT-Store-Pfad); Registry-static_assert bleibt wahr.
    // Dieser u16-Wrapper bleibt Registry-Tier + Aequivalenz-Referenz; die 320er-Messung laeuft ueber das Organ.

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 12>; // S12

    /// A8-S5 SCHNITT-FORM (B): der Speicher haengt an der Allokator-ACHSE -- hier ueber das GETEILTE
    /// Layout-Substrat (s. Kopf-Doku "Eytzinger-Sonderfall"), nicht ueber eigene Vektoren.
    using allocator_type = Alloc;
    static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<allocator_type>,
                  "A8-S5: der gebundene Allokator erfuellt das axis_06-Achsen-Concept nicht mehr -- dann liefe "
                  "das Layout-Substrat wieder an der Allokator-Achse vorbei (Schnitt-Regel Dossier 3.4).");

    /// Das Substrat -- DASSELBE, das der Organ-Pfad fuehrt (composed_eytzinger_search.hpp), ueber
    /// DENSELBEN Alloc parametriert. Genau EIN Versorger je Organ-Instanz.
    using layout_store_type = composable::EytzingerLayoutStore<Alloc>;

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_fanout() noexcept { return 65536; } // u16 Keyraum
    /// SERIALISIERUNGS-SCHLUESSEL -- bewusst OHNE jeden Allokator-Bezug (s. name()-Invarianz-Wache
    /// unter der Fassade): die T6-Wahl darf nie in den serialize-/binary_id-Schluessel lecken.
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "eytzinger"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "EytzingerSearchAlgo (cache-conscious BFS layout, branch-free — Khuong/Morin JEA 2017)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "EYTZINGER"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0c";

    [[nodiscard]] static constexpr bool supports_simd() noexcept {
        return false;
    } // branch-free + prefetch, nicht vektorisiert
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; }      // sortierte Quelle
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }                // sparse sortiert
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; } // Kern-Vorteil des Layouts

    EytzingerSearchAlgoCore() = default;

    /// KF-6-NAHT (Posten 62, LEDGER 04.08. abend-12): eine vor-parametrierte Strategie-Instanz
    /// uebernehmen. Sie wandert DURCH bis in den Store -- es gibt keinen zweiten Ort, an dem sie landen
    /// koennte (das ist der ganze Punkt des Sonderfalls).
    explicit EytzingerSearchAlgoCore(allocator_type a) : store_(std::move(a)) {}

    /// Copy: der Store rebindet SELBST an sein eigenes allocator_ und verwirft die Kopier-Pollution per
    /// restore_statistics (eytzinger_layout_store.hpp:54-62) -- hier bleibt nur der Stat-POD.
    /// MOVE bewusst NICHT deklariert (der user-definierte Copy unterdrueckt ihn implizit).
    EytzingerSearchAlgoCore(EytzingerSearchAlgoCore const& o) : store_(o.store_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_ = o.stats_;
#endif
    }

    EytzingerSearchAlgoCore& operator=(EytzingerSearchAlgoCore const& o) {
        if (this != &o) {
            store_ = o.store_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_ = o.stats_;
#endif
        }
        return *this;
    }

    ~EytzingerSearchAlgoCore() = default;

    [[nodiscard]] bool operator==(EytzingerSearchAlgoCore const& other) const noexcept {
        return store_.slot_count() == other.store_.slot_count();
    }

    /// Quelle der Wahrheit ist die sortierte Basis des Stores; das Eytzinger-Layout wird lazy
    /// (bei naechstem lookup) neu gebaut. SONDERFALL [[allocation-failure-exception]]: std::bad_alloc.
    void insert(key_type k, value_type v) {
        std::size_t const idx = lower_bound_index(k);
        if (idx < store_.slot_count() && store_.key_at(idx) == static_cast<store_key_type>(k)) {
            store_.set_value_at(idx, v); // update (Layout-Werte aendern sich -> dirty)
        } else {
            store_.insert_slot_at(idx, static_cast<store_key_type>(k), v);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (store_.slot_count() > stats_.peak_occupancy) stats_.peak_occupancy = store_.slot_count();
        observer_.notify(stats_);
#endif
    }

    /// Eytzinger branch-free Suche (Khuong/Morin 2017). Baut das Layout bei Bedarf neu auf.
    [[nodiscard]] std::optional<value_type> lookup(key_type x) const {
        std::optional<value_type> result = std::nullopt;
        std::size_t const         n      = store_.slot_count();
        if (n != 0) {
            store_.rebuild_if_dirty();
            std::size_t k = 1;
            while (k <= n) {
                // 0 = links (>=x), 1 = rechts (<x). Der Store fuehrt uint64-Keys; der u16-Schluessel
                // wird dafuer hochgezogen -- die Ordnung ist dieselbe (echte Teilmenge des Keyraums).
                k = 2 * k + (store_.eyt_key_at(k) < static_cast<store_key_type>(x) ? 1u : 0u);
            }
            // lower_bound-Eytzinger-Index: trailing-1s + 1 wegshiften (0 ⇒ kein Element >= x).
            std::size_t const idx = k >> (static_cast<unsigned>(std::countr_one(k)) + 1u);
            if (idx >= 1 && idx <= n && store_.eyt_key_at(idx) == static_cast<store_key_type>(x))
                result = store_.eyt_value_at(idx);
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

    bool erase(key_type k) {
        std::size_t const idx = lower_bound_index(k);
        if (idx >= store_.slot_count() || store_.key_at(idx) != static_cast<store_key_type>(k)) return false;
        store_.erase_slot_at(idx);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return store_.slot_count(); }
    [[nodiscard]] double    density_percent() const noexcept {
        return 100.0 * static_cast<double>(store_.slot_count()) / 65536.0;
    }
    void clear() noexcept { store_.clear(); }

    /// DensityClassifiedStrategy [[density-classified-strategy]]: Belegungs-basierte Klassifikation.
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        std::size_t const n = store_.slot_count();
        if (n > 1024) return concepts::DensityClass::Dense;
        if (n > 64) return concepts::DensityClass::Balanced;
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

    /// EINSAMMEL-NAHT der T6-Durchbindung (Owner-KERN abend-11, Pflicht (a)) -- NUR die Naht.
    /// Hier eine WEITERLEITUNG auf den Store-Snapshot: das Organ HAT keinen zweiten Zaehler (Sonderfall,
    /// s. Kopf-Doku). Der VIERTE Name bleibt trotzdem getrennt gefuehrt -- er ist die Absicherung gegen
    /// stillschweigende Doppelzaehlung durch einen kuenftigen generischen Leser (Mess-Schnitt-Fenster).
    using allocator_snapshot_t = typename allocator_type::snapshot_t;
    [[nodiscard]] allocator_snapshot_t search_allocator_statistics() const noexcept {
        return store_.store_allocator_statistics();
    }
#endif

private:
    using store_key_type = typename layout_store_type::key_type;

    /// lower_bound ueber die sortierte Store-Basis (der Store exponiert Indizes, keine Iteratoren).
    /// Semantisch identisch zum vormaligen std::lower_bound auf dem eigenen keys_-Vektor.
    [[nodiscard]] std::size_t lower_bound_index(key_type k) const noexcept {
        std::size_t lo = 0;
        std::size_t hi = store_.slot_count();
        while (lo < hi) {
            std::size_t const mid = lo + (hi - lo) / 2u;
            if (store_.key_at(mid) < static_cast<store_key_type>(k))
                lo = mid + 1u;
            else
                hi = mid;
        }
        return lo;
    }

    /// BEWUSST NICHT mutable: der lazy Rebuild (dokumentierte Mess-Eigenschaft des Eytzinger-Tiers)
    /// laeuft ueber die CONST-Schnittstelle des Stores, der seine Rebuild-Puffer selbst mutable haelt.
    /// Ein zusaetzliches mutable hier wuerde die const-Zusicherung des Organs unnoetig aufweichen.
    layout_store_type store_{};
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace detail

/// DIE IDENTITAET -- das Registry-Organ S12. Nicht-Template, exakt der historische Typ-Name.
class EytzingerSearchAlgo final
    : public detail::EytzingerSearchAlgoCore<::comdare::cache_engine::alloc::ExgenAllocator, EytzingerSearchAlgo> {
public:
    /// Die Default-Bindung der Identitaets-Ebene: der BENANNTE Achsen-Default, nie std::allocator.
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;

    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::EytzingerSearchAlgo",
                                  "axes/lookup/axis_03a_search_algo_eytzinger.hpp");

    /// Der Migrations-Ausweis (composable::AllocatorRebindableSearchAlgo).
    template <class A2>
    using rebind_allocator = EytzingerSearchAlgoRebound<A2>;

    using detail::EytzingerSearchAlgoCore<default_allocator_type, EytzingerSearchAlgo>::EytzingerSearchAlgoCore;
};

/// DIE GEBUNDENE FORM -- traegt BEWUSST KEIN COMDARE_DEFINE_ORGAN_LOCATION.
template <class A2>
class EytzingerSearchAlgoRebound final : public detail::EytzingerSearchAlgoCore<A2, EytzingerSearchAlgoRebound<A2>> {
public:
    /// Der EBENEN-AUSWEIS (s. composable::IsReboundSearchAlgoLeaf).
    using axis03a_rebound_tag = void;

    using detail::EytzingerSearchAlgoCore<A2, EytzingerSearchAlgoRebound<A2>>::EytzingerSearchAlgoCore;
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<EytzingerSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<EytzingerSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<EytzingerSearchAlgo>);

// ---------------------------------------------------------------------------------------------
// Der Zwei-Ebenen-Vertrag, self-proving an der Datei, die ihn eingeht (Pilot-Rezept, linear_scan:352).
// ---------------------------------------------------------------------------------------------

/// LEVEL 0 (der golden-Pfad): die Kompositions-Naht liefert am Achsen-Default die FASSADE SELBST.
static_assert(std::is_same_v<composable::search_algo_for_composition_t<EytzingerSearchAlgo,
                                                                       ::comdare::cache_engine::alloc::ExgenAllocator>,
                             EytzingerSearchAlgo>,
              "01c Level-0-IDENTITAET verletzt: die Kompositions-Naht liefert am ACHSEN-DEFAULT nicht mehr die "
              "Fassade selbst. Damit laege ein anderer Typ auf dem golden-Pfad -- Typ-Neutralitaet weg.");

static_assert(
    composable::AllocatorRebindableSearchAlgo<EytzingerSearchAlgo, ::comdare::cache_engine::alloc::ExgenAllocator>,
    "01c: EytzingerSearchAlgo traegt keinen rebind_allocator mehr -- nicht migriert.");

/// EBENEN-TRENNUNG.
static_assert(!composable::IsReboundSearchAlgoLeaf<EytzingerSearchAlgo>,
              "01c EBENEN-VERMISCHUNG: die Fassade traegt den Rebound-Tag.");
static_assert(
    composable::IsReboundSearchAlgoLeaf<EytzingerSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>,
    "01c: der Rebound-Leaf traegt seinen Ausweis nicht.");

/// name()-ALLOKATOR-INVARIANZ.
static_assert(composable::search_algo_name_is_allocator_invariant_v<EytzingerSearchAlgo,
                                                                    ::comdare::cache_engine::alloc::ExgenAllocator>,
              "01c name()-INVARIANZ (Level 0) verletzt.");
static_assert(EytzingerSearchAlgo::name() ==
                  EytzingerSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>::name(),
              "01c name()-INVARIANZ verletzt: der Rebound-Leaf traegt einen anderen Organ-Namen als die Fassade.");

/// DER SONDERFALL, GEPINNT: das Substrat der Fassade ist DASSELBE, das der Organ-Pfad fuehrt -- und es
/// traegt DENSELBEN Allokator wie das Organ. Ein zweiter Versorger waere genau die stille zweite
/// Strategie, die 01c abstellt (Owner-Definition abend-11).
static_assert(std::is_same_v<typename EytzingerSearchAlgo::layout_store_type,
                             composable::EytzingerLayoutStore<::comdare::cache_engine::alloc::ExgenAllocator>>,
              "01c Eytzinger-Sonderfall: die Fassade fuehrt nicht mehr das geteilte Layout-Substrat.");
static_assert(std::is_same_v<typename EytzingerSearchAlgo::layout_store_type::allocator_type,
                             typename EytzingerSearchAlgo::allocator_type>,
              "01c Eytzinger-Sonderfall: Substrat und Organ fuehren VERSCHIEDENE Allokator-Strategien -- das waere "
              "der zweite Versorger im selben Organ.");
static_assert(
    std::is_same_v<
        typename EytzingerSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>::layout_store_type,
        composable::EytzingerLayoutStore<::comdare::cache_engine::alloc::ExgenAllocator>>,
    "01c Eytzinger-Sonderfall: der Rebound-Leaf reicht seine Strategie nicht an das Substrat durch.");

/// Der Rebound-Leaf ist ein VOLLWERTIGES Organ.
static_assert(concepts::SearchAlgoVariant<EytzingerSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<
              EytzingerSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(
    concepts::DensityClassifiedStrategy<EytzingerSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
} // namespace comdare::cache_engine::lookup
