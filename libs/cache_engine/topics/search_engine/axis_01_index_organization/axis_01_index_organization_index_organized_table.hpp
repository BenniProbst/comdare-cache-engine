#pragma once
// V41.F.6.1.R7.5.h axis_01 IotIndexOrganization (IOT, Daten in Leaf-Pages)

#include "axis_01_index_organization_strategy_base.hpp"
#include "axis_01_index_organization_subaxes_io1_to_io3.hpp"
#include "concepts/axis_01_index_organization_cache_engine_permutation_concept.hpp"
#include "axis_01_index_organization_flags.hpp"
#include "../concepts/topic_search_engine_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::search_engine::axis_01_index_organization {

/// IotIndexOrganization — Daten direkt in Index-Leaf-Pages eingebettet (Oracle IOT).
/// Spart Pointer-Indirektion vs NonClustered. Aequivalent zu SQL Server
/// CLUSTERED INDEX (Daten in B+Tree-Leaves). Standard fuer ART/HOT/Wormhole
/// (Daten direkt im Trie-Knoten).
class IotIndexOrganization : public IndexOrganizationStrategyBase<IotIndexOrganization> {
public:
    using topic_tag = ::comdare::cache_engine::search_engine::concepts::SearchEngineTopicTag;
    using axis_tag  = subaxes::data_embedding_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::index_organized_table_enabled;

    [[nodiscard]] static constexpr bool             is_clustered()          noexcept { return true; }
    [[nodiscard]] static constexpr bool             has_secondary_indexes() noexcept { return true; }
    [[nodiscard]] static constexpr bool             data_embedded_in_leaf() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()                  noexcept { return "index_org_index_organized_table"; }
    [[nodiscard]] static constexpr std::string_view family_name()           noexcept { return "IotIndexOrganization (Oracle IOT, Daten in B+Tree-Leaves, ART/HOT-style)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()           noexcept { return "INDEX_ORGANIZED_TABLE"; }
};

}  // namespace

namespace comdare::cache_engine::search_engine::axis_01_index_organization {
    static_assert(concepts::IndexOrganizationStrategy<IotIndexOrganization>);
    static_assert(concepts::CacheEnginePermutationStrategy<IotIndexOrganization>);
}
