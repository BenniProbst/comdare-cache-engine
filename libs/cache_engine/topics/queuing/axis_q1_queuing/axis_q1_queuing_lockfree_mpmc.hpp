#pragma once
// V41.F.6.1 axis_q1_queuing LockFreeMPMC Q-MPMC (2026-05-26)
//
// @topic queuing @achse Q1 @family Q13b LockFreeMPMC
// @subaxis QS6 lock_free_access
//
// Lock-Free Multi-Producer/Multi-Consumer Bounded Ring-Queue nach Vyukov
// (https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue,
// alternativ: Michael/Scott PODC 1996 Two-Lock Concurrent Queue als
// linked-list-Variante). Pro Cell eine atomic Sequence-Number; Producer und
// Consumer treffen sich exakt einmal pro Cell ueber CAS-Loop auf der Sequence.
//
// **Algorithmus (Vyukov bounded MPMC):**
//   - Cell.sequence == pos  -> Producer kann schreiben (CAS pos -> pos+1)
//   - Cell.sequence == pos+1 -> Consumer kann lesen (CAS pos -> pos+1)
//   - sonst -> Retry (yield)
//   - pos = enqueue_pos_ % cap (capacity muss Power-of-2 sein!)
//
// **ECHTE Lock-Freedom** mit CAS-Retry-Loop (im Worst-Case multiple CAS bei
// Contention — wait-free pro Operation NICHT garantiert).
//
// **Erste Q1-Strategie mit:**
//   - supports_concurrent_producers()=true (M = Multi)
//   - supports_concurrent_consumers()=true (M = Multi)
// 4. Strategie mit iterable_aspect_t (analog SPSC, aber Power-of-2 Pflicht).
//
// **CAPACITY PFLICHT POWER-OF-2** (Vyukov-Constraint fuer Modulo via bitmask).
// Constructor wirft std::invalid_argument bei nicht-Power-of-2 oder cap=0
// ([[zero-size-allocation-exception]]).
//
// Allocation: std::vector(cap) + atomics — wirft std::bad_alloc bei OOM
// ([[allocation-failure-exception]]).

#include "axis_q1_queuing_base.hpp"
#include "axis_q1_queuing_subaxes_qs1_to_qs6.hpp"
#include "concepts/axis_q1_queuing_concept.hpp"
#include "concepts/axis_q1_queuing_cache_engine_permutation_concept.hpp"
#include "../concepts/topic_queuing_concept.hpp"

#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_flags.hpp>
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
#include <vector>

namespace comdare::cache_engine::queuing::axis_q1_queuing {

class LockFreeMPMC : public BufferStrategyBase<LockFreeMPMC> {
public:
    static constexpr bool enabled = flags::lockfree_mpmc_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::lock_free_access_tag;
    using family_id    = std::integral_constant<int, 14>;  // Q13b (interne ID 14)

    /// iterable_aspect_t — Power-of-2 Pflicht (Vyukov-Modulo via bitmask).
    using iterable_aspect_t = std::size_t;
    static constexpr std::array<std::size_t, 5> kIterableCapacities{8u, 64u, 1024u, 16384u, 65536u};
    [[nodiscard]] static constexpr std::span<std::size_t const> iterable_values() noexcept {
        return std::span<std::size_t const>{kIterableCapacities.data(), kIterableCapacities.size()};
    }

    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return true; }
    [[nodiscard]] static constexpr bool        is_bounded()        noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t default_capacity()  noexcept { return 1024; }  // Power-of-2
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "lockfree_mpmc"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "LockFreeMPMC (Vyukov bounded MPMC, per-Cell-Sequence, 1024cores.net)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "LOCKFREE_MPMC"; }

    /// SONDERFALL: ERSTE Q1-Strategien mit supports_concurrent_producers/consumers=true.
    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return true; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return true; }
    [[nodiscard]] static constexpr bool supports_priority_ordering()    noexcept { return false; }
    [[nodiscard]] static constexpr bool is_versioned()                  noexcept { return false; }
    /// SONDERFALL: ProgressGuarantee::LockFree (CAS-Retry, nicht wait-free).
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::LockFree;
    }

    LockFreeMPMC() : LockFreeMPMC(default_capacity()) {}

    /// SONDERFALL [[zero-size-allocation-exception]]: cap=0 oder nicht-Power-of-2 wirft.
    explicit LockFreeMPMC(std::size_t cap)
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

    // MPMC ist nicht copy/move-fähig (atomics + unique_ptr Sequence)
    LockFreeMPMC(LockFreeMPMC const&) = delete;
    LockFreeMPMC& operator=(LockFreeMPMC const&) = delete;
    LockFreeMPMC(LockFreeMPMC&&) = delete;
    LockFreeMPMC& operator=(LockFreeMPMC&&) = delete;

    [[nodiscard]] bool operator==(LockFreeMPMC const& other) const noexcept {
        return capacity_ == other.capacity_;
    }

    /// Vyukov-Enqueue: CAS-Loop auf enqueue_pos_, dann Slot-Sequence Update.
    /// Bei vollem Buffer: drop (kein Block).
    void put(element_type v) {
        Cell* cell;
        std::size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
        for (;;) {
            cell = &cells_[pos & mask_];
            std::size_t seq = cell->sequence.load(std::memory_order_acquire);
            std::intptr_t diff = static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos);
            if (diff == 0) {
                if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;  // Cell reserviert
                }
            } else if (diff < 0) {
                // Buffer voll — drop
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

    /// Vyukov-Dequeue: CAS-Loop auf dequeue_pos_, dann Slot-Sequence Update.
    [[nodiscard]] std::optional<element_type> get() {
        Cell* cell;
        std::size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
        for (;;) {
            cell = &cells_[pos & mask_];
            std::size_t seq = cell->sequence.load(std::memory_order_acquire);
            std::intptr_t diff = static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos + 1);
            if (diff == 0) {
                if (dequeue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                // Buffer leer
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
    [[nodiscard]] bool      is_empty() const noexcept { return size() == 0; }
    /// clear() ist nur sicher wenn keine Producer/Consumer aktiv.
    void                    clear()          noexcept {
        enqueue_pos_.store(0, std::memory_order_relaxed);
        dequeue_pos_.store(0, std::memory_order_relaxed);
        for (std::size_t i = 0; i < capacity_; ++i) {
            cells_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    [[nodiscard]] size_type capacity() const noexcept { return capacity_; }

    // std::queue-API — Snapshot bei MPMC ist inherent inkohaerent unter Contention.
    // Wir geben "best effort" Werte (kein Strong-Guarantee).
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
    struct Cell {
        std::atomic<std::size_t> sequence{0};
        element_type             data{0};
    };

    static std::size_t validate_capacity(std::size_t cap) {
        if (cap == 0) {
            throw std::invalid_argument(
                "LockFreeMPMC: capacity must be > 0 (cap=0 division-by-zero in bitmask)");
        }
        // Power-of-2 check (Vyukov-Constraint)
        if ((cap & (cap - 1)) != 0) {
            throw std::invalid_argument(
                "LockFreeMPMC: capacity must be Power-of-2 (Vyukov-Constraint for bitmask modulo)");
        }
        return cap;
    }

    std::size_t              capacity_;
    std::size_t              mask_;
    std::unique_ptr<Cell[]>  cells_;
    std::atomic<std::size_t> enqueue_pos_;
    std::atomic<std::size_t> dequeue_pos_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::queuing::axis_q1_queuing {
    static_assert(concepts::BufferStrategy<LockFreeMPMC>);
    static_assert(concepts::CacheEngineBufferPermutationStrategy<LockFreeMPMC>);
}
