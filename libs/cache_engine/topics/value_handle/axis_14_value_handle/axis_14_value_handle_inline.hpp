#pragma once
// V41.F.6.1.F2 axis_14 InlineHandle Default-Wrapper

#include "axis_14_value_handle_base.hpp"
#include "../concepts/topic_value_handle_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::value_handle::axis_14_value_handle {
class InlineHandle : public ValueHandleBase<InlineHandle> {
public:
    using topic_tag = ::comdare::cache_engine::value_handle::concepts::ValueHandleTopicTag;
    using family_id = std::integral_constant<int, 1>;
    [[nodiscard]] static constexpr bool is_inline() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "inline_handle"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "InlineHandle (value embedded in slot)"; }
};
}  // namespace
namespace comdare::cache_engine::value_handle::axis_14_value_handle {
    static_assert(concepts::ValueHandleStrategy<InlineHandle>);
}
