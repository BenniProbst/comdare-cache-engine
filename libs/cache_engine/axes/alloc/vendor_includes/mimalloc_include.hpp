#pragma once
// V41.F.6.1.C Stufe 1 Vendor-Header-Shim: Mimalloc (W6-Pattern)
//
// @vendor A04 Mimalloc
//
// **EINZIGE Stelle** mit `#if` fuer Mimalloc-Aktivierung im Source-Tree.
// Wrapper-Klasse axis_06_allocator_mimalloc.hpp nutzt diese Shim und bleibt
// damit komplett #ifdef-frei.
//
// Wenn `flags::mimalloc_enabled == true`: echtes <mimalloc.h>.
// Wenn `false`: Forward-Stubs (no-op) — damit `if constexpr (false)`-Zweig
// im Wrapper syntaktisch valide bleibt. Stubs werden NIE aufgerufen
// (Discarded Statement, Compiler eliminiert).

#include <axes/alloc/axis_06_allocator_flags.hpp>

#include <cstddef>
#include <cstdlib>

#if COMDARE_AXIS_06_USE_MIMALLOC
// Echter Vendor-Header (nur eingebunden wenn USE-Flag ON = ENABLE && HAVE)
#include <mimalloc.h>
#else
// Forward-Stubs damit Wrapper-Code (`if constexpr (false)`-Zweig) syntaktisch
// valide bleibt. NIEMALS aufgerufen — Compiler verwirft die Calls per Dead-Code-Elim.
extern "C" {
inline void* mi_malloc_aligned(std::size_t /*size*/, std::size_t /*alignment*/) noexcept { return nullptr; }
inline void  mi_free(void* /*p*/) noexcept {}
inline void* mi_calloc(std::size_t /*count*/, std::size_t /*size*/) noexcept { return nullptr; }
inline void* mi_realloc_aligned(void* /*p*/, std::size_t /*newsize*/, std::size_t /*alignment*/) noexcept {
    return nullptr;
}
inline std::size_t mi_usable_size(const void* /*p*/) noexcept { return 0; }
inline void        mi_collect(int /*force*/) noexcept {}
}
#endif
