#pragma once
// V41.F.6.1.F.6 axis_03a_search_algo Array65535SearchAlgo S09 (2026-05-29)
//
// @topic traversal @achse 03a @family S09 Array65535SearchAlgo
// @subaxis SA4 direct_multibyte_access
//
// **Herkunft:** F.6-Migration aus prt-art `internal_search/array_65535.hpp`
// (REV 6 §5.17 — "Density 25-50 %"). Direkt-adressiertes Array mit
// std::uint16_t-Diskriminator, hoehere Verzweigungs-Tiefe als Array256
// (Single-Byte) und das direkt-adressierte Gegenstueck zu VectorU16U16
// (sortiert) im selben uint16-Fanout-Bereich.
//
// **Algorithmus-Pattern:** ART-artige direkte Adressierung, aber auf
// 2-Byte-Diskriminator erweitert (Mid-Density-Tier zwischen Array256 dense
// und sparse Patricia). Kein externes Paper — prt-art-Eigenentwurf.
//
// **Korrektheit ggü. prt-art-Original:** prt-art nutzte `kCapacity = 65535`
// mit `slots_[discriminator]` — ein uint16-Diskriminator kann jedoch 0..65535
// annehmen (65536 Werte), d.h. `discriminator == 65535` war im Original
// Out-of-Bounds (UB). Hier auf 65536 Slots korrigiert (voller uint16-Bereich),
// konsistent mit VectorU16U16 (Nenner 65536). Der Klassen-/Familienname bleibt
// "Array65535" als etablierte Taxonomie-Bezeichnung des ~64K-Fanout-Tiers.
//
// Erfuellt:
//   - SearchAlgoVariant (Pflicht-API)
//   - CacheEngineSearchAlgoPermutationStrategy (cache-engine-spec)
//   - DensityClassifiedStrategy (DensityClass::Balanced — Mid-Density 25-50 %)
//   - **NICHT** SimdCapableStrategy (direkter O(1)-Index, kein SIMD-Vorteil;
//     65536-Slot-Scan waere zudem zu breit fuer sinnvolle Vektorisierung)
//
// Allocation: NUR ueber das Allokator-Achsen-Interface (axis_06) -- s. den ZWEI-EBENEN-SCHNITT unten.
// [[allocation-failure-exception]]: die zwei Konstruktor-Allokationen koennen std::bad_alloc werfen
// (StdAllocatorAdapter, Posten 64). Presence-Vektor statt Sentinel-Wert, damit jeder value_type
// (inkl. ~0ull) als gueltiger Slot-Wert darstellbar bleibt (keine Sentinel-Kollision).
//
// ===================================================================================================
// A8-S5 Familie 01c, Scheibe 3 (2026-08-05) -- ZWEI-EBENEN-SCHNITT (Pilot-Rezept linear_scan)
// ===================================================================================================
// KLASSEN-ENTSCHEID: **VOLL-REZEPT** -- zwei Default-Allokator-Vektoren (data_/present_, 2 der 39
// Rest-Zeilen der 01b-Schlussbilanz). BESONDERS TRAGEND HIER: dieses Organ alloziert seine beiden
// 65536-Slot-Puffer BEREITS IM KONSTRUKTOR (data_ 512 KiB + present_ 64 KiB je Instanz). Am Alt-Stand
// lief genau diese groesste Einzel-Allokation der Achse am Allokator-Achsen-Interface vorbei -- die
// T6-Spalte haette sie nie gesehen. Konstruktion/Begruendung zeilengleich zum Pilot
// (axis_03a_search_algo_linear_scan.hpp:22-58), hier referenziert statt wiederholt.
//
// ORGAN_LOCATION NEU (Default-OFF -> Byte-Effekt NULL, s. XML-ABWESENHEITS-Probe (8b) der Familien-Wache).

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <organ_axes/alloc/axis_06_allocator_exgen.hpp>
#include <organ_axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <organ_axes/lookup/composable/capacity_constraint.hpp>
#include <organ_axes/lookup/axis_03a_search_algo_flags.hpp>
#include <organ_axes/lookup/composable/search_algo_rebind.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <limits>
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
class Array65535SearchAlgoRebound;

namespace detail {

/// DIE SUBSTANZ des S09-Organs: direkt-adressierte 65536-Slot-Puffer ueber die Allokator-ACHSE.
///
/// @tparam Alloc  Die Allokator-Achsen-Strategie (axis_06). Default-Bindung der Fassade: ExgenAllocator.
/// @tparam Self   Der most-derived Typ -- PFLICHT wegen der CRTP-Guards der SearchAlgoBase.
template <class Alloc, class Self>
class Array65535SearchAlgoCore : public SearchAlgoBase<Self> {
public:
    static constexpr bool enabled = flags::array65535_enabled;
    // #188-4c-ii: faithful Flach-Store-Pfad via DirectAddressTraversal; #217-2b: Wrapper-key_type bleibt heute u16.
    static constexpr bool axis_03a_store_traversable = true;

