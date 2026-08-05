#pragma once
// V41.F.6.1 R7.2 axis_03a_search_algo InterpolationSearchAlgo S11 (2026-05-29)
//
// @topic traversal @achse 03a @family S11 InterpolationSearchAlgo
// @subaxis SA2 sparse_access (sortierte Keys, Such-METHODE = interpolierend statt halbierend)
//
// **Algorithmus:** Interpolationssuche — schaetzt die wahrscheinliche Position des Ziels per
// linearer Interpolation zwischen den Intervall-Endpunkten statt blinder Halbierung. Beschrieben in:
//   Y. Perl, A. Itai, H. Avni: "Interpolation search — a log log N search."
//   Communications of the ACM 21(7), 1978, S. 550-553. DOI 10.1145/359545.359557.
//
// Positionsschaetzung in [lo,hi]:  pos = lo + (key - keys[lo]) * (hi - lo) / (keys[hi] - keys[lo]).
// Bei gleichverteilten Keys O(log log N) im Mittel (vs. O(log N) Binärsuche); Worst-Case O(N)
// bei stark schiefer Verteilung. Distinkt von KArySearchAlgo (partitionsbasiert) und den
// lower_bound-Wrappern (halbierend): VERTEILUNGS-bewusste Such-METHODE — trifft das F15-Thema
// (Such-Methode + Key-Verteilung im std::map-Innenleben → Performance). Datenabhaengiger Index
// → NICHT SIMD-vektorisierbar (supports_simd=false, kein simd_lookup, kein SimdCapableStrategy).
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** klassischer Lehrbuch-Algorithmus,
// kein kanonischer permissiver Repo-Code → originalgetreue C++23-Re-Implementierung,
// is_original = false (via AxisBase-Default).
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
// Owner-KERN: LEDGER 04.08.2026 abend-11 ("Option B strikt"), Design-Entscheid abend-14 (D1),
// Manager-Entscheid 05.08. nacht-2 (D1 bleibt fuer die Vor-Anker-Strecke).
//
// Die Konstruktion ist ZEILENGLEICH zum Pilot (axis_03a_search_algo_linear_scan.hpp:22-58); die dortige
// Begruendung gilt hier unveraendert und wird NICHT wiederholt, sondern referenziert:
//   (1) detail::InterpolationSearchAlgoCore<Alloc, Self>  -- DIE SUBSTANZ (CRTP mit Self, weil
//       SearchAlgoBase CRTP ist und seine Ctor-Guards auf Derived laufen).
//   (2) InterpolationSearchAlgo                           -- DIE IDENTITAET. Nicht-Template, EXAKT der
//       alte Typ-Name: traegt COMDARE_DEFINE_ORGAN_LOCATION, steht in AllStrategies, ihr type_name
//       reist in die committete cache_engine_axis_registry.xml (F30-Guard, axis_registry_gen
//       main.cpp:161-162/:255-263). Ein Template-Kopf HIER wuerde `type=`/`wrapper=` drehen.
//   (3) InterpolationSearchAlgoRebound<A2>                -- DIE GEBUNDENE FORM, materialisiert nur am
//       Genus-Erst-Instanziierungs-Punkt (abend-6-Ausnahme).
// Vorwaerts-Deklaration `class InterpolationSearchAlgo;` in composable/traversal_for_search_algo.hpp:36
// -- auch hier haette eine Alias-Loesung sofort gebrochen; die Fassade haelt sie unveraendert.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/alloc/axis_06_allocator_exgen.hpp>
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <axes/lookup/axis_03a_search_algo_flags.hpp>
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
class InterpolationSearchAlgoRebound;

namespace detail {

/// DIE SUBSTANZ des S11-Organs: Interpolationssuche ueber sortierte Keys, Speicher ueber die
/// Allokator-ACHSE.
///
/// @tparam Alloc  Die Allokator-Achsen-Strategie (axis_06). Default-Bindung der Fassade: ExgenAllocator.
/// @tparam Self   Der most-derived Typ (Fassade oder Rebound-Leaf) -- PFLICHT wegen der CRTP-Guards
///                der SearchAlgoBase (axis_03a_search_algo_base.hpp:47-59).
template <class Alloc, class Self>
class InterpolationSearchAlgoCore : public SearchAlgoBase<Self> {
public:
    static constexpr bool enabled = flags::interpolation_enabled;
    // (E-Welle-A2 / Befund-2 / A2.4-S1) Array-Familie (Interpolationssuche über sortiert-aufsteigenden flachen Slot-Store)
    // → store-traversierbar: Suche über DENSELBEN LayoutAwareChunkedStore (node/layout/allocator wirken real). G3-klassifiziert.
    static constexpr bool axis_03a_store_traversable = true;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 11>; // S11

