#pragma once
// V41.F.6.1 F.6 axis_01 DenseBytePageType (Seitentyp 1: dichte byte-indizierte Seite)

#include "axis_01_page_type_strategy_base.hpp"
#include "axis_01_page_type_subaxes_pg1_to_pg3.hpp"
#include "concepts/axis_01_page_type_cache_engine_permutation_concept.hpp"
#include "axis_01_page_type_flags.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_01_page_type {

/// DenseBytePageType — dichte byte-indizierte Verzweigungsseite (1 Lookup, direct-addressed).
class DenseBytePageType : public PageTypeStrategyBase<DenseBytePageType> {
public:
    using topic_tag = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag  = subaxes::structure_role_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::dense_byte_enabled;

    [[nodiscard]] static constexpr concepts::PageKind page_kind() noexcept { return concepts::PageKind::DenseByte; }
    [[nodiscard]] static constexpr bool               is_branch() noexcept { return true; }
    [[nodiscard]] static constexpr bool               is_leaf() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view   name() noexcept { return "page_dense_byte"; }
    [[nodiscard]] static constexpr std::string_view   family_name() noexcept {
        return "DenseBytePageType (dense byte-indexed branch page, direct-addressed)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "DENSE_BYTE"; }
};

} // namespace comdare::cache_engine::nodes::axis_01_page_type

namespace comdare::cache_engine::nodes::axis_01_page_type {
static_assert(concepts::PageTypeStrategy<DenseBytePageType>);
static_assert(concepts::CacheEnginePermutationStrategy<DenseBytePageType>);
} // namespace comdare::cache_engine::nodes::axis_01_page_type
