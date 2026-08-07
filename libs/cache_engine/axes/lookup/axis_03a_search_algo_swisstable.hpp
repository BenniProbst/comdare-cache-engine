#pragma once
// AP-7a/#241 axis_03a_search_algo SwissTableSearchAlgo S22 (2026-07-03)
//
// @topic traversal @achse 03a @family S22 SwissTableSearchAlgo
// @subaxis SA2 sparse_access (UNGEORDNETE Swiss-Table-Hash-Struktur -- kein Range-Scan)
//
// **Algorithmus:** CE-native Re-Implementation des Swiss-Table-Prinzips aus Google/abseil
// raw_hash_set ("Swiss Tables", CppCon 2017): offene Adressierung mit einem Control-Byte je Slot
// (kEmpty=0x80, kDeleted=0xFE, sonst 7-Bit-H2-Fingerprint), Gruppen von 16 Slots, H1/H2-Split
// (H1 -> Gruppen-Startposition, H2 -> Fingerprint) und Tombstones fuer erase.
//
// **AP-7b:** Weg-B-Organ (SwissTableOrgan) fuer den echten Mess-Pfad noch offen -- bis dahin Flag
// Default-OFF (nur registriert/konform, wie S18-S21 per-K). Ohne AP-7b darf S22 nicht als echte
// SwissTable-Messung interpretiert werden.
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** C++23-Re-Impl nach dem veroeffentlichten
// Swiss-Table-Design, kein abseil/ext-Wrap, is_original=false.
//
// Allocation: NUR ueber das Allokator-Achsen-Interface (axis_06) -- s. den ZWEI-EBENEN-SCHNITT unten.
// [[allocation-failure-exception]]: insert/rehash koennen std::bad_alloc werfen (StdAllocatorAdapter,
// Posten 64).
//
// ===================================================================================================
// A8-S5 Familie 01c, Scheibe 3 (2026-08-05) -- ZWEI-EBENEN-SCHNITT (Pilot-Rezept linear_scan)
// ===================================================================================================
// KLASSEN-ENTSCHEID: **VOLL-REZEPT** -- VIER Default-Allokator-Vektoren (slots_ :261, ctrl_ :262 und
// die beiden Rehash-Zwischenpuffer old_slots :238 / old_ctrl :239 -- 4 der 39 Rest-Zeilen der
// 01b-Schlussbilanz, der groesste Einzelposten der Liste).
//
// DIE ZWEI SEITEN-TABELLEN SIND HIER DER PUNKT: die Swiss-Table haelt Kontroll-Bytes GETRENNT von den
// Slots -- das ist ihr Design-Merkmal (Gruppen-Probe ueber die Control-Zeile), und es bedeutet ZWEI
// wachsende Puffer plus ZWEI Zwischenpuffer beim Rehash. Alle vier gehoeren in DASSELBE Allokations-
// Buch (EINE Strategie-Instanz je Organ, Owner-KERN abend-11); haenge nur die Slots an die Achse, und
// die T6-Spalte unterschlaegt genau die Nebenkosten, die dieses Organ von hash_search unterscheiden.
//
// Konstruktion/Begruendung im Uebrigen zeilengleich zum Pilot (axis_03a_search_algo_linear_scan.hpp:22-58).
// ORGAN_LOCATION NEU (Default-OFF -> Byte-Effekt NULL, s. XML-ABWESENHEITS-Probe (8b) der Familien-Wache).

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
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
class SwissTableSearchAlgoRebound;

namespace detail {

/// DIE SUBSTANZ des S22-Organs: Slot-Tabelle + getrennte Control-Zeile ueber die Allokator-ACHSE --
/// inklusive beider Rehash-Zwischenpuffer.
///
/// @tparam Alloc  Die Allokator-Achsen-Strategie (axis_06). Default-Bindung der Fassade: ExgenAllocator.
/// @tparam Self   Der most-derived Typ -- PFLICHT wegen der CRTP-Guards der SearchAlgoBase.
template <class Alloc, class Self>
class SwissTableSearchAlgoCore : public SearchAlgoBase<Self> {
public:
    static constexpr bool enabled = flags::swisstable_enabled;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 22>; // S22

