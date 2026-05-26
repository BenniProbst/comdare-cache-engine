#pragma once
// V41.F.6.1 axis_03a_search_algo CRTP-Basis + Concept-Guard (2026-05-26)
//
// @topic traversal @achse 03a

#include "concepts/axis_03a_search_algo_concept.hpp"
#include "../../axis_base.hpp"

#include <type_traits>

namespace comdare::cache_engine::traversal::axis_03a_search_algo {

/**
 * @brief SearchAlgoBase — CRTP-Basis fuer 03a-Wrapper
 *
 * Concept-Guard via static_assert im Konstruktor (CRTP-Henne-Ei-Pattern aus
 * Allocator-Achse).
 *
 * Erbt von ::topics::AxisBase fuer cross-axis Pflicht-Property get_compiler()
 * (Default "original", per Wrapper ueberschreibbar).
 */
template <typename Derived>
class SearchAlgoBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    SearchAlgoBase() noexcept {
        static_assert(concepts::SearchAlgoVariant<Derived>,
            "Pflicht: Derived muss SearchAlgoVariant erfuellen "
            "(insert/lookup/erase/occupied_count/density_percent/clear)");
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>,
            "Pflicht: Derived erfuellt AxisBaseConcept (get_compiler() Default 'original' + is_original_module = false via AxisBase)");
    }
    // V41.F.6.1.P2.C ENTFERNT: Defaults kommen via AxisBase (cross-axis generisch).
};

}  // namespace
