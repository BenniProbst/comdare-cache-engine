#pragma once
// V41.F.6.1 R7.2 axis_03a_search_algo HashSearchAlgo S14 (2026-05-29)
//
// @topic traversal @achse 03a @family S14 HashSearchAlgo
// @subaxis SA2 sparse_access (UNGEORDNETE Hash-Struktur — kein Range-Scan)
//
// **Algorithmus:** Open-Addressing-Hashtabelle mit Linear Probing + multiplikativem (Fibonacci-)
// Hashing, beschrieben in: Donald E. Knuth, "The Art of Computer Programming, Vol. 3: Sorting and
// Searching", 2nd Ed. 1998, §6.4 (Hashing). Konstante 2^64/φ ≈ 11400714819323198485.
//
// Das fundamentale Such-Paradigma, das axis_03a noch fehlte: WEDER Array (Array256/65535) noch
// sortierter Vektor (VectorU8/U16) noch Such-METHODE auf Sortierung (k-ary/interpolation/eytzinger)
// noch verzeigerte geordnete Struktur (skip-list) noch Radix-Trie (ART/HOT/…), sondern eine
// UNGEORDNETE Hashtabelle: O(1) erwartet bei load_factor < 0.7, KEINE Schluessel-Ordnung
// (supports_range_scan=false — die distinkte Klassifikation). Komplettiert die Such-Bibliothek.
//
// **KORREKTES erase** ([[algorithm-correctness-when-named]]): Tombstone-Markierung (NICHT bloßes
// Leeren — das wuerde bei Linear Probing die Probe-Kette nachfolgender Schluessel brechen). Tombstones
// werden bei insert wiederverwendet + beim Rehash (Resize) entfernt.
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** Lehrbuch-Algorithmus → C++23-Re-Impl,
// is_original=false.
//
// Allocation: NUR ueber das Allokator-Achsen-Interface (axis_06) -- s. den ZWEI-EBENEN-SCHNITT unten.
// [[allocation-failure-exception]]: insert/rehash koennen std::bad_alloc werfen (StdAllocatorAdapter,
// Posten 64).
//
// ===================================================================================================
// A8-S5 Familie 01c, Scheibe 3 (2026-08-05) -- ZWEI-EBENEN-SCHNITT (Pilot-Rezept linear_scan)
// ===================================================================================================
// KLASSEN-ENTSCHEID: **VOLL-REZEPT** -- Bucket-Tabelle + der REHASH-Zwischenpuffer (buckets_ :229 und
// die lokale `old`-Tabelle im rehash :201, 2 der 39 Rest-Zeilen der 01b-Schlussbilanz).
//
// DIE LOKALE `old`-TABELLE IST HIER KEIN DETAIL, SONDERN DER GROESSTE POSTEN: beim Wachsen haelt das
// Organ kurzzeitig ZWEI Tabellen gleichzeitig (alte + neue). Bliebe der Zwischenpuffer am
// Default-Allokator, saehe die T6-Spalte den gesamten Wachstums-Pfad nicht -- und genau der ist bei
// einer Hashtabelle die interessante Allokations-Geschichte. Er wird deshalb mit derselben
// Strategie-Instanz konstruiert wie die Zieltabelle (EIN Buch je Organ, Owner-KERN abend-11).
//
// Konstruktion/Begruendung im Uebrigen zeilengleich zum Pilot (axis_03a_search_algo_linear_scan.hpp:22-58).
// ORGAN_LOCATION NEU (Default-OFF -> Byte-Effekt NULL, s. XML-ABWESENHEITS-Probe (8b) der Familien-Wache).

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
class HashSearchAlgoRebound;

namespace detail {

/// DIE SUBSTANZ des S14-Organs: Open-Addressing-Bucket-Tabelle ueber die Allokator-ACHSE --
/// inklusive des Rehash-Zwischenpuffers.
///
/// @tparam Alloc  Die Allokator-Achsen-Strategie (axis_06). Default-Bindung der Fassade: ExgenAllocator.
/// @tparam Self   Der most-derived Typ -- PFLICHT wegen der CRTP-Guards der SearchAlgoBase.
template <class Alloc, class Self>
class HashSearchAlgoCore : public SearchAlgoBase<Self> {
public:
    static constexpr bool enabled = flags::hash_search_enabled;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 14>; // S14

