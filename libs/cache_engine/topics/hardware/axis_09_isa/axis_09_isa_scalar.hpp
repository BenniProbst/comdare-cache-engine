#pragma once
// V41.F.6.1.F2 axis_09 IsaScalar Default-Wrapper (scalar baseline)

#include "axis_09_isa_base.hpp"
#include "../concepts/topic_hardware_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::hardware::axis_09_isa {
class IsaScalar : public IsaBase<IsaScalar> {
public:
    using topic_tag = ::comdare::cache_engine::hardware::concepts::HardwareTopicTag;
    using family_id = std::integral_constant<int, 0>;
    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "isa_scalar"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "IsaScalar (no SIMD baseline)"; }
};
}  // namespace
namespace comdare::cache_engine::hardware::axis_09_isa {
    static_assert(concepts::IsaStrategy<IsaScalar>);
}
