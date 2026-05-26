#pragma once
// V41.F.6.1.F3 axis_filter BloomFilter Default-Wrapper

#include "axis_filter_base.hpp"
#include "../concepts/topic_filter_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::filter::axis_filter {
class BloomFilter : public FilterBase<BloomFilter> {
public:
    using topic_tag = ::comdare::cache_engine::filter::concepts::FilterTopicTag;
    using family_id = std::integral_constant<int, 1>;
    [[nodiscard]] static constexpr bool supports_range_query() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "bloom_filter"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "BloomFilter (Bloom 1970, point-query only)"; }
};
}  // namespace
namespace comdare::cache_engine::filter::axis_filter {
    static_assert(concepts::FilterStrategy<BloomFilter>);
}
