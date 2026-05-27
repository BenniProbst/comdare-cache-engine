#pragma once
// V41.F.6.1.R7.5.c axis_10 RawBinarySerialization (Goldstandard-Update)

#include "axis_10_serialization_strategy_base.hpp"
#include "axis_10_serialization_subaxes_sr1_to_sr3.hpp"
#include "concepts/axis_10_serialization_cache_engine_permutation_concept.hpp"
#include "axis_10_serialization_flags.hpp"
#include "../concepts/topic_serialization_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::serialization::axis_10_serialization {

/// RawBinarySerialization — Default: memcpy raw bytes (baseline).
class RawBinarySerialization : public SerializationStrategyBase<RawBinarySerialization> {
public:
    using topic_tag = ::comdare::cache_engine::serialization::concepts::SerializationTopicTag;
    using axis_tag  = subaxes::byte_order_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::raw_binary_enabled;

    [[nodiscard]] static constexpr bool             supports_compression() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "serialization_raw_binary"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "RawBinarySerialization (memcpy raw bytes baseline)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "RAW_BINARY"; }
};

}  // namespace

namespace comdare::cache_engine::serialization::axis_10_serialization {
    static_assert(concepts::SerializationStrategy<RawBinarySerialization>);
    static_assert(concepts::CacheEnginePermutationStrategy<RawBinarySerialization>);
}
