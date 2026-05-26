#pragma once
// V41.F.6.1 axis_q1_queuing NoBuffer Q-NONE (2026-05-26)
//
// @topic queuing @achse Q1 @family Q01 NoBuffer (Passthrough)
// @subaxis QS1 sequential_access
//
// Identitaets-Strategie: keine Pufferung. put() ist no-op, get() liefert
// immer std::nullopt. Sinnvoll als Baseline-Vergleich + im B+/ART
// klassisch (direkt-zum-Leaf).

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

namespace comdare::cache_engine::queuing::axis_q1_queuing {

class NoBuffer : public BufferStrategyBase<NoBuffer> {
public:
    static constexpr bool enabled = flags::no_buffer_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::sequential_access_tag;
    using family_id    = std::integral_constant<int, 1>;  // Q01

    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return true; }  // no-op = trivially TS
    [[nodiscard]] static constexpr bool        is_bounded()        noexcept { return true; }  // Kapazitaet 0
    [[nodiscard]] static constexpr std::size_t default_capacity()  noexcept { return 0; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "no_buffer"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "NoBuffer Passthrough (B+/ART direkt-zum-Leaf)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "NO_BUFFER"; }

    // Vendor-Sonderfall-Properties ([[vendor-sonderfaelle-als-pflicht-property]])
    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return true; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return true; }
    [[nodiscard]] static constexpr bool supports_priority_ordering()    noexcept { return false; }
    [[nodiscard]] static constexpr bool is_versioned()                  noexcept { return false; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::WaitFree;  // no-op = trivially wait-free
    }

    [[nodiscard]] bool operator==(NoBuffer const&) const noexcept { return true; }

    void put(element_type v) {
        (void)v;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<element_type> get() {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.underflow_count;
        observer_.notify(stats_);
#endif
        return std::nullopt;
    }

    [[nodiscard]] size_type size()     const noexcept { return 0; }
    [[nodiscard]] bool      is_empty() const noexcept { return true; }
    void                    clear()          noexcept {}

    // std::queue-API analog (Passthrough: alle Peek-Calls liefern nullopt)
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept { return std::nullopt; }
    [[nodiscard]] std::optional<element_type> peek_back()  const noexcept { return std::nullopt; }
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
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::queuing::axis_q1_queuing {
    static_assert(concepts::BufferStrategy<NoBuffer>);
    static_assert(concepts::CacheEngineBufferPermutationStrategy<NoBuffer>);
}
