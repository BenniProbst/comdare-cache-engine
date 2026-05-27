#pragma once
// V41.F.6.1.R7.5.h axis_01 HeapOrganization (baseline kein Index)

#include "axis_01_index_organization_strategy_base.hpp"
#include "axis_01_index_organization_subaxes_io1_to_io3.hpp"
#include "concepts/axis_01_index_organization_cache_engine_permutation_concept.hpp"
#include "axis_01_index_organization_flags.hpp"
#include "../concepts/topic_search_engine_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::search_engine::axis_01_index_organization {

/// HeapOrganization — Storage-Order = Insert-Order, KEIN Index (Baseline).
/// PostgreSQL Heap (unbenannt), HBase Heap-Tables. Lookup = Full-Scan O(n).
class HeapOrganization : public IndexOrganizationStrategyBase<HeapOrganization> {
public:
    using topic_tag = ::comdare::cache_engine::search_engine::concepts::SearchEngineTopicTag;
    using axis_tag  = subaxes::storage_order_tag;
    using family_id = std::integral_constant<int, 0>;

    static constexpr bool enabled = flags::heap_enabled;

    [[nodiscard]] static constexpr bool             is_clustered()          noexcept { return false; }
    [[nodiscard]] static constexpr bool             has_secondary_indexes() noexcept { return false; }
    [[nodiscard]] static constexpr bool             data_embedded_in_leaf() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()                  noexcept { return "index_org_heap"; }
    [[nodiscard]] static constexpr std::string_view family_name()           noexcept { return "HeapOrganization (no index, storage=insert-order, baseline)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()           noexcept { return "HEAP"; }
};

}  // namespace

namespace comdare::cache_engine::search_engine::axis_01_index_organization {
    static_assert(concepts::IndexOrganizationStrategy<HeapOrganization>);
    static_assert(concepts::CacheEnginePermutationStrategy<HeapOrganization>);
}
