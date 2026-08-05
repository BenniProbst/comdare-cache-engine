#pragma once
// V41.F.6.1 axis_03a_search_algo VectorU16U16SearchAlgo S03 (2026-05-26)
//
// @topic traversal @achse 03a @family S03 VectorU16U16SearchAlgo
// @subaxis SA3 multilevel_access
//
// **Algorithmus-Pattern:** Multi-Byte Discriminator mit Cost-DP-Splitting
// (START Self-Tuning Adaptive Radix Tree, Fent et al., ICDEW 2020).
//
// Standalone-Implementation: sortierte std::vector<uint16> Keys + parallel
// std::vector<uint64> Values, std::lower_bound O(log N) Lookup. Aequivalent
// zu START-multibyte Decision-Points (vereinfacht ohne Cost-Modell — das
// wuerde adaptive Permutationen erfordern, hier als Pilot weggelassen).
//
// Erfuellt:
//   - SearchAlgoVariant (Pflicht-API)
//   - CacheEngineSearchAlgoPermutationStrategy (cache-engine-spec)
//   - DensityClassifiedStrategy (DensityClass::Balanced)
//   - **NICHT** SimdCapableStrategy (Cost-DP nicht SIMD-vektorisierbar)
//
// Allocation: NUR ueber das Allokator-Achsen-Interface (axis_06) -- s. den ZWEI-EBENEN-SCHNITT unten.
// [[allocation-failure-exception]]: insert kann std::bad_alloc werfen (StdAllocatorAdapter, Posten 64).
//
// ===================================================================================================
// A8-S5 Familie 01c, Scheibe 3 (2026-08-05) -- ZWEI-EBENEN-SCHNITT (Pilot-Rezept linear_scan)
// ===================================================================================================
// KLASSEN-ENTSCHEID (Scheibe 3 weist je Organ aus, welche der beiden Formen gilt): **VOLL-REZEPT.**
// Dieses Organ traegt ZWEI Default-Allokator-Vektoren (keys_/values_, die 01b-Schlussbilanz fuehrt sie
// als 2 der 39 Rest-Zeilen) -- der Container-Tausch ist Substanz, nicht Zeremonie.
//
// Die Konstruktion ist zeilengleich zum Pilot (axis_03a_search_algo_linear_scan.hpp:22-58) und zur
// Scheibe 2 (axis_03a_search_algo_interpolation.hpp:27-46); die dortige Begruendung gilt hier
// unveraendert und wird NICHT wiederholt, sondern referenziert:
//   (1) detail::VectorU16U16SearchAlgoCore<Alloc, Self> -- DIE SUBSTANZ (CRTP mit Self wegen der
//       Ctor-Guards der SearchAlgoBase).
//   (2) VectorU16U16SearchAlgo                          -- DIE IDENTITAET. Nicht-Template, EXAKT der
//       alte Typ-Name; ein Template-Kopf HIER wuerde `type=`/`wrapper=` der Registry-XML drehen
//       (F30-Guard, axis_registry_gen main.cpp:161-162/:255-263).
//   (3) VectorU16U16SearchAlgoRebound<A2>               -- DIE GEBUNDENE FORM.
// Vorwaerts-Deklaration `class VectorU16U16SearchAlgo;` in composable/traversal_for_search_algo.hpp:34
// -- eine Alias-Loesung haette sie sofort gebrochen; die Fassade haelt sie unveraendert.
//
// ORGAN_LOCATION NEU (dieselbe Bewegung wie die per-K-Leaf-Hebung der Scheibe 2): dieses Organ ist
// Default-OFF und stand deshalb nie in der committeten Registry-XML -- der F30-Guard konnte seine
// type=-Form folglich NIE pruefen. Die Fassaden-Konstruktion macht die Relation jetzt pinbar, und die
// Familien-Wache prueft sie am TYP (Ebene (9)) BEVOR irgendwer das Flag anschaltet. Byte-Effekt am Ist:
// NULL -- der Generator reflektiert ausschliesslich Enabled* (topic_traversal_config_set.hpp:23),
// belegt am Artefakt durch die XML-ABWESENHEITS-Probe (8b) der Familien-Wache.

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
class VectorU16U16SearchAlgoRebound;

