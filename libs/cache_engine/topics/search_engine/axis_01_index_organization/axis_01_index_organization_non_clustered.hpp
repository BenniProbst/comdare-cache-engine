#pragma once
// V41.F.6.1.R7.5.h axis_01 NonClusteredIndexOrganization (Secondary Index)

#include "axis_01_index_organization_strategy_base.hpp"
#include "axis_01_index_organization_subaxes_io1_to_io3.hpp"
#include "concepts/axis_01_index_organization_cache_engine_permutation_concept.hpp"
#include "axis_01_index_organization_flags.hpp"
#include "../concepts/topic_search_engine_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::search_engine::axis_01_index_organization {

/// NonClusteredIndexOrganization — Index-Order != Storage-Order, N Secondary-Indizes.
/// SQL Server NONCLUSTERED, PostgreSQL CREATE INDEX. Lookup ueber Index +
/// zusaetzlicher Pointer-Hop zur Daten-Row. Standard fuer OLTP-Workloads mit
/// multiplen Suchfeldern.
class NonClusteredIndexOrganization : public IndexOrganizationStrategyBase<NonClusteredIndexOrganization> {
public:
    using topic_tag = ::comdare::cache_engine::search_engine::concepts::SearchEngineTopicTag;
    using axis_tag  = subaxes::index_count_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::non_clustered_enabled;

    [[nodiscard]] static constexpr bool             is_clustered()          noexcept { return false; }
    [[nodiscard]] static constexpr bool             has_secondary_indexes() noexcept { return true; }
    [[nodiscard]] static constexpr bool             data_embedded_in_leaf() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()                  noexcept { return "index_org_non_clustered"; }
    [[nodiscard]] static constexpr std::string_view family_name()           noexcept { return "NonClusteredIndexOrganization (Secondary Index, N-pro-Tabelle, SQL Server/PostgreSQL)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()           noexcept { return "NON_CLUSTERED"; }
};

}  // namespace

namespace comdare::cache_engine::search_engine::axis_01_index_organization {
    static_assert(concepts::IndexOrganizationStrategy<NonClusteredIndexOrganization>);
    static_assert(concepts::CacheEnginePermutationStrategy<NonClusteredIndexOrganization>);
}
