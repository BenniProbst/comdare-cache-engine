#pragma once
// V41.F.6.1 axis_03m_mapping CRTP-Basis (2026-05-26)

#include "concepts/axis_03m_mapping_concept.hpp"
#include <topics/axis_base.hpp>

#include <type_traits>

namespace comdare::cache_engine::mapping {

/**
 * @brief MappingBase — CRTP-Basis fuer 03m-Wrapper
 *
 * Erbt von ::topics::AxisBase fuer cross-axis Pflicht-Property get_compiler()
 * (Default "original", per Wrapper ueberschreibbar).
 */
template <typename Derived>
class MappingBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    MappingBase() noexcept {
        static_assert(concepts::MappingVariant<Derived>,
            "Pflicht: Derived muss MappingVariant erfuellen "
            "(register_slot/resolve_offset/reverse_lookup/mapped_count/clear)");
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>,
            "Pflicht: Derived erfuellt AxisBaseConcept (get_compiler() Default 'original' + is_original_module = false via AxisBase)");
    }
    // V41.F.6.1.P2.C ENTFERNT: Defaults kommen via AxisBase (cross-axis generisch).
};

}  // namespace
