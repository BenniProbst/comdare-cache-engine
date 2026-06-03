#pragma once
// D4b / L-75 (2026-06-02) — Container Module ABI v1 (Factory-Makro) — MODUL-AUTOR-Seite der CONTAINER-Gattung.
//
// Analog COMDARE_DEFINE_ANATOMY_MODULE (anatomy_module_abi_v1.hpp), aber die Factory liefert einen
// AdapterAbiAdapter (Adapter-Gattung). Eine generierte Container-Permutations-.cpp materialisiert damit ihre
// .dll-Export-Symbole. WICHTIG: die 4 extern-"C"-Symbole + ABI-Version/Magic sind IDENTISCH zur SearchAlgorithm-
// Seite (comdare_anatomy_abi_version/magic/create_anatomy/destroy_anatomy) → der GLEICHE gattungs-agnostische
// AnatomyModuleLoader lädt beide; die Gattung wird runtime über anatomy()->genus() bestimmt, der Container-Dock
// fragt dynamic_cast<IAdapterTier*>. So braucht der Loader KEINE Änderung (Doc 24 §8.8).
//
// @doku docs/architecture/27 §3 BR-4 (analog) + docs/architecture/24 §8.8 (Prüf-Dock je Gattung)

#include "anatomy_module_abi_v1_decl.hpp"             // ABI-Version/Magic/Export-Macro + Factory-Symbol-Decls
#include "../../../anatomy/adapter_abi_adapter.hpp"  // AdapterAbiAdapter (Makro-Materialisierung)

#include <new>

// ─────────────────────────────────────────────────────────────────────────────
// COMDARE_DEFINE_ADAPTER_MODULE(T0..T11, Inner) — die 4 Pflicht-extern-C-Symbole einer Container-Permutations-.dll.
// #87+#90 (2026-06-03, Doku 14 §28): Adapter-Tier-Unterklasse = 13 Achsen (12 geteilt/delegiert + inner_container),
// KEINE „ordering"-Achse. Variadisch — AdapterComposition<T0..T11, Inner> enthält Kommata → ein __VA_ARGS__.
// ─────────────────────────────────────────────────────────────────────────────
#define COMDARE_DEFINE_ADAPTER_MODULE(...)                                         \
    using ComdareAdapterPermutationComposition =                                  \
        ::comdare::cache_engine::anatomy::AdapterComposition<__VA_ARGS__>;        \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t                             \
    comdare_anatomy_abi_version() noexcept {                                        \
        return (static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR) << 32) |      \
               static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MINOR);               \
    }                                                                                \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t                             \
    comdare_anatomy_abi_magic() noexcept {                                          \
        return COMDARE_ANATOMY_ABI_MAGIC;                                            \
    }                                                                                \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT                                           \
    ::comdare::cache_engine::anatomy::IAnatomyBase*                                 \
    comdare_create_anatomy() noexcept {                                              \
        using AnatomyType =                                                          \
            ::comdare::cache_engine::anatomy::AdapterAnatomy<ComdareAdapterPermutationComposition>; \
        return new (::std::nothrow)                                                  \
            ::comdare::cache_engine::anatomy::AdapterAbiAdapter<AnatomyType>{};   \
    }                                                                                \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT void                                      \
    comdare_destroy_anatomy(                                                         \
        ::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept {             \
        delete ptr;                                                                  \
    }
