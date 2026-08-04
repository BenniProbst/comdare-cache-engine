#pragma once
// V41 Umstufung-A (Task #41) — HashBucketPoolStore: Open-Addressing-Slot-Substrat (erfuellt HashBucketPool).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Such-Logik — 1:1-Port des Slot-Arrays aus axis_03a_search_algo_hash_search.hpp
// (Slot{key,val,state}; ein Slot-Vektor buckets_ -- dort noch am DEFAULT-Allokator, hier an der Achse,
// s. A8-S5-Absatz unten; mask_; size_; tombstones_; kInitialCapacity=16;
// rehash() verbatim Z.175-194), aber generisch ueber uint64-Key und mit getrennter Verantwortung: die
// Hash-/Probe-Navigation lebt im HashProbeTraversalOrgan, NICHT hier (genetisches Experiment, Doku 14 §1.2).
//
// place_occupied() ist selbst-buchend (Tombstone->Occupied dekrementiert tombstones_, ++size_), sodass das
// Organ nur die Probe-Position waehlt; mark_deleted() setzt Tombstone (Probe-Kette intakt); rehash() entfernt
// Tombstones beim Resize. hash_index() bleibt im Store (kennt mask_+kFibonacciMul) fuer die Re-Distribution.
//
// A8-S5 Familie 01a (2026-08-04): der Slot-/Ketten-Speicher kommt REAL aus der Allocator-Achse (axis_06),
// Muster BTreeNodePoolStore. Der Store trug den Allokator-Template-Kopf schon vorher -- geflippt wurde die
// BINDUNG: Default und Rebind gehen jetzt ueber den StdAllocatorAdapter der Achse statt ueber std::allocator.
// Beide Shape-Zweige (OA-Slots und Chaining-Heads/Nodes/Free) haengen an DEMSELBEN allocator_; COW-Kopie
// rebindet und verwirft die Kopier-Pollution per restore_statistics-Memento (Move -> degradiert zu Copy).

#include "hash_bucket_pool_concept.hpp"
#include <axes/alloc/axis_06_allocator_exgen.hpp>            // axis_06-Default-Strategie + StdAllocatorAdapter
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp> // AllocatorStrategy-Concept (compile-time-strikt)
#include <topics/nodes/axis_hash_probe_shape/axis_hash_probe_shape_oa_lf70.hpp>
#include <topics/nodes/axis_hash_probe_shape/concepts/axis_hash_probe_shape_concept.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

namespace detail {

enum class HashSlotState : std::uint8_t { Empty, Occupied, Deleted };

struct HashOaSlot {
    std::uint64_t key{};
    std::uint64_t val{};
    HashSlotState state{HashSlotState::Empty};
};

struct HashChainSlot {
    std::uint64_t key{};
    std::uint64_t value{};
    HashSlotState state{HashSlotState::Empty};
    std::size_t   next{std::numeric_limits<std::size_t>::max()};
};

} // namespace detail

/// Open-Addressing-Bucket-Pool: Slots behalten ihre Position (KEINE Index-Shifts); Tombstones erhalten die
/// Probe-Kette. Kapazitaet stets Power-of-2 (mask_ = cap-1), Start 16, Verdopplung bei Shape-Load-Grenze.
template <typename Shape = ::comdare::cache_engine::nodes::axis_hash_probe_shape::HashOaLf70,
          class Alloc    = ::comdare::cache_engine::alloc::ExgenAllocator>
    requires ::comdare::cache_engine::alloc::concepts::AllocatorStrategy<Alloc>
