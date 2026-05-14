// V31.K3 (2026-05-14) — P10-SuRF Adapter
// Wrapper around SuRF (Zhang/Lim/Leis et al. 2018, SIGMOD).
// Activation: -DCOMDARE_HAVE_SURF=ON. License: Apache-2.0.
#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

#if defined(COMDARE_HAVE_SURF)
#  include "surf.hpp"
#endif

namespace comdare::adapter::p10_surf {

class SurfAdapter {
public:
    using key_type = std::string_view;

    explicit SurfAdapter(const std::vector<std::string> &keys = {}) {
#if defined(COMDARE_HAVE_SURF)
        surf_ = std::make_unique<surf::SuRF>(keys);
#else
        (void)keys;
#endif
    }

    [[nodiscard]] bool contains(key_type key) const {
#if defined(COMDARE_HAVE_SURF)
        return surf_->lookupKey(std::string(key));
#else
        (void)key;
        throw std::runtime_error("COMDARE_HAVE_SURF not enabled");
#endif
    }

    [[nodiscard]] static constexpr const char *paper_id() noexcept {
        return "P10-SuRF (Zhang et al. 2018)";
    }

private:
#if defined(COMDARE_HAVE_SURF)
    std::unique_ptr<surf::SuRF> surf_;
#endif
};

} // namespace comdare::adapter::p10_surf
