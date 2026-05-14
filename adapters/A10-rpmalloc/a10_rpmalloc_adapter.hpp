// V31.K4 (2026-05-14) — A10-rpmalloc Allocator Adapter
// Wrapper around rpmalloc (Jansson 2017+). License: Public Domain / MIT.
// Activation: -DCOMDARE_HAVE_RPMALLOC=ON.
#pragma once

#include <cstddef>
#include <cstdlib>

#if defined(COMDARE_HAVE_RPMALLOC)
#  include "rpmalloc.h"
#endif

namespace comdare::adapter::a10_rpmalloc {

class RpmallocAdapter {
public:
    RpmallocAdapter() {
#if defined(COMDARE_HAVE_RPMALLOC)
        rpmalloc_initialize();
#endif
    }
    ~RpmallocAdapter() {
#if defined(COMDARE_HAVE_RPMALLOC)
        rpmalloc_finalize();
#endif
    }
    RpmallocAdapter(const RpmallocAdapter &) = delete;
    RpmallocAdapter &operator=(const RpmallocAdapter &) = delete;

    [[nodiscard]] void *allocate(std::size_t size) {
#if defined(COMDARE_HAVE_RPMALLOC)
        return rpmalloc(size);
#else
        return std::malloc(size);
#endif
    }
    void deallocate(void *p) noexcept {
#if defined(COMDARE_HAVE_RPMALLOC)
        rpfree(p);
#else
        std::free(p);
#endif
    }
    [[nodiscard]] static constexpr const char *paper_id() noexcept {
        return "A10-rpmalloc (Jansson 2017+)";
    }
};

} // namespace comdare::adapter::a10_rpmalloc
