#pragma once
// D10-DLL / L-76b (2026-06-02) — Sequence Module ABI v1 (Factory-Makro), MODUL-AUTOR-Seite der SEQUENCE-Gattung.
// Analog COMDARE_DEFINE_ADAPTER_MODULE: die Factory liefert einen SequenceAbiAdapter. 4 extern-"C"-Symbole +
// ABI-Version/Magic IDENTISCH zur SearchAlgorithm-Seite → DERSELBE gattungs-agnostische AnatomyModuleLoader; die
// Gattung wird runtime über anatomy()->genus() erkannt, der Sequence-Dock fragt dynamic_cast<ISequenceTier*> (Doc 24 §8.8).

#include "anatomy_module_abi_v1_decl.hpp"             // ABI-Version/Magic/Export-Macro + Factory-Symbol-Decls
#include "../../../anatomy/sequence_abi_adapter.hpp"   // SequenceAbiAdapter (Makro-Materialisierung)

#include <new>

/// COMDARE_DEFINE_SEQUENCE_MODULE(T0..T9 [, Growth]) — die 4 Pflicht-extern-C-Symbole einer Sequence-Permutations-.dll.
/// Variadisch (SequenceComposition<...> enthält Kommata → sonst mehrere Makro-Argumente).
#define COMDARE_DEFINE_SEQUENCE_MODULE(...)                                          \
    using ComdareSequencePermutationComposition =                                   \
        ::comdare::cache_engine::anatomy::SequenceComposition<__VA_ARGS__>;          \
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
            ::comdare::cache_engine::anatomy::SequenceAnatomy<ComdareSequencePermutationComposition>; \
        return new (::std::nothrow)                                                  \
            ::comdare::cache_engine::anatomy::SequenceAbiAdapter<AnatomyType>{};    \
    }                                                                                \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT void                                      \
    comdare_destroy_anatomy(                                                         \
        ::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept { delete ptr; }
