#pragma once
// V41.F.6.1.R7.5.f axis_io DirectIo (O_DIRECT, NVMe-optimal)

#include "axis_io_strategy_base.hpp"
#include "axis_io_subaxes_io1_to_io3.hpp"
#include "concepts/axis_io_cache_engine_permutation_concept.hpp"
#include "axis_io_flags.hpp"
#include "../concepts/topic_io_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::io::axis_io {

/// DirectIo — O_DIRECT bypass OS-Page-Cache.
/// Optimal fuer NVMe-SSD + DBMS-eigene Cache-Strategien (RocksDB, MySQL).
/// 512B/4KB aligned, kein double-buffering. Hoehere CPU-Cost, vorhersehbare
/// Latenz (kein Cache-Pollution durch Background-Activity).
class DirectIo : public IoStrategyBase<DirectIo> {
public:
    using topic_tag = ::comdare::cache_engine::io::concepts::IoTopicTag;
    using axis_tag  = subaxes::caching_strategy_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::direct_enabled;

    [[nodiscard]] static constexpr bool             is_in_memory_only() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()              noexcept { return "io_direct"; }
    [[nodiscard]] static constexpr std::string_view family_name()       noexcept { return "DirectIo (O_DIRECT, bypass OS page-cache, NVMe-optimal)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()       noexcept { return "DIRECT"; }
};

}  // namespace

namespace comdare::cache_engine::io::axis_io {
    static_assert(concepts::IoStrategy<DirectIo>);
    static_assert(concepts::CacheEnginePermutationStrategy<DirectIo>);
}
