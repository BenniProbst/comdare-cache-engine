#pragma once
// AP-7b/#262 -- SwissGroupPoolStore: faithful SwissTable slot/control-byte substrate.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Faithful port of the AP-7a SwissTable wrapper substrate from axis_03a_search_algo_swisstable.hpp:
// slots_ + ctrl_ split storage, mask_, size_, tombstones_, 7/8 rehash threshold, and tombstone reuse.
// This mirrors the HashBucketPoolStore responsibility split: the store manages slots/control bytes and
// rehash; the scalar H1/H2 group-probe search logic lives in SwissGroupProbeTraversalOrgan.

#include "swiss_group_pool_concept.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

namespace detail {

struct SwissSlot {
    std::uint64_t key{};
    std::uint64_t val{};
};

} // namespace detail

template <class A = std::allocator<detail::SwissSlot>>
class SwissGroupPoolStore {
public:
    using key_type            = std::uint64_t;
    using value_type          = std::uint64_t;
    using node_type           = detail::SwissSlot;
    using Slot                = detail::SwissSlot;
    using allocator_type      = A;
    using slot_allocator_type = typename std::allocator_traits<A>::template rebind_alloc<Slot>;
    using ctrl_allocator_type = typename std::allocator_traits<A>::template rebind_alloc<std::uint8_t>;

    static constexpr std::uint64_t kFibonacciMul    = 11400714819323198485ULL;
    static constexpr std::size_t   kGroupWidth      = 16;
    static constexpr std::size_t   kInitialCapacity = 16;
    static constexpr std::uint8_t  kEmpty           = 0x80u;
    static constexpr std::uint8_t  kDeleted         = 0xFEu;

    SwissGroupPoolStore() : slots_(kInitialCapacity), ctrl_(kInitialCapacity, kEmpty), mask_(kInitialCapacity - 1) {
        record_capacity_growth_(0, slots_.capacity(), sizeof(Slot));
        record_capacity_growth_(0, ctrl_.capacity(), sizeof(std::uint8_t));
    }

    [[nodiscard]] std::size_t slot_count() const noexcept { return mask_ + 1u; }
    [[nodiscard]] std::size_t group_count() const noexcept { return slot_count() / kGroupWidth; }
    [[nodiscard]] std::size_t occupied() const noexcept { return size_; }
    [[nodiscard]] std::size_t tombstones() const noexcept { return tombstones_; }
    [[nodiscard]] bool        needs_rehash_for_insert() const noexcept {
        return (size_ + tombstones_ + 1u) * 8u >= slot_count() * 7u;
    }

    [[nodiscard]] std::uint8_t        control_byte(std::size_t i) const noexcept { return ctrl_[i]; }
    [[nodiscard]] std::uint8_t const* control_group_ptr(std::size_t group_start) const noexcept {
        return ctrl_.data() + group_start;
    }
    [[nodiscard]] bool slot_is_empty(std::size_t i) const noexcept { return ctrl_[i] == kEmpty; }
    [[nodiscard]] bool slot_is_deleted(std::size_t i) const noexcept { return ctrl_[i] == kDeleted; }
    [[nodiscard]] bool slot_is_occupied(std::size_t i) const noexcept {
        return ctrl_[i] != kEmpty && ctrl_[i] != kDeleted;
    }
    [[nodiscard]] key_type   slot_key(std::size_t i) const noexcept { return slots_[i].key; }
    [[nodiscard]] value_type slot_value(std::size_t i) const noexcept { return slots_[i].val; }

    void place_occupied(std::size_t i, key_type k, value_type v, std::uint8_t h2) noexcept {
        if (ctrl_[i] == kDeleted) --tombstones_;
        ctrl_[i]  = h2;
        slots_[i] = Slot{k, v};
        ++size_;
    }

    void set_slot_value(std::size_t i, value_type v) noexcept { slots_[i].val = v; }

    void mark_deleted(std::size_t i) noexcept {
        ctrl_[i]  = kDeleted;
        slots_[i] = Slot{};
        --size_;
        ++tombstones_;
    }