    /// A8-S5 SCHNITT-FORM (B): Bucket-Tabelle UND Rehash-Zwischenpuffer haengen an DERSELBEN
    /// Strategie-Instanz.
    using allocator_type = Alloc;
    static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<allocator_type>,
                  "A8-S5: der gebundene Allokator erfuellt das axis_06-Achsen-Concept nicht mehr -- dann liefe "
                  "der Wachstums-Pfad der Hashtabelle wieder an der Allokator-Achse vorbei (Dossier 3.4).");

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_fanout() noexcept { return 65536; }
    /// SERIALISIERUNGS-SCHLUESSEL -- bewusst OHNE jeden Allokator-Bezug (name()-Invarianz, s. unten).
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "hash_search"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "HashSearchAlgo (open-addressing Fibonacci hash, linear probing — Knuth TAOCP 3 §6.4)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "HASH_SEARCH"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; } // Hash-Probing
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept {
        return false;
    } // UNGEORDNET — Kern-Unterschied
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return false; }

    static constexpr std::uint64_t kFibonacciMul    = 11400714819323198485ULL;
    static constexpr std::size_t   kInitialCapacity = 16; // Power-of-2

private:
    enum class SlotState : std::uint8_t { Empty, Occupied, Deleted };
    struct Slot {
        key_type   key{};
        value_type val{};
        SlotState  state{SlotState::Empty};
    };
    static constexpr std::size_t kNpos = static_cast<std::size_t>(-1);

    using slot_alloc  = typename Alloc::template StdAllocatorAdapter<Slot>;
    using slot_vector = std::vector<Slot, slot_alloc>;

