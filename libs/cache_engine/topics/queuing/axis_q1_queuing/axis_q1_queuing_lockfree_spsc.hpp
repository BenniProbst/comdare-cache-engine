#pragma once
// V41.F.6.1 axis_q1_queuing LockFreeSPSC Q-SPSC (2026-05-26)
//
// @topic queuing @achse Q1 @family Q13a LockFreeSPSC
// @subaxis QS6 lock_free_access (ERSTE QS6-Strategie)
//
// Lock-Free Single-Producer/Single-Consumer Ring-Queue nach Lamport's
// klassischem Konzept (Lamport 1983 "Specifying Concurrent Program Modules"):
// 1 Producer schreibt nur an tail_, 1 Consumer liest nur von head_ — kein
// Contention dank disjoint Modified-Sets. atomic-loads/stores mit acquire/
// release reichen aus (kein CAS noetig).
//
// **ECHTE Lock-Freedom** (sogar wait-free fuer SPSC, da kein Retry-Loop):
//   - put() ist wait-free (1 atomic.store + 1 atomic.load)
//   - get() ist wait-free (1 atomic.load + 1 atomic.store)
//
// **Erste Q1-Strategie mit ProgressGuarantee::LockFree** + erste QS6 lock_free
// Subaxis-Belegung. **3. Strategie mit iterable_aspect_t** (nach BoundedRing
// + EpochBuffer) — Capacity ist iterable Aspekt.
//
// Overflow-Verhalten: put() bei vollem Buffer ist dropping (kein Block, kein
// Throw — typisch SPSC Disruptor-Pattern, Producer ist "verantwortlich").
//
// Pflicht-Vertrag: **Genau 1 Producer-Thread + genau 1 Consumer-Thread.**
// Mehrere Threads auf einer Seite brechen das Lamport-Modell → UB.
//
// Allocation: std::vector(cap) im Constructor — wirft std::bad_alloc bei OOM
// ([[allocation-failure-exception]]). cap=0 wirft std::invalid_argument
// ([[zero-size-allocation-exception]]).

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
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::queuing::axis_q1_queuing {

class LockFreeSPSC : public BufferStrategyBase<LockFreeSPSC> {
public:
    static constexpr bool enabled = flags::lockfree_spsc_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::lock_free_access_tag;
    using family_id    = std::integral_constant<int, 13>;  // Q13a

    /// iterable_aspect_t (F.6.1.E hybride Laufzeit-Permutation)
    using iterable_aspect_t = std::size_t;
    static constexpr std::array<std::size_t, 5> kIterableCapacities{8u, 64u, 1024u, 16384u, 65536u};
    [[nodiscard]] static constexpr std::span<std::size_t const> iterable_values() noexcept {
        return std::span<std::size_t const>{kIterableCapacities.data(), kIterableCapacities.size()};
    }

    /// SPSC-Vertrag: thread-safe NUR fuer exakt 1 Producer + 1 Consumer.
    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return true; }
    [[nodiscard]] static constexpr bool        is_bounded()        noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t default_capacity()  noexcept { return 1024; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "lockfree_spsc"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "LockFreeSPSC (Lamport 1983, wait-free SPSC Ring)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "LOCKFREE_SPSC"; }

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }  // S = Single
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }  // S = Single
    [[nodiscard]] static constexpr bool supports_priority_ordering()    noexcept { return false; }
    [[nodiscard]] static constexpr bool is_versioned()                  noexcept { return false; }
    /// SONDERFALL: 1. Q1-Strategie mit ProgressGuarantee::LockFree.
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::LockFree;
    }

    LockFreeSPSC() : LockFreeSPSC(default_capacity()) {}
    /// SONDERFALL [[zero-size-allocation-exception]]: cap=0 wirft std::invalid_argument
    /// (UB-Vermeidung: tail_=(tail_+1)%capacity_ ist Division-By-Zero bei cap=0).
    explicit LockFreeSPSC(std::size_t cap)
        : buffer_((cap == 0 ? throw std::invalid_argument(
                                  "LockFreeSPSC: capacity must be > 0 (cap=0 division-by-zero in modulo)")
                            : cap))
        , capacity_(cap), head_(0), tail_(0) {}

    // SPSC ist nicht copy/move-fähig (atomics) — Vergleich nur ueber Capacity.
    LockFreeSPSC(LockFreeSPSC const&) = delete;
    LockFreeSPSC& operator=(LockFreeSPSC const&) = delete;
    LockFreeSPSC(LockFreeSPSC&&) = delete;
    LockFreeSPSC& operator=(LockFreeSPSC&&) = delete;

    [[nodiscard]] bool operator==(LockFreeSPSC const& other) const noexcept {
        return capacity_ == other.capacity_;
    }

    /// Lamport-Producer: schreibt nur tail_. Bei vollem Buffer: drop (kein Block).
    /// Wait-free (kein Retry-Loop).
    void put(element_type v) {
        std::size_t const tail = tail_.load(std::memory_order_relaxed);
        std::size_t const next = (tail + 1) % capacity_;
        std::size_t const head = head_.load(std::memory_order_acquire);
        if (next == head) {
            // Buffer voll — Drop (typisch SPSC Disruptor-Pattern)
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.overflow_count;
            observer_.notify(stats_);
#endif
            return;
        }
        buffer_[tail] = v;
        tail_.store(next, std::memory_order_release);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        std::size_t cur_size = (next + capacity_ - head) % capacity_;
        if (cur_size > stats_.peak_size) stats_.peak_size = cur_size;
        observer_.notify(stats_);
#endif
    }

    /// Lamport-Consumer: liest nur head_. Wait-free.
    [[nodiscard]] std::optional<element_type> get() {
        std::size_t const head = head_.load(std::memory_order_relaxed);
        std::size_t const tail = tail_.load(std::memory_order_acquire);
        if (head == tail) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.underflow_count;
            observer_.notify(stats_);
#endif
            return std::nullopt;
        }
        element_type v = buffer_[head];
        head_.store((head + 1) % capacity_, std::memory_order_release);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_get_count;
        observer_.notify(stats_);
