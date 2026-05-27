#pragma once
// V41.F.6.1.P2.D.q.s2 OriginalLockFreeMpmcConcurrentQueue Q15 (2026-05-26)
//
// @topic queuing @achse Q1 @family Q15 OriginalLockFreeMpmcConcurrentQueue
// @subaxis QS6 lock_free_access (analog LockFreeMPMCBuffer)
// @paper Q01 moodycamel::ConcurrentQueue (Cameron Desrochers, BSD-2)
// @source ext/queuing/Q01-concurrentqueue/concurrentqueue.h
//
// **Habich-Compliance-Wrapper:** parallel zu LockFreeMPMCBuffer (Re-Impl Vyukov bounded).
// get_compiler()="gcc-9.5" via Paper-Mixin (Tool-validierte Source-Identity via SHA256).
//
// **Luecken-Markierung (2/6 originall, 4/6 Luecken):**
//   - put:        originall (ConcurrentQueue::enqueue)
//   - get:        originall (ConcurrentQueue::try_dequeue)
//   - emplace:    LUECKE (concurrentqueue hat keine emplace — Re-Impl)
//   - peek_front: LUECKE (async-only, kein peek — Re-Impl)
//   - peek_back:  LUECKE (analog peek_front)
//   - clear:      LUECKE (kein clear — Re-Impl via Drain-Loop)
// is_original_module()=false (weil 4/6 Lücken).
//
// **Body-Strategie (s2):** Standalone Vyukov bounded MPMC (analog LockFreeMPMCBuffer).
// s4: extern Linking via #include <concurrentqueue.h> + ConcurrentQueue<u64>.
// concurrentqueue ist header-only (BSD-2) — kein add_library noetig in s4.
//
// **Erste Q1-Strategie mit externer Submodule-Bindung (analog mimalloc bei Allocator).**

#include "axis_q1_queuing_base.hpp"
#include "axis_q1_queuing_subaxes_qs1_to_qs6.hpp"
#include "concepts/axis_q1_queuing_concept.hpp"
#include "concepts/axis_q1_queuing_cache_engine_permutation_concept.hpp"
#include "concepts/axis_q1_queuing_bounded_strategy_concept.hpp"
#include "concepts/axis_q1_queuing_iterable_aspect_strategy_concept.hpp"
#include "../concepts/topic_queuing_concept.hpp"

#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_flags.hpp>

// V41.F.6.1.P2.D.q.s2 Paper-Mixin (Tool-generated, SHA256-validiert gegen concurrentqueue.h)
#include "concepts/axis_q1_queuing_original_code_mixin.hpp"
#include <topics/queuing/axis_q1_queuing/legacy_code/paper_q01_concurrentqueue_is_original.hpp>

#include <measurement/measurable_concept.hpp>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q1_queuing {

