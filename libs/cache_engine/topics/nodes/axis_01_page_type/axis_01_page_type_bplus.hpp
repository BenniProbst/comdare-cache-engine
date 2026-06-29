#pragma once
// V41.F.6.1 F.6 axis_01 BPlusPageType (Seitentyp 6: B+-artige Verzweigungsseite, Masstree-Familie)

#include "axis_01_page_type_strategy_base.hpp"
#include "axis_01_page_type_subaxes_pg1_to_pg3.hpp"
#include "concepts/axis_01_page_type_cache_engine_permutation_concept.hpp"
#include "axis_01_page_type_flags.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_01_page_type {

/// BPlusPageType — B+-artige sortierte Verzweigungsseite (Masstree-Border/Internal-Familie).
class BPlusPageType : public PageTypeStrategyBase<BPlusPageType> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::structure_role_tag;
    using family_id = std::integral_constant<int, 6>;

    static constexpr bool enabled = flags::bplus_enabled;

    [[nodiscard]] static constexpr concepts::PageKind page_kind() noexcept { return concepts::PageKind::BPlus; }
    [[nodiscard]] static constexpr bool               is_branch() noexcept { return true; }
    [[nodiscard]] static constexpr bool               is_leaf() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view   name() noexcept { return "page_bplus"; }
    [[nodiscard]] static constexpr std::string_view   family_name() noexcept {
        return "BPlusPageType (sorted B+-style branch page, Masstree family)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "BPLUS"; }
};

} // namespace comdare::cache_engine::nodes::axis_01_page_type

namespace comdare::cache_engine::nodes::axis_01_page_type {
static_assert(concepts::PageTypeStrategy<BPlusPageType>);
static_assert(concepts::CacheEnginePermutationStrategy<BPlusPageType>);
} // namespace comdare::cache_engine::nodes::axis_01_page_type
