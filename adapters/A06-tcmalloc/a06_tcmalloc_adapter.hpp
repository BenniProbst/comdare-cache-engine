// V31.K4 (2026-05-14) — A06-tcmalloc Allocator Adapter
// Wrapper around tcmalloc (Ghemawat/Menage 2009 + Zhou et al. 2024 ASPLOS).
// License: Apache-2.0. Activation: -DCOMDARE_HAVE_TCMALLOC=ON.
#pragma once

#include <cstddef>
#include <cstdlib>

#if defined(COMDARE_HAVE_TCMALLOC)
#include <gperftools/tcmalloc.h>
#endif

namespace comdare::adapter::a06_tcmalloc {

class TcmallocAdapter {
public:
    [[nodiscard]] void* allocate(std::size_t size) {
#if defined(COMDARE_HAVE_TCMALLOC)
        return tc_malloc(size);
#else
        return std::malloc(size);
#endif
    }
    void deallocate(void* p) noexcept {
#if defined(COMDARE_HAVE_TCMALLOC)
        tc_free(p);
#else
        std::free(p);
#endif
    }
    [[nodiscard]] static constexpr const char* paper_id() noexcept { return "A06-tcmalloc (Ghemawat/Menage 2009)"; }
};

} // namespace comdare::adapter::a06_tcmalloc
