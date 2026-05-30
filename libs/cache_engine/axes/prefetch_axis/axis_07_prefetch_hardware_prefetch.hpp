#pragma once
// V41.F.6.1.R7.5.a axis_07 HardwarePrefetch (Wormhole)
//
// R7.6 Paper-Reference (Task #723):
// Wu, X., Ni, F., Jiang, S. "Wormhole: A Fast Ordered Index for In-memory
// Data Management." Proceedings of EuroSys 2019.
// DOI: 10.1145/3302424.3303955
// URL: https://dl.acm.org/doi/10.1145/3302424.3303955
// Code: https://github.com/wuxb45/wormhole (GPL-3.0 License — blockiert Linking)
//
// Original-Pattern: PREFETCHT0 explizit auf Hash-Anchor-Chain ohne Software-
// Heuristik (CPU schaetzt Distance autonom). Implementiert via __builtin_prefetch.

#include "axis_07_prefetch_strategy_base.hpp"
#include "axis_07_prefetch_subaxes_pf1_to_pf3.hpp"
#include "concepts/axis_07_prefetch_cache_engine_permutation_concept.hpp"
#include <axes/prefetch_axis/axis_07_prefetch_flags.hpp>
#include <topics/prefetch/concepts/topic_prefetch_concept.hpp>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::prefetch_axis {

/// HardwarePrefetch — Explizite HW-Prefetch-Instructions (PREFETCHT0/T1/T2/NTA).
/// Standard fuer Wormhole (Wu SIGMOD 2019): nutzt PREFETCHT0 fuer Hash-Anchor-
/// Chain ohne Software-Heuristik. CPU schaetzt Distance autonom.
class HardwarePrefetch : public PrefetchStrategyBase<HardwarePrefetch> {
public:
    using topic_tag = ::comdare::cache_engine::prefetch::concepts::PrefetchTopicTag;
    using axis_tag  = subaxes::trigger_mechanism_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::hardware_prefetch_enabled;

    [[nodiscard]] static constexpr bool             is_active()    noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "prefetch_hardware"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "HardwarePrefetch (Wormhole PREFETCHT0, CPU-managed distance)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "HARDWARE_PREFETCH"; }
};

}  // namespace

namespace comdare::cache_engine::prefetch_axis {
    static_assert(concepts::PrefetchStrategy<HardwarePrefetch>);
    static_assert(concepts::CacheEnginePermutationStrategy<HardwarePrefetch>);
}
