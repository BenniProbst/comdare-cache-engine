#pragma once
// V41.F.6.1.F3 axis_io InMemoryOnly Default-Wrapper

#include "axis_io_base.hpp"
#include "../concepts/topic_io_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::io::axis_io {
class InMemoryOnly : public IoBase<InMemoryOnly> {
public:
    using topic_tag = ::comdare::cache_engine::io::concepts::IoTopicTag;
    using family_id = std::integral_constant<int, 1>;
    [[nodiscard]] static constexpr bool is_in_memory_only() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "in_memory_only"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "InMemoryOnly (no spill, no IO, RAM-resident)"; }
};
}  // namespace
namespace comdare::cache_engine::io::axis_io {
    static_assert(concepts::IoStrategy<InMemoryOnly>);
}
