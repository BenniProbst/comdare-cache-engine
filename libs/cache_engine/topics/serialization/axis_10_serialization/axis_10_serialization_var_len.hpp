#pragma once
// V41.F.6.1.R7.5.c axis_10 VarLenSerialization (ART signaling-bits VarInt)

#include "axis_10_serialization_strategy_base.hpp"
#include "axis_10_serialization_subaxes_sr1_to_sr3.hpp"
#include "concepts/axis_10_serialization_cache_engine_permutation_concept.hpp"
#include "axis_10_serialization_flags.hpp"
#include "../concepts/topic_serialization_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::serialization::axis_10_serialization {

/// VarLenSerialization — Variable-Length-Encoding mit Signaling-Bits.
/// Standard fuer ART (Leis ICDE 2013): kleine Werte = wenige Bytes,
/// signaling-bits markieren Laenge. Typisch fuer integer-Schluessel.
class VarLenSerialization : public SerializationStrategyBase<VarLenSerialization> {
public:
    using topic_tag = ::comdare::cache_engine::serialization::concepts::SerializationTopicTag;
    using axis_tag  = subaxes::byte_order_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::var_len_enabled;

    [[nodiscard]] static constexpr bool             supports_compression() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()                 noexcept { return "serialization_var_len"; }
    [[nodiscard]] static constexpr std::string_view family_name()          noexcept { return "VarLenSerialization (ART signaling-bits VarInt encoding)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()          noexcept { return "VAR_LEN"; }
};

}  // namespace

namespace comdare::cache_engine::serialization::axis_10_serialization {
    static_assert(concepts::SerializationStrategy<VarLenSerialization>);
    static_assert(concepts::CacheEnginePermutationStrategy<VarLenSerialization>);
}