    /// Voller uint16-Diskriminator-Bereich [0, 65535] = 65536 Slots (Korrektur
    /// der prt-art-65535-Off-by-one). Density-Zielband aus REV 6 §5.17.
    static constexpr std::size_t kCapacity          = 65536;
    static constexpr double      kDensityMinPercent = 25.0;
    static constexpr double      kDensityMaxPercent = 50.0;

    using key_type   = std::uint16_t; // #217-2b: native Wrapper-Breite bleibt heute; Umbau spaeter.
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::direct_multibyte_access_tag;
    using family_id  = std::integral_constant<int, 9>; // S09

    /// A8-S5 SCHNITT-FORM (B): BEIDE Konstruktor-Puffer haengen an der Allokator-ACHSE. Diese Zeile IST
    /// der Ausweis, den die Familien-Konformitaets-Wache liest (tests/unit/s5_family_alloc_conformance.hpp).
    using allocator_type = Alloc;
    static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<allocator_type>,
                  "A8-S5: der gebundene Allokator erfuellt das axis_06-Achsen-Concept nicht mehr -- dann liefen "
                  "die 65536-Slot-Puffer wieder an der Allokator-Achse vorbei (Schnitt-Regel Dossier 3.4).");

    static_assert(static_cast<std::uint64_t>(std::numeric_limits<key_type>::max()) >=
                      static_cast<std::uint64_t>(kCapacity - 1u),
                  "Array65535SearchAlgo key_type muss jeden deklarierten Static-Slot adressieren koennen");

    [[nodiscard]] static constexpr bool                           is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t                    max_fanout() noexcept { return kCapacity; }
    [[nodiscard]] static constexpr composable::CapacityConstraint container_capacity() noexcept {
        return {0, kCapacity, composable::CapacityKind::Static};
    }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "array65535"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Array65535SearchAlgo (prt-art REV6 §5.17 mid-density direct-addressed uint16)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "ARRAY65535"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

    /// SONDERFALL: kein SIMD — direkter O(1)-Index, kein Vektorisierungs-Vorteil.
    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // index-geordnet
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }           // Mid-Density
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

private:
    using value_alloc    = typename Alloc::template StdAllocatorAdapter<value_type>;
    using present_alloc  = typename Alloc::template StdAllocatorAdapter<std::uint8_t>;
    using value_vector   = std::vector<value_type, value_alloc>;
    using present_vector = std::vector<std::uint8_t, present_alloc>;

public:
    /// SONDERFALL [[allocation-failure-exception]]: die zwei vector(kCapacity) koennen std::bad_alloc
    /// werfen -- ab jetzt aus dem StdAllocatorAdapter (Posten 64), nicht mehr aus dem Default-Allokator.
    Array65535SearchAlgoCore()
        : data_(kCapacity, allocator_.template as_std_allocator<value_type>()),
          present_(kCapacity, 0u, allocator_.template as_std_allocator<std::uint8_t>()), count_(0) {}

    /// KF-6-NAHT (Posten 62, LEDGER 04.08. abend-12): eine vor-parametrierte Strategie-Instanz
    /// uebernehmen, statt sie default zu konstruieren. Heute nirgends benutzt und bewusst `explicit`.
    explicit Array65535SearchAlgoCore(allocator_type a)
        : allocator_(std::move(a)), data_(kCapacity, allocator_.template as_std_allocator<value_type>()),
          present_(kCapacity, 0u, allocator_.template as_std_allocator<std::uint8_t>()), count_(0) {}

    /// Copy: Strategie mitkopieren, beide Puffer an das EIGENE allocator_ binden, dann die transiente
    /// Kopier-Allokation aus der Statistik nehmen (Memento) -- 1:1 btree_node_pool_store.hpp:86.
    Array65535SearchAlgoCore(Array65535SearchAlgoCore const& o)
        : allocator_(o.allocator_), data_(o.data_, allocator_.template as_std_allocator<value_type>()),
          present_(o.present_, allocator_.template as_std_allocator<std::uint8_t>()), count_(o.count_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_ = o.stats_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }

    Array65535SearchAlgoCore& operator=(Array65535SearchAlgoCore const& o) {
        if (this != &o) {
            data_    = o.data_;
            present_ = o.present_;
            count_   = o.count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_ = o.stats_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }

    ~Array65535SearchAlgoCore() = default;

    [[nodiscard]] bool operator==(Array65535SearchAlgoCore const& other) const noexcept {
        return count_ == other.count_;
    }

    void insert(key_type k, value_type v) {
        if (present_[k] == 0u) ++count_;
        data_[k]    = v;
        present_[k] = 1u;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (count_ > stats_.peak_occupancy) stats_.peak_occupancy = count_;
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        bool hit = (present_[k] != 0u);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        if (hit)
            ++stats_.total_hit_count;
        else
            ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        if (!hit) return std::nullopt;
        return data_[k];
    }

    bool erase(key_type k) {
        if (present_[k] == 0u) return false;
        present_[k] = 0u;
        --count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return count_; }
    [[nodiscard]] double    density_percent() const noexcept {
        return 100.0 * static_cast<double>(count_) / static_cast<double>(kCapacity);
    }
    void clear() noexcept {
        for (auto& p : present_) p = 0u;
        count_ = 0;
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]]:
    /// Mid-Density-Tier (Zielband 25-50 %) — Balanced.
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

    /// EINSAMMEL-NAHT der T6-Durchbindung (Owner-KERN abend-11, Pflicht (a)) -- NUR die Naht,
    /// BEWUSST unter einem VIERTEN Namen (Doppelzaehlungs-Absicherung, Pilot-Begruendung).
    using allocator_snapshot_t = typename allocator_type::snapshot_t;
    [[nodiscard]] allocator_snapshot_t search_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    // allocator_ MUSS VOR data_/present_ stehen: der Adapter haelt &allocator_, und die Member-
    // Initialisierungs-/Zerstoerungsreihenfolge ist die Deklarationsreihenfolge.
    allocator_type allocator_{};
    value_vector   data_;
    present_vector present_;
    std::size_t    count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace detail

