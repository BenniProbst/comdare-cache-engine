// L-74a Build-Varianten-DLL mit den ECHTEN Wrappern (Avx2-Variante) — Gegenstück zu genus_buildvariant_real_avx512.
// DIESELBE echte page_type/hw (DenseBytePageType / X86_64HardwareProfile), nur die echte simd_extension-Achse ist
// Avx2SimdExtension (256-bit) statt Avx512. Exportiert dasselbe Symbol comdare_build_variant_inspect → der Host
// unterscheidet die 2 Build-Varianten DERSELBEN ECHTEN Komposition literal (256 vs 512) über die .dll-Grenze.
//
// GO-3 A1 (Task #5 Hebel-A-Rest, 2026-07-12) — DEKLARATIONS-WAHRHEIT: diese DLL baut jetzt MIT der echten
// AVX2-Flag (comdare_apply_simd_extension_flags(<target> AVX2), s. tests/unit/CMakeLists.txt) und emittiert
// das CHECKED-Makro (consteval-Kohaerenz-Guard: deklarierte Extension 256 == Build-ISA-Stufe __AVX2__).
// Vorher deklarierte diese DLL Avx2 OHNE -mavx2 — Mess-Etikett != Mess-Gegenstand (Dossier GO3 §2.3).
// Ausfuehrung host-gated (die Binary enthaelt jetzt echten AVX2-Maschinencode).

#include <cache_engine/abi/build_variant_inspection.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_dense_byte.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_avx2.hpp>
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_x86_64.hpp>

#include <cstdint>

// CHECKED-Makro (build_variant_inspection.hpp) ist cppcheck ohne vollstaendige Include-Konfiguration
// unbekannt; der Compiler prueft es (static_assert-Kohaerenz-Guard). Daher (bare Direktive, kein Trailing-Text):
// cppcheck-suppress unknownMacro
COMDARE_DEFINE_BUILD_VARIANT_INSPECTION_CHECKED(
    comdare_build_variant_inspect, ::comdare::cache_engine::nodes::axis_01_page_type::DenseBytePageType,
    ::comdare::cache_engine::hardware::axis_09b_simd_extension::Avx2SimdExtension,
    ::comdare::cache_engine::hardware::axis_12_general_hardware::X86_64HardwareProfile)

// GO-3 A1 Realitaets-Probe: dieselbe ZENTRALE consteval-Kaskade (axis_09b_build_coherence.hpp), ausgewertet
// in DIESER Uebersetzungseinheit — der Host vergleicht Etikett (POD simd_width_bits) und Realitaet
// (Build-ISA-Stufe) DERSELBEN Binary literal ueber die .dll-Grenze (test_ap5_simd_extension_coherence).
extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_build_isa_width_probe() noexcept {
    constexpr int kWidth = ::comdare::cache_engine::hardware::axis_09b_simd_extension::actual_build_simd_width_bits();
    return static_cast<std::uint64_t>(kWidth);
}
