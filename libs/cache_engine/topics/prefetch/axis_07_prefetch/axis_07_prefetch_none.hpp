#pragma once
// V41.F.6.1.F1 axis_07 PrefetchNone Default-Wrapper (Skelett-Stufe-A, no-prefetch baseline)

#include "axis_07_prefetch_base.hpp"
#include "../concepts/topic_prefetch_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::prefetch::axis_07_prefetch {

/// PrefetchNone — Default: kein Prefetch (Baseline fuer Mess-Reihen).
class PrefetchNone : public PrefetchBase<PrefetchNone> {
public:
    using topic_tag = ::comdare::cache_engine::prefetch::concepts::PrefetchTopicTag;
    using family_id = std::integral_constant<int, 0>;

    [[nodiscard]] static constexpr bool is_active() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "prefetch_none"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "PrefetchNone (no prefetch baseline)"; }
};

}  // namespace

namespace comdare::cache_engine::prefetch::axis_07_prefetch {
    static_assert(concepts::PrefetchStrategy<PrefetchNone>);
}