/// DIE IDENTITAET -- das Registry-Organ S09. Nicht-Template, exakt der historische Typ-Name.
class Array65535SearchAlgo final
    : public detail::Array65535SearchAlgoCore<::comdare::cache_engine::alloc::ExgenAllocator, Array65535SearchAlgo> {
public:
    /// Die Default-Bindung der Identitaets-Ebene: der BENANNTE Achsen-Default, nie std::allocator.
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;

    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::Array65535SearchAlgo",
                                  "organ_axes/lookup/axis_03a_search_algo_array65535.hpp");

    /// Der Migrations-Ausweis (composable::AllocatorRebindableSearchAlgo).
    template <class A2>
    using rebind_allocator = Array65535SearchAlgoRebound<A2>;

    using detail::Array65535SearchAlgoCore<default_allocator_type, Array65535SearchAlgo>::Array65535SearchAlgoCore;
};

/// DIE GEBUNDENE FORM -- traegt BEWUSST KEIN COMDARE_DEFINE_ORGAN_LOCATION.
template <class A2>
class Array65535SearchAlgoRebound final : public detail::Array65535SearchAlgoCore<A2, Array65535SearchAlgoRebound<A2>> {
public:
    /// Der EBENEN-AUSWEIS (s. composable::IsReboundSearchAlgoLeaf).
    using axis03a_rebound_tag = void;

    using detail::Array65535SearchAlgoCore<A2, Array65535SearchAlgoRebound<A2>>::Array65535SearchAlgoCore;
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<Array65535SearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<Array65535SearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<Array65535SearchAlgo>);
// NICHT: SimdCapableStrategy (direkter O(1)-Index, kein SIMD-Vorteil)

// ---------------------------------------------------------------------------------------------
// Der Zwei-Ebenen-Vertrag, self-proving an der Datei, die ihn eingeht (Pilot-Rezept, linear_scan:352).
// ---------------------------------------------------------------------------------------------
static_assert(std::is_same_v<composable::search_algo_for_composition_t<Array65535SearchAlgo,
                                                                       ::comdare::cache_engine::alloc::ExgenAllocator>,
                             Array65535SearchAlgo>,
              "01c Level-0-IDENTITAET verletzt: die Kompositions-Naht liefert am ACHSEN-DEFAULT nicht mehr die "
              "Fassade selbst. Damit laege ein anderer Typ auf dem golden-Pfad -- Typ-Neutralitaet weg.");
static_assert(
    composable::AllocatorRebindableSearchAlgo<Array65535SearchAlgo, ::comdare::cache_engine::alloc::ExgenAllocator>,
    "01c: Array65535SearchAlgo traegt keinen rebind_allocator mehr -- nicht migriert.");
static_assert(!composable::IsReboundSearchAlgoLeaf<Array65535SearchAlgo>,
              "01c EBENEN-VERMISCHUNG: die Fassade traegt den Rebound-Tag -- Emitter-type_name-Reise und "
              "Registry-Reflektion zeigten dann auf die Substanz-Ebene.");
static_assert(
    composable::IsReboundSearchAlgoLeaf<Array65535SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>,
    "01c: der Rebound-Leaf traegt seinen Ausweis nicht -- der Identitaets-Pin kann nicht mehr greifen.");
static_assert(composable::search_algo_name_is_allocator_invariant_v<Array65535SearchAlgo,
                                                                    ::comdare::cache_engine::alloc::ExgenAllocator>,
              "01c name()-INVARIANZ (Level 0) verletzt.");
static_assert(Array65535SearchAlgo::name() ==
                  Array65535SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>::name(),
              "01c name()-INVARIANZ verletzt: der Rebound-Leaf traegt einen anderen Organ-Namen als die Fassade -- "
              "die T6-Wahl leckte in den serialize-/binary_id-Schluessel.");
static_assert(concepts::SearchAlgoVariant<Array65535SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<
              Array65535SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(
    concepts::DensityClassifiedStrategy<Array65535SearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
} // namespace comdare::cache_engine::lookup
