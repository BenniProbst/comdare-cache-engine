// V31.K3 (2026-05-14) — P07-Wormhole Adapter
// Wrapper around wormhole (Wu/Ni/Jiang 2019, EuroSys).
// Activation: -DCOMDARE_HAVE_WORMHOLE=ON.
// License: GPL-3.0 — covered by Architekt-Direktive II 2026-05-14
// (permutation-axis extraction = new work).
#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>

#if defined(COMDARE_HAVE_WORMHOLE)
extern "C" {
#  include "wh.h"
}
#endif

namespace comdare::adapter::p07_wormhole {

class WormholeAdapter {
public:
    using key_type = std::string_view;
    using value_type = std::uint64_t;

    WormholeAdapter() {
#if defined(COMDARE_HAVE_WORMHOLE)
        wh_ = wh_create();
#endif
    }

    ~WormholeAdapter() {
#if defined(COMDARE_HAVE_WORMHOLE)
        if (wh_) wh_destroy(wh_);
#endif
    }

    WormholeAdapter(const WormholeAdapter &) = delete;
    WormholeAdapter &operator=(const WormholeAdapter &) = delete;

    bool insert(key_type key, value_type value) {
#if defined(COMDARE_HAVE_WORMHOLE)
        return wh_put(wh_, key.data(), key.size(), &value, sizeof(value));
#else
        (void)key; (void)value;
        throw std::runtime_error("COMDARE_HAVE_WORMHOLE not enabled");
#endif
    }

    [[nodiscard]] static constexpr const char *paper_id() noexcept {
        return "P07-Wormhole (Wu/Ni/Jiang 2019)";
    }

private:
#if defined(COMDARE_HAVE_WORMHOLE)
    struct wormhole *wh_ = nullptr;
#endif
};

} // namespace comdare::adapter::p07_wormhole
