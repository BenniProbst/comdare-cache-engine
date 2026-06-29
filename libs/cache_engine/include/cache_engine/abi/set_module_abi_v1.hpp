#pragma once
// D9.3 / L-76a (2026-06-02) — Set Module ABI v1 (Factory-Makro), MODUL-AUTOR-Seite der SET-Gattung.
// Analog COMDARE_DEFINE_CONTAINER/SEQUENCE/VIEW_MODULE: Factory liefert einen SetAbiAdapter. 4 extern-"C"-Symbole +
// ABI-Version/Magic IDENTISCH → DERSELBE Loader; der Set-Dock fragt dynamic_cast<ISetTier*> (Doc 24 §8.8).

#include "anatomy_module_abi_v1_decl.hpp"
#include "../../../anatomy/set_abi_adapter.hpp" // SetAbiAdapter

#include <new>

/// COMDARE_DEFINE_SET_MODULE(T0..T14) — die 4 Pflicht-extern-C-Symbole einer Set-Permutations-.dll.
/// T0 = search_algo-Kern-Organ (K=V); T1..T14 = restliche 14 Set-Achsen. Variadisch (Kommata).
#define COMDARE_DEFINE_SET_MODULE(...)                                                                                 \
    using ComdareSetPermutationComposition = ::comdare::cache_engine::anatomy::SetComposition<__VA_ARGS__>;            \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_version() noexcept {                       \
        return (static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR) << 32) |                                         \
               static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MINOR);                                                  \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_magic() noexcept {                         \
        return COMDARE_ANATOMY_ABI_MAGIC;                                                                              \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT ::comdare::cache_engine::anatomy::IAnatomyBase*                              \
    comdare_create_anatomy() noexcept {                                                                                \
        using AnatomyType = ::comdare::cache_engine::anatomy::SetAnatomy<ComdareSetPermutationComposition>;            \
        return new (::std::nothrow)::comdare::cache_engine::anatomy::SetAbiAdapter<AnatomyType>{};                     \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT void comdare_destroy_anatomy(                                                \
        ::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept {                                                \
        delete ptr;                                                                                                    \
    }
