// L-74a Build-Varianten-DLL (Avx512-Variante) — die MODUL-AUTOR-Seite EINER Build-Variante: ein extern-"C"-
// Inspection-Symbol (COMDARE_DEFINE_BUILD_VARIANT_INSPECTION), das die je-Binary eingebackene
// BuildVariantDefinitionV1 der 3 Build-Achsen (page_type/09b/12) durch die DLL-Grenze zieht. Gebaut als
// perm_buildvariant_avx512.dll (cl /LD /DCOMDARE_ANATOMY_MODULE_BUILD); der Host zieht das Symbol via GetProcAddress.
//
// Zweck (L-74a): beweist über die ECHTE .dll-Grenze, dass 2 Build-Varianten DERSELBEN page/hw (nur simd verschieden)
// host-seitig literal unterscheidbar sind — der ABI-Pull-Pfad, der das DefinitionOnly-Etikett zu einer realen,
// abrufbaren Build-Identität macht (test_d7a beweist das nur in-process via COMDARE_ANATOMY_ABI_STATIC).
//
// Die simd-Typen spiegeln die API-FORM der ECHTEN axis_09b-Wrapper: Avx512Ext trägt `provides_avx512f()` (wie
// Avx512SimdExtension — NICHT `provides_avx512()`), exerziert also den toleranten detect_avx512-Pfad der Factory.
// (Die echten Library-Klassen direkt einzubauen braucht die cmake-generierten flags.hpp-Includes → Folgeschritt.)

#include <cache_engine/abi/build_variant_inspection.hpp>

namespace bv {
struct DenseBytePage { static constexpr int      page_kind()        noexcept { return 0; }   // DenseByte
                       static constexpr bool      is_branch()        noexcept { return true; }
                       static constexpr bool      is_leaf()          noexcept { return false; } };
struct Avx512Ext     { static constexpr unsigned  vector_width_bits() noexcept { return 512; }
                       static constexpr bool       provides_avx512f() noexcept { return true; } };  // echte axis_09b-API
struct X86_64Hw      { static constexpr unsigned  cache_line_size()  noexcept { return 64; }
                       static constexpr bool       numa_capable()     noexcept { return true; } };
}  // namespace bv

COMDARE_DEFINE_BUILD_VARIANT_INSPECTION(comdare_build_variant_inspect, bv::DenseBytePage, bv::Avx512Ext, bv::X86_64Hw)
