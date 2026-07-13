// V31.K4 (2026-05-14) — A03-Michael Lock-Free Allocator Adapter
// Wrapper around scotts/michael Re-Implementation of Maged Michael 2004 PLDI.
// Activation: -DCOMDARE_HAVE_MICHAEL=ON. License: LGPL-2.1-or-later.
// IBM Patent US 2006/0190697 may apply (see NOTICE).
#pragma once

#include <cstddef>
#include <cstdlib>
#include <stdexcept>

#if defined(COMDARE_HAVE_MICHAEL)
#include "michael.h"
#endif

namespace comdare::adapter::a03_michael {

class MichaelAdapter {
public:
    [[nodiscard]] void* allocate(std::size_t size) {
#if defined(COMDARE_HAVE_MICHAEL)
        return malloc(size);
#else
        return std::malloc(size);
#endif
    }
    void deallocate(void* p) noexcept {
#if defined(COMDARE_HAVE_MICHAEL)
        free(p);
#else
        std::free(p);
#endif
    }
    [[nodiscard]] static constexpr const char* paper_id() noexcept { return "A03-Michael Lock-Free (Michael 2004)"; }
};

} // namespace comdare::adapter::a03_michael
