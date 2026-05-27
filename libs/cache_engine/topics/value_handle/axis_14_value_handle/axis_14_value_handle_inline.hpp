#pragma once
// V41.F.6.1.R7.5.d axis_14 InlineHandle (Goldstandard-Update)

#include "axis_14_value_handle_strategy_base.hpp"
#include "axis_14_value_handle_subaxes_vh1_to_vh3.hpp"
#include "concepts/axis_14_value_handle_cache_engine_permutation_concept.hpp"
#include "axis_14_value_handle_flags.hpp"
#include "../concepts/topic_value_handle_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::value_handle::axis_14_value_handle {

/// InlineHandle — Default: value embedded direkt in Node-Slot.
/// Standard fuer kleine Values (uint64_t). Vermeidet Pointer-Indirektion +
/// Cache-Miss bei Lookup. Trade-off: groessere Nodes.
class InlineHandle : public ValueHandleStrategyBase<InlineHandle> {
public:
    using topic_tag = ::comdare::cache_engine::value_handle::concepts::ValueHandleTopicTag;
    using axis_tag  = subaxes::storage_location_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::inline_enabled;

    [[nodiscard]] static constexpr bool             is_inline()    noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "value_handle_inline"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "InlineHandle (value embedded in slot, no indirection)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "INLINE"; }
};

}  // namespace

namespace comdare::cache_engine::value_handle::axis_14_value_handle {
    static_assert(concepts::ValueHandleStrategy<InlineHandle>);
    static_assert(concepts::CacheEnginePermutationStrategy<InlineHandle>);
}
