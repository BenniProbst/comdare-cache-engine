// L-74a Build-Varianten-DLL mit den ECHTEN Wrappern (Avx2-Variante) — Gegenstück zu genus_buildvariant_real_avx512.
// DIESELBE echte page_type/hw (DenseBytePageType / X86_64HardwareProfile), nur die echte simd_extension-Achse ist
// Avx2SimdExtension (256-bit) statt Avx512. Exportiert dasselbe Symbol comdare_build_variant_inspect → der Host
// unterscheidet die 2 Build-Varianten DERSELBEN ECHTEN Komposition literal (256 vs 512) über die .dll-Grenze.

#include <cache_engine/abi/build_variant_inspection.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_dense_byte.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_avx2.hpp>
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_x86_64.hpp>

COMDARE_DEFINE_BUILD_VARIANT_INSPECTION(
    comdare_build_variant_inspect, ::comdare::cache_engine::nodes::axis_01_page_type::DenseBytePageType,
    ::comdare::cache_engine::hardware::axis_09b_simd_extension::Avx2SimdExtension,
    ::comdare::cache_engine::hardware::axis_12_general_hardware::X86_64HardwareProfile)
