// V31.K3 (2026-05-14) — P25-Mahling Microbenchmark-Adapter
// Wrapper around prefetching (Mahling/Weisgut/Rabl 2025, DaMoN).
// Activation: -DCOMDARE_HAVE_MAHLING=ON.
// License: NO LICENSE — covered by Architekt-Direktive II 2026-05-14
// (citation-only). Microbenchmark-Suite, NOT a search structure.
#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace comdare::adapter::p25_mahling {

// Microbenchmark probe: measures fill-buffer occupancy under
// strong vs weak prefetch strategies. The actual measurement
// kernels live in ext/P25-Mahling/prefetching/ when activated.
class FillBufferProbe {
public:
    enum class Mode { Strong, Weak };

    void run(Mode mode, std::size_t footprint_bytes) {
#if defined(COMDARE_HAVE_MAHLING)
        (void)mode;
        (void)footprint_bytes;
#else
        (void)mode;
        (void)footprint_bytes;
        throw std::runtime_error("COMDARE_HAVE_MAHLING not enabled");
#endif
    }

    [[nodiscard]] static constexpr const char* paper_id() noexcept { return "P25-Mahling/Weisgut/Rabl 2025"; }
};

} // namespace comdare::adapter::p25_mahling