    void rehash(std::size_t new_capacity) {
        std::vector<Slot, slot_allocator_type>         old_slots;
        std::vector<std::uint8_t, ctrl_allocator_type> old_ctrl;
        old_slots.swap(slots_);
        old_ctrl.swap(ctrl_);
        std::size_t const old_slots_capacity = slots_.capacity();
        std::size_t const old_ctrl_capacity  = ctrl_.capacity();
        slots_.assign(new_capacity, Slot{});
        record_capacity_growth_(old_slots_capacity, slots_.capacity(), sizeof(Slot));
        ctrl_.assign(new_capacity, kEmpty);
        record_capacity_growth_(old_ctrl_capacity, ctrl_.capacity(), sizeof(std::uint8_t));
        mask_       = new_capacity - 1u;
        size_       = 0;
        tombstones_ = 0;
        for (std::size_t i = 0; i < old_ctrl.size(); ++i) {
            if (old_ctrl[i] == kEmpty || old_ctrl[i] == kDeleted) continue;
            insert_rehashed(old_slots[i].key, old_slots[i].val);
        }
    }

    void clear() noexcept {
        for (auto& s : slots_) s = Slot{};
        std::fill(ctrl_.begin(), ctrl_.end(), kEmpty);
        size_       = 0;
        tombstones_ = 0;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    struct allocator_statistics_snapshot {
        std::uint64_t alloc_calls     = 0;
        std::uint64_t bytes_allocated = 0;
        std::uint64_t live_nodes      = 0;
    };

    [[nodiscard]] allocator_statistics_snapshot store_allocator_statistics() const noexcept {
        return allocator_statistics_snapshot{
            alloc_calls_,
            bytes_allocated_,
            static_cast<std::uint64_t>(size_),
        };
    }

    template <class IsaOrgan>
    std::uint64_t organ_observe_isa(IsaOrgan& org) const
        requires requires(IsaOrgan& o, unsigned char const* b, std::size_t words) {
            o.observe_simd_field_sum(b, words);
        }
    {
        std::uint64_t acc = 0;
        if (!slots_.empty()) {
            std::size_t const words = (slots_.size() * sizeof(Slot)) / sizeof(std::uint32_t);
            acc += org.observe_simd_field_sum(reinterpret_cast<unsigned char const*>(slots_.data()), words);
        }
        if (!ctrl_.empty()) {
            std::size_t const words = ctrl_.size() / sizeof(std::uint32_t);
            acc += org.observe_simd_field_sum(reinterpret_cast<unsigned char const*>(ctrl_.data()), words);
        }
        return acc;
    }
#endif

private:
    static constexpr std::size_t kNpos = static_cast<std::size_t>(-1);

    [[nodiscard]] static constexpr std::uint64_t mixed_hash(key_type k) noexcept {
        return static_cast<std::uint64_t>(k) * kFibonacciMul;
    }

    [[nodiscard]] static constexpr std::uint8_t h2_fingerprint(std::uint64_t hash) noexcept {
        return static_cast<std::uint8_t>(hash & 0x7Fu);
    }

    [[nodiscard]] std::size_t group_start_for(std::uint64_t hash, std::size_t probe) const noexcept {
        std::size_t const group_mask = group_count() - 1u;
        std::size_t const h1_group   = static_cast<std::size_t>(hash >> 7u) & group_mask;
        return ((h1_group + probe) & group_mask) * kGroupWidth;
    }

    void insert_rehashed(key_type k, value_type v) {
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
                    if (first_deleted != kNpos) --tombstones_;
                    ctrl_[target]  = h2;
                    slots_[target] = Slot{k, v};
                    ++size_;
                    return;
                }
            }
        }

        rehash(slot_count() * 2u);
        insert_rehashed(k, v);
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    // Ehrliche Allokator-Metrik: gezaehlt werden nur erfolgreiche vector-capacity-Zuwaechse, als Capacity-Delta
    // mal Elementgroesse. Reuse/clear ohne Capacity-Wachstum erzeugt bewusst keine kuenstlichen Werte.
    void record_capacity_growth_(std::size_t old_capacity, std::size_t new_capacity, std::size_t elem_bytes) noexcept {
        if (new_capacity <= old_capacity) return;
        ++alloc_calls_;
        bytes_allocated_ +=
            static_cast<std::uint64_t>(new_capacity - old_capacity) * static_cast<std::uint64_t>(elem_bytes);
    }
#else
    static void record_capacity_growth_(std::size_t, std::size_t, std::size_t) noexcept {}
#endif

    std::vector<Slot, slot_allocator_type>         slots_;
    std::vector<std::uint8_t, ctrl_allocator_type> ctrl_;
    std::size_t                                    mask_;
    std::size_t                                    size_       = 0;
    std::size_t                                    tombstones_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    std::uint64_t alloc_calls_     = 0;
    std::uint64_t bytes_allocated_ = 0;
#endif
};

static_assert(SwissGroupPool<SwissGroupPoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
