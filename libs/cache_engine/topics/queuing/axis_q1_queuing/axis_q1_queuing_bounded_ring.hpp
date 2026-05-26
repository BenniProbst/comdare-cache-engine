#pragma once
// V41.F.6.1 axis_q1_queuing BoundedRing Q-RING (2026-05-26)
//
// @topic queuing @achse Q1 @family Q05 BoundedRing<N>
// @subaxis QS3 cyclic_access
//
// Bounded Ring-Buffer (Disruptor-Pattern, LMAX 2011): zyklischer Buffer
// fester Kapazitaet. Overflow-Verhalten: drop_oldest (Default).
//
// **iterable_aspect_t Sonderfall:** Capacity ist iterable Aspekt fuer
// hybride Laufzeit-Permutation (Doku §15.5). PermutationEngine erkennt
// HasIterableAspect<BoundedRing> und generiert 1 Binary mit Runtime-Loop
// ueber kIterableCapacities statt 5 separate Binaries.

#include "axis_q1_queuing_base.hpp"
#include "axis_q1_queuing_subaxes_qs1_to_qs6.hpp"
#include "concepts/axis_q1_queuing_concept.hpp"
#include "concepts/axis_q1_queuing_cache_engine_permutation_concept.hpp"
#include "concepts/axis_q1_queuing_bounded_strategy_concept.hpp"
#include "concepts/axis_q1_queuing_iterable_aspect_strategy_concept.hpp"
#include "../concepts/topic_queuing_concept.hpp"

#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::queuing::axis_q1_queuing {

class BoundedRing : public BufferStrategyBase<BoundedRing> {
public:
    static constexpr bool enabled = flags::bounded_ring_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::cyclic_access_tag;
    using family_id    = std::integral_constant<int, 5>;  // Q05

    /// iterable_aspect_t (F.6.1.E hybride Laufzeit-Permutation, [[no-runtime-switch]] Ausnahme)
    /// PermutationEngine erkennt via HasIterableAspect<V> und generiert
    /// 1 Binary mit Runtime-Loop ueber kIterableCapacities.
    using iterable_aspect_t = std::size_t;
    static constexpr std::array<std::size_t, 5> kIterableCapacities{8u, 64u, 1024u, 16384u, 65536u};
    [[nodiscard]] static constexpr std::span<std::size_t const> iterable_values() noexcept {
        return std::span<std::size_t const>{kIterableCapacities.data(), kIterableCapacities.size()};
    }

    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return false; }
    [[nodiscard]] static constexpr bool        is_bounded()        noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t default_capacity()  noexcept { return 64; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "bounded_ring"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "BoundedRing<N> (Disruptor-Pattern LMAX 2011, SPSC/MPMC)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "BOUNDED_RING"; }

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }  // SPSC-Variante
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_priority_ordering()    noexcept { return false; }
    [[nodiscard]] static constexpr bool is_versioned()                  noexcept { return false; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    BoundedRing() : BoundedRing(default_capacity()) {}
    /// SONDERFALL [[zero-size-allocation-exception]]: cap=0 wirft std::invalid_argument
    /// (UB-Vermeidung: head_=(head_+1)%capacity_ ist Division-By-Zero bei cap=0).
    explicit BoundedRing(std::size_t cap)
        : buffer_((cap == 0 ? throw std::invalid_argument(
                                  "BoundedRing: capacity must be > 0 (cap=0 division-by-zero in modulo)")
                            : cap))
        , capacity_(cap), head_(0), tail_(0), count_(0) {}

    [[nodiscard]] bool operator==(BoundedRing const& other) const noexcept {
        return capacity_ == other.capacity_;
    }

    void put(element_type v) {
        if (count_ == capacity_) {
            // Overflow: drop_oldest (Default-Verhalten)
            head_ = (head_ + 1) % capacity_;
            --count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.overflow_count;
#endif
        }
        buffer_[tail_] = v;
        tail_ = (tail_ + 1) % capacity_;
        ++count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        if (count_ > stats_.peak_size) stats_.peak_size = count_;
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<element_type> get() {
        if (count_ == 0) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.underflow_count;
            observer_.notify(stats_);
#endif
            return std::nullopt;
        }
        element_type v = buffer_[head_];
        head_ = (head_ + 1) % capacity_;
        --count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_get_count;
        observer_.notify(stats_);
#endif
        return v;
    }

    [[nodiscard]] size_type size()     const noexcept { return count_; }
    [[nodiscard]] bool      is_empty() const noexcept { return count_ == 0; }
    void                    clear()          noexcept { head_ = tail_ = count_ = 0; }

    [[nodiscard]] size_type capacity() const noexcept { return capacity_; }

    // std::queue-API: peek_front=head (oldest), peek_back=tail-1 (newest)
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept {
        if (count_ == 0) return std::nullopt;
        return buffer_[head_];
    }
    [[nodiscard]] std::optional<element_type> peek_back() const noexcept {
        if (count_ == 0) return std::nullopt;
        std::size_t last_idx = (tail_ + capacity_ - 1) % capacity_;
        return buffer_[last_idx];
    }
    void emplace(element_type v) { put(v); }

    /// Setter fuer Runtime-Capacity-Switch ([[iterable-aspect-strategy]] Sub-Concept).
    /// Konsolidierter Name `set_iterable_aspect` analog allen anderen iterable Strategien.
    /// SONDERFALL [[allocation-failure-exception]]: assign kann std::bad_alloc werfen.
    /// SONDERFALL [[zero-size-allocation-exception]]: new_cap=0 wirft std::invalid_argument.
    void set_iterable_aspect(std::size_t new_cap) {
        if (new_cap == 0) {
            throw std::invalid_argument(
                "BoundedRing::set_iterable_aspect: capacity must be > 0");
        }
        buffer_.assign(new_cap, 0);
        capacity_ = new_cap;
        head_ = tail_ = count_ = 0;
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
    std::size_t head_;
    std::size_t tail_;
    std::size_t count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::queuing::axis_q1_queuing {
    static_assert(concepts::BufferStrategy<BoundedRing>);
    static_assert(concepts::CacheEngineBufferPermutationStrategy<BoundedRing>);
    static_assert(concepts::BoundedBufferStrategy<BoundedRing>);
    static_assert(concepts::IterableAspectStrategy<BoundedRing>);
}
