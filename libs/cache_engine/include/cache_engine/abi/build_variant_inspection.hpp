#pragma once
// D7 / L-74a (2026-06-02) — Build-Variant-Inspection-ABI: ein extern-"C"-Symbol, das die je-Binary eingebackene
// BuildVariantDefinitionV1 der 3 Build-Achsen (page_type/09b/12) durch die ABI-Grenze zieht. So ist die
// Build-Variante DERSELBEN SearchAlgorithm-Binary host-seitig literal unterscheidbar (Avx512-Variante vs Avx2-
// Variante derselben 17-Komposition) — der ABI-Pull-Pfad, der das DefinitionOnly-Etikett zu einer realen,
// abrufbaren Definition macht. Additive ABI-Erweiterung (eigenes Symbol; alte Loader ignorieren es).
//
// Die 19-Slot-Gattungs-Invariante bleibt (KEINE AdHocComposition<20>); die 3 Build-Achsen sind Codegen-Parameter
// /Inspection NEBEN der 17-Komposition (Doc 27 §0.1). Header-only.

#include "anatomy_module_abi_v1_decl.hpp" // COMDARE_ANATOMY_ABI_EXPORT
#include "../../../anatomy/build_variant_definition.hpp"
#include "../../../topics/hardware/axis_09b_simd_extension/axis_09b_build_coherence.hpp" // GO-3 A1 Kohaerenz-Guard

/// COMDARE_DEFINE_BUILD_VARIANT_INSPECTION(SymbolName, PT, SE, HW) — emittiert ein extern-"C"-Symbol, das die
/// BuildVariantDefinitionV1 der Build-Achsen-Tripel (PT page_type, SE simd_extension, HW general_hardware) liefert.
/// In einer Permutations-.dll NEBEN den 4 Anatomie-Symbolen einsetzbar; der Host zieht die Build-Identität via dlsym.
#define COMDARE_DEFINE_BUILD_VARIANT_INSPECTION(SymbolName, PT, SE, HW)                                                \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT void SymbolName(                                                             \
        ::comdare::cache_engine::anatomy::BuildVariantDefinitionV1* out) noexcept {                                    \
        if (out != nullptr) *out = ::comdare::cache_engine::anatomy::build_variant_definition<PT, SE, HW>();           \
    }

/// GO-3 A1 (Task #5 Hebel-A-Rest, 2026-07-12) — ADDITIVES Kohaerenz-Makro: identisch zum bestehenden
/// COMDARE_DEFINE_BUILD_VARIANT_INSPECTION plus consteval-Kohaerenz-Guard (Deklarations-Wahrheit: deklarierte
/// simd_extension == reale Build-ISA-Stufe, s. axis_09b_build_coherence.hpp). Das BESTEHENDE Makro und der
/// Golden-Pfad (COMDARE_DEFINE_ANATOMY_MODULE/_ADHOC/_BUILDVARIANT) bleiben byte-unveraendert — exakt die
/// SHAPED-Praezedenz (anatomy_module_abi_v1.hpp: "Der Golden-Pfad wird nicht angefasst"). Verlangt die ECHTE
/// axis_09b-Wrapper-API (BuildCoherenceCheckableSimdExtension); real-geformte Stubs bleiben beim Legacy-Makro.
/// Die zum SE passende Compiler-Flag setzt comdare_apply_simd_extension_flags(<target> <EXT>) in
/// cmake/isa_features.cmake — fehlt sie, bricht der static_assert den Build (Etikett != Maschinencode).
#define COMDARE_DEFINE_BUILD_VARIANT_INSPECTION_CHECKED(SymbolName, PT, SE, HW)                                        \
    static_assert(                                                                                                     \
        ::comdare::cache_engine::hardware::axis_09b_simd_extension::declared_extension_matches_build<SE>(),            \
        "axis_09b: deklarierte SIMD-Extension != Build-ISA-Stufe -- comdare_apply_simd_extension_flags(<target> "      \
        "<EXT>) fehlt (cmake/isa_features.cmake)");                                                                    \
    COMDARE_DEFINE_BUILD_VARIANT_INSPECTION(SymbolName, PT, SE, HW)
