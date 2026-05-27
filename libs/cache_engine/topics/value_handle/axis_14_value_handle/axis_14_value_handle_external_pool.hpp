#pragma once
// V41.F.6.1.R7.5.d axis_14 ExternalPoolHandle (Wormhole)

#include "axis_14_value_handle_strategy_base.hpp"
#include "axis_14_value_handle_subaxes_vh1_to_vh3.hpp"
#include "concepts/axis_14_value_handle_cache_engine_permutation_concept.hpp"
#include "axis_14_value_handle_flags.hpp"
#include "../concepts/topic_value_handle_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::value_handle::axis_14_value_handle {

/// ExternalPoolHandle — Value extern in Pool, Node speichert nur Pool-Offset.
/// Standard fuer Wormhole (Wu SIGMOD 2019): kompakte Nodes + Variable-Size
/// Values via Pool. Pointer-Indirektion kostet 1 Cache-Miss pro Lookup.
class ExternalPoolHandle : public ValueHandleStrategyBase<ExternalPoolHandle> {
public:
    using topic_tag = ::comdare::cache_engine::value_handle::concepts::ValueHandleTopicTag;
    using axis_tag  = subaxes::storage_location_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::external_pool_enabled;

    [[nodiscard]] static constexpr bool             is_inline()    noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "value_handle_external_pool"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "ExternalPoolHandle (Wormhole pool-offset, variable-size values)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "EXTERNAL_POOL"; }
};

}  // namespace

namespace comdare::cache_engine::value_handle::axis_14_value_handle {
    static_assert(concepts::ValueHandleStrategy<ExternalPoolHandle>);
    static_assert(concepts::CacheEnginePermutationStrategy<ExternalPoolHandle>);
}