    /// A8-S5 SCHNITT-FORM (B): Keys UND Values haengen an der Allokator-ACHSE. Diese Zeile IST der
    /// Ausweis, den die Familien-Konformitaets-Wache liest (tests/unit/s5_family_alloc_conformance.hpp).
    /// Anders als in 01a/01d ist sie NICHT fest, sondern der Kompositions-Parameter -- genau das ist der
    /// Unterschied zwischen "an der Achse" (Zwischenstand) und "Option B strikt".
    using allocator_type = Alloc;
    static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<allocator_type>,
                  "A8-S5: der gebundene Allokator erfuellt das axis_06-Achsen-Concept nicht mehr -- dann liefen "
                  "Keys/Values wieder an der Allokator-Achse vorbei (Schnitt-Regel Dossier 3.4).");

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_fanout() noexcept { return 65536; } // u16 Keyraum
    /// SERIALISIERUNGS-SCHLUESSEL -- bewusst OHNE jeden Allokator-Bezug. Die T6-Wahl darf hier NIE
    /// hineinlecken: sonst truege eine mimalloc-gebundene Komposition einen anderen Organ-Namen als
    /// dieselbe Komposition mit exgen, und binary_id-/serialize-Pfad drifteten gegen die Allokator-
    /// Achse. Die Wache dazu steht als static_assert unter der Fassade (name()-Invarianz).
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "interpolation"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "InterpolationSearchAlgo (interpolation search — Perl/Itai/Avni CACM 1978)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "INTERPOLATION"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    ///
    /// NICHT gebumpt in dieser Scheibe -- deklariert, nicht stillschweigend: der Allokations-PFAD aendert
    /// sich, der Algorithmus nicht (Pilot-Praezedenz linear_scan; Bump-Entscheid fuer alle 16 Varianten
    /// liegt im Mess-/A13-Fenster, S5-04-Praezedenz v1.0.0c).
    static constexpr std::string_view algo_version = "v1.0.0c";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; }      // datenabhaengiger Index
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // sortiert
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }           // sparse sortiert
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

private:
    using key_alloc    = typename Alloc::template StdAllocatorAdapter<key_type>;
    using value_alloc  = typename Alloc::template StdAllocatorAdapter<value_type>;
    using key_vector   = std::vector<key_type, key_alloc>;
    using value_vector = std::vector<value_type, value_alloc>;

