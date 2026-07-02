// V31.K4 (2026-05-14) — A07-snmalloc Allocator Adapter
// Wrapper around snmalloc (Liétar et al. 2019, ISMM). License: MIT.
// Activation: -DCOMDARE_HAVE_SNMALLOC=ON.
#pragma once

#include <cstddef>
#include <cstdlib>

#if defined(COMDARE_HAVE_SNMALLOC)
#include <snmalloc/snmalloc.h>
#endif

namespace comdare::adapter::a07_snmalloc {

class SnmallocAdapter {
public:
    [[nodiscard]] void* allocate(std::size_t size) {
#if defined(COMDARE_HAVE_SNMALLOC)
        return snmalloc::ThreadAlloc::get().alloc(size);
#else
        return std::malloc(size);
#endif
    }
    void deallocate(void* p) noexcept {
#if defined(COMDARE_HAVE_SNMALLOC)
        snmalloc::ThreadAlloc::get().dealloc(p);
#else
        std::free(p);
#endif
    }
    [[nodiscard]] static constexpr const char* paper_id() noexcept { return "A07-snmalloc (Liétar et al. 2019)"; }
};

} // namespace comdare::adapter::a07_snmalloc
