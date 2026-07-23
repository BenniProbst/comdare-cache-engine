#pragma once
// V41.F.6.1 axis_q1_queuing FIFOQueueBuffer Q-FIFO (2026-05-26)
//
// @topic queuing @achse Q1 @family Q03 FIFOQueueBuffer (Ring/Deque)
// @subaxis QS1 sequential_access
//
// First-In-First-Out: klassische LSM-MemTable + Write-Coalescing-Pattern.
// Unbounded — std::deque-basiert.

#include "axis_q1_queuing_base.hpp"
#include "axis_q1_queuing_subaxes_qs1_to_qs6.hpp"
#include "concepts/axis_q1_queuing_concept.hpp"
#include "concepts/axis_q1_queuing_cache_engine_permutation_concept.hpp"
#include "../concepts/topic_queuing_concept.hpp"

#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::queuing::axis_q1_queuing {

class FIFOQueueBuffer : public BufferStrategyBase<FIFOQueueBuffer> {
public:
    static constexpr bool enabled = flags::fifo_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::sequential_access_tag;
    using family_id    = std::integral_constant<int, 3>; // Q03

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr bool             is_bounded() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      default_capacity() noexcept { return 0; } // unbounded
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "fifo_queue"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::queuing::axis_q1_queuing::FIFOQueueBuffer",
                                  "topics/queuing/axis_q1_queuing/axis_q1_queuing_fifo.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "FIFOQueueBuffer (Ring/Deque — LSM MemTable + Write-Coalescing)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "FIFO"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_priority_ordering() noexcept { return false; }
    [[nodiscard]] static constexpr bool is_versioned() noexcept { return false; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    [[nodiscard]] bool operator==(FIFOQueueBuffer const& other) const noexcept {
        return data_.size() == other.data_.size(); // gleicher Inhalt zu pruefen waere teuer
    }

    void put(element_type v) {
        data_.push_back(v);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        if (data_.size() > stats_.peak_size) stats_.peak_size = data_.size();
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<element_type> get() {
        if (data_.empty()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.underflow_count;
            observer_.notify(stats_);
#endif
            return std::nullopt;
        }
        element_type v = data_.front();
        data_.pop_front();
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_get_count;
        observer_.notify(stats_);
#endif
        return v;
    }

    [[nodiscard]] size_type size() const noexcept { return data_.size(); }
    [[nodiscard]] bool      is_empty() const noexcept { return data_.empty(); }
    void                    clear() noexcept { data_.clear(); }

    // std::queue-API: peek_front=oldest (deque::front), peek_back=newest (deque::back)
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept {
        if (data_.empty()) return std::nullopt;
        return data_.front();
    }
    [[nodiscard]] std::optional<element_type> peek_back() const noexcept {
        if (data_.empty()) return std::nullopt;
        return data_.back();
    }
    void emplace(element_type v) { put(v); }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::BufferStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }
#endif

private:
    std::deque<element_type> data_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t                 observer_{};
#endif
};

} // namespace comdare::cache_engine::queuing::axis_q1_queuing

namespace comdare::cache_engine::queuing::axis_q1_queuing {
static_assert(concepts::BufferStrategy<FIFOQueueBuffer>);
static_assert(concepts::CacheEngineBufferPermutationStrategy<FIFOQueueBuffer>);
} // namespace comdare::cache_engine::queuing::axis_q1_queuing
