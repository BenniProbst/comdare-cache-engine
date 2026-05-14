// V31.K4 (2026-05-14) — A08-scalloc Allocator Adapter
// Wrapper around scalloc (Aigner/Kirsch/Lippautz/Sokolova 2015, OOPSLA).
// License: BSD-3. Activation: -DCOMDARE_HAVE_SCALLOC=ON.
#pragma once

#include <cstddef>
#include <cstdlib>

#if defined(COMDARE_HAVE_SCALLOC)
extern "C" {
    void *scalloc_malloc(std::size_t);
    void  scalloc_free(void *);
}
#endif

namespace comdare::adapter::a08_scalloc {

class ScallocAdapter {
public:
    [[nodiscard]] void *allocate(std::size_t size) {
#if defined(COMDARE_HAVE_SCALLOC)
        return scalloc_malloc(size);
#else
        return std::malloc(size);
#endif
    }
    void deallocate(void *p) noexcept {
#if defined(COMDARE_HAVE_SCALLOC)
        scalloc_free(p);
#else
        std::free(p);
#endif
    }
    [[nodiscard]] static constexpr const char *paper_id() noexcept {
        return "A08-scalloc (Aigner et al. 2015)";
    }
};

} // namespace comdare::adapter::a08_scalloc