#endif
        return v;
    }

    [[nodiscard]] size_type size() const noexcept {
        std::size_t const t = tail_.load(std::memory_order_acquire);
        std::size_t const h = head_.load(std::memory_order_acquire);
        return (t + capacity_ - h) % capacity_;
    }
    [[nodiscard]] bool      is_empty() const noexcept { return size() == 0; }
    /// clear() ist nur sicher wenn weder Producer noch Consumer aktiv.
    void                    clear()          noexcept {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

    [[nodiscard]] size_type capacity() const noexcept { return capacity_; }

    // std::queue-API: peek_front=oldest (head), peek_back=newest (tail-1).
    // Achtung: peek bei concurrent put/get ist nur Snapshot.
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept {
        std::size_t const h = head_.load(std::memory_order_acquire);
        std::size_t const t = tail_.load(std::memory_order_acquire);
        if (h == t) return std::nullopt;
        return buffer_[h];
    }
    [[nodiscard]] std::optional<element_type> peek_back() const noexcept {
        std::size_t const h = head_.load(std::memory_order_acquire);
        std::size_t const t = tail_.load(std::memory_order_acquire);
        if (h == t) return std::nullopt;
        return buffer_[(t + capacity_ - 1) % capacity_];
    }
    void emplace(element_type v) { put(v); }

    /// Setter fuer Runtime-Capacity-Switch (iterable_aspect_t Pattern).
    /// Nur sicher wenn weder Producer noch Consumer aktiv (Reconfigure-Time).
    void set_capacity(std::size_t new_cap) {
        if (new_cap == 0) throw std::invalid_argument("LockFreeSPSC::set_capacity: cap must be > 0");
        buffer_.assign(new_cap, 0);
        capacity_ = new_cap;
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

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
    std::vector<element_type> buffer_;
    std::size_t capacity_;
    std::atomic<std::size_t> head_;
    std::atomic<std::size_t> tail_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::queuing::axis_q1_queuing {
    static_assert(concepts::BufferStrategy<LockFreeSPSC>);
    static_assert(concepts::CacheEngineBufferPermutationStrategy<LockFreeSPSC>);
}
