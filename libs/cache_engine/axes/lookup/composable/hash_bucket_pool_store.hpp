#pragma once
// V41 Umstufung-A (Task #41) — HashBucketPoolStore: Open-Addressing-Slot-Substrat (erfuellt HashBucketPool).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Such-Logik — 1:1-Port des Slot-Arrays aus axis_03a_search_algo_hash_search.hpp
// (Slot{key,val,state}; std::vector<Slot> buckets_; mask_; size_; tombstones_; kInitialCapacity=16;
// rehash() verbatim Z.175-194), aber generisch ueber uint64-Key und mit getrennter Verantwortung: die
// Hash-/Probe-Navigation lebt im HashProbeTraversalOrgan, NICHT hier (genetisches Experiment, Doku 14 §1.2).
//
// place_occupied() ist selbst-buchend (Tombstone->Occupied dekrementiert tombstones_, ++size_), sodass das
// Organ nur die Probe-Position waehlt; mark_deleted() setzt Tombstone (Probe-Kette intakt); rehash() entfernt
// Tombstones beim Resize. hash_index() bleibt im Store (kennt mask_+kFibonacciMul) fuer die Re-Distribution.

#include "hash_bucket_pool_concept.hpp"
#include <topics/nodes/axis_hash_probe_shape/axis_hash_probe_shape_oa_lf70.hpp>
#include <topics/nodes/axis_hash_probe_shape/concepts/axis_hash_probe_shape_concept.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

/// Open-Addressing-Bucket-Pool: Slots behalten ihre Position (KEINE Index-Shifts); Tombstones erhalten die
/// Probe-Kette. Kapazitaet stets Power-of-2 (mask_ = cap-1), Start 16, Verdopplung bei Shape-Load-Grenze.
template <typename Shape = ::comdare::cache_engine::nodes::axis_hash_probe_shape::HashOaLf70>
class HashBucketPoolStore {
    static_assert(::comdare::cache_engine::nodes::axis_hash_probe_shape::concepts::HashProbeShape<Shape>);

public:
    using key_type                                = std::uint64_t;
    using value_type                              = std::uint64_t;
    using mapped_type                             = value_type;
    static constexpr std::size_t kInitialCapacity = 16; // Power-of-2
    static constexpr bool        kOpenAddressing  = Shape::kOpenAddressing;
    static constexpr int         kLoadNumerator   = Shape::kLoadNumerator;
    static constexpr int         kLoadDenominator = Shape::kLoadDenominator;

    // #234-F4: reale Shape-Auswahl des Substrats — OA-Slots oder Chaining-Heads+Nodes.
    enum class SlotState : std::uint8_t { Empty, Occupied, Deleted };
    struct Slot {
        key_type   key{};
        value_type val{};
        SlotState  state{SlotState::Empty};
    };
    static constexpr std::size_t kNil = std::numeric_limits<std::size_t>::max();
    struct ChainSlot {
        key_type    key{};
        mapped_type value{};
        SlotState   state{SlotState::Empty};
        std::size_t next{kNil};
    };
    struct OaData {
        std::vector<Slot> buckets;
    };
    struct ChainData {
        std::vector<std::size_t> heads;
        std::vector<ChainSlot>   nodes;
        std::vector<std::size_t> free;
    };
    using storage_t = std::conditional_t<kOpenAddressing, OaData, ChainData>;

    HashBucketPoolStore() : st_(make_initial_storage()), mask_(kInitialCapacity - 1) {}