namespace detail {

/// DIE SUBSTANZ des S03-Organs: sortierte Keys + parallele Values, Speicher ueber die Allokator-ACHSE.
///
/// @tparam Alloc  Die Allokator-Achsen-Strategie (axis_06). Default-Bindung der Fassade: ExgenAllocator.
/// @tparam Self   Der most-derived Typ (Fassade oder Rebound-Leaf) -- PFLICHT wegen der CRTP-Guards
///                der SearchAlgoBase (axis_03a_search_algo_base.hpp:47-59).
template <class Alloc, class Self>
class VectorU16U16SearchAlgoCore : public SearchAlgoBase<Self> {
public:
    static constexpr bool enabled = flags::vector_u16u16_enabled;
    // #188-4c-ii: faithful Flach-Store-Pfad via SortedVectorTraversal; Wrapper-Eigen-API/key_type bleibt u16.
    static constexpr bool axis_03a_store_traversable = true;

    using key_type   = std::uint16_t; // Multi-Byte Discriminator
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::multilevel_access_tag;
    using family_id  = std::integral_constant<int, 3>; // S03

    /// A8-S5 SCHNITT-FORM (B): Keys UND Values haengen an der Allokator-ACHSE. Diese Zeile IST der
    /// Ausweis, den die Familien-Konformitaets-Wache liest (tests/unit/s5_family_alloc_conformance.hpp).
    /// Anders als in 01a/01d ist sie NICHT fest, sondern der Kompositions-Parameter -- genau das ist der
    /// Unterschied zwischen "an der Achse" (Zwischenstand) und "Option B strikt".
    using allocator_type = Alloc;
    static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<allocator_type>,
                  "A8-S5: der gebundene Allokator erfuellt das axis_06-Achsen-Concept nicht mehr -- dann liefen "
                  "Keys/Values wieder an der Allokator-Achse vorbei (Schnitt-Regel Dossier 3.4).");

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_fanout() noexcept { return 65536; }
    /// SERIALISIERUNGS-SCHLUESSEL -- bewusst OHNE jeden Allokator-Bezug. Die T6-Wahl darf hier NIE
    /// hineinlecken: sonst truege eine mimalloc-gebundene Komposition einen anderen Organ-Namen als
    /// dieselbe Komposition mit exgen, und binary_id-/serialize-Pfad drifteten gegen die Allokator-
    /// Achse. Die Wache dazu steht als static_assert unter der Fassade (name()-Invarianz).
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "vector_u16u16"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "VectorU16U16SearchAlgo (START multi-byte Cost-DP, Fent et al. ICDEW 2020)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "VECTOR_U16U16"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    ///
    /// NICHT gebumpt in dieser Scheibe -- deklariert, nicht stillschweigend: der Allokations-PFAD aendert
    /// sich, der Algorithmus nicht (Pilot-Praezedenz linear_scan; Bump-Entscheid fuer die Achse liegt im
    /// Mess-/A13-Fenster, S5-04-Praezedenz v1.0.0c).
    static constexpr std::string_view algo_version = "v1.0.0c";

    /// SONDERFALL: kein SIMD — Cost-DP-Algorithmus ist nicht vectorisierbar.
    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; }
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

private:
    using key_alloc    = typename Alloc::template StdAllocatorAdapter<key_type>;
    using value_alloc  = typename Alloc::template StdAllocatorAdapter<value_type>;
    using key_vector   = std::vector<key_type, key_alloc>;
    using value_vector = std::vector<value_type, value_alloc>;