    /// A8-S5 SCHNITT-FORM (B): Slot-Tabelle, Control-Zeile UND beide Rehash-Zwischenpuffer haengen an
    /// DERSELBEN Strategie-Instanz.
    using allocator_type = Alloc;
    static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<allocator_type>,
                  "A8-S5: der gebundene Allokator erfuellt das axis_06-Achsen-Concept nicht mehr -- dann liefen "
                  "Slots und Control-Zeile wieder an der Allokator-Achse vorbei (Dossier 3.4).");

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_fanout() noexcept { return 65536; }
    /// SERIALISIERUNGS-SCHLUESSEL -- bewusst OHNE jeden Allokator-Bezug (name()-Invarianz, s. unten).
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "swisstable"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "SwissTableSearchAlgo (Google Swiss Tables, CppCon 2017 / abseil raw_hash_set -- "
               "CE-native Reimpl, is_original=false)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "SWISSTABLE"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; } // AP-7-Follow: SIMD-Gruppen-Probe
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return false; } // UNGEORDNET -- Pool-Familie
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return false; }

    static constexpr std::uint64_t kFibonacciMul    = 11400714819323198485ULL;
    static constexpr std::size_t   kGroupWidth      = 16;
    static constexpr std::size_t   kInitialCapacity = 16; // Power-of-2, exakt eine Control-Gruppe
    static constexpr std::uint8_t  kEmpty           = 0x80u;
    static constexpr std::uint8_t  kDeleted         = 0xFEu;

private:
    struct Slot {
        key_type   key{};
        value_type val{};
    };
    static constexpr std::size_t kNpos = static_cast<std::size_t>(-1);

    using slot_alloc  = typename Alloc::template StdAllocatorAdapter<Slot>;
    using ctrl_alloc  = typename Alloc::template StdAllocatorAdapter<std::uint8_t>;
    using slot_vector = std::vector<Slot, slot_alloc>;
    using ctrl_vector = std::vector<std::uint8_t, ctrl_alloc>;

