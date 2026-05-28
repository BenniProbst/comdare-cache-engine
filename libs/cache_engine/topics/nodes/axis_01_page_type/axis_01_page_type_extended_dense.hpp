#pragma once
// V41.F.6.1 F.6 axis_01 ExtendedDensePageType (Seitentyp 2: erweiterte dichte/mehrstufige Seite)

#include "axis_01_page_type_strategy_base.hpp"
#include "axis_01_page_type_subaxes_pg1_to_pg3.hpp"
#include "concepts/axis_01_page_type_cache_engine_permutation_concept.hpp"
#include "axis_01_page_type_flags.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_01_page_type {

/// ExtendedDensePageType — erweiterte dichte Seite (>256 Slots / mehrstufige Indexierung).
class ExtendedDensePageType : public PageTypeStrategyBase<ExtendedDensePageType> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::density_class_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::extended_dense_enabled;

    [[nodiscard]] static constexpr concepts::PageKind page_kind() noexcept { return concepts::PageKind::ExtendedDense; }
    [[nodiscard]] static constexpr bool             is_branch()   noexcept { return true; }
    [[nodiscard]] static constexpr bool             is_leaf()     noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "page_extended_dense"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "ExtendedDensePageType (extended/multi-level dense page, >256 slots)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "EXTENDED_DENSE"; }
};

}  // namespace

namespace comdare::cache_engine::nodes::axis_01_page_type {
    static_assert(concepts::PageTypeStrategy<ExtendedDensePageType>);
    static_assert(concepts::CacheEnginePermutationStrategy<ExtendedDensePageType>);
}
