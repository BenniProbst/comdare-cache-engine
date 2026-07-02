// V31.K4 (2026-05-14) — A11-LRMalloc Allocator Adapter
// Wrapper around lrmalloc (Leite/Rocha 2018/2019, VECPAR). License: MIT.
// Activation: -DCOMDARE_HAVE_LRMALLOC=ON.
#pragma once

#include <cstddef>
#include <cstdlib>

#if defined(COMDARE_HAVE_LRMALLOC)
extern "C" {
void* lr_malloc(std::size_t);
void  lr_free(void*);
}
#endif

namespace comdare::adapter::a11_lrmalloc {

class LrmallocAdapter {
public:
    [[nodiscard]] void* allocate(std::size_t size) {
#if defined(COMDARE_HAVE_LRMALLOC)
        return lr_malloc(size);
#else
        return std::malloc(size);
#endif
    }
    void deallocate(void* p) noexcept {
#if defined(COMDARE_HAVE_LRMALLOC)
        lr_free(p);
#else
        std::free(p);
#endif
    }
    [[nodiscard]] static constexpr const char* paper_id() noexcept { return "A11-LRMalloc (Leite/Rocha 2018)"; }
};

} // namespace comdare::adapter::a11_lrmalloc
