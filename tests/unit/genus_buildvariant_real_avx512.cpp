// L-74a Build-Varianten-DLL mit den ECHTEN axis_01/09b/12-Library-Wrapper-KLASSEN (statt real-geformter Stubs).
// Baut NUR im CMake-Kontext (braucht die generierten flags.hpp-Dirs + Boost::mp11 → siehe die Registrierung in
// tests/unit/CMakeLists.txt). Beweist, dass die REALEN Wrapper (DenseBytePageType / Avx512SimdExtension /
// X86_64HardwareProfile) in eine Build-Varianten-DLL einbaubar sind und ihre Build-Identität über die .dll-Grenze
// gezogen wird (detect_avx512 trifft den `provides_avx512f`-Pfad der ECHTEN axis_09b-Achse → simd_avx512==1).

#include <cache_engine/abi/build_variant_inspection.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_dense_byte.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_avx512.hpp>
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_x86_64.hpp>

COMDARE_DEFINE_BUILD_VARIANT_INSPECTION(
    comdare_build_variant_inspect, ::comdare::cache_engine::nodes::axis_01_page_type::DenseBytePageType,
    ::comdare::cache_engine::hardware::axis_09b_simd_extension::Avx512SimdExtension,
    ::comdare::cache_engine::hardware::axis_12_general_hardware::X86_64HardwareProfile)
