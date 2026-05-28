#pragma once
// V41.F.6.1 F.6 axis_01 SparsePatriciaPageType (Seitentyp 3: spaerlich, Patricia-komprimiert)

#include "axis_01_page_type_strategy_base.hpp"
#include "axis_01_page_type_subaxes_pg1_to_pg3.hpp"
#include "concepts/axis_01_page_type_cache_engine_permutation_concept.hpp"
#include "axis_01_page_type_flags.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_01_page_type {

/// SparsePatriciaPageType — spaerlich besetzte Seite mit Patricia-Pfadkompression.
class SparsePatriciaPageType : public PageTypeStrategyBase<SparsePatriciaPageType> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::density_class_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::sparse_patricia_enabled;

    [[nodiscard]] static constexpr concepts::PageKind page_kind() noexcept { return concepts::PageKind::SparsePatricia; }
    [[nodiscard]] static constexpr bool             is_branch()   noexcept { return true; }
    [[nodiscard]] static constexpr bool             is_leaf()     noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "page_sparse_patricia"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "SparsePatriciaPageType (sparse Patricia-compressed page)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "SPARSE_PATRICIA"; }
};

}  // namespace

namespace comdare::cache_engine::nodes::axis_01_page_type {
    static_assert(concepts::PageTypeStrategy<SparsePatriciaPageType>);
    static_assert(concepts::CacheEnginePermutationStrategy<SparsePatriciaPageType>);
}
