#pragma once
// V41.F.6.1 axis_03b_cache_traversal CRTP-Basis (2026-05-26)

#include "concepts/axis_03b_cache_traversal_concept.hpp"
#include <topics/axis_base.hpp>

#include <type_traits>

namespace comdare::cache_engine::cache_traversal {

/**
 * @brief CacheTraversalBase — CRTP-Basis fuer 03b-Wrapper
 *
 * Erbt von ::topics::AxisBase fuer cross-axis Pflicht-Property get_compiler()
 * (Default "original", per Wrapper ueberschreibbar).
 */
template <typename Derived>
class CacheTraversalBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    CacheTraversalBase() noexcept {
        static_assert(concepts::CacheTraversalVariant<Derived>,
                      "Pflicht: Derived muss CacheTraversalVariant erfuellen "
                      "(register_entry/resolve/unregister/tracked_count/clear)");
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>,
                      "Pflicht: Derived erfuellt AxisBaseConcept (get_compiler() Default 'original' + "
                      "is_original_module = false via AxisBase)");
    }
    // V41.F.6.1.P2.C ENTFERNT: Defaults kommen via AxisBase (cross-axis generisch).
};

} // namespace comdare::cache_engine::cache_traversal