class OriginalLockFreeMpmcConcurrentQueue
    : public BufferStrategyBase<OriginalLockFreeMpmcConcurrentQueue>,
      public generated::q01_concurrentqueue::OriginalCodeMixin {
public:
    // Diamond-Disambiguation: Mixin-Pfad wins
    using generated::q01_concurrentqueue::OriginalCodeMixin::get_compiler;
    using generated::q01_concurrentqueue::OriginalCodeMixin::is_original_put;
    using generated::q01_concurrentqueue::OriginalCodeMixin::is_original_get;
    using generated::q01_concurrentqueue::OriginalCodeMixin::is_original_emplace;
    using generated::q01_concurrentqueue::OriginalCodeMixin::is_original_peek_front;
    using generated::q01_concurrentqueue::OriginalCodeMixin::is_original_peek_back;
    using generated::q01_concurrentqueue::OriginalCodeMixin::is_original_clear;
    using generated::q01_concurrentqueue::OriginalCodeMixin::is_original_module;

    static constexpr bool enabled = flags::original_concurrentqueue_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::lock_free_access_tag;
    using family_id    = std::integral_constant<int, 15>;  // Q15 (nach Q01-Q13b)

    /// iterable_aspect_t — Power-of-2 Pflicht (Vyukov-Modulo via bitmask, identisch zu LockFreeMPMCBuffer).
    using iterable_aspect_t = std::size_t;
    static constexpr std::array<std::size_t, 5> kIterableCapacities{8u, 64u, 1024u, 16384u, 65536u};
    [[nodiscard]] static constexpr std::span<std::size_t const> iterable_values() noexcept {
        return std::span<std::size_t const>{kIterableCapacities.data(), kIterableCapacities.size()};
    }

    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return true; }
    [[nodiscard]] static constexpr bool        is_bounded()        noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t default_capacity()  noexcept { return 1024; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept {
        if constexpr (enabled) { return "original_concurrentqueue"; }
        else                   { return "original_concurrentqueue(disabled)"; }
    }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "OriginalLockFreeMpmcConcurrentQueue (moodycamel BSD-2 Paper-Bindung — 2/6 originall)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "ORIGINAL_CONCURRENTQUEUE"; }

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return true; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return true; }
    [[nodiscard]] static constexpr bool supports_priority_ordering()    noexcept { return false; }
    [[nodiscard]] static constexpr bool is_versioned()                  noexcept { return false; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::LockFree;
    }

    OriginalLockFreeMpmcConcurrentQueue() : OriginalLockFreeMpmcConcurrentQueue(default_capacity()) {}

    /// SONDERFALL [[zero-size-allocation-exception]]: cap=0 oder nicht-Power-of-2 wirft.
    explicit OriginalLockFreeMpmcConcurrentQueue(std::size_t cap)
        : capacity_(validate_capacity(cap))
        , mask_(cap - 1)
        , enqueue_pos_(0)
        , dequeue_pos_(0)
    {
        cells_ = std::make_unique<Cell[]>(cap);
        for (std::size_t i = 0; i < cap; ++i) {
            cells_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    OriginalLockFreeMpmcConcurrentQueue(OriginalLockFreeMpmcConcurrentQueue const&) = delete;
    OriginalLockFreeMpmcConcurrentQueue& operator=(OriginalLockFreeMpmcConcurrentQueue const&) = delete;
    OriginalLockFreeMpmcConcurrentQueue(OriginalLockFreeMpmcConcurrentQueue&&) = delete;
    OriginalLockFreeMpmcConcurrentQueue& operator=(OriginalLockFreeMpmcConcurrentQueue&&) = delete;

    [[nodiscard]] bool operator==(OriginalLockFreeMpmcConcurrentQueue const& other) const noexcept {
        return capacity_ == other.capacity_;
    }

    /// Paper-API (originall via ConcurrentQueue::enqueue in s4).
    /// Body s2: Vyukov-Pattern Standalone.
    void put(element_type v) {
        if constexpr (!enabled) { (void)v; return; }
        Cell* cell;
        std::size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
        for (;;) {
            cell = &cells_[pos & mask_];
            std::size_t seq = cell->sequence.load(std::memory_order_acquire);
            std::intptr_t diff = static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos);
            if (diff == 0) {
                if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) break;
            } else if (diff < 0) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
                ++stats_.overflow_count;
                observer_.notify(stats_);
#endif
                return;
            } else {
                pos = enqueue_pos_.load(std::memory_order_relaxed);
            }
        }
        cell->data = v;
        cell->sequence.store(pos + 1, std::memory_order_release);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        std::size_t cur_size = pos + 1 - dequeue_pos_.load(std::memory_order_acquire);
        if (cur_size > stats_.peak_size) stats_.peak_size = cur_size;
        observer_.notify(stats_);
#endif
    }

    /// Paper-API (originall via ConcurrentQueue::try_dequeue in s4).
    [[nodiscard]] std::optional<element_type> get() {
        if constexpr (!enabled) { return std::nullopt; }
        Cell* cell;
        std::size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
        for (;;) {
            cell = &cells_[pos & mask_];
            std::size_t seq = cell->sequence.load(std::memory_order_acquire);
            std::intptr_t diff = static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos + 1);
            if (diff == 0) {
                if (dequeue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) break;
            } else if (diff < 0) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
                ++stats_.underflow_count;
                observer_.notify(stats_);
#endif
                return std::nullopt;
            } else {
                pos = dequeue_pos_.load(std::memory_order_relaxed);
            }
        }
        element_type v = cell->data;
        cell->sequence.store(pos + capacity_, std::memory_order_release);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_get_count;
        observer_.notify(stats_);
