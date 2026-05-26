#pragma once
// V41.F.6.1 axis_03b_cache_traversal CRTP-Basis (2026-05-26)

#include "concepts/axis_03b_cache_traversal_concept.hpp"

#include <type_traits>

namespace comdare::cache_engine::traversal::axis_03b_cache_traversal {

template <typename Derived>
class CacheTraversalBase {
protected:
    CacheTraversalBase() noexcept {
        static_assert(concepts::CacheTraversalVariant<Derived>,
            "Pflicht: Derived muss CacheTraversalVariant erfuellen "
            "(register_entry/resolve/unregister/tracked_count/clear)");
    }
};

}  // namespace
