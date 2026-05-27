#pragma once
// V41.F.6.1 axis_q1_queuing PriorityHeapBuffer Q-PRIO (2026-05-26)
//
// @topic queuing @achse Q1 @family Q06 PriorityHeapBuffer (Hot-Key Promotion)
// @subaxis QS2 ordered_access
//
// Priority-basierter Buffer: get() liefert IMMER hoechste Prioritaet (max-heap).
// Implementation via std::priority_queue<std::uint64_t>. Anwendung: LRU-Approximation,
// Hot-Key-Promotion (z.B. fuer Eviction-Queues, wo der "wichtigste" Eintrag zuerst
// geflusht/erhalten werden soll).
//
// **Erste Strategie mit supports_priority_ordering()=true** — Markant fuer
// PermutationEngine: nur Strategien mit dieser Property koennen in
// Hot-Key/Eviction-Pfaden eingesetzt werden.
//
// Allocation: std::priority_queue baut auf std::vector — Heap-Wachstum kann
// std::bad_alloc werfen ([[allocation-failure-exception]]).

#include "axis_q1_queuing_base.hpp"
#include "axis_q1_queuing_subaxes_qs1_to_qs6.hpp"
#include "concepts/axis_q1_queuing_concept.hpp"
#include "concepts/axis_q1_queuing_cache_engine_permutation_concept.hpp"
#include "../concepts/topic_queuing_concept.hpp"

#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <queue>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::queuing::axis_q1_queuing {

class PriorityHeapBuffer : public BufferStrategyBase<PriorityHeapBuffer> {
public:
    static constexpr bool enabled = flags::priority_heap_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::ordered_access_tag;
    using family_id    = std::integral_constant<int, 6>;  // Q06

    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return false; }
    [[nodiscard]] static constexpr bool        is_bounded()        noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t default_capacity()  noexcept { return 0; }  // unbounded
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "priority_heap"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "PriorityHeapBuffer (Max-Heap, LRU-Approx + Hot-Key Promotion)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "PRIORITY_HEAP"; }

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    /// SONDERFALL: erste Q1-Strategie mit Priority-Ordering=TRUE.
    [[nodiscard]] static constexpr bool supports_priority_ordering()    noexcept { return true; }
    [[nodiscard]] static constexpr bool is_versioned()                  noexcept { return false; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    [[nodiscard]] bool operator==(PriorityHeapBuffer const& other) const noexcept {
        return heap_.size() == other.heap_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: priority_queue::push wirft
    /// std::bad_alloc bei OOM. Nicht noexcept.
    void put(element_type v) {
        heap_.push(v);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        if (heap_.size() > stats_.peak_size) stats_.peak_size = heap_.size();
        observer_.notify(stats_);
#endif
    }

    /// Entfernt + liefert MAX-Element (Hot-Key first).
    [[nodiscard]] std::optional<element_type> get() {
        if (heap_.empty()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.underflow_count;
            observer_.notify(stats_);
#endif
            return std::nullopt;
        }
        element_type v = heap_.top();
        heap_.pop();
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_get_count;
        observer_.notify(stats_);
#endif
        return v;
    }

    [[nodiscard]] size_type size()     const noexcept { return heap_.size(); }
    [[nodiscard]] bool      is_empty() const noexcept { return heap_.empty(); }
    void                    clear()          noexcept { heap_ = decltype(heap_){}; }

    // std::queue-API auf max-heap:
    //   peek_front=highest priority (top, was als naechstes get() liefert)
    //   peek_back=lowest priority (Approximation — std::priority_queue exponiert kein min direkt)
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept {
        if (heap_.empty()) return std::nullopt;
        return heap_.top();
    }
    /// Approximation: lineare Suche nach min im Heap-internen Container.
    /// std::priority_queue exponiert keinen Iterator — wir nutzen die unsichere
    /// (aber portable) c-Member-Konvention NICHT, sondern duplizieren via O(N) scan.
    [[nodiscard]] std::optional<element_type> peek_back() const noexcept {
        if (heap_.empty()) return std::nullopt;
        // Tradeoff: O(N) scan ist akzeptabel da peek_back() bei PriorityHeapBuffer selten genutzt
        // (typisch nur Diagnostik). Wir kopieren den Heap einmalig und drainen.
        auto copy = heap_;
        element_type min_val = copy.top();
        while (!copy.empty()) {
            element_type v = copy.top();
            if (v < min_val) min_val = v;
            copy.pop();
        }
        return min_val;
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
    std::priority_queue<element_type> heap_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::queuing::axis_q1_queuing {
    static_assert(concepts::BufferStrategy<PriorityHeapBuffer>);
    static_assert(concepts::CacheEngineBufferPermutationStrategy<PriorityHeapBuffer>);
}
