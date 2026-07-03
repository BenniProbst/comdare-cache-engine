#pragma once
// #217-2a (2026-07-03) -- optionale Container-Kapazitaets-Eigenschaft.
//
// `max_fanout()` bleibt die historische/theoretische Fanout-Angabe der 03a-Suchstrategie.
// Harte Laufzeit-Grenzen werden nur ueber eine explizite Static-Container-Capacity deklariert.

#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::lookup::composable {

enum class CapacityKind { Advisory, Static };

struct CapacityConstraint {
    std::uint64_t min_fill = 0;
    std::uint64_t max_size = 0; // 0 == unbounded / kein Guard
    CapacityKind  kind     = CapacityKind::Advisory;
};

static_assert(std::is_standard_layout_v<CapacityConstraint>);

template <class T>
consteval CapacityConstraint capacity_constraint_of() noexcept {
    if constexpr (requires { T::container_capacity(); }) {
        return T::container_capacity();
    } else {
        // Absente Eigenschaft: bewusst Advisory + unbounded. Ein vorhandenes max_fanout()
        // wird NICHT automatisch als harte Grenze interpretiert.
        return {0, 0, CapacityKind::Advisory};
    }
}

} // namespace comdare::cache_engine::lookup::composable