public:
    /// Beide Vektoren werden an das EIGENE allocator_ gebunden (der Adapter haelt &allocator_).
    VectorU16U16SearchAlgoCore()
        : keys_(allocator_.template as_std_allocator<key_type>()),
          values_(allocator_.template as_std_allocator<value_type>()) {}

    /// KF-6-NAHT (Posten 62, LEDGER 04.08. abend-12): eine vor-parametrierte Strategie-Instanz
    /// uebernehmen, statt sie default zu konstruieren. Heute nirgends benutzt und bewusst `explicit`.
    explicit VectorU16U16SearchAlgoCore(allocator_type a)
        : allocator_(std::move(a)), keys_(allocator_.template as_std_allocator<key_type>()),
          values_(allocator_.template as_std_allocator<value_type>()) {}

    /// Copy: Strategie mitkopieren, beide Vektoren an das EIGENE allocator_ binden, dann die transiente
    /// Kopier-Allokation aus der Statistik nehmen (Memento) -- 1:1 btree_node_pool_store.hpp:86.
    /// MOVE ist bewusst NICHT deklariert (der user-definierte Copy unterdrueckt ihn implizit).
    VectorU16U16SearchAlgoCore(VectorU16U16SearchAlgoCore const& o)
        : allocator_(o.allocator_), keys_(o.keys_, allocator_.template as_std_allocator<key_type>()),
          values_(o.values_, allocator_.template as_std_allocator<value_type>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_ = o.stats_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }

    VectorU16U16SearchAlgoCore& operator=(VectorU16U16SearchAlgoCore const& o) {
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

    ~VectorU16U16SearchAlgoCore() = default;

    [[nodiscard]] bool operator==(VectorU16U16SearchAlgoCore const& other) const noexcept {
        return keys_.size() == other.keys_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: push_back kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        auto        it  = std::lower_bound(keys_.begin(), keys_.end(), k);
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        if (it != keys_.end() && *it == k) {
            values_[idx] = v;
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
    [[nodiscard]] double    density_percent() const noexcept {
        return 100.0 * static_cast<double>(keys_.size()) / 65536.0;
    }
    void clear() noexcept {
        keys_.clear();
        values_.clear();
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]]:
    /// Balanced default — Multilevel-Cost-DP optimiert sich automatisch
    /// fuer mittlere Density-Bereiche.
    [[nodiscard]] concepts::DensityClass density_class() const noexcept { return concepts::DensityClass::Balanced; }

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
    /// konstitutiven Store-Snapshot. Die Doppelzaehlungs-Regel ist der EXPLIZITE Schritt des
    /// Mess-Schnitt-Fensters VOR Messbeginn, nicht dieser Scheibe.
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

/// DIE IDENTITAET -- das Registry-Organ S03. Nicht-Template, exakt der historische Typ-Name.
/// `final`: die Ebenen-Trennung ist Vertrag, nicht Vorschlag -- ein weiterer Ableitungs-Schritt
/// unter der Fassade wuerde den CRTP-Self-Vertrag des Cores brechen.
class VectorU16U16SearchAlgo final
    : public detail::VectorU16U16SearchAlgoCore<::comdare::cache_engine::alloc::ExgenAllocator,
                                                VectorU16U16SearchAlgo> {
public:
    /// Die Default-Bindung der Identitaets-Ebene: der BENANNTE Achsen-Default, nie std::allocator.
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;

    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::VectorU16U16SearchAlgo",
                                  "axes/lookup/axis_03a_search_algo_vector_u16u16.hpp");

    /// Der Migrations-Ausweis (composable::AllocatorRebindableSearchAlgo): auf WELCHEN Typ bindet die
    /// Kompositions-Naht um, wenn die Komposition eine fremde Strategie fuehrt?
    template <class A2>
    using rebind_allocator = VectorU16U16SearchAlgoRebound<A2>;

    using detail::VectorU16U16SearchAlgoCore<default_allocator_type,
                                             VectorU16U16SearchAlgo>::VectorU16U16SearchAlgoCore;
};

/// DIE GEBUNDENE FORM -- materialisiert nur am Genus-Erst-Instanziierungs-Punkt. Erbt name(),
/// family_id und algo_version unveraendert aus dem Core. Traegt BEWUSST KEIN
/// COMDARE_DEFINE_ORGAN_LOCATION -- er ist kein Registry-Organ.
template <class A2>
class VectorU16U16SearchAlgoRebound final
    : public detail::VectorU16U16SearchAlgoCore<A2, VectorU16U16SearchAlgoRebound<A2>> {
public:
    /// Der EBENEN-AUSWEIS (s. composable::IsReboundSearchAlgoLeaf).
    using axis03a_rebound_tag = void;

    using detail::VectorU16U16SearchAlgoCore<A2, VectorU16U16SearchAlgoRebound<A2>>::VectorU16U16SearchAlgoCore;
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<VectorU16U16SearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<VectorU16U16SearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<VectorU16U16SearchAlgo>);
// NICHT: SimdCapableStrategy (Cost-DP ist nicht vektorisierbar)

// ---------------------------------------------------------------------------------------------
// Der Zwei-Ebenen-Vertrag, self-proving an der Datei, die ihn eingeht (Pilot-Rezept, linear_scan:352).
// ---------------------------------------------------------------------------------------------

/// LEVEL 0 (der golden-Pfad): die Kompositions-Naht mit dem Achsen-Default liefert die FASSADE SELBST.
static_assert(std::is_same_v<composable::search_algo_for_composition_t<VectorU16U16SearchAlgo,
                                                                       ::comdare::cache_engine::alloc::ExgenAllocator>,
                             VectorU16U16SearchAlgo>,
              "01c Level-0-IDENTITAET verletzt: die Kompositions-Naht liefert am ACHSEN-DEFAULT nicht mehr die "
              "Fassade selbst. Damit laege ein anderer Typ auf dem golden-Pfad -- Typ-Neutralitaet weg.");

/// Der Migrations-Ausweis ist da (sonst faele das Organ still auf Level 2 = unveraendert zurueck).
static_assert(
    composable::AllocatorRebindableSearchAlgo<VectorU16U16SearchAlgo, ::comdare::cache_engine::alloc::ExgenAllocator>,
    "01c: VectorU16U16SearchAlgo traegt keinen rebind_allocator mehr -- nicht migriert.");

/// EBENEN-TRENNUNG: die Identitaets-Ebene traegt NIE den Rebound-Ausweis, der Leaf traegt ihn IMMER.
static_assert(!composable::IsReboundSearchAlgoLeaf<VectorU16U16SearchAlgo>,
              "01c EBENEN-VERMISCHUNG: die Fassade traegt den Rebound-Tag -- Emitter-type_name-Reise und "
              "Registry-Reflektion zeigten dann auf die Substanz-Ebene.");
static_assert(
    composable::IsReboundSearchAlgoLeaf<VectorU16U16SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>,
    "01c: der Rebound-Leaf traegt seinen Ausweis nicht -- der Identitaets-Pin kann nicht mehr greifen.");

/// name()-ALLOKATOR-INVARIANZ: der serialize-Schluessel darf sich mit der T6-Wahl NICHT bewegen.
static_assert(composable::search_algo_name_is_allocator_invariant_v<VectorU16U16SearchAlgo,
                                                                    ::comdare::cache_engine::alloc::ExgenAllocator>,
              "01c name()-INVARIANZ (Level 0) verletzt.");
static_assert(VectorU16U16SearchAlgo::name() ==
                  VectorU16U16SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>::name(),
              "01c name()-INVARIANZ verletzt: der Rebound-Leaf traegt einen anderen Organ-Namen als die Fassade -- "
              "die T6-Wahl leckte in den serialize-/binary_id-Schluessel.");

/// Der Rebound-Leaf ist ein VOLLWERTIGES Organ, nicht nur eine Typ-Huelle (die CRTP-Guard-Kette laeuft
/// auf beiden Leaves -- R2 des Design-Risikoblatts).
static_assert(
    concepts::SearchAlgoVariant<VectorU16U16SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<
              VectorU16U16SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(
    concepts::DensityClassifiedStrategy<VectorU16U16SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
} // namespace comdare::cache_engine::lookup