public:
    /// Beide Tabellen werden an das EIGENE allocator_ gebunden (der Adapter haelt &allocator_).
    /// SONDERFALL [[allocation-failure-exception]]: die Initial-Tabellen koennen std::bad_alloc werfen.
    SwissTableSearchAlgoCore()
        : slots_(kInitialCapacity, Slot{}, allocator_.template as_std_allocator<Slot>()),
          ctrl_(kInitialCapacity, kEmpty, allocator_.template as_std_allocator<std::uint8_t>()),
          mask_(kInitialCapacity - 1) {}

    /// KF-6-NAHT (Posten 62, LEDGER 04.08. abend-12): eine vor-parametrierte Strategie-Instanz
    /// uebernehmen, statt sie default zu konstruieren. Heute nirgends benutzt und bewusst `explicit`.
    explicit SwissTableSearchAlgoCore(allocator_type a)
        : allocator_(std::move(a)), slots_(kInitialCapacity, Slot{}, allocator_.template as_std_allocator<Slot>()),
          ctrl_(kInitialCapacity, kEmpty, allocator_.template as_std_allocator<std::uint8_t>()),
          mask_(kInitialCapacity - 1) {}

    /// Copy: Strategie mitkopieren, beide Tabellen an das EIGENE allocator_ binden, dann die transiente
    /// Kopier-Allokation aus der Statistik nehmen (Memento) -- 1:1 btree_node_pool_store.hpp:86.
    SwissTableSearchAlgoCore(SwissTableSearchAlgoCore const& o)
        : allocator_(o.allocator_), slots_(o.slots_, allocator_.template as_std_allocator<Slot>()),
          ctrl_(o.ctrl_, allocator_.template as_std_allocator<std::uint8_t>()), mask_(o.mask_), size_(o.size_),
          tombstones_(o.tombstones_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_ = o.stats_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }

    SwissTableSearchAlgoCore& operator=(SwissTableSearchAlgoCore const& o) {
        if (this != &o) {
            slots_      = o.slots_;
            ctrl_       = o.ctrl_;
            mask_       = o.mask_;
            size_       = o.size_;
            tombstones_ = o.tombstones_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_ = o.stats_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }

    ~SwissTableSearchAlgoCore() = default;

    [[nodiscard]] bool operator==(SwissTableSearchAlgoCore const& other) const noexcept { return size_ == other.size_; }

    /// SONDERFALL [[allocation-failure-exception]]: rehash kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        if ((size_ + tombstones_ + 1u) * 8u >= (mask_ + 1u) * 7u) rehash((mask_ + 1u) * 2u);
        insert_impl(k, v, true);
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        std::uint64_t const       hash   = mixed_hash(k);
        std::uint8_t const        h2     = h2_fingerprint(hash);
        std::size_t const         groups = group_count();
        for (std::size_t probe = 0; probe < groups; ++probe) {
            std::size_t const group_start = group_start_for(hash, probe);
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (ctrl_[pos] == h2 && slots_[pos].key == k) {
                    result = slots_[pos].val;
                    goto done;
                }
            }
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                if (ctrl_[group_start + i] == kEmpty) goto done; // Probe-Kette zu Ende -> Miss
            }
        }
    done:
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
        std::uint64_t const hash   = mixed_hash(k);
        std::uint8_t const  h2     = h2_fingerprint(hash);
        std::size_t const   groups = group_count();
        for (std::size_t probe = 0; probe < groups; ++probe) {
            std::size_t const group_start = group_start_for(hash, probe);
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (ctrl_[pos] == h2 && slots_[pos].key == k) {
                    ctrl_[pos]  = kDeleted; // Tombstone -- Probe-Kette bleibt intakt
                    slots_[pos] = Slot{};
                    --size_;
                    ++tombstones_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
                    ++stats_.total_erase_count;
                    observer_.notify(stats_);
#endif
                    return true;
                }
            }
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                if (ctrl_[group_start + i] == kEmpty) return false;
            }
        }
        return false;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return size_; }
    [[nodiscard]] double    density_percent() const noexcept { return 100.0 * static_cast<double>(size_) / 65536.0; }
    void                    clear() noexcept {
        for (auto& s : slots_) s = Slot{};
        std::fill(ctrl_.begin(), ctrl_.end(), kEmpty);
        size_       = 0;
        tombstones_ = 0;
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]].
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        if (size_ > 1024) return concepts::DensityClass::Dense;
        if (size_ > 64) return concepts::DensityClass::Balanced;
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
    using allocator_snapshot_t = typename allocator_type::snapshot_t;
    [[nodiscard]] allocator_snapshot_t search_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    [[nodiscard]] static constexpr std::uint64_t mixed_hash(key_type k) noexcept {
        return static_cast<std::uint64_t>(k) * kFibonacciMul;
    }

    [[nodiscard]] static constexpr std::uint8_t h2_fingerprint(std::uint64_t hash) noexcept {
        return static_cast<std::uint8_t>(hash & 0x7Fu);
    }

    [[nodiscard]] std::size_t group_count() const noexcept { return (mask_ + 1u) / kGroupWidth; }

    [[nodiscard]] std::size_t group_start_for(std::uint64_t hash, std::size_t probe) const noexcept {
        std::size_t const group_mask = group_count() - 1u;
        std::size_t const h1_group   = static_cast<std::size_t>(hash >> 7u) & group_mask;
        return ((h1_group + probe) & group_mask) * kGroupWidth;
    }

    void insert_impl(key_type k, value_type v, bool notify) {
        std::uint64_t const hash          = mixed_hash(k);
        std::uint8_t const  h2            = h2_fingerprint(hash);
        std::size_t const   groups        = group_count();
        std::size_t         first_deleted = kNpos;
        for (std::size_t probe = 0; probe < groups; ++probe) {
            std::size_t const group_start = group_start_for(hash, probe);
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (ctrl_[pos] == h2 && slots_[pos].key == k) {
                    slots_[pos].val = v;
                    if (notify) notify_insert();
                    return;
                }
            }
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (ctrl_[pos] == kDeleted && first_deleted == kNpos) first_deleted = pos;
            }
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (ctrl_[pos] == kEmpty) {
                    std::size_t const target = (first_deleted != kNpos) ? first_deleted : pos;
                    if (first_deleted != kNpos) --tombstones_; // Tombstone wiederverwendet
                    ctrl_[target]  = h2;
                    slots_[target] = Slot{k, v};
                    ++size_;
                    if (notify) notify_insert();
                    return;
                }
            }
        }

        rehash((mask_ + 1u) * 2u);
        insert_impl(k, v, notify);
    }

    void rehash(std::size_t new_capacity) {
        // BEIDE Zwischenpuffer mit DERSELBEN Strategie-Instanz wie die Zieltabellen: beim Wachsen haelt
        // das Organ kurzzeitig vier Tabellen, und alle vier gehoeren in DASSELBE Allokations-Buch
        // (Owner-KERN abend-11 -- eine Strategie-Instanz je Organ). Der swap tauscht nur Zeiger.
        slot_vector old_slots(allocator_.template as_std_allocator<Slot>());
        ctrl_vector old_ctrl(allocator_.template as_std_allocator<std::uint8_t>());
        old_slots.swap(slots_);
        old_ctrl.swap(ctrl_);
        slots_.assign(new_capacity, Slot{});
        ctrl_.assign(new_capacity, kEmpty);
        mask_       = new_capacity - 1u;
        size_       = 0;
        tombstones_ = 0;
        for (std::size_t i = 0; i < old_ctrl.size(); ++i) {
            if (old_ctrl[i] == kEmpty || old_ctrl[i] == kDeleted) continue;
            insert_impl(old_slots[i].key, old_slots[i].val, false);
        }
    }

    void notify_insert() noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (size_ > stats_.peak_occupancy) stats_.peak_occupancy = size_;
        observer_.notify(stats_);
