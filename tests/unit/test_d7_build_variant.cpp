// D7 / L-74b — BuildVariantDefinitionV1 + Reader: die 3 Build-Achsen (page_type/09b/12) tragen je eine REALE,
// compile-time abrufbare, ABI-stabile Definition (statt nur einer statischen DefinitionOnly-Klassifikation).
// Beweis: 2 Build-Varianten DERSELBEN page/hw unterscheiden sich literal in der simd-Definition (verschiedene
// Build-Identität). constexpr → reine Definition, KEIN Laufzeit-Observer (Doc 27 §3). Build: cl /I libs/cache_engine.

#include "anatomy/build_variant_definition.hpp"

#include <cstdint>
#include <iostream>
#include <string>

namespace cea = comdare::cache_engine::anatomy;

// Property-API-Stubs der 3 Build-Achsen (alle static constexpr — wie die echten page_type/09b/12-Wrapper).
struct StubDenseBytePage {
    static constexpr int  page_kind() noexcept { return 0; } // DenseByte
    static constexpr bool is_branch() noexcept { return false; }
    static constexpr bool is_leaf() noexcept { return true; }
};
struct StubBPlusPage {
    static constexpr int  page_kind() noexcept { return 5; } // BPlus
    static constexpr bool is_branch() noexcept { return true; }
    static constexpr bool is_leaf() noexcept { return false; }
};
struct StubAvx512 {
    static constexpr unsigned vector_width_bits() noexcept { return 512; }
    static constexpr bool     provides_avx512() noexcept { return true; }
};
struct StubAvx2 {
    static constexpr unsigned vector_width_bits() noexcept { return 256; }
    static constexpr bool     provides_avx512() noexcept { return false; }
};
struct StubX86_64 {
    static constexpr unsigned cache_line_size() noexcept { return 64; }
    static constexpr bool     numa_capable() noexcept { return true; }
};

// COMPILE-TIME: die Definition ist constexpr (reine Build-Konstante, kein Laufzeit-Observer).
constexpr auto kD1 = cea::build_variant_definition<StubDenseBytePage, StubAvx512, StubX86_64>();
constexpr auto kD2 = cea::build_variant_definition<StubDenseBytePage, StubAvx2, StubX86_64>();
constexpr auto kD3 = cea::build_variant_definition<StubBPlusPage, StubAvx512, StubX86_64>();
static_assert(kD1.simd_width_bits == 512, "Avx512 → 512 (compile-time)");
static_assert(kD2.simd_width_bits == 256, "Avx2 → 256 (compile-time)");
static_assert(kD1 != kD2, "2 Build-Varianten (Avx512 vs Avx2) DERSELBEN page/hw → verschiedene Build-Identitaet");
static_assert(kD1 != kD3, "2 Build-Varianten (DenseByte vs BPlus) → verschiedene Build-Identitaet");

static int g_fail = 0;
template <class A, class B>
static void eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) {
        std::cout << " (erwartet " << e << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
static void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

int main() {
    std::cout << "==== D7 BuildVariantDefinitionV1 (Definition-statt-Observer der 3 Build-Achsen) ====\n";
    // A6 (G2-2, 2026-07-23): der POD waechst um simd_avx10_version (append-only ans Ende) 8->9 uint64 = 64->72 Byte;
    // kBuildVariantDefinitionVersion 1->2. Die 8 Bestandsfelder behalten Offset 0..63 (nur die Groesse waechst).
    eq("BuildVariantDefinitionV1 = 9 uint64 (A6: +simd_avx10_version)", sizeof(cea::BuildVariantDefinitionV1),
       std::size_t{9 * 8});
    eq("kBuildVariantDefinitionVersion == 2 (A6)", cea::kBuildVariantDefinitionVersion, std::uint32_t{2});

    eq("D1.page_kind == 0 (DenseByte)", kD1.page_kind, std::uint64_t{0});
    eq("D1.page_is_leaf == 1", kD1.page_is_leaf, std::uint64_t{1});
    eq("D1.simd_width_bits == 512 (Avx512)", kD1.simd_width_bits, std::uint64_t{512});
    eq("D1.simd_avx512 == 1", kD1.simd_avx512, std::uint64_t{1});
    eq("D1.simd_avx10_version == 0 (A6: kein AVX10-Wrapper deklariert)", kD1.simd_avx10_version, std::uint64_t{0});
    eq("D1.hw_cache_line == 64", kD1.hw_cache_line, std::uint64_t{64});
    eq("D1.present_mask == 7 (alle 3 Build-Achsen belegt)", kD1.present_mask, std::uint64_t{7});

    eq("D2.simd_width_bits == 256 (Avx2)", kD2.simd_width_bits, std::uint64_t{256});
    eq("D2.simd_avx512 == 0", kD2.simd_avx512, std::uint64_t{0});
    eq("D3.page_kind == 5 (BPlus)", kD3.page_kind, std::uint64_t{5});

    // Kern-Beleg L-74b: je Build-Achsen-Tripel eine REALE Definition; Build-Varianten literal unterscheidbar.
    tr("D1 != D2 (Avx512 vs Avx2 — verschiedene Build-Variante derselben page/hw)", kD1 != kD2);
    tr("D1 != D3 (DenseByte vs BPlus — verschiedene Build-Variante)", kD1 != kD3);

    std::cout << "\n==== D7 BuildVariant: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
