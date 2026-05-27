#pragma once
// V41.F.6.1.R7.5.h axis_01 ClusteredOrganization (Index-Order = Storage-Order)

#include "axis_01_index_organization_strategy_base.hpp"
#include "axis_01_index_organization_subaxes_io1_to_io3.hpp"
#include "concepts/axis_01_index_organization_cache_engine_permutation_concept.hpp"
#include "axis_01_index_organization_flags.hpp"
#include "../concepts/topic_search_engine_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::search_engine::axis_01_index_organization {

/// ClusteredOrganization — Storage-Order entspricht Index-Order (1 Primary Key).
/// MySQL InnoDB Default, PostgreSQL CLUSTER. Optimal fuer Range-Scans entlang
/// Primary-Key (sequentielle Disk-Reads). Daten getrennt vom Index-Tree.
class ClusteredOrganization : public IndexOrganizationStrategyBase<ClusteredOrganization> {
public:
    using topic_tag = ::comdare::cache_engine::search_engine::concepts::SearchEngineTopicTag;
    using axis_tag  = subaxes::storage_order_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::clustered_enabled;

    [[nodiscard]] static constexpr bool             is_clustered()          noexcept { return true; }
    [[nodiscard]] static constexpr bool             has_secondary_indexes() noexcept { return false; }
    [[nodiscard]] static constexpr bool             data_embedded_in_leaf() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()                  noexcept { return "index_org_clustered"; }
    [[nodiscard]] static constexpr std::string_view family_name()           noexcept { return "ClusteredOrganization (Index-Order = Storage-Order, MySQL InnoDB)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()           noexcept { return "CLUSTERED"; }
};

}  // namespace

namespace comdare::cache_engine::search_engine::axis_01_index_organization {
    static_assert(concepts::IndexOrganizationStrategy<ClusteredOrganization>);
    static_assert(concepts::CacheEnginePermutationStrategy<ClusteredOrganization>);
}