public:
    /// Die Bucket-Tabelle wird an das EIGENE allocator_ gebunden (der Adapter haelt &allocator_).
    /// SONDERFALL [[allocation-failure-exception]]: die Initial-Tabelle kann std::bad_alloc werfen.
    HashSearchAlgoCore()
        : buckets_(kInitialCapacity, Slot{}, allocator_.template as_std_allocator<Slot>()),
          mask_(kInitialCapacity - 1) {}

    /// KF-6-NAHT (Posten 62, LEDGER 04.08. abend-12): eine vor-parametrierte Strategie-Instanz
    /// uebernehmen, statt sie default zu konstruieren. Heute nirgends benutzt und bewusst `explicit`.
    explicit HashSearchAlgoCore(allocator_type a)
        : allocator_(std::move(a)), buckets_(kInitialCapacity, Slot{}, allocator_.template as_std_allocator<Slot>()),
          mask_(kInitialCapacity - 1) {}

    /// Copy: Strategie mitkopieren, die Tabelle an das EIGENE allocator_ binden, dann die transiente
    /// Kopier-Allokation aus der Statistik nehmen (Memento) -- 1:1 btree_node_pool_store.hpp:86.
    HashSearchAlgoCore(HashSearchAlgoCore const& o)
        : allocator_(o.allocator_), buckets_(o.buckets_, allocator_.template as_std_allocator<Slot>()), mask_(o.mask_),
          size_(o.size_), tombstones_(o.tombstones_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_ = o.stats_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }

    HashSearchAlgoCore& operator=(HashSearchAlgoCore const& o) {
        if (this != &o) {
            buckets_    = o.buckets_;
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

    ~HashSearchAlgoCore() = default;

    [[nodiscard]] bool operator==(HashSearchAlgoCore const& other) const noexcept { return size_ == other.size_; }

    /// SONDERFALL [[allocation-failure-exception]]: rehash kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        if ((size_ + tombstones_) * 10 >= (mask_ + 1) * 7) rehash((mask_ + 1) * 2);
        std::size_t const cap           = mask_ + 1;
        std::size_t const start         = hash_index(k);
        std::size_t       first_deleted = kNpos;
        for (std::size_t i = 0; i < cap; ++i) {
            std::size_t const pos = (start + i) & mask_;
            Slot&             s   = buckets_[pos];
            if (s.state == SlotState::Empty) {
                std::size_t const target = (first_deleted != kNpos) ? first_deleted : pos;
                if (first_deleted != kNpos) --tombstones_; // Tombstone wiederverwendet
                buckets_[target] = Slot{k, v, SlotState::Occupied};
                ++size_;
                notify_insert();
                return;
            }
            if (s.state == SlotState::Deleted) {
                if (first_deleted == kNpos) first_deleted = pos;
            } else if (s.key == k) { // Occupied + gleicher Key → Update
                s.val = v;
                notify_insert();
                return;
            }
        }
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        std::size_t const         cap    = mask_ + 1;
        std::size_t const         start  = hash_index(k);
        for (std::size_t i = 0; i < cap; ++i) {
            std::size_t const pos = (start + i) & mask_;
            Slot const&       s   = buckets_[pos];
            if (s.state == SlotState::Empty) break; // Kette zu Ende → Miss
            if (s.state == SlotState::Occupied && s.key == k) {
                result = s.val;
                break;
            }
            // Deleted oder Occupied(anderer Key) → weiter proben
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
        std::size_t const cap   = mask_ + 1;
        std::size_t const start = hash_index(k);
        for (std::size_t i = 0; i < cap; ++i) {
            std::size_t const pos = (start + i) & mask_;
            Slot&             s   = buckets_[pos];
            if (s.state == SlotState::Empty) return false;
            if (s.state == SlotState::Occupied && s.key == k) {
                s.state = SlotState::Deleted; // Tombstone — Probe-Kette bleibt intakt
                --size_;
                ++tombstones_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
                ++stats_.total_erase_count;
                observer_.notify(stats_);
#endif
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return size_; }
    [[nodiscard]] double    density_percent() const noexcept { return 100.0 * static_cast<double>(size_) / 65536.0; }
    void                    clear() noexcept {
        for (auto& s : buckets_) s = Slot{};
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
    [[nodiscard]] std::size_t hash_index(key_type k) const noexcept {
        return static_cast<std::size_t>((static_cast<std::uint64_t>(k) * kFibonacciMul)) & mask_;
    }

    void rehash(std::size_t new_capacity) {
        // Der Zwischenpuffer wird MIT DERSELBEN Strategie-Instanz konstruiert wie die Zieltabelle: beim
        // Wachsen haelt das Organ kurzzeitig zwei Tabellen, und beide gehoeren in DASSELBE Allokations-
        // Buch (Owner-KERN abend-11 -- eine Strategie-Instanz je Organ). Der swap tauscht nur Zeiger; die
        // Allokator-Objekte beider Vektoren zeigen ohnehin auf dasselbe allocator_.
        slot_vector old(allocator_.template as_std_allocator<Slot>());
        old.swap(buckets_);
        buckets_.assign(new_capacity, Slot{});
        mask_       = new_capacity - 1;
        size_       = 0;
        tombstones_ = 0;
        for (auto const& s : old) {
            if (s.state != SlotState::Occupied) continue; // Tombstones entfallen beim Rehash
            std::size_t const start = hash_index(s.key);
            for (std::size_t i = 0; i < new_capacity; ++i) {
                std::size_t const pos = (start + i) & mask_;
                if (buckets_[pos].state == SlotState::Empty) {
                    buckets_[pos] = Slot{s.key, s.val, SlotState::Occupied};
                    ++size_;
                    break;
                }
            }
        }
    }

    void notify_insert() noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (size_ > stats_.peak_occupancy) stats_.peak_occupancy = size_;
        observer_.notify(stats_);
#endif
    }

    // allocator_ MUSS VOR buckets_ stehen: der Adapter haelt &allocator_, und die Member-
    // Initialisierungs-/Zerstoerungsreihenfolge ist die Deklarationsreihenfolge.
    allocator_type allocator_{};
    slot_vector    buckets_;
    std::size_t    mask_;
    std::size_t    size_       = 0;
    std::size_t    tombstones_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace detail

/// DIE IDENTITAET -- das Registry-Organ S14. Nicht-Template, exakt der historische Typ-Name.
class HashSearchAlgo final
    : public detail::HashSearchAlgoCore<::comdare::cache_engine::alloc::ExgenAllocator, HashSearchAlgo> {
public:
    /// Die Default-Bindung der Identitaets-Ebene: der BENANNTE Achsen-Default, nie std::allocator.
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;

    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::HashSearchAlgo",
                                  "axes/lookup/axis_03a_search_algo_hash_search.hpp");

    /// Der Migrations-Ausweis (composable::AllocatorRebindableSearchAlgo).
    template <class A2>
    using rebind_allocator = HashSearchAlgoRebound<A2>;

    using detail::HashSearchAlgoCore<default_allocator_type, HashSearchAlgo>::HashSearchAlgoCore;
};

/// DIE GEBUNDENE FORM -- traegt BEWUSST KEIN COMDARE_DEFINE_ORGAN_LOCATION.
template <class A2>
class HashSearchAlgoRebound final : public detail::HashSearchAlgoCore<A2, HashSearchAlgoRebound<A2>> {
public:
    /// Der EBENEN-AUSWEIS (s. composable::IsReboundSearchAlgoLeaf).
    using axis03a_rebound_tag = void;

    using detail::HashSearchAlgoCore<A2, HashSearchAlgoRebound<A2>>::HashSearchAlgoCore;
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<HashSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<HashSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<HashSearchAlgo>);

// ---------------------------------------------------------------------------------------------
// Der Zwei-Ebenen-Vertrag, self-proving an der Datei, die ihn eingeht (Pilot-Rezept, linear_scan:352).
// ---------------------------------------------------------------------------------------------
static_assert(std::is_same_v<composable::search_algo_for_composition_t<HashSearchAlgo,
                                                                       ::comdare::cache_engine::alloc::ExgenAllocator>,
                             HashSearchAlgo>,
              "01c Level-0-IDENTITAET verletzt: die Kompositions-Naht liefert am ACHSEN-DEFAULT nicht mehr die "
              "Fassade selbst. Damit laege ein anderer Typ auf dem golden-Pfad -- Typ-Neutralitaet weg.");
static_assert(composable::AllocatorRebindableSearchAlgo<HashSearchAlgo, ::comdare::cache_engine::alloc::ExgenAllocator>,
              "01c: HashSearchAlgo traegt keinen rebind_allocator mehr -- nicht migriert.");
static_assert(!composable::IsReboundSearchAlgoLeaf<HashSearchAlgo>,
              "01c EBENEN-VERMISCHUNG: die Fassade traegt den Rebound-Tag -- Emitter-type_name-Reise und "
              "Registry-Reflektion zeigten dann auf die Substanz-Ebene.");
static_assert(
    composable::IsReboundSearchAlgoLeaf<HashSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>,
    "01c: der Rebound-Leaf traegt seinen Ausweis nicht -- der Identitaets-Pin kann nicht mehr greifen.");
static_assert(composable::search_algo_name_is_allocator_invariant_v<HashSearchAlgo,
                                                                    ::comdare::cache_engine::alloc::ExgenAllocator>,
              "01c name()-INVARIANZ (Level 0) verletzt.");
static_assert(HashSearchAlgo::name() == HashSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>::name(),
              "01c name()-INVARIANZ verletzt: der Rebound-Leaf traegt einen anderen Organ-Namen als die Fassade -- "
              "die T6-Wahl leckte in den serialize-/binary_id-Schluessel.");
static_assert(concepts::SearchAlgoVariant<HashSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<
              HashSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(
    concepts::DensityClassifiedStrategy<HashSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
} // namespace comdare::cache_engine::lookup
