// D7 / L-74a — Build-Variant-Inspection-ABI: die Build-Variante DERSELBEN Binary ist via extern-"C"-Symbol
// abrufbar (ABI-Pull-Pfad). 2 Build-Varianten derselben page/hw (Avx512 vs Avx2) liefern über IHRE Inspection-
// Symbole verschiedene BuildVariantDefinitionV1 → host-seitig literal unterscheidbar. In-process (COMDARE_ANATOMY_
// ABI_STATIC); der echte dlsym-Pull über die .dll-Grenze ist die DLL-Variante. Build: cl /I libs/cache_engine/include.

#define COMDARE_ANATOMY_ABI_STATIC   // in-process: EXPORT-Macro = leer (kein dllimport/dllexport)
#include "cache_engine/abi/build_variant_inspection.hpp"

#include <cstdint>
#include <iostream>
#include <string>

namespace cea = comdare::cache_engine::anatomy;

struct StubDenseBytePage { static constexpr int  page_kind() noexcept { return 0; }
                           static constexpr bool is_branch() noexcept { return false; }
                           static constexpr bool is_leaf()   noexcept { return true; } };
struct StubAvx512 { static constexpr unsigned vector_width_bits() noexcept { return 512; }
                    static constexpr bool     provides_avx512()   noexcept { return true; } };
struct StubAvx2   { static constexpr unsigned vector_width_bits() noexcept { return 256; }
                    static constexpr bool     provides_avx512()   noexcept { return false; } };
struct StubX86_64 { static constexpr unsigned cache_line_size() noexcept { return 64; }
                    static constexpr bool     numa_capable()    noexcept { return true; } };

// 2 Build-Varianten DERSELBEN page/hw, nur simd verschieden — je ein extern-"C"-Inspection-Symbol.
COMDARE_DEFINE_BUILD_VARIANT_INSPECTION(inspect_variant_avx512, StubDenseBytePage, StubAvx512, StubX86_64)
COMDARE_DEFINE_BUILD_VARIANT_INSPECTION(inspect_variant_avx2,   StubDenseBytePage, StubAvx2,   StubX86_64)

static int g_fail = 0;
template <class A, class B> static void eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e); std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) { std::cout << " (erwartet " << e << ")"; ++g_fail; } std::cout << "\n"; }
static void tr(char const* w, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n"; if (!c) ++g_fail; }

int main() {
    std::cout << "==== D7/L-74a Build-Variant-Inspection-ABI (extern-C-Pull) ====\n";
    cea::BuildVariantDefinitionV1 a{}, b{};
    inspect_variant_avx512(&a);   // Build-Identität der Avx512-Variante über das ABI-Symbol ziehen
    inspect_variant_avx2(&b);     // Build-Identität der Avx2-Variante
    eq("Avx512-Variante: simd_width_bits == 512 (via ABI-Symbol)", a.simd_width_bits, std::uint64_t{512});
    eq("Avx2-Variante:   simd_width_bits == 256 (via ABI-Symbol)", b.simd_width_bits, std::uint64_t{256});
    eq("Avx512-Variante: simd_avx512 == 1", a.simd_avx512, std::uint64_t{1});
    eq("beide: hw_cache_line == 64 (gleiche hw-Achse)", a.hw_cache_line, std::uint64_t{64});
    eq("beide: present_mask == 7", a.present_mask, std::uint64_t{7});
    tr("2 Build-Varianten DERSELBEN page/hw via ABI literal unterscheidbar (a != b)", a != b);
    std::cout << "\n==== D7/L-74a: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
