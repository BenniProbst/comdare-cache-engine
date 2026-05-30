#pragma once
// V41.F.6.1.R7.5.f axis_io BufferedIo (OS Page-Cache standard)

#include "axis_io_strategy_base.hpp"
#include "axis_io_subaxes_io1_to_io3.hpp"
#include "concepts/axis_io_cache_engine_permutation_concept.hpp"
#include <axes/io_dispatch/axis_io_flags.hpp>
#include <topics/io/concepts/topic_io_concept.hpp>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::io_dispatch {

/// BufferedIo — Standard read()/write() via OS-Page-Cache.
/// Default fuer general-purpose DBMS. OS managed read-ahead + write-back.
/// Trade-off: bequem aber double-buffering (App-Cache + OS-Cache).
class BufferedIo : public IoStrategyBase<BufferedIo> {
public:
    using topic_tag = ::comdare::cache_engine::io::concepts::IoTopicTag;
    using axis_tag  = subaxes::caching_strategy_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::buffered_enabled;

    [[nodiscard]] static constexpr bool             is_in_memory_only() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()              noexcept { return "io_buffered"; }
    [[nodiscard]] static constexpr std::string_view family_name()       noexcept { return "BufferedIo (OS page-cache, read-ahead + write-back, default)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()       noexcept { return "BUFFERED"; }
};

}  // namespace

namespace comdare::cache_engine::io_dispatch {
    static_assert(concepts::IoStrategy<BufferedIo>);
    static_assert(concepts::CacheEnginePermutationStrategy<BufferedIo>);
}