    [[nodiscard]] std::size_t bucket_count() const noexcept {
        if constexpr (kOpenAddressing) {
            return mask_ + 1;
        } else {
            return st_.heads.size();
        }
    }
    [[nodiscard]] bool slot_is_empty(std::size_t i) const noexcept {
        if constexpr (kOpenAddressing) {
            return st_.buckets[i].state == SlotState::Empty;
        } else {
            return i >= st_.nodes.size() || st_.nodes[i].state != SlotState::Occupied;
        }
    }
    [[nodiscard]] bool slot_is_occupied(std::size_t i) const noexcept {
        if constexpr (kOpenAddressing) {
            return st_.buckets[i].state == SlotState::Occupied;
        } else {
            return i < st_.nodes.size() && st_.nodes[i].state == SlotState::Occupied;
        }
    }
    [[nodiscard]] bool slot_is_deleted(std::size_t i) const noexcept {
        if constexpr (kOpenAddressing) {
            return st_.buckets[i].state == SlotState::Deleted;
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
        requires (kOpenAddressing)
    {
        if (st_.buckets[i].state == SlotState::Deleted) --tombstones_;
        st_.buckets[i] = Slot{k, v, SlotState::Occupied};
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
        requires (kOpenAddressing)
    {
        st_.buckets[i].state = SlotState::Deleted; // Tombstone — Probe-Kette bleibt intakt
        --size_;
        ++tombstones_;
    }

    [[nodiscard]] std::size_t chain_head(std::size_t bucket) const noexcept
        requires (!kOpenAddressing)
    {
        return st_.heads[bucket];
    }
    [[nodiscard]] std::size_t node_next(std::size_t node) const noexcept
        requires (!kOpenAddressing)
    {
        return st_.nodes[node].next;
    }
    [[nodiscard]] std::size_t node_slot_count() const noexcept
        requires (!kOpenAddressing)
    {
        return st_.nodes.size();
    }
    void allocate_chained(std::size_t bucket, key_type k, value_type v)
        requires (!kOpenAddressing)
    {
        std::size_t node = kNil;
        if (!st_.free.empty()) {
            node = st_.free.back();
            st_.free.pop_back();
            st_.nodes[node] = ChainSlot{k, v, SlotState::Occupied, st_.heads[bucket]};
        } else {
            node = st_.nodes.size();
            st_.nodes.push_back(ChainSlot{k, v, SlotState::Occupied, st_.heads[bucket]});
        }
        st_.heads[bucket] = node;
        ++size_;
    }
    void unlink_erase(std::size_t bucket, std::size_t node, std::size_t prev)
        requires (!kOpenAddressing)
    {
        std::size_t const next = st_.nodes[node].next;
        if (prev == kNil) {
            st_.heads[bucket] = next;
        } else {
            st_.nodes[prev].next = next;
        }
        st_.nodes[node].state = SlotState::Empty;
        st_.nodes[node].next  = kNil;
        st_.free.push_back(node);
        --size_;
    }

    /// SONDERFALL [[allocation-failure-exception]]: rehash kann std::bad_alloc werfen.
    void rehash(std::size_t new_capacity) {
        if constexpr (kOpenAddressing) {
            std::vector<Slot> old;
            old.swap(st_.buckets);
            st_.buckets.assign(new_capacity, Slot{});
            mask_       = new_capacity - 1;
            size_       = 0;
            tombstones_ = 0;
            for (auto const& s : old) {
                if (s.state != SlotState::Occupied) continue; // Tombstones entfallen beim Rehash
                std::size_t const start = hash_index(s.key);
                for (std::size_t i = 0; i < new_capacity; ++i) {
                    std::size_t const pos = (start + i) & mask_;
                    if (st_.buckets[pos].state == SlotState::Empty) {
                        st_.buckets[pos] = Slot{s.key, s.val, SlotState::Occupied};
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
                if (st_.nodes[node].state != SlotState::Occupied) continue;
                std::size_t const bucket = hash_index(st_.nodes[node].key) & new_mask;
                st_.nodes[node].next     = st_.heads[bucket];
                st_.heads[bucket]        = node;
            }
        }
    }

    void clear() noexcept {
        if constexpr (kOpenAddressing) {
            for (auto& s : st_.buckets) s = Slot{};
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

private:
    static constexpr std::uint64_t kFibonacciMul = 11400714819323198485ULL;

    [[nodiscard]] static storage_t make_initial_storage() {
        if constexpr (kOpenAddressing) {
            return OaData{std::vector<Slot>(kInitialCapacity)};
        } else {
            return ChainData{std::vector<std::size_t>(kInitialCapacity, kNil), {}, {}};
        }
    }

    [[nodiscard]] std::size_t hash_index(key_type k) const noexcept {
        return static_cast<std::size_t>(k * kFibonacciMul) & mask_;
    }

    storage_t   st_;
    std::size_t mask_;
    std::size_t size_       = 0;
    std::size_t tombstones_ = 0;
};

// Selbstbeweis: das Substrat erfuellt das HashBucketPool-Concept.
static_assert(HashBucketPool<HashBucketPoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
