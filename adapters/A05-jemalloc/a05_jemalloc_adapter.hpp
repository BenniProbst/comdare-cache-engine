// V31.K4 (2026-05-14) — A05-jemalloc Allocator Adapter
// Wrapper around jemalloc (Evans 2006, BSDCan). License: BSD-2.
// Activation: -DCOMDARE_HAVE_JEMALLOC=ON.
#pragma once

#include <cstddef>
#include <cstdlib>

#if defined(COMDARE_HAVE_JEMALLOC)
#include <jemalloc/jemalloc.h>
#endif

namespace comdare::adapter::a05_jemalloc {

class JemallocAdapter {
public:
    [[nodiscard]] void* allocate(std::size_t size) {
#if defined(COMDARE_HAVE_JEMALLOC)
        return je_malloc(size);
#else
        return std::malloc(size);
#endif
    }
    void deallocate(void* p) noexcept {
#if defined(COMDARE_HAVE_JEMALLOC)
        je_free(p);
#else
        std::free(p);
#endif
    }
    [[nodiscard]] static constexpr const char* paper_id() noexcept { return "A05-jemalloc (Evans 2006)"; }
};

} // namespace comdare::adapter::a05_jemalloc
