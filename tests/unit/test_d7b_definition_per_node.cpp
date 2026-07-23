// D7b / L-74b — per-Knoten getragene Build-Achsen-DEFINITION: NodeValue trägt build_def (BuildVariantDefinitionV1)
// + build_def_real. Gemessene Knoten tragen die reale, ABI-gezogene Build-Identität der 3 Build-Achsen
// (page_type/09b/12); ungemessene bleiben build_def_real=false (SPARSE-Kontrast, exakt wie observer_real).
// + 26-Vollständigkeit (Reklass 2026-07-20 P-OBS): 17 SearchAlgorithmObserver + 9 DefinitionOnly + 0 ContainerObserver == 26, keine fällt weg.
// Build: cl /I libs/cache_engine (kein Boost).

#include "builder/experiment_tree/experiment_tree.hpp"                 // NodeValue
#include "builder/experiment_tree/build_variant_definition_reader.hpp" // read_build_variant
#include "builder/experiment_tree/axis_observer_classification.hpp"    // kAxisObserverClasses / count_observer_kind

#include <cstdint>
#include <iostream>
#include <string>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace cea = comdare::cache_engine::anatomy;

// Build-Achsen-Property-Typen in der API-FORM der ECHTEN Wrapper (DenseBytePageType / Avx512SimdExtension /
// X86_64HardwareProfile): SE trägt provides_avx512f() (NICHT provides_avx512) → exerziert detect_avx512.
struct DenseBytePage {
    static constexpr int  page_kind() noexcept { return 0; }
    static constexpr bool is_branch() noexcept { return true; }
    static constexpr bool is_leaf() noexcept { return false; }
};
struct Avx512Ext {
    static constexpr unsigned vector_width_bits() noexcept { return 512; }
    static constexpr bool     provides_avx512f() noexcept { return true; }
};
struct Avx2Ext {
    static constexpr unsigned vector_width_bits() noexcept { return 256; }
    static constexpr bool     provides_avx512f() noexcept { return false; }
};
struct X86_64Hw {
    static constexpr unsigned cache_line_size() noexcept { return 64; }
    static constexpr bool     numa_capable() noexcept { return true; }
};
// A6 (G2-2): AVX10-API-Formen fuer detect_avx10_version. Praezise API traegt die konvergierte Version als Wert;
// grobe API die ja/nein-Form; die bestehenden Avx512Ext/Avx2Ext tragen KEINE -> Fallback 0.
struct Avx10PreciseExt {
    static constexpr unsigned      vector_width_bits() noexcept { return 512; }
    static constexpr bool          provides_avx512f() noexcept { return true; }
    static constexpr std::uint32_t provides_avx10_version() noexcept { return 2; } // AVX10.2
};
struct Avx10CoarseExt {
    static constexpr unsigned vector_width_bits() noexcept { return 256; }
    static constexpr bool     provides_avx512f() noexcept { return false; }
    static constexpr bool     provides_avx10() noexcept { return true; } // grobe ja/nein-API -> 1
};

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
    std::cout << "==== D7b / L-74b — per-Knoten Build-Achsen-Definition + 22-Vollständigkeit ====\n";

    // (a) Die Definition ist compile-time abrufbar (reine Build-Konstante).
    constexpr auto def512 = cea::build_variant_definition<DenseBytePage, Avx512Ext, X86_64Hw>();
    static_assert(def512.simd_width_bits == 512, "Avx512 → 512 (compile-time)");
    eq("build_variant_definition Avx512 → simd_width_bits == 512", def512.simd_width_bits, std::uint64_t{512});

    // (a2 / A6 G2-2) AVX10-Feld: detect_avx10_version -- praezise API traegt den Wert, grobe API 1/0, Fallback 0.
    constexpr auto def_avx10p = cea::build_variant_definition<DenseBytePage, Avx10PreciseExt, X86_64Hw>();
    static_assert(def_avx10p.simd_avx10_version == 2, "provides_avx10_version()==2 -> Feld 2 (compile-time)");
    eq("A6 AVX10 praezise: simd_avx10_version == 2", def_avx10p.simd_avx10_version, std::uint64_t{2});
    constexpr auto def_avx10c = cea::build_variant_definition<DenseBytePage, Avx10CoarseExt, X86_64Hw>();
    eq("A6 AVX10 grob (provides_avx10): simd_avx10_version == 1", def_avx10c.simd_avx10_version, std::uint64_t{1});
    eq("A6 Legacy ohne AVX10 (Avx512Ext): simd_avx10_version == 0 (Fallback)", def512.simd_avx10_version,
       std::uint64_t{0});
    // AVX10 gehoert unter die simd-Achse -> KEIN neues present-Bit (present_mask bleibt 7 = alle 3 Achsen).
    eq("A6 present_mask == 7 (AVX10 fuegt KEIN present-Bit hinzu)", def_avx10p.present_mask, std::uint64_t{7});

    // (b) GEMESSENER Knoten trägt die reale Build-Definition (build_def_real==true); UNGEMESSENER bleibt false (SPARSE).
    std::cout << "\n-- per-Knoten: gemessen (real) vs ungemessen (SPARSE) --\n";
    ex::NodeValue measured;
    ex::read_build_variant<DenseBytePage, Avx512Ext, X86_64Hw>(measured);
    tr("gemessener Knoten: build_def_real == true", measured.build_def_real);
    eq("gemessener Knoten: build_def.simd_width_bits == 512", measured.build_def.simd_width_bits, std::uint64_t{512});
    eq("gemessener Knoten: build_def.simd_avx512 == 1 (via provides_avx512f)", measured.build_def.simd_avx512,
       std::uint64_t{1});
    eq("gemessener Knoten: build_def.hw_cache_line == 64", measured.build_def.hw_cache_line, std::uint64_t{64});

    ex::NodeValue unmeasured; // nie read_build_variant → Default
    tr("ungemessener Knoten: build_def_real == false (SPARSE-Kontrast)", !unmeasured.build_def_real);
    tr("ungemessener Knoten: build_def == Default{} (kein Pseudo-Wert)",
       unmeasured.build_def == cea::BuildVariantDefinitionV1{});

    // Zweite Build-Variante (Avx2) DERSELBEN page/hw → andere reale Definition (Build-Identität unterscheidbar).
    ex::NodeValue measured_avx2;
    ex::read_build_variant<DenseBytePage, Avx2Ext, X86_64Hw>(measured_avx2);
    eq("Avx2-Knoten: build_def.simd_width_bits == 256", measured_avx2.build_def.simd_width_bits, std::uint64_t{256});
    tr("Avx512-Knoten != Avx2-Knoten (Build-Varianten-Definition unterscheidbar)",
       !(measured.build_def == measured_avx2.build_def));

    // (c) 26-Vollständigkeit (korr. 2026-06-03, Doc 30 §8.0; Reklass 2026-07-20 P-OBS): 17 SearchAlgorithmObserver
    // (kCompositionAxisNames inkl. queuing q1/q2, OHNE telemetry/isa) + 9 DefinitionOnly (3 build-only + telemetry
    // (INC-2c) + isa (INC-2d) System-Achsen + 4 node-shape #234-K) + 0 ContainerObserver == 26. ContainerObserver reserviert (#87).
    std::cout << "\n-- 26-Vollständigkeit (keine Achse fällt weg) --\n";
    constexpr auto n_sa  = ex::count_observer_kind(ex::AxisObserverKind::SearchAlgorithmObserver);
    constexpr auto n_def = ex::count_observer_kind(ex::AxisObserverKind::DefinitionOnly);
    constexpr auto n_ctr = ex::count_observer_kind(ex::AxisObserverKind::ContainerObserver);
    static_assert(n_sa + n_def + n_ctr == 26, "17 + 9 + 0 == 26");
    eq("SearchAlgorithmObserver == 17 (kCompositionAxisNames, inkl. queuing q1/q2)", n_sa, std::size_t{17});
    eq("DefinitionOnly (page_type/09b/12 + telemetry + isa + 4 node-shape #234-K) == 9", n_def, std::size_t{9});
    eq("ContainerObserver == 0 (queuing→SA; reserviert für echte Container-Gattung #87)", n_ctr, std::size_t{0});
    eq("Summe == 26 (kAxisObserverClasses)", ex::kAxisObserverClasses.size(), std::size_t{26});

    std::cout << "\n==== D7b / L-74b: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