class HashBucketPoolStore {
    static_assert(::comdare::cache_engine::nodes::axis_hash_probe_shape::concepts::HashProbeShape<Shape>);

public:
    using key_type            = std::uint64_t;
    using value_type          = std::uint64_t;
    using mapped_type         = value_type;
    using node_type           = std::conditional_t<Shape::kOpenAddressing, detail::HashOaSlot, detail::HashChainSlot>;
    using allocator_type      = Alloc;
    using slot_allocator_type = typename Alloc::template StdAllocatorAdapter<detail::HashOaSlot>;
    using chain_slot_allocator_type               = typename Alloc::template StdAllocatorAdapter<detail::HashChainSlot>;
    using index_allocator_type                    = typename Alloc::template StdAllocatorAdapter<std::size_t>;
    static constexpr std::size_t kInitialCapacity = 16; // Power-of-2
    static constexpr bool        kOpenAddressing  = Shape::kOpenAddressing;
    static constexpr int         kLoadNumerator   = Shape::kLoadNumerator;
    static constexpr int         kLoadDenominator = Shape::kLoadDenominator;

private:
    // #234-F4: reale Shape-Auswahl des Substrats — OA-Slots oder Chaining-Heads+Nodes.
    struct OaData {
        std::vector<detail::HashOaSlot, slot_allocator_type> buckets;
    };
    struct ChainData {
        std::vector<std::size_t, index_allocator_type>                heads;
        std::vector<detail::HashChainSlot, chain_slot_allocator_type> nodes;
        std::vector<std::size_t, index_allocator_type>                free;
    };
    using storage_t = std::conditional_t<kOpenAddressing, OaData, ChainData>;

public:
    static constexpr std::size_t kNil = std::numeric_limits<std::size_t>::max();

