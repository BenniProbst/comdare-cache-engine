#pragma once
// V41.F.6.1.F2 axis_10 RawBinarySerialization Default-Wrapper

#include "axis_10_serialization_base.hpp"
#include "../concepts/topic_serialization_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::serialization::axis_10_serialization {
class RawBinarySerialization : public SerializationBase<RawBinarySerialization> {
public:
    using topic_tag = ::comdare::cache_engine::serialization::concepts::SerializationTopicTag;
    using family_id = std::integral_constant<int, 1>;
    [[nodiscard]] static constexpr bool supports_compression() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "raw_binary"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "RawBinarySerialization (memcpy raw bytes baseline)"; }
};
}  // namespace
namespace comdare::cache_engine::serialization::axis_10_serialization {
    static_assert(concepts::SerializationStrategy<RawBinarySerialization>);
}
