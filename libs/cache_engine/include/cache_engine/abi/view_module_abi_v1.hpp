#pragma once
// D11-DLL / L-76c (2026-06-02) — View Module ABI v1 (Factory-Makro), MODUL-AUTOR-Seite der VIEW-Gattung.
// Analog COMDARE_DEFINE_CONTAINER_MODULE: Factory liefert einen ViewAbiAdapter. 4 extern-"C"-Symbole + ABI-Version/
// Magic IDENTISCH → DERSELBE Loader; der View-Dock fragt dynamic_cast<IViewTier*> (Doc 24 §8.8).

#include "anatomy_module_abi_v1_decl.hpp"
#include "../../../anatomy/view_abi_adapter.hpp"   // ViewAbiAdapter

#include <new>

/// COMDARE_DEFINE_VIEW_MODULE(T0..T3 [, Extent, Layout, Accessor]) — die 4 Pflicht-extern-C-Symbole einer View-DLL.
#define COMDARE_DEFINE_VIEW_MODULE(...)                                              \
    using ComdareViewPermutationComposition =                                       \
        ::comdare::cache_engine::anatomy::ViewComposition<__VA_ARGS__>;              \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t                             \
    comdare_anatomy_abi_version() noexcept {                                        \
        return (static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR) << 32) |      \
               static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MINOR);               \
    }                                                                                \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t                             \
    comdare_anatomy_abi_magic() noexcept { return COMDARE_ANATOMY_ABI_MAGIC; }      \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT                                           \
    ::comdare::cache_engine::anatomy::IAnatomyBase*                                 \
    comdare_create_anatomy() noexcept {                                              \
        using AnatomyType =                                                          \
            ::comdare::cache_engine::anatomy::ViewAnatomy<ComdareViewPermutationComposition>; \
        return new (::std::nothrow)                                                  \
            ::comdare::cache_engine::anatomy::ViewAbiAdapter<AnatomyType>{};        \
    }                                                                                \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT void                                      \
    comdare_destroy_anatomy(                                                         \
        ::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept { delete ptr; }
