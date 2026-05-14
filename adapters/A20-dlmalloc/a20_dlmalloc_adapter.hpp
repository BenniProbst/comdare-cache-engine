// V31.K4 (2026-05-14) — A20-dlmalloc Allocator Adapter
// Wrapper around dlmalloc (Doug Lea, Public Domain).
// Activation: -DCOMDARE_HAVE_DLMALLOC=ON.
#pragma once

#include <cstddef>
#include <cstdlib>

#if defined(COMDARE_HAVE_DLMALLOC)
extern "C" {
    void *dlmalloc(std::size_t);
    void  dlfree(void *);
}
#endif

namespace comdare::adapter::a20_dlmalloc {

class DlmallocAdapter {
public:
    [[nodiscard]] void *allocate(std::size_t size) {
#if defined(COMDARE_HAVE_DLMALLOC)
        return dlmalloc(size);
#else
        return std::malloc(size);
#endif
    }
    void deallocate(void *p) noexcept {
#if defined(COMDARE_HAVE_DLMALLOC)
        dlfree(p);
#else
        std::free(p);
#endif
    }
    [[nodiscard]] static constexpr const char *paper_id() noexcept {
        return "A20-dlmalloc (Doug Lea, 1987-)";
    }
};

} // namespace comdare::adapter::a20_dlmalloc
