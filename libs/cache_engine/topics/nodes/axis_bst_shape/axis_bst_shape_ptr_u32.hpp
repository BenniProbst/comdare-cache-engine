#pragma once
// DOSSIER W1/234-K axis_bst_shape PTR_U32

#include "axis_bst_shape_flags.hpp"
#include "axis_bst_shape_strategy_base.hpp"
#include "../concepts/topic_nodes_concept.hpp"
#include <cstdint>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::nodes::axis_bst_shape {

class BstPtrU32 : public BstShapeStrategyBase<BstPtrU32> {
public:
    using topic_tag  = ::comdare::cache_engine::nodes::concepts::NodesTopicTag;
    using axis_tag   = bst_shape_family_tag;
    using family_id  = std::integral_constant<int, 32>;
    using index_type = std::uint32_t;

    static constexpr bool        enabled     = flags::ptr_u32_enabled;
    static constexpr std::size_t kIndexBytes = sizeof(index_type);

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "bst_ptr_u32"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "BstPtrU32"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "PTR_U32"; }
};

static_assert(concepts::BstShape<BstPtrU32>);
static_assert(concepts::CacheEnginePermutationStrategy<BstPtrU32>);

} // namespace comdare::cache_engine::nodes::axis_bst_shape