// V31.K3 (2026-05-14) — P29 Userspace-RCU Adapter
// Wrapper around userspace-rcu/liburcu (McKenney 2001, OLS).
// Activation: -DCOMDARE_HAVE_LIBURCU=ON.
// License: LGPL-2.1+ — covered by Architekt-Direktive II 2026-05-14.
//
// NOTE: comdare has its own RCU-Implementation (Memory-Direktive Phase 6
// "Eigene RCU-Implementation statt liburcu"). This adapter exists only
// for cross-reference baseline measurements.
#pragma once

#include <stdexcept>

#if defined(COMDARE_HAVE_LIBURCU)
#  include <urcu.h>
#endif

namespace comdare::adapter::p29_rcu {

class LiburcuAdapter {
public:
    LiburcuAdapter() {
#if defined(COMDARE_HAVE_LIBURCU)
        rcu_register_thread();
#endif
    }

    ~LiburcuAdapter() {
#if defined(COMDARE_HAVE_LIBURCU)
        rcu_unregister_thread();
#endif
    }

    LiburcuAdapter(const LiburcuAdapter &) = delete;
    LiburcuAdapter &operator=(const LiburcuAdapter &) = delete;

    void read_lock() {
#if defined(COMDARE_HAVE_LIBURCU)
        rcu_read_lock();
#else
        throw std::runtime_error("COMDARE_HAVE_LIBURCU not enabled");
#endif
    }

    void read_unlock() {
#if defined(COMDARE_HAVE_LIBURCU)
        rcu_read_unlock();
#else
        throw std::runtime_error("COMDARE_HAVE_LIBURCU not enabled");
#endif
    }

    void synchronize() {
#if defined(COMDARE_HAVE_LIBURCU)
        synchronize_rcu();
#else
        throw std::runtime_error("COMDARE_HAVE_LIBURCU not enabled");
#endif
    }

    [[nodiscard]] static constexpr const char *paper_id() noexcept {
        return "P29-RCU (McKenney et al. 2001)";
    }
};

} // namespace comdare::adapter::p29_rcu
