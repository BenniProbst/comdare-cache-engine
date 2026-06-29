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

#include <cstddef>
#include <cstdint>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

/// Open-Addressing-Bucket-Pool: Slots behalten ihre Position (KEINE Index-Shifts); Tombstones erhalten die
/// Probe-Kette. Kapazitaet stets Power-of-2 (mask_ = cap-1), Start 16, Verdopplung bei Load >= 0.7.
class HashBucketPoolStore {
public:
    using key_type                                = std::uint64_t;
    using value_type                              = std::uint64_t;
    static constexpr std::size_t kInitialCapacity = 16; // Power-of-2

    HashBucketPoolStore() : buckets_(kInitialCapacity), mask_(kInitialCapacity - 1) {}

    [[nodiscard]] std::size_t bucket_count() const noexcept { return mask_ + 1; }
    [[nodiscard]] bool slot_is_empty(std::size_t i) const noexcept { return buckets_[i].state == SlotState::Empty; }
    [[nodiscard]] bool slot_is_occupied(std::size_t i) const noexcept {
        return buckets_[i].state == SlotState::Occupied;
    }
    [[nodiscard]] bool slot_is_deleted(std::size_t i) const noexcept { return buckets_[i].state == SlotState::Deleted; }
    [[nodiscard]] key_type    slot_key(std::size_t i) const noexcept { return buckets_[i].key; }
    [[nodiscard]] value_type  slot_value(std::size_t i) const noexcept { return buckets_[i].val; }
    [[nodiscard]] std::size_t occupied() const noexcept { return size_; }
    [[nodiscard]] std::size_t tombstones() const noexcept { return tombstones_; }

    /// Belegt Slot i. Wiederverwendung eines Tombstones (Deleted->Occupied) wird automatisch verbucht.
    void place_occupied(std::size_t i, key_type k, value_type v) noexcept {
        if (buckets_[i].state == SlotState::Deleted) --tombstones_;
        buckets_[i] = Slot{k, v, SlotState::Occupied};
        ++size_;
    }
    void set_slot_value(std::size_t i, value_type v) noexcept { buckets_[i].val = v; }
    void mark_deleted(std::size_t i) noexcept {
        buckets_[i].state = SlotState::Deleted; // Tombstone — Probe-Kette bleibt intakt
        --size_;
        ++tombstones_;
    }

    /// SONDERFALL [[allocation-failure-exception]]: rehash kann std::bad_alloc werfen.
    void rehash(std::size_t new_capacity) {
        std::vector<Slot> old;
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

    void clear() noexcept {
        for (auto& s : buckets_) s = Slot{};
        size_       = 0;
        tombstones_ = 0;
    }

private:
    enum class SlotState : std::uint8_t { Empty, Occupied, Deleted };
    struct Slot {
        key_type   key{};
        value_type val{};
        SlotState  state{SlotState::Empty};
    };
    static constexpr std::uint64_t kFibonacciMul = 11400714819323198485ULL;

    [[nodiscard]] std::size_t hash_index(key_type k) const noexcept {
        return static_cast<std::size_t>(k * kFibonacciMul) & mask_;
    }

    std::vector<Slot> buckets_;
    std::size_t       mask_;
    std::size_t       size_       = 0;
    std::size_t       tombstones_ = 0;
};

// Selbstbeweis: das Substrat erfuellt das HashBucketPool-Concept.
static_assert(HashBucketPool<HashBucketPoolStore>);

} // namespace comdare::cache_engine::lookup::composable
