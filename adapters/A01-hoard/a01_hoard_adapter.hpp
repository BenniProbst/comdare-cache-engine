// V31.K4 (2026-05-14) — A01-Hoard Allocator Adapter
// Wrapper around Hoard (Berger/McKinley/Blumofe/Wilson 2000, ASPLOS-IX).
// Activation: -DCOMDARE_HAVE_HOARD=ON. License: Apache-2.0.
#pragma once

#include <cstddef>
#include <cstdlib>
#include <stdexcept>

#if defined(COMDARE_HAVE_HOARD)
extern "C" {
void*       xxmalloc(std::size_t);
void        xxfree(void*);
std::size_t xxmalloc_usable_size(void*);
}
#endif

namespace comdare::adapter::a01_hoard {

class HoardAdapter {
public:
    [[nodiscard]] void* allocate(std::size_t size) {
#if defined(COMDARE_HAVE_HOARD)
        return xxmalloc(size);
#else
        return std::malloc(size);
#endif
    }
    void deallocate(void* p) noexcept {
#if defined(COMDARE_HAVE_HOARD)
        xxfree(p);
#else
        std::free(p);
#endif
    }
    [[nodiscard]] static constexpr const char* paper_id() noexcept { return "A01-Hoard (Berger et al. 2000)"; }
    [[nodiscard]] static constexpr bool        original_active() noexcept {
#if defined(COMDARE_HAVE_HOARD)
        return true;
#else
        return false;
#endif
    }
};

} // namespace comdare::adapter::a01_hoard
