// V31.K3 (2026-05-14) — P30 Hazard Pointers Adapter
// Wrapper around haz_ptr (Michael 2004, IEEE TPDS 15(6)).
// Activation: -DCOMDARE_HAVE_HAZ_PTR=ON.
// License: NO LICENSE — covered by Architekt-Direktive II 2026-05-14.
//
// NOTE: NOT actively used in PRT-ART (architecture decision F12-K:
// memory-leak == undefined behavior, RCU preferred over hazard
// pointers). Skeleton retained for completeness.
#pragma once

#include <atomic>
#include <stdexcept>

#if defined(COMDARE_HAVE_HAZ_PTR)
#  include "haz_ptr.h"
#endif

namespace comdare::adapter::p30_hazard_pointers {

template <typename T>
class HazardPointerAdapter {
public:
    T *acquire(std::atomic<T *> &slot) {
#if defined(COMDARE_HAVE_HAZ_PTR)
        return slot.load(std::memory_order_acquire);
#else
        (void)slot;
        throw std::runtime_error("COMDARE_HAVE_HAZ_PTR not enabled");
#endif
    }

    void release(T *ptr) {
#if defined(COMDARE_HAVE_HAZ_PTR)
        (void)ptr;
#else
        (void)ptr;
        throw std::runtime_error("COMDARE_HAVE_HAZ_PTR not enabled");
#endif
    }

    [[nodiscard]] static constexpr const char *paper_id() noexcept {
        return "P30-HazardPointers (Michael 2004)";
    }
};

} // namespace comdare::adapter::p30_hazard_pointers