#endif
    }

    // allocator_ MUSS VOR slots_/ctrl_ stehen: der Adapter haelt &allocator_, und die Member-
    // Initialisierungs-/Zerstoerungsreihenfolge ist die Deklarationsreihenfolge.
    allocator_type allocator_{};
    slot_vector    slots_;
    ctrl_vector    ctrl_;
    std::size_t    mask_;
    std::size_t    size_       = 0;
    std::size_t    tombstones_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace detail

/// DIE IDENTITAET -- das Registry-Organ S22. Nicht-Template, exakt der historische Typ-Name.
class SwissTableSearchAlgo final
    : public detail::SwissTableSearchAlgoCore<::comdare::cache_engine::alloc::ExgenAllocator, SwissTableSearchAlgo> {
public:
    /// Die Default-Bindung der Identitaets-Ebene: der BENANNTE Achsen-Default, nie std::allocator.
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;

    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::SwissTableSearchAlgo",
                                  "axes/lookup/axis_03a_search_algo_swisstable.hpp");

    /// Der Migrations-Ausweis (composable::AllocatorRebindableSearchAlgo).
    template <class A2>
    using rebind_allocator = SwissTableSearchAlgoRebound<A2>;

    using detail::SwissTableSearchAlgoCore<default_allocator_type, SwissTableSearchAlgo>::SwissTableSearchAlgoCore;
};

/// DIE GEBUNDENE FORM -- traegt BEWUSST KEIN COMDARE_DEFINE_ORGAN_LOCATION.
template <class A2>
class SwissTableSearchAlgoRebound final : public detail::SwissTableSearchAlgoCore<A2, SwissTableSearchAlgoRebound<A2>> {
public:
    /// Der EBENEN-AUSWEIS (s. composable::IsReboundSearchAlgoLeaf).
    using axis03a_rebound_tag = void;

    using detail::SwissTableSearchAlgoCore<A2, SwissTableSearchAlgoRebound<A2>>::SwissTableSearchAlgoCore;
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<SwissTableSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<SwissTableSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<SwissTableSearchAlgo>);

// ---------------------------------------------------------------------------------------------
// Der Zwei-Ebenen-Vertrag, self-proving an der Datei, die ihn eingeht (Pilot-Rezept, linear_scan:352).
// ---------------------------------------------------------------------------------------------
static_assert(std::is_same_v<composable::search_algo_for_composition_t<SwissTableSearchAlgo,
                                                                       ::comdare::cache_engine::alloc::ExgenAllocator>,
                             SwissTableSearchAlgo>,
              "01c Level-0-IDENTITAET verletzt: die Kompositions-Naht liefert am ACHSEN-DEFAULT nicht mehr die "
              "Fassade selbst. Damit laege ein anderer Typ auf dem golden-Pfad -- Typ-Neutralitaet weg.");
static_assert(
    composable::AllocatorRebindableSearchAlgo<SwissTableSearchAlgo, ::comdare::cache_engine::alloc::ExgenAllocator>,
    "01c: SwissTableSearchAlgo traegt keinen rebind_allocator mehr -- nicht migriert.");
static_assert(!composable::IsReboundSearchAlgoLeaf<SwissTableSearchAlgo>,
              "01c EBENEN-VERMISCHUNG: die Fassade traegt den Rebound-Tag -- Emitter-type_name-Reise und "
              "Registry-Reflektion zeigten dann auf die Substanz-Ebene.");
static_assert(
    composable::IsReboundSearchAlgoLeaf<SwissTableSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>,
    "01c: der Rebound-Leaf traegt seinen Ausweis nicht -- der Identitaets-Pin kann nicht mehr greifen.");
static_assert(composable::search_algo_name_is_allocator_invariant_v<SwissTableSearchAlgo,
                                                                    ::comdare::cache_engine::alloc::ExgenAllocator>,
              "01c name()-INVARIANZ (Level 0) verletzt.");
static_assert(SwissTableSearchAlgo::name() ==
                  SwissTableSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>::name(),
              "01c name()-INVARIANZ verletzt: der Rebound-Leaf traegt einen anderen Organ-Namen als die Fassade -- "
              "die T6-Wahl leckte in den serialize-/binary_id-Schluessel.");
static_assert(concepts::SearchAlgoVariant<SwissTableSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<
              SwissTableSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(
    concepts::DensityClassifiedStrategy<SwissTableSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
} // namespace comdare::cache_engine::lookup