    HashBucketPoolStore() : st_(make_initial_storage()), mask_(kInitialCapacity - 1) {}
    // COW-Pflicht (Memento): allocator_ mitkopieren, die Vektoren an DAS EIGENE allocator_ rebinden
    // (copy_storage_from_ statt Aggregat-Kopie -- der Adapter haelt &allocator_) und die durch die
    // Vollkopie entstandene transiente Re-Allokations-Pollution per restore_statistics verwerfen.
    // Move NICHT deklariert -> degradiert sicher zu Copy (kein dangling &allocator_).
    HashBucketPoolStore(HashBucketPoolStore const& o)
        : allocator_(o.allocator_), st_(copy_storage_from_(o.st_)), mask_(o.mask_), size_(o.size_),
          tombstones_(o.tombstones_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    HashBucketPoolStore& operator=(HashBucketPoolStore const& o) {
        if (this != &o) {
            st_         = copy_storage_from_(o.st_);
            mask_       = o.mask_;
            size_       = o.size_;
            tombstones_ = o.tombstones_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~HashBucketPoolStore() = default;

    [[nodiscard]] std::size_t bucket_count() const noexcept {
        if constexpr (kOpenAddressing) {
            return mask_ + 1;
        } else {
            return st_.heads.size();
        }
    }
    [[nodiscard]] bool slot_is_empty(std::size_t i) const noexcept {
        if constexpr (kOpenAddressing) {
            return st_.buckets[i].state == detail::HashSlotState::Empty;
        } else {
            return i >= st_.nodes.size() || st_.nodes[i].state != detail::HashSlotState::Occupied;
        }
    }
    [[nodiscard]] bool slot_is_occupied(std::size_t i) const noexcept {
        if constexpr (kOpenAddressing) {
            return st_.buckets[i].state == detail::HashSlotState::Occupied;
        } else {
            return i < st_.nodes.size() && st_.nodes[i].state == detail::HashSlotState::Occupied;
        }
    }
    [[nodiscard]] bool slot_is_deleted(std::size_t i) const noexcept {
        if constexpr (kOpenAddressing) {
            return st_.buckets[i].state == detail::HashSlotState::Deleted;
        } else {
            (void)i;
            return false;
        }
    }
    [[nodiscard]] key_type slot_key(std::size_t i) const noexcept {
        if constexpr (kOpenAddressing) {
            return st_.buckets[i].key;
        } else {
            return st_.nodes[i].key;
        }
    }
    [[nodiscard]] value_type slot_value(std::size_t i) const noexcept {
        if constexpr (kOpenAddressing) {
            return st_.buckets[i].val;
        } else {
            return st_.nodes[i].value;
        }
    }
    [[nodiscard]] std::size_t occupied() const noexcept { return size_; }
    [[nodiscard]] std::size_t tombstones() const noexcept {
        if constexpr (kOpenAddressing) {
            return tombstones_;
        } else {
            return 0;
        }
    }

    /// Belegt Slot i. Wiederverwendung eines Tombstones (Deleted->Occupied) wird automatisch verbucht.
    void place_occupied(std::size_t i, key_type k, value_type v) noexcept
        requires(kOpenAddressing)
    {
        if (st_.buckets[i].state == detail::HashSlotState::Deleted) --tombstones_;
        st_.buckets[i] = detail::HashOaSlot{k, v, detail::HashSlotState::Occupied};
        ++size_;
    }
    void set_slot_value(std::size_t i, value_type v) noexcept {
        if constexpr (kOpenAddressing) {
            st_.buckets[i].val = v;
        } else {
            st_.nodes[i].value = v;
        }
    }
    void mark_deleted(std::size_t i) noexcept
        requires(kOpenAddressing)
    {
        st_.buckets[i].state = detail::HashSlotState::Deleted; // Tombstone — Probe-Kette bleibt intakt
        --size_;
        ++tombstones_;
    }

    [[nodiscard]] std::size_t chain_head(std::size_t bucket) const noexcept
        requires(!kOpenAddressing)
    {
        return st_.heads[bucket];
    }
    [[nodiscard]] std::size_t node_next(std::size_t node) const noexcept
        requires(!kOpenAddressing)
    {
        return st_.nodes[node].next;
    }
    [[nodiscard]] std::size_t node_slot_count() const noexcept
        requires(!kOpenAddressing)
    {
        return st_.nodes.size();
    }
    void allocate_chained(std::size_t bucket, key_type k, value_type v)
        requires(!kOpenAddressing)
    {
        std::size_t node = kNil;
        if (!st_.free.empty()) {
            node = st_.free.back();
            st_.free.pop_back();
            st_.nodes[node] = detail::HashChainSlot{k, v, detail::HashSlotState::Occupied, st_.heads[bucket]};
        } else {
            node = st_.nodes.size();
            st_.nodes.push_back(detail::HashChainSlot{k, v, detail::HashSlotState::Occupied, st_.heads[bucket]});
        }
        st_.heads[bucket] = node;
        ++size_;
    }
    void unlink_erase(std::size_t bucket, std::size_t node, std::size_t prev)
        requires(!kOpenAddressing)
    {
        std::size_t const next = st_.nodes[node].next;
        if (prev == kNil) {
            st_.heads[bucket] = next;
        } else {
            st_.nodes[prev].next = next;
        }
        st_.nodes[node].state = detail::HashSlotState::Empty;
        st_.nodes[node].next  = kNil;
        st_.free.push_back(node);
        --size_;
    }

    /// SONDERFALL [[allocation-failure-exception]]: rehash kann std::bad_alloc werfen.
    /// KAUSALITAET PRAEZISIERT (Posten 70, 2026-08-04): der Wurf kommt seit dem A8-S5-Schnitt NICHT mehr
    /// vom Default-Allokator, sondern vom StdAllocatorAdapter der Allokator-ACHSE. Die Strategie meldet
    /// OOM per nullptr; der Adapter uebersetzt das an EINER Stelle in std::bad_alloc
    /// (Posten 64, axis_06_allocator_strategy_base.hpp, StdAllocatorAdapter::allocate). Die Aussage
    /// dieser Zeile ist damit wieder wahr; die Fehlerklasse bleibt der FK-5-Boden der Allokator-Achse.
    void rehash(std::size_t new_capacity) {
        if constexpr (kOpenAddressing) {
            // Der Zwischenpuffer haengt ebenfalls an der Achse (Adapter ist nicht default-konstruierbar) --
            // sonst waere ausgerechnet die groesste Umschaufel-Allokation des Rehash an ihr vorbeigelaufen.
            std::vector<detail::HashOaSlot, slot_allocator_type> old(
                allocator_.template as_std_allocator<detail::HashOaSlot>());
            old.swap(st_.buckets);
            st_.buckets.assign(new_capacity, detail::HashOaSlot{});
            mask_       = new_capacity - 1;
            size_       = 0;
            tombstones_ = 0;
            for (auto const& s : old) {
                if (s.state != detail::HashSlotState::Occupied) continue; // Tombstones entfallen beim Rehash
                std::size_t const start = hash_index(s.key);
                for (std::size_t i = 0; i < new_capacity; ++i) {
                    std::size_t const pos = (start + i) & mask_;
                    if (st_.buckets[pos].state == detail::HashSlotState::Empty) {
                        st_.buckets[pos] = detail::HashOaSlot{s.key, s.val, detail::HashSlotState::Occupied};
                        ++size_;
                        break;
                    }
                }
            }
        } else {
            std::size_t const new_mask = new_capacity - 1;
            st_.heads.assign(new_capacity, kNil);
            mask_ = new_mask;
            for (std::size_t node = 0; node < st_.nodes.size(); ++node) {
                if (st_.nodes[node].state != detail::HashSlotState::Occupied) continue;
                std::size_t const bucket = hash_index(st_.nodes[node].key) & new_mask;
                st_.nodes[node].next     = st_.heads[bucket];
                st_.heads[bucket]        = node;
            }
        }
    }

    void clear() noexcept {
        if constexpr (kOpenAddressing) {
            for (auto& s : st_.buckets) s = detail::HashOaSlot{};
            size_       = 0;
            tombstones_ = 0;
        } else {
            for (auto& h : st_.heads) h = kNil;
            st_.nodes.clear();
            st_.free.clear();
            size_       = 0;
            tombstones_ = 0;
        }
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using allocator_snapshot_t = typename Alloc::snapshot_t;
    /// T6-Route (A8-S5): die ECHTE Allocator-Achsen-Statistik (rich AllocationStatistics, 5 Felder) statt der
    /// frueheren Store-eigenen Capacity-Delta-Schaetzung. Die alten Zaehler waren allocator-UNABHAENGIG und
    /// haetten neben der Achsen-Statistik eine zweite, driftende Wahrheit gefuehrt; live_nodes speist der
    /// ABI-Adapter im Rich-Zweig aus occupied_count() des Organs (abi_adapter.hpp, T6-Route).
    [[nodiscard]] allocator_snapshot_t store_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    static constexpr std::uint64_t kFibonacciMul = 11400714819323198485ULL;

    /// Erst-Belegung des Shape-Zweigs -- JEDER Vektor wird explizit an das eigene allocator_ gebunden
    /// (kein `{}`-Default: der Achsen-Adapter ist bestimmungsgemaess nicht default-konstruierbar).
    [[nodiscard]] storage_t make_initial_storage() {
        if constexpr (kOpenAddressing) {
            return OaData{std::vector<detail::HashOaSlot, slot_allocator_type>(
                kInitialCapacity, detail::HashOaSlot{}, allocator_.template as_std_allocator<detail::HashOaSlot>())};
        } else {
            return ChainData{
                std::vector<std::size_t, index_allocator_type>(kInitialCapacity, kNil,
                                                               allocator_.template as_std_allocator<std::size_t>()),
                std::vector<detail::HashChainSlot, chain_slot_allocator_type>(
                    allocator_.template as_std_allocator<detail::HashChainSlot>()),
                std::vector<std::size_t, index_allocator_type>(allocator_.template as_std_allocator<std::size_t>())};
        }
    }

    /// COW-Uebernahme: Inhalt aus der Quelle, Bindung an das EIGENE allocator_ (Vektor-Copy-Ctor mit
    /// explizitem Allokator -- sonst uebernaehme die Kopie den Adapter der Quelle und allozierte fremd).
    [[nodiscard]] storage_t copy_storage_from_(storage_t const& src) {
        if constexpr (kOpenAddressing) {
            return OaData{std::vector<detail::HashOaSlot, slot_allocator_type>(
                src.buckets, allocator_.template as_std_allocator<detail::HashOaSlot>())};
        } else {
            return ChainData{std::vector<std::size_t, index_allocator_type>(
                                 src.heads, allocator_.template as_std_allocator<std::size_t>()),
                             std::vector<detail::HashChainSlot, chain_slot_allocator_type>(
                                 src.nodes, allocator_.template as_std_allocator<detail::HashChainSlot>()),
                             std::vector<std::size_t, index_allocator_type>(
                                 src.free, allocator_.template as_std_allocator<std::size_t>())};
        }
    }

    [[nodiscard]] std::size_t hash_index(key_type k) const noexcept {
        return static_cast<std::size_t>(k * kFibonacciMul) & mask_;
    }

    // allocator_ VOR dem Storage (der Adapter haelt &allocator_ -- Init-/Destruktions-Reihenfolge).
    Alloc       allocator_{};
    storage_t   st_;
    std::size_t mask_;
    std::size_t size_       = 0;
    std::size_t tombstones_ = 0;
};

// Selbstbeweis: das Substrat erfuellt das HashBucketPool-Concept.
static_assert(HashBucketPool<HashBucketPoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
