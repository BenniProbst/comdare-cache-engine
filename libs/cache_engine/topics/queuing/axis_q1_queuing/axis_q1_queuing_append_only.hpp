#pragma once
// V41.F.6.1 axis_q1_queuing AppendOnlyBuffer Q-APP (2026-05-26)
//
// @topic queuing @achse Q1 @family Q02 AppendOnlyBuffer (Linear)
// @subaxis QS1 sequential_access
//
// Append-Only Buffer: monoton wachsender Linear-Buffer ohne mittiges Loeschen.
// Anwendung: LSM-MemTable (Write-Buffer vor Compact), Bw-Tree Delta-Chain
// (Levandoski 2013). get() entfernt vom Anfang (FIFO-Drain). put() hat amortisiert
// O(1) — bei Heap-Wachstum kann std::bad_alloc fliegen ([[allocation-failure-exception]]).
//
// Unterschied zu FIFOQueueBuffer:
//   - FIFOQueueBuffer: std::deque, kann mittig effizient erweitert/entleert werden
//   - AppendOnlyBuffer: std::vector, OPTIMIERT fuer reine Append + Bulk-Drain (cache-freundlicher,
//     bessere Locality bei Sequenz-Scan; nicht fuer haeufige Single-get() optimiert)

#include "axis_q1_queuing_base.hpp"
#include "axis_q1_queuing_subaxes_qs1_to_qs6.hpp"
#include "concepts/axis_q1_queuing_concept.hpp"
#include "concepts/axis_q1_queuing_cache_engine_permutation_concept.hpp"
#include "../concepts/topic_queuing_concept.hpp"

#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::queuing::axis_q1_queuing {

class AppendOnlyBuffer : public BufferStrategyBase<AppendOnlyBuffer> {
public:
    static constexpr bool enabled = flags::append_only_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::sequential_access_tag;
    using family_id    = std::integral_constant<int, 2>;  // Q02

    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return false; }
    [[nodiscard]] static constexpr bool        is_bounded()        noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t default_capacity()  noexcept { return 0; }  // unbounded
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "append_only"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "AppendOnlyBuffer (LSM-MemTable + Bw-Tree Delta-Chain, Levandoski 2013)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "APPEND_ONLY"; }

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_priority_ordering()    noexcept { return false; }
    [[nodiscard]] static constexpr bool is_versioned()                  noexcept { return false; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    [[nodiscard]] bool operator==(AppendOnlyBuffer const& other) const noexcept {
        return data_.size() == other.data_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: std::vector::push_back kann
    /// std::bad_alloc werfen wenn die Reallocation OOM trifft. Nicht noexcept.
    void put(element_type v) {
        data_.push_back(v);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        if (data_.size() > stats_.peak_size) stats_.peak_size = data_.size();
        observer_.notify(stats_);
#endif
    }

    /// Drain vom Anfang (FIFO-Semantik). O(N) Verschieben — fuer Bulk-Drain
    /// drain_all() noch effizienter (TODO Vollausbau).
    [[nodiscard]] std::optional<element_type> get() {
        if (drain_pos_ >= data_.size()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.underflow_count;
            observer_.notify(stats_);
#endif
            return std::nullopt;
        }
        element_type v = data_[drain_pos_++];
        // Garbage-Collection bei vollem Drain: vector resetten um Speicher freizugeben
        if (drain_pos_ >= data_.size()) {
            data_.clear();
            drain_pos_ = 0;
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_get_count;
        observer_.notify(stats_);
#endif
        return v;
    }

    [[nodiscard]] size_type size()     const noexcept { return data_.size() - drain_pos_; }
    [[nodiscard]] bool      is_empty() const noexcept { return drain_pos_ >= data_.size(); }
    void                    clear()          noexcept { data_.clear(); drain_pos_ = 0; }

    // std::queue-API: peek_front=oldest non-drained, peek_back=newest
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept {
        if (drain_pos_ >= data_.size()) return std::nullopt;
        return data_[drain_pos_];
    }
    [[nodiscard]] std::optional<element_type> peek_back() const noexcept {
        if (drain_pos_ >= data_.size()) return std::nullopt;
        return data_.back();
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
    std::vector<element_type> data_;
    std::size_t drain_pos_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::queuing::axis_q1_queuing {
    static_assert(concepts::BufferStrategy<AppendOnlyBuffer>);
    static_assert(concepts::CacheEngineBufferPermutationStrategy<AppendOnlyBuffer>);
}