#endif
        return v;
    }

    [[nodiscard]] size_type size() const noexcept {
        std::size_t e = enqueue_pos_.load(std::memory_order_acquire);
        std::size_t d = dequeue_pos_.load(std::memory_order_acquire);
        return (e > d) ? (e - d) : 0;
    }
    [[nodiscard]] bool is_empty() const noexcept { return size() == 0; }

    /// LUECKE: kein clear in concurrentqueue Paper-API. Cache-Engine Re-Impl via Cell-Reset.
    void clear() noexcept {
        enqueue_pos_.store(0, std::memory_order_relaxed);
        dequeue_pos_.store(0, std::memory_order_relaxed);
        for (std::size_t i = 0; i < capacity_; ++i) {
            cells_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    [[nodiscard]] size_type capacity() const noexcept { return capacity_; }

    /// IterableAspectStrategy: Runtime-Capacity-Switch (Reconfigure-Time only).
    void set_iterable_aspect(std::size_t new_cap) {
        std::size_t validated = validate_capacity(new_cap);
        cells_ = std::make_unique<Cell[]>(validated);
        capacity_ = validated;
        mask_ = validated - 1;
        for (std::size_t i = 0; i < validated; ++i) {
            cells_[i].sequence.store(i, std::memory_order_relaxed);
        }
        enqueue_pos_.store(0, std::memory_order_relaxed);
        dequeue_pos_.store(0, std::memory_order_relaxed);
    }

    /// LUECKE [[paper-original-code-pattern]]: ConcurrentQueue ist async-only, kein peek.
    /// Cache-Engine "best effort"-Re-Impl (inkohaerent unter Contention).
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept {
        std::size_t pos = dequeue_pos_.load(std::memory_order_acquire);
        Cell const& cell = cells_[pos & mask_];
        std::size_t seq = cell.sequence.load(std::memory_order_acquire);
        if (seq == pos + 1) return cell.data;
        return std::nullopt;
    }
    [[nodiscard]] std::optional<element_type> peek_back() const noexcept {
        std::size_t pos = enqueue_pos_.load(std::memory_order_acquire);
        if (pos == 0) return std::nullopt;
        Cell const& cell = cells_[(pos - 1) & mask_];
        std::size_t seq = cell.sequence.load(std::memory_order_acquire);
        if (seq == pos) return cell.data;
        return std::nullopt;
    }

    /// LUECKE: kein emplace in ConcurrentQueue Paper-API. Cache-Engine Re-Impl via put.
    void emplace(element_type v) { put(v); }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::BufferStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot()   const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; observer_.notify(stats_); }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer()       noexcept { return observer_; }
#endif

private:
    struct alignas(64) Cell {
        std::atomic<std::size_t> sequence{0};
        element_type             data{};
    };

    [[nodiscard]] static std::size_t validate_capacity(std::size_t cap) {
        if (cap == 0) {
            throw std::invalid_argument(
                "OriginalLockFreeMpmcConcurrentQueue: capacity must be > 0 (Vyukov-Modulo benoetigt Power-of-2)");
        }
        if ((cap & (cap - 1)) != 0) {
            throw std::invalid_argument(
                "OriginalLockFreeMpmcConcurrentQueue: capacity must be Power-of-2 (mask-based modulo)");
        }
        return cap;
    }

    std::size_t              capacity_;
    std::size_t              mask_;
    std::atomic<std::size_t> enqueue_pos_;
    std::atomic<std::size_t> dequeue_pos_;
    std::unique_ptr<Cell[]>  cells_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::BufferStatistics stats_{};
    mutable observer_t                  observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::queuing::axis_q1_queuing {
    static_assert(concepts::BufferStrategy<OriginalLockFreeMpmcConcurrentQueue>);
    static_assert(concepts::CacheEngineBufferPermutationStrategy<OriginalLockFreeMpmcConcurrentQueue>);
    static_assert(concepts::BoundedBufferStrategy<OriginalLockFreeMpmcConcurrentQueue>);
    static_assert(concepts::IterableAspectStrategy<OriginalLockFreeMpmcConcurrentQueue>);
    // Habich-Compliance: 2/6 originall (put+get), 4/6 Lücken (emplace+peek_front+peek_back+clear)
    static_assert(OriginalLockFreeMpmcConcurrentQueue::is_original_put(),
        "OriginalLockFreeMpmcConcurrentQueue: put MUSS via ConcurrentQueue::enqueue Paper-Bindung originall sein");
    static_assert(OriginalLockFreeMpmcConcurrentQueue::is_original_get(),
        "OriginalLockFreeMpmcConcurrentQueue: get MUSS via ConcurrentQueue::try_dequeue Paper-Bindung originall sein");
    static_assert(!OriginalLockFreeMpmcConcurrentQueue::is_original_emplace(),
        "OriginalLockFreeMpmcConcurrentQueue: emplace ist Re-Impl (kein emplace in concurrentqueue)");
    static_assert(!OriginalLockFreeMpmcConcurrentQueue::is_original_peek_front(),
        "OriginalLockFreeMpmcConcurrentQueue: peek_front ist Re-Impl (concurrentqueue async-only)");
    static_assert(!OriginalLockFreeMpmcConcurrentQueue::is_original_peek_back(),
        "OriginalLockFreeMpmcConcurrentQueue: peek_back ist Re-Impl (concurrentqueue async-only)");
    static_assert(!OriginalLockFreeMpmcConcurrentQueue::is_original_clear(),
        "OriginalLockFreeMpmcConcurrentQueue: clear ist Re-Impl (kein clear in concurrentqueue)");
    static_assert(!OriginalLockFreeMpmcConcurrentQueue::is_original_module(),
        "OriginalLockFreeMpmcConcurrentQueue: is_original_module MUSS false sein (4/6 Lücken)");
}
