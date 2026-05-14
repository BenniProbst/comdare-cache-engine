// V31.K2 (2026-05-14) — A04-mimalloc Adapter
//
// Wrapper around mimalloc (https://github.com/microsoft/mimalloc),
// MSR-TR-2019-18, "Mimalloc: Free List Sharding in Action" (Leijen/
// Zorn/de Moura, APLAS 2019).
//
// Activated via `-DCOMDARE_HAVE_MIMALLOC=ON`. Inactive: stub falls
// back to std::malloc/std::free, so the adapter API is always
// usable in the build but characteristic mimalloc behavior is
// only present when the original library is linked.
//
// License: see LICENSE_AUDIT_EXT.md (MIT) and NOTICE Architekt-
// Direktive II (2026-05-14).
#pragma once

#include <cstddef>
#include <cstdlib>
#include <new>

#if defined(COMDARE_HAVE_MIMALLOC)
#  include "mimalloc.h"
#endif

namespace comdare::adapter::a04_mimalloc {

class MimallocAdapter {
public:
    [[nodiscard]] void *allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) {
#if defined(COMDARE_HAVE_MIMALLOC)
        return mi_malloc_aligned(size, alignment);
#else
        // Fallback: aligned alloc via std (works for power-of-two alignment).
        if (alignment <= alignof(std::max_align_t)) {
            return std::malloc(size);
        }
#  if defined(_WIN32)
        return _aligned_malloc(size, alignment);
#  else
        void *p = nullptr;
        if (posix_memalign(&p, alignment, size) != 0) return nullptr;
        return p;
#  endif
#endif
    }

    void deallocate(void *ptr) noexcept {
        if (ptr == nullptr) return;
#if defined(COMDARE_HAVE_MIMALLOC)
        mi_free(ptr);
#elif defined(_WIN32)
        _aligned_free(ptr);
#else
        std::free(ptr);
#endif
    }

    [[nodiscard]] static constexpr const char *paper_id() noexcept {
        return "A04-mimalloc (Leijen/Zorn/de Moura 2019)";
    }

    [[nodiscard]] static constexpr bool original_active() noexcept {
#if defined(COMDARE_HAVE_MIMALLOC)
        return true;
#else
        return false;
#endif
    }
};

} // namespace comdare::adapter::a04_mimalloc
