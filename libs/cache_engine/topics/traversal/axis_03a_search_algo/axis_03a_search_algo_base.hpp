#pragma once
// V41.F.6.1 axis_03a_search_algo CRTP-Basis + Concept-Guard (2026-05-26)
//
// @topic traversal @achse 03a

#include "concepts/axis_03a_search_algo_concept.hpp"

#include <type_traits>

namespace comdare::cache_engine::traversal::axis_03a_search_algo {

/**
 * @brief SearchAlgoBase — CRTP-Basis fuer 03a-Wrapper
 *
 * Concept-Guard via static_assert im Konstruktor (CRTP-Henne-Ei-Pattern aus
 * Allocator-Achse).
 */
template <typename Derived>
class SearchAlgoBase {
protected:
    SearchAlgoBase() noexcept {
        static_assert(concepts::SearchAlgoVariant<Derived>,
            "Pflicht: Derived muss SearchAlgoVariant erfuellen "
            "(insert/lookup/erase/occupied_count/density_percent/clear)");
    }
};

}  // namespace
