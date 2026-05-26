#pragma once
// V41.F.6.1.F3 axis_01 StdMapLike Default-Wrapper (Sub 01a sorted+unique+ordered)

#include "axis_01_index_organization_base.hpp"
#include "../concepts/topic_search_engine_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::search_engine::axis_01_index_organization {
class StdMapLike : public IndexOrganizationBase<StdMapLike> {
public:
    using topic_tag = ::comdare::cache_engine::search_engine::concepts::SearchEngineTopicTag;
    using family_id = std::integral_constant<int, 1>;
    [[nodiscard]] static constexpr bool is_ordered() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "std_map_like"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "StdMapLike (sorted+unique+ordered, Sub 01a)"; }
};
}  // namespace
namespace comdare::cache_engine::search_engine::axis_01_index_organization {
    static_assert(concepts::IndexOrganizationStrategy<StdMapLike>);
}
