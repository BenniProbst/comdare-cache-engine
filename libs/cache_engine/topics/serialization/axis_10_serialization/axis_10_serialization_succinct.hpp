#pragma once
// V41.F.6.1.R7.5.c axis_10 SuccinctSerialization (LOUDS/SuRF)

#include "axis_10_serialization_strategy_base.hpp"
#include "axis_10_serialization_subaxes_sr1_to_sr3.hpp"
#include "concepts/axis_10_serialization_cache_engine_permutation_concept.hpp"
#include "axis_10_serialization_flags.hpp"
#include "../concepts/topic_serialization_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::serialization::axis_10_serialization {

/// SuccinctSerialization — Bit-Packed Succinct Encoding (LOUDS/SuRF).
/// Approaches information-theoretic lower bound (n*H + o(n) bits).
/// Read-only nach Build, optimal fuer Approx-Filter (SuRF) + Read-Heavy.
class SuccinctSerialization : public SerializationStrategyBase<SuccinctSerialization> {
public:
    using topic_tag = ::comdare::cache_engine::serialization::concepts::SerializationTopicTag;
    using axis_tag  = subaxes::density_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::succinct_enabled;

    [[nodiscard]] static constexpr bool             supports_compression() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "serialization_succinct"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "SuccinctSerialization (LOUDS/SuRF bit-packed, n*H + o(n) bits)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "SUCCINCT"; }
};

}  // namespace

namespace comdare::cache_engine::serialization::axis_10_serialization {
    static_assert(concepts::SerializationStrategy<SuccinctSerialization>);
    static_assert(concepts::CacheEnginePermutationStrategy<SuccinctSerialization>);
}
