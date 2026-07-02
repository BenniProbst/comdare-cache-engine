#pragma once
// DOSSIER W1/234-K axis_bst_shape PTR_SIZE_T (Level-0 Default)

#include "axis_bst_shape_flags.hpp"
#include "axis_bst_shape_strategy_base.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_bst_shape {

class BstPtrSizeT : public BstShapeStrategyBase<BstPtrSizeT> {
public:
    using topic_tag  = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag   = bst_shape_family_tag;
    using family_id  = std::integral_constant<int, 0>;
    using index_type = std::size_t;

    static constexpr bool        enabled     = flags::ptr_size_t_enabled;
    static constexpr std::size_t kIndexBytes = sizeof(index_type);

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "bst_ptr_size_t"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "BstPtrSizeT"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "PTR_SIZE_T"; }
};

static_assert(concepts::BstShape<BstPtrSizeT>);
static_assert(concepts::CacheEnginePermutationStrategy<BstPtrSizeT>);

} // namespace comdare::cache_engine::nodes::axis_bst_shape