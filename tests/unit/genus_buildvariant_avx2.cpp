// L-74a Build-Varianten-DLL (Avx2-Variante) — Gegenstück zu genus_buildvariant_avx512.cpp: DIESELBE page_type/hw,
// nur die simd_extension-Achse ist Avx2 (256-bit) statt Avx512 (512-bit). Gebaut als perm_buildvariant_avx2.dll.
// Exportiert DASSELBE Symbol comdare_build_variant_inspect → der Host zieht es aus beiden DLLs und unterscheidet die
// 2 Build-Varianten DERSELBEN 17-Komposition literal (simd_width_bits 256 vs 512) über die echte .dll-Grenze.
//
// Avx2Ext trägt `provides_avx512f()==false` (echte axis_09b-API-Form) → detect_avx512-Pfad liefert simd_avx512==0.

#include <cache_engine/abi/build_variant_inspection.hpp>

namespace bv {
struct DenseBytePage {
    static constexpr int  page_kind() noexcept { return 0; } // DenseByte (identisch zur avx512-Variante)
    static constexpr bool is_branch() noexcept { return true; }
    static constexpr bool is_leaf() noexcept { return false; }
};
struct Avx2Ext {
    static constexpr unsigned vector_width_bits() noexcept { return 256; }
    static constexpr bool     provides_avx512f() noexcept { return false; }
}; // echte axis_09b-API
struct X86_64Hw {
    static constexpr unsigned cache_line_size() noexcept { return 64; } // identisch zur avx512-Variante
    static constexpr bool     numa_capable() noexcept { return true; }
};
} // namespace bv

COMDARE_DEFINE_BUILD_VARIANT_INSPECTION(comdare_build_variant_inspect, bv::DenseBytePage, bv::Avx2Ext, bv::X86_64Hw)
