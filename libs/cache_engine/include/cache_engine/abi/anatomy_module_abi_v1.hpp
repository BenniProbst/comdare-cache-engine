#pragma once
// V41.F.6.1.R5.D — Anatomy Module ABI v1 (Minimal Factory-Pattern) — MODUL-AUTOR-Seite.
//
// User-Direktive 2026-05-27 (Doku 14 §41.5):
// "Per Doku 14 §41.5 reicht: comdare_create_anatomy() → IAnatomyBase* + comdare_destroy_anatomy(IAnatomyBase*)."
//
// **R6 Inkrement 2b (2026-05-30) — Entkopplung:** Die leichte ABI-Schnittstelle (Factory-Deklarationen,
// Version/Magic, Interface-Typen, AnatomyAbiVersion-Helper) lebt jetzt in `anatomy_module_abi_v1_decl.hpp`
// (das der host-seitige Loader inkludiert — OHNE die schwere Adapter-Template). DIESES Header ist die
// MODUL-AUTOR-Seite: es inkludiert das Decl + die schwere `abi_adapter.hpp` + `SearchAlgorithmAnatomy` +
// `AdHocComposition` und stellt die `COMDARE_DEFINE_ANATOMY_MODULE`-Makros bereit, mit denen eine generierte
// Permutations-.cpp ihre .so/.dll-Export-Symbole materialisiert. Inhaltlich unveraendert fuer DLLs/Tests.
//
// Vorteile gegenueber module_abi_v1.hpp:
// 1. Eine .so/.dll exportiert nur 2 Symbole (Factory + Destroy), nicht 9+
// 2. Anatomy-API-Erweiterungen brechen die ABI NICHT (Virtual-Interface erweiterbar ueber neue Sub-Interfaces)
// 3. C++ RAII statt manueller Pointer-Tracking
// 4. Type-Safety: kein void*-Cast erforderlich
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §41.5 + §43; docs/architecture/24 §8.6
// @task #706 V41.F.6.1.R5.D
// @related [[execution-engine-als-wurzel]] [[anatomie-gattungen]]

#include "anatomy_module_abi_v1_decl.hpp"             // leichte ABI-Schnittstelle (Version/Magic/Factory-Decls/Helper)
#include "../../../anatomy/abi_adapter.hpp"            // SearchAlgorithmAbiAdapter (Makro-Materialisierung)
#include "../../../anatomy/search_algorithm_anatomy.hpp"
#include "../../../anatomy/composition_factory.hpp"   // R5.G: AdHocComposition für Auto-Permutations-Codegen
#include "build_variant_inspection.hpp"               // L-74a: COMDARE_DEFINE_BUILD_VARIANT_INSPECTION (BUILDVARIANT-Variante)

#include <new>

// ─────────────────────────────────────────────────────────────────────────────
// COMDARE_DEFINE_ANATOMY_MODULE — Convenience-Macro fuer Permutations-Binary
// ─────────────────────────────────────────────────────────────────────────────

/// COMDARE_DEFINE_ANATOMY_MODULE(CompositionType) generiert die 4 Pflicht-extern-C-Symbole eines
/// Anatomy-Permutations-Binary. CompositionType muss IsComposition erfuellen UND fuer SearchAlgorithm-
/// Gattung zugelassen sein.
///
/// Verwendung:
/// ```cpp
/// // generated_perm_<hash>.cpp
/// #include <cache_engine/abi/anatomy_module_abi_v1.hpp>
/// #include "art_reference.hpp"  // konkrete Composition
/// COMDARE_DEFINE_ANATOMY_MODULE(comdare::cache_engine::compositions::ArtComposition)
/// ```
#define COMDARE_DEFINE_ANATOMY_MODULE(CompositionType)                              \
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
            ::comdare::cache_engine::anatomy::SearchAlgorithmAnatomy<CompositionType>; \
        return new (::std::nothrow)                                                  \
            ::comdare::cache_engine::anatomy::SearchAlgorithmAbiAdapter<AnatomyType>{}; \
    }                                                                                \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT void                                      \
    comdare_destroy_anatomy(                                                         \
        ::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept {             \
        delete ptr;                                                                  \
    }

/// COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(...) — R5.G: Materialisiert eine AUTO-ENUMERIERTE Permutation
/// (AdHocComposition) als Permutations-Binary, OHNE benannten Composition-Header. Nimmt die 17
/// Achsen-Vendor-Typen VARIADISCH (T0..T16) und baut die Composition intern als Alias — das löst das
/// Komma-im-Makro-Argument-Problem von AdHocComposition<A,B,…> (Komma = sonst mehrere Makro-Args).
#define COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(...)                                     \
    using ComdareAdHocPermutationComposition =                                      \
        ::comdare::cache_engine::anatomy::AdHocComposition<__VA_ARGS__>;            \
    COMDARE_DEFINE_ANATOMY_MODULE(ComdareAdHocPermutationComposition)

/// COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT(PT, SE, HW, <17 Anatomie-Achsen>) — L-74a: EINE DLL trägt
/// SOWOHL die 17-Slot-SearchAlgorithm-Anatomie (4 ABI-Symbole, genus()==SearchAlgorithm, organ_count()==17) ALS
/// AUCH die Build-Identität der 3 Build-Achsen (page_type/09b/12) als extern-"C"-Inspection-Symbol
/// (comdare_build_variant_inspect). Beweist Doc 27 §0.1: die 3 Build-Achsen sind Build-Parameter DERSELBEN
/// 17-Slot-Binary (Sub/Build-Varianten DESSELBEN Algorithmus), NICHT eine eigene Gattung (KEINE AdHocComposition<20>).
/// Reihenfolge: die 3 Build-Achsen ZUERST (named), dann die 17 Anatomie-Achsen variadisch (Komma-Problem von
/// AdHocComposition<17>). Host: genus über den Loader + Build-Identität über GetProcAddress/dlsym aus DERSELBEN .dll.
#define COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT(PT, SE, HW, ...)             \
    COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(__VA_ARGS__)                                  \
    COMDARE_DEFINE_BUILD_VARIANT_INSPECTION(comdare_build_variant_inspect, PT, SE, HW)
