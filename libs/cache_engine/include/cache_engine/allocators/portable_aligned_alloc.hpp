#pragma once
// Portable Aligned Allocation - Cross-Platform Wrapper (POSIX vs MSVC)
//
// std::aligned_alloc ist POSIX-only und in MSVC nicht verfuegbar.
// Dieser Header bietet portable Alternative ueber Plattform-spezifische APIs.

#include <cstddef>
#include <cstdlib>

#ifdef _MSC_VER
    #include <malloc.h>
#endif

namespace comdare::cache_engine::allocator {

// Allokation mit Alignment-Garantie auf allen Plattformen
[[nodiscard]] inline void* portable_aligned_alloc(std::size_t alignment, std::size_t bytes) noexcept {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        return nullptr;  // alignment must be power of 2
    }
    // Round bytes up to multiple of alignment (Pflicht von std::aligned_alloc)
    std::size_t const padded = ((bytes + alignment - 1) / alignment) * alignment;
#ifdef _MSC_VER
    return _aligned_malloc(padded, alignment);
#else
    return std::aligned_alloc(alignment, padded);
#endif
}

inline void portable_aligned_free(void* p) noexcept {
#ifdef _MSC_VER
    _aligned_free(p);
#else
    std::free(p);
#endif
}

}  // namespace comdare::cache_engine::allocator
