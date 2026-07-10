#pragma once
// V41.F.6.1.R7.3 axis_08 ReaderWriterConcurrency (shared_mutex, 1-W/N-R)

#include "axis_08_concurrency_strategy_base.hpp"
#include "axis_08_concurrency_subaxes_cc1_to_cc2.hpp"
#include "concepts/axis_08_concurrency_cache_engine_permutation_concept.hpp"
#include <axes/concurrency_axis/axis_08_concurrency_flags.hpp>
#include <topics/concurrency/concepts/topic_concurrency_concept.hpp>
#include <shared_mutex>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::concurrency_axis {

/// ReaderWriterConcurrency — Single-Writer/Multi-Reader via std::shared_mutex.
/// Allocator-Basisdisziplin A3-Default. Gut bei read-lastigen Workloads.
class ReaderWriterConcurrency : public ConcurrencyStrategyBase<ReaderWriterConcurrency> {
public:
    using topic_tag = ::comdare::cache_engine::concurrency::concepts::ConcurrencyTopicTag;
    using axis_tag  = subaxes::synchronization_pattern_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::reader_writer_enabled;

    [[nodiscard]] static constexpr concepts::ConcurrencyPattern concurrency_pattern() noexcept {
        return concepts::ConcurrencyPattern::ReaderWriter;
    }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "concurrency_reader_writer"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "ReaderWriterConcurrency (std::shared_mutex, single-writer/multi-reader)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "READER_WRITER"; }

    // V41 F15 Pfad-A — treibbare Concurrency-Op (acquire/release-Paar). ReaderWriter behaelt die
    // strategie-definierende SHARED-(Reader-)Bahn (distinkter Mittelwert ggue. Blocking); try_acquire()
    // zaehlt echte Contention NUR bei Writer-Konkurrenz (try_lock_shared scheitert nicht an Readern).
    static bool try_acquire() noexcept { return lock_().try_lock_shared(); }
    static void acquire() noexcept { lock_().lock_shared(); }
    static void release() noexcept { lock_().unlock_shared(); }

private:
    [[nodiscard]] static std::shared_mutex& lock_() noexcept {
        static std::shared_mutex m;
        return m;
    }
};

} // namespace comdare::cache_engine::concurrency_axis

namespace comdare::cache_engine::concurrency_axis {
static_assert(concepts::ConcurrencyStrategy<ReaderWriterConcurrency>);
static_assert(concepts::CacheEnginePermutationStrategy<ReaderWriterConcurrency>);
} // namespace comdare::cache_engine::concurrency_axis
