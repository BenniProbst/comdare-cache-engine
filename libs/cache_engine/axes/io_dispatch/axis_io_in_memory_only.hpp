#pragma once
// V41.F.6.1.R7.5.f axis_io InMemoryOnly (Goldstandard-Update)

#include "axis_io_strategy_base.hpp"
#include "axis_io_subaxes_io1_to_io3.hpp"
#include "concepts/axis_io_cache_engine_permutation_concept.hpp"
#include <axes/io_dispatch/axis_io_flags.hpp>
#include <topics/io/concepts/topic_io_concept.hpp>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::io_dispatch {

/// InMemoryOnly — Default: kein IO, alles im RAM (Pure In-Memory Index).
/// Baseline fuer Mess-Reihen ohne Persistence-Overhead.
class InMemoryOnly : public IoStrategyBase<InMemoryOnly> {
public:
    using topic_tag = ::comdare::cache_engine::io::concepts::IoTopicTag;
    using axis_tag  = subaxes::persistence_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::in_memory_only_enabled;

    [[nodiscard]] static constexpr bool             is_in_memory_only() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()              noexcept { return "io_in_memory_only"; }
    [[nodiscard]] static constexpr std::string_view family_name()       noexcept { return "InMemoryOnly (no IO, RAM-only baseline)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()       noexcept { return "IN_MEMORY_ONLY"; }
};

}  // namespace

namespace comdare::cache_engine::io_dispatch {
    static_assert(concepts::IoStrategy<InMemoryOnly>);
    static_assert(concepts::CacheEnginePermutationStrategy<InMemoryOnly>);
}
