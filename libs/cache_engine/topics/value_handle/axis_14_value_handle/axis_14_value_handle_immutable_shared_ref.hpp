#pragma once
// V41.F.6.1.R7.5.d axis_14 ImmutableSharedRefValueHandle (RCU-style sharing)

#include "axis_14_value_handle_strategy_base.hpp"
#include "axis_14_value_handle_subaxes_vh1_to_vh3.hpp"
#include "concepts/axis_14_value_handle_cache_engine_permutation_concept.hpp"
#include "axis_14_value_handle_flags.hpp"
#include "../concepts/topic_value_handle_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::value_handle::axis_14_value_handle {

/// ImmutableSharedRefValueHandle — Immutable Value shared via reference-counted
/// Pointer. Optimal fuer RCU-style Concurrency (Reader sieht stabile Snapshots,
/// Writer alloziert neuen Snapshot). Default fuer Persistent ART/Tries.
class ImmutableSharedRefValueHandle : public ValueHandleStrategyBase<ImmutableSharedRefValueHandle> {
public:
    using topic_tag = ::comdare::cache_engine::value_handle::concepts::ValueHandleTopicTag;
    using axis_tag  = subaxes::ownership_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::immutable_shared_ref_enabled;

    [[nodiscard]] static constexpr bool             is_inline()    noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "value_handle_immutable_shared_ref"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "ImmutableSharedRefValueHandle (RCU shared_ptr, snapshot-stable readers)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "IMMUTABLE_SHARED_REF"; }
};

}  // namespace

namespace comdare::cache_engine::value_handle::axis_14_value_handle {
    static_assert(concepts::ValueHandleStrategy<ImmutableSharedRefValueHandle>);
    static_assert(concepts::CacheEnginePermutationStrategy<ImmutableSharedRefValueHandle>);
}
