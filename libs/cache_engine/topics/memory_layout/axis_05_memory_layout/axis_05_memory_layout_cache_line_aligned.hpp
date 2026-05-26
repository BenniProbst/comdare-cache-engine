#pragma once
// V41.F.6.1.F1 axis_05 CacheLineAlignedLayout Default-Wrapper (Skelett-Stufe-A)

#include "axis_05_memory_layout_base.hpp"
#include "../concepts/topic_memory_layout_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {

/// CacheLineAlignedLayout — Default: 64-byte aligned AoS layout (most cache architectures).
class CacheLineAlignedLayout : public MemoryLayoutBase<CacheLineAlignedLayout> {
public:
    using topic_tag = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    using family_id = std::integral_constant<int, 1>;

    [[nodiscard]] static constexpr std::size_t cache_line_size() noexcept { return 64; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "cache_line_aligned"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "CacheLineAlignedLayout (64-byte AoS, standard cache architectures)"; }
};

}  // namespace

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {
    static_assert(concepts::MemoryLayoutStrategy<CacheLineAlignedLayout>);
}
