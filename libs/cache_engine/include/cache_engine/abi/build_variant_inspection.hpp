#pragma once
// D7 / L-74a (2026-06-02) — Build-Variant-Inspection-ABI: ein extern-"C"-Symbol, das die je-Binary eingebackene
// BuildVariantDefinitionV1 der 3 Build-Achsen (page_type/09b/12) durch die ABI-Grenze zieht. So ist die
// Build-Variante DERSELBEN SearchAlgorithm-Binary host-seitig literal unterscheidbar (Avx512-Variante vs Avx2-
// Variante derselben 17-Komposition) — der ABI-Pull-Pfad, der das DefinitionOnly-Etikett zu einer realen,
// abrufbaren Definition macht. Additive ABI-Erweiterung (eigenes Symbol; alte Loader ignorieren es).
//
// Die 17-Slot-Gattungs-Invariante bleibt (KEINE AdHocComposition<20>); die 3 Build-Achsen sind Codegen-Parameter
// /Inspection NEBEN der 17-Komposition (Doc 27 §0.1). Header-only.

#include "anatomy_module_abi_v1_decl.hpp" // COMDARE_ANATOMY_ABI_EXPORT
#include "../../../anatomy/build_variant_definition.hpp"

/// COMDARE_DEFINE_BUILD_VARIANT_INSPECTION(SymbolName, PT, SE, HW) — emittiert ein extern-"C"-Symbol, das die
/// BuildVariantDefinitionV1 der Build-Achsen-Tripel (PT page_type, SE simd_extension, HW general_hardware) liefert.
/// In einer Permutations-.dll NEBEN den 4 Anatomie-Symbolen einsetzbar; der Host zieht die Build-Identität via dlsym.
#define COMDARE_DEFINE_BUILD_VARIANT_INSPECTION(SymbolName, PT, SE, HW)                                                \
    extern "C" COMDARE_ANATOMY_ABI_EXPORT void SymbolName(                                                             \
        ::comdare::cache_engine::anatomy::BuildVariantDefinitionV1* out) noexcept {                                    \
        if (out != nullptr) *out = ::comdare::cache_engine::anatomy::build_variant_definition<PT, SE, HW>();           \
    }
