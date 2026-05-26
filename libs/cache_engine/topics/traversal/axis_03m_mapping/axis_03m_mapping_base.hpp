#pragma once
// V41.F.6.1 axis_03m_mapping CRTP-Basis (2026-05-26)

#include "concepts/axis_03m_mapping_concept.hpp"

#include <type_traits>

namespace comdare::cache_engine::traversal::axis_03m_mapping {

template <typename Derived>
class MappingBase {
protected:
    MappingBase() noexcept {
        static_assert(concepts::MappingVariant<Derived>,
            "Pflicht: Derived muss MappingVariant erfuellen "
            "(register_slot/resolve_offset/reverse_lookup/mapped_count/clear)");
    }
};

}  // namespace
