#pragma once
// V41.F.6.1.F1 axis_08 OlcOptimisticConcurrency Default-Wrapper (Skelett-Stufe-A)

#include "axis_08_concurrency_base.hpp"
#include "../concepts/topic_concurrency_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::concurrency::axis_08_concurrency {

/// OlcOptimisticConcurrency — Default: Optimistic Lock-Coupling (ART-Sync, PRT-ART Pattern).
class OlcOptimisticConcurrency : public ConcurrencyBase<OlcOptimisticConcurrency> {
public:
    using topic_tag = ::comdare::cache_engine::concurrency::concepts::ConcurrencyTopicTag;
    using family_id = std::integral_constant<int, 1>;

    [[nodiscard]] static constexpr concepts::ConcurrencyPattern concurrency_pattern() noexcept {
        return concepts::ConcurrencyPattern::Optimistic;
    }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "olc_optimistic"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "OlcOptimisticConcurrency (Optimistic Lock-Coupling, ART-Sync Pattern)"; }
};

}  // namespace

namespace comdare::cache_engine::concurrency::axis_08_concurrency {
    static_assert(concepts::ConcurrencyStrategy<OlcOptimisticConcurrency>);
}
