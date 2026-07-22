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

#include "anatomy_module_abi_v1_decl.hpp" // leichte ABI-Schnittstelle (Version/Magic/Factory-Decls/Helper)
#include "anatomy_fingerprint.hpp" // K7b-3: consteval SHA-512-Fingerprint der 4 Stempel-Zeilen (anatomy_fingerprint_hex)
#include "../../../anatomy/abi_adapter.hpp" // SearchAlgorithmAbiAdapter (Makro-Materialisierung)
#include "../../../anatomy/search_algorithm_anatomy.hpp"
#include "../../../anatomy/composition_factory.hpp" // R5.G: AdHocComposition für Auto-Permutations-Codegen
#include "build_variant_inspection.hpp" // L-74a: COMDARE_DEFINE_BUILD_VARIANT_INSPECTION (BUILDVARIANT-Variante)

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
#define COMDARE_DEFINE_ANATOMY_MODULE(CompositionType)                                                                 \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_version() noexcept {                       \
        return (static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR) << 32) |                                         \
               static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MINOR);                                                  \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_magic() noexcept {                         \
        return COMDARE_ANATOMY_ABI_MAGIC;                                                                              \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT ::comdare::cache_engine::anatomy::IAnatomyBase*                              \
    comdare_create_anatomy() noexcept {                                                                                \
        using AnatomyType = ::comdare::cache_engine::anatomy::SearchAlgorithmAnatomy<CompositionType>;                 \
        return new (::std::nothrow)::comdare::cache_engine::anatomy::SearchAlgorithmAbiAdapter<AnatomyType>{};         \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT void comdare_destroy_anatomy(                                                \
        ::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept {                                                \
        delete ptr;                                                                                                    \
    }

/// COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(...) — R5.G: Materialisiert eine AUTO-ENUMERIERTE Permutation
/// (AdHocComposition) als Permutations-Binary, OHNE benannten Composition-Header. Nimmt die 17
/// Achsen-Vendor-Typen VARIADISCH (T0..T16 — 15 Such-Achsen + queuing q1/q2, Doc 30 §8.0) und baut die
/// Composition intern als Alias — das löst das Komma-im-Makro-Argument-Problem von AdHocComposition<A,B,…>.
#define COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(...)                                                                       \
    using ComdareAdHocPermutationComposition = ::comdare::cache_engine::anatomy::AdHocComposition<__VA_ARGS__>;        \
    COMDARE_DEFINE_ANATOMY_MODULE(ComdareAdHocPermutationComposition)

/// COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT(PT, SE, HW, <17 Anatomie-Achsen>) — L-74a: EINE DLL trägt
/// SOWOHL die 17-Slot-SearchAlgorithm-Anatomie (4 ABI-Symbole, genus()==SearchAlgorithm, organ_count()==17 — 15
/// Such-Achsen + queuing q1/q2, Doc 30 §8.0; INC-2c/2d: telemetry+isa sind System-Achsen) ALS AUCH die
/// Build-Identität der 3 Build-Achsen (page_type/09b/12)
/// als extern-"C"-Inspection-Symbol (comdare_build_variant_inspect). Beweist Doc 27 §0.1: die 3 Build-Achsen sind
/// Build-Parameter DERSELBEN Binary (Sub/Build-Varianten DESSELBEN Algorithmus), NICHT eine eigene Gattung.
/// Reihenfolge: die 3 Build-Achsen ZUERST (named), dann die 17 Anatomie-Achsen variadisch (Komma-Problem von
/// AdHocComposition<17>). Host: genus über den Loader + Build-Identität über GetProcAddress/dlsym aus DERSELBEN .dll.
#define COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_BUILDVARIANT(PT, SE, HW, ...)                                              \
    COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(__VA_ARGS__)                                                                   \
    COMDARE_DEFINE_BUILD_VARIANT_INSPECTION(comdare_build_variant_inspect, PT, SE, HW)

/// COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_SHAPED(ShapeType, <17 Anatomie-Achsen>) — 234-V-a (Option A,
/// User-GO 07.07.): Materialisiert eine auto-enumerierte Permutation MIT Shape-Traeger (z.B.
/// axis_btree_order::BtreeOrderKt8). Der Shape faehrt als ADAPTER-Traeger mit (2. Template-Parameter des
/// SearchAlgorithmAbiAdapter) und waehlt ueber `organ_for_search_algo_shaped<S,Shape>` das Shaped-Organ
/// der Pool-Familie — er ist KEIN 18. Composition-Slot: die 17-Slot-ABI-Invariante, organ_count()==17,
/// alle POD-Layouts und ABI-MAJOR bleiben unveraendert. Reihenfolge nach BUILDVARIANT-Praezedenz:
/// benannter Shape ZUERST, dann die 17 Anatomie-Achsen variadisch (Komma-Problem von AdHocComposition).
/// Bewusst SELBSTSTAENDIG (4 Symbole erneut definiert statt Basis-Makro-Refactor): der Golden-Pfad
/// (COMDARE_DEFINE_ANATOMY_MODULE/_ADHOC/_BUILDVARIANT) wird nicht angefasst.
#define COMDARE_DEFINE_ANATOMY_MODULE_ADHOC_SHAPED(ShapeType, ...)                                                     \
    using ComdareAdHocPermutationComposition = ::comdare::cache_engine::anatomy::AdHocComposition<__VA_ARGS__>;        \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_version() noexcept {                       \
        return (static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR) << 32) |                                         \
               static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MINOR);                                                  \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_magic() noexcept {                         \
        return COMDARE_ANATOMY_ABI_MAGIC;                                                                              \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT ::comdare::cache_engine::anatomy::IAnatomyBase*                              \
    comdare_create_anatomy() noexcept {                                                                                \
        using AnatomyType =                                                                                            \
            ::comdare::cache_engine::anatomy::SearchAlgorithmAnatomy<ComdareAdHocPermutationComposition>;              \
        return new (                                                                                                   \
            ::std::nothrow)::comdare::cache_engine::anatomy::SearchAlgorithmAbiAdapter<AnatomyType, ShapeType>{};      \
    }                                                                                                                  \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT void comdare_destroy_anatomy(                                                \
        ::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept {                                                \
        delete ptr;                                                                                                    \
    }

/// COMDARE_ANATOMY_VERSION_STAMP_MERGE(organ_lit, system_lit, measurement_lit, merge_lit) -- K7a (Section 59,
/// 2026-07-20): der VOLLE 4-String-Materialisierer. Materialisiert das OPTIONALE extern-"C"-Probe-Symbol
/// comdare_anatomy_version_lines() aus VIER String-Literalen (Organ-, System-, Mess-Tooling-HAUPT- und dem DRITTEN
/// Merge-Kombinations-Stempel; kMergeAxisVersionLine traegt die Merge-Art + Namen/Versionen der beteiligten Achsen-
/// Algorithmen, ce-only/self -> ""). Die Literale werden im Modul als static constexpr char[] hinterlegt (KEIN
/// std::string im Modul); der zurueckgegebene POD traegt nur Zeiger + Laengen. KEIN Loader-Pflicht-Symbol -> KEIN
/// ABI-Bruch. Stempel-Strings sind C-literal-sicher (nur =@;.+_ und alnum, keine Quotes/Backslashes).
/// K7b-3 (Section 62-B, 2026-07-22): INNEN wird zusaetzlich der SHA-512-Fingerprint von concat(organ+system+
/// measurement+merge) materialisiert (anatomy_fingerprint_hex, consteval) und als 5. POD-Feld sha512_line/sha512_len
/// abgelegt. Die EINGABE bleibt 4 String-Literale -> der emittierte Makro-Call ist byte-identisch (golden-neutral);
/// der Fingerprint entsteht rein in der Makro-Expansion, nicht im emittierten Quelltext.
#define COMDARE_ANATOMY_VERSION_STAMP_MERGE(organ_lit, system_lit, measurement_lit, merge_lit)                         \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT ::comdare::cache_engine::abi::AnatomyVersionLines const*                     \
    comdare_anatomy_version_lines() noexcept {                                                                         \
        static constexpr char kO[] = organ_lit;                                                                        \
        static constexpr char kS[] = system_lit;                                                                       \
        static constexpr char kM[] = measurement_lit;                                                                  \
        static constexpr char kG[] = merge_lit;                                                                        \
        static constexpr auto kFP =                                                                                    \
            ::comdare::cache_engine::abi::anatomy_fingerprint_hex(organ_lit, system_lit, measurement_lit, merge_lit);  \
        static constexpr ::comdare::cache_engine::abi::AnatomyVersionLines kL{                                         \
            ::comdare::cache_engine::abi::kAnatomyVersionLinesLayout,                                                  \
            0u,                                                                                                        \
            kO,                                                                                                        \
            sizeof(kO) - 1,                                                                                            \
            kS,                                                                                                        \
            sizeof(kS) - 1,                                                                                            \
            kM,                                                                                                        \
            sizeof(kM) - 1,                                                                                            \
            kG,                                                                                                        \
            sizeof(kG) - 1,                                                                                            \
            kFP.data(),                                                                                                \
            kFP.size() - 1};                                                                                           \
        return &kL;                                                                                                    \
    }

/// COMDARE_ANATOMY_VERSION_STAMP_M(organ_lit, system_lit, measurement_lit) -- W12-A3 (Section 43) 3-arg-Form:
/// leitet an die 4-arg _MERGE-Form mit LEEREM Merge-Stempel weiter (merge_line -> "", merge_len -> 0). Traegt
/// die gewaehlte Mess-Tooling-HAUPT-Wahl {wallclock/macro/micro} (kMeasurementAxisVersionLine, NUR die Haupt-Achse).
#define COMDARE_ANATOMY_VERSION_STAMP_M(organ_lit, system_lit, measurement_lit)                                        \
    COMDARE_ANATOMY_VERSION_STAMP_MERGE(organ_lit, system_lit, measurement_lit, "")

/// COMDARE_ANATOMY_VERSION_STAMP(organ_lit, system_lit) -- W12-A2 Rueckwaerts-kompatible 2-arg-Form: leitet an
/// die 3-arg _M-Form mit LEEREM Mess-Tooling-Stempel weiter (measurement_line -> "", measurement_len -> 0). Der
/// Emitter (adhoc_emitter.hpp) haengt diese Makro-Zeile NACH COMDARE_DEFINE_ANATOMY_MODULE_ADHOC an; bis die
/// Mess-Tooling-HAUPT-Auffaecherung (S4/P-MESSTOOL) den gewaehlten Tooling-Stempel durchreicht, bleibt das
/// Mess-Feld leer (ehrlich: kein Tooling einkompiliert), waehrend das POD-Layout bereits final ist. Der ce-only-/
/// Katalog-Pfad emittiert weiterhin GENAU diese 2-arg-Form -> der emittierte Quelltext bleibt byte-identisch
/// (merge_line -> "" via _M/_MERGE-Weiterleitung; golden-CRC unberuehrt).
#define COMDARE_ANATOMY_VERSION_STAMP(organ_lit, system_lit) COMDARE_ANATOMY_VERSION_STAMP_M(organ_lit, system_lit, "")