public:
    /// Beide Vektoren werden an das EIGENE allocator_ gebunden (der Adapter haelt &allocator_).
    InterpolationSearchAlgoCore()
        : keys_(allocator_.template as_std_allocator<key_type>()),
          values_(allocator_.template as_std_allocator<value_type>()) {}

    /// KF-6-NAHT (Posten 62, LEDGER 04.08. abend-12): eine vor-parametrierte Strategie-Instanz
    /// uebernehmen, statt sie default zu konstruieren. Heute nirgends benutzt und bewusst `explicit` --
    /// die Naht existiert, damit das KF-6-Fenster (CacheLineCfg-NTTP durch die Cores) sie nicht erst
    /// nachtraeglich in 16 Dateien schneiden muss.
    explicit InterpolationSearchAlgoCore(allocator_type a)
        : allocator_(std::move(a)), keys_(allocator_.template as_std_allocator<key_type>()),
          values_(allocator_.template as_std_allocator<value_type>()) {}

    /// Copy: Strategie mitkopieren, beide Vektoren an das EIGENE allocator_ binden, dann die transiente
    /// Kopier-Allokation aus der Statistik nehmen (Memento) -- 1:1 btree_node_pool_store.hpp:86.
    /// MOVE ist bewusst NICHT deklariert (der user-definierte Copy unterdrueckt ihn implizit): jede
    /// Bewegung faellt damit auf den REBINDENDEN Copy zurueck, statt Vektoren mitsamt fremdem
    /// Adapter-Zeiger zu verschieben.
    InterpolationSearchAlgoCore(InterpolationSearchAlgoCore const& o)
        : allocator_(o.allocator_), keys_(o.keys_, allocator_.template as_std_allocator<key_type>()),
          values_(o.values_, allocator_.template as_std_allocator<value_type>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_ = o.stats_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }

    InterpolationSearchAlgoCore& operator=(InterpolationSearchAlgoCore const& o) {
        if (this != &o) {
            // POCCA=false (der Adapter fuehrt keine propagate_-Typedefs) -> die Vektoren behalten ihr an
            // this-allocator_ gebundenes Adapter-Objekt; die Zuweisung re-alloziert ueber this-allocator_.
            keys_   = o.keys_;
            values_ = o.values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_ = o.stats_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }

    ~InterpolationSearchAlgoCore() = default;

    [[nodiscard]] bool operator==(InterpolationSearchAlgoCore const& other) const noexcept {
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

    /// Interpolationssuche (Perl/Itai/Avni 1978): Positionsschaetzung per linearer Interpolation.
    /// Keys sind eindeutig + sortiert → keys[hi] > keys[lo] solange lo < hi (Divisor >= 1).
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        std::size_t const         n      = keys_.size();
        if (n != 0) {
            std::size_t lo = 0, hi = n - 1;
            while (lo <= hi && k >= keys_[lo] && k <= keys_[hi]) {
                if (lo == hi) {
                    if (keys_[lo] == k) result = values_[lo];
                    break;
                }
                key_type const key_lo = keys_[lo];
                key_type const key_hi = keys_[hi]; // > key_lo (eindeutig sortiert, lo < hi)
                // pos = lo + (k - key_lo) * (hi - lo) / (key_hi - key_lo); Produkt passt in u64.
                std::uint64_t const num =
                    static_cast<std::uint64_t>(static_cast<unsigned>(k) - static_cast<unsigned>(key_lo)) *
                    static_cast<std::uint64_t>(hi - lo);
                std::size_t const pos =
                    lo + static_cast<std::size_t>(num / static_cast<std::uint64_t>(static_cast<unsigned>(key_hi) -
                                                                                   static_cast<unsigned>(key_lo)));
                // pos in [lo, hi] (k <= key_hi ⇒ num/divisor <= hi - lo).
                key_type const at = keys_[pos];
                if (at == k) {
                    result = values_[pos];
                    break;
                }
                if (at < k)
                    lo = pos + 1; // pos < hi hier (sonst at == key_hi >= k)
                else
                    hi = pos - 1; // pos > lo hier (sonst at == key_lo <= k) ⇒ kein Underflow
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
    /// konstitutiven Store-Snapshot. Ob und wie sie in die T6-CSV-Spalte summiert werden
    /// (Doppelzaehlungs-Regel), ist der EXPLIZITE Schritt des Mess-Schnitt-Fensters VOR Messbeginn.
    using allocator_snapshot_t = typename allocator_type::snapshot_t;
    [[nodiscard]] allocator_snapshot_t search_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    // allocator_ MUSS VOR keys_/values_ stehen: der Adapter haelt &allocator_, und die Member-
    // Initialisierungs-/Zerstoerungsreihenfolge ist die Deklarationsreihenfolge (die Vektoren muessen
    // VOR der Strategie sterben). Dieselbe Reihenfolge wie in den Pool-Stores (01a), 01d und im Pilot.
    allocator_type allocator_{};
    key_vector     keys_;
    value_vector   values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace detail

/// DIE IDENTITAET -- das Registry-Organ S11. Nicht-Template, exakt der historische Typ-Name.
/// `final`: die Ebenen-Trennung ist Vertrag, nicht Vorschlag -- ein weiterer Ableitungs-Schritt
/// unter der Fassade wuerde den CRTP-Self-Vertrag des Cores brechen.
class InterpolationSearchAlgo final
    : public detail::InterpolationSearchAlgoCore<::comdare::cache_engine::alloc::ExgenAllocator,
                                                 InterpolationSearchAlgo> {
public:
    /// Die Default-Bindung der Identitaets-Ebene: der BENANNTE Achsen-Default, nie std::allocator.
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;

    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::InterpolationSearchAlgo",
                                  "axes/lookup/axis_03a_search_algo_interpolation.hpp");

    /// Der Migrations-Ausweis (composable::AllocatorRebindableSearchAlgo): auf WELCHEN Typ bindet die
    /// Kompositions-Naht um, wenn die Komposition eine fremde Strategie fuehrt?
    template <class A2>
    using rebind_allocator = InterpolationSearchAlgoRebound<A2>;

    using detail::InterpolationSearchAlgoCore<default_allocator_type,
                                              InterpolationSearchAlgo>::InterpolationSearchAlgoCore;
};

/// DIE GEBUNDENE FORM -- materialisiert nur am Genus-Erst-Instanziierungs-Punkt. Erbt name(),
/// family_id und algo_version unveraendert aus dem Core. Traegt BEWUSST KEIN
/// COMDARE_DEFINE_ORGAN_LOCATION -- er ist kein Registry-Organ.
template <class A2>
class InterpolationSearchAlgoRebound final
    : public detail::InterpolationSearchAlgoCore<A2, InterpolationSearchAlgoRebound<A2>> {
public:
    /// Der EBENEN-AUSWEIS (s. composable::IsReboundSearchAlgoLeaf).
    using axis03a_rebound_tag = void;

    using detail::InterpolationSearchAlgoCore<A2, InterpolationSearchAlgoRebound<A2>>::InterpolationSearchAlgoCore;
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<InterpolationSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<InterpolationSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<InterpolationSearchAlgo>);

// ---------------------------------------------------------------------------------------------
// Der Zwei-Ebenen-Vertrag, self-proving an der Datei, die ihn eingeht (Pilot-Rezept, linear_scan:352).
// ---------------------------------------------------------------------------------------------

/// LEVEL 0 (der golden-Pfad): die Kompositions-Naht mit dem Achsen-Default liefert die FASSADE SELBST
/// zurueck -- kein Rebound-Typ, kein Typ-Shift, kein neuer Symbolname.
static_assert(std::is_same_v<composable::search_algo_for_composition_t<InterpolationSearchAlgo,
                                                                       ::comdare::cache_engine::alloc::ExgenAllocator>,
                             InterpolationSearchAlgo>,
              "01c Level-0-IDENTITAET verletzt: die Kompositions-Naht liefert am ACHSEN-DEFAULT nicht mehr die "
              "Fassade selbst. Damit laege ein anderer Typ auf dem golden-Pfad -- Typ-Neutralitaet weg.");

/// Der Migrations-Ausweis ist da (sonst faele das Organ still auf Level 2 = unveraendert zurueck).
static_assert(
    composable::AllocatorRebindableSearchAlgo<InterpolationSearchAlgo, ::comdare::cache_engine::alloc::ExgenAllocator>,
    "01c: InterpolationSearchAlgo traegt keinen rebind_allocator mehr -- nicht migriert.");

/// EBENEN-TRENNUNG: die Identitaets-Ebene traegt NIE den Rebound-Ausweis, der Leaf traegt ihn IMMER.
static_assert(!composable::IsReboundSearchAlgoLeaf<InterpolationSearchAlgo>,
              "01c EBENEN-VERMISCHUNG: die Fassade traegt den Rebound-Tag -- Emitter-type_name-Reise und "
              "Registry-Reflektion zeigten dann auf die Substanz-Ebene.");
static_assert(
    composable::IsReboundSearchAlgoLeaf<InterpolationSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>,
    "01c: der Rebound-Leaf traegt seinen Ausweis nicht -- der Identitaets-Pin kann nicht mehr greifen.");

/// name()-ALLOKATOR-INVARIANZ: der serialize-Schluessel darf sich mit der T6-Wahl NICHT bewegen.
static_assert(composable::search_algo_name_is_allocator_invariant_v<InterpolationSearchAlgo,
                                                                    ::comdare::cache_engine::alloc::ExgenAllocator>,
              "01c name()-INVARIANZ (Level 0) verletzt.");
static_assert(InterpolationSearchAlgo::name() ==
                  InterpolationSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>::name(),
              "01c name()-INVARIANZ verletzt: der Rebound-Leaf traegt einen anderen Organ-Namen als die Fassade -- "
              "die T6-Wahl leckte in den serialize-/binary_id-Schluessel.");

/// Der Rebound-Leaf ist ein VOLLWERTIGES Organ, nicht nur eine Typ-Huelle (die CRTP-Guard-Kette laeuft
/// auf beiden Leaves -- R2 des Design-Risikoblatts).
static_assert(
    concepts::SearchAlgoVariant<InterpolationSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<
              InterpolationSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::DensityClassifiedStrategy<
              InterpolationSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
} // namespace comdare::cache_engine::lookup
