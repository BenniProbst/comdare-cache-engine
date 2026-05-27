#pragma once
// V41.F.6.1.R7.5.f axis_io MmapIo (mmap-based, Persistent Memory)

#include "axis_io_strategy_base.hpp"
#include "axis_io_subaxes_io1_to_io3.hpp"
#include "concepts/axis_io_cache_engine_permutation_concept.hpp"
#include "axis_io_flags.hpp"
#include "../concepts/topic_io_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::io::axis_io {

/// MmapIo — mmap()-based File-Backed Memory.
/// Optimal fuer Persistent Memory (Intel Optane) + Read-Heavy Workloads.
/// OS pageret automatisch. Kein Copy zwischen User+Kernel-Space.
/// Trade-off: page-fault-Latenz spike vs Reuse-friendly.
class MmapIo : public IoStrategyBase<MmapIo> {
public:
    using topic_tag = ::comdare::cache_engine::io::concepts::IoTopicTag;
    using axis_tag  = subaxes::persistence_tag;
    using family_id = std::integral_constant<int, 4>;

    static constexpr bool enabled = flags::mmap_enabled;

    [[nodiscard]] static constexpr bool             is_in_memory_only() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()              noexcept { return "io_mmap"; }
    [[nodiscard]] static constexpr std::string_view family_name()       noexcept { return "MmapIo (mmap file-backed, Persistent Memory, read-heavy)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()       noexcept { return "MMAP"; }
};

}  // namespace

namespace comdare::cache_engine::io::axis_io {
    static_assert(concepts::IoStrategy<MmapIo>);
    static_assert(concepts::CacheEnginePermutationStrategy<MmapIo>);
}
