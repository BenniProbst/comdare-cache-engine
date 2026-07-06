// L-74a Build-Varianten-DLL-Round-Trip — der ECHTE dlsym-Pull über die .dll-Grenze (test_d7a beweist denselben
// ABI-Pull nur in-process via COMDARE_ANATOMY_ABI_STATIC). Lädt 2 Build-Varianten-DLLs DERSELBEN page/hw (nur
// simd verschieden: Avx512 vs Avx2), zieht aus JEDER das extern-"C"-Symbol comdare_build_variant_inspect via
// dll_sym (LoadLibrary/dlopen-Shim) und prüft, dass die BuildVariantDefinitionV1 host-seitig literal unterscheidbar ist (512 vs 256).
//
// Beweist L-74a-Kern: die 3 Build-Achsen (page_type/09b/12) sind als reale, ABI-gezogene Build-Identität DERSELBEN
// Binary über die DLL-Grenze abrufbar — DefinitionOnly-Etikett → reale, cross-boundary abrufbare Definition.
// Aufruf: test_buildvariant_dll <avx512.dll> <avx2.dll>. Build: siehe tests/unit/buildvariant_dll_roundtrip.ps1.

#include "anatomy/build_variant_definition.hpp"

// Portables DLL-Shim (#278, Job 214584): identischer Symbol-Roundtrip auf Windows UND Linux —
// kein Gate/Skip, der Test läuft auf beiden Plattformen echt.
#if defined(_WIN32)
#include <windows.h>
using DllHandle = HMODULE;
static DllHandle dll_open(char const* p) { return LoadLibraryA(p); }
static void*     dll_sym(DllHandle h, char const* s) { return reinterpret_cast<void*>(GetProcAddress(h, s)); }
static void      dll_close(DllHandle h) { dll_close(h); }
#else
#include <dlfcn.h>
using DllHandle = void*;
static DllHandle dll_open(char const* p) { return dlopen(p, RTLD_NOW); }
static void*     dll_sym(DllHandle h, char const* s) { return dlsym(h, s); }
static void      dll_close(DllHandle h) { dlclose(h); }
#endif

#include <cstdint>
#include <iostream>
#include <string>

namespace cea = comdare::cache_engine::anatomy;

using InspectFn = void (*)(cea::BuildVariantDefinitionV1*);

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

// Lädt eine DLL, zieht das Inspection-Symbol und füllt die Build-Identität (errno-style: false = Lade-/Symbol-Fehler).
static bool pull_build_variant(char const* dll_path, cea::BuildVariantDefinitionV1& out) {
    DllHandle h = dll_open(dll_path);
    if (h == nullptr) {
        std::cout << "  [ERR] dll_open fehlgeschlagen: " << dll_path << "\n";
        ++g_fail;
        return false;
    }
    auto fn = reinterpret_cast<InspectFn>(dll_sym(h, "comdare_build_variant_inspect"));
    if (fn == nullptr) {
        std::cout << "  [ERR] dll_sym(comdare_build_variant_inspect) == null: " << dll_path << "\n";
        ++g_fail;
        dll_close(h);
        return false;
    }
    fn(&out); // Build-Identität über die ECHTE .dll-Grenze ziehen
    dll_close(h);
    return true;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: test_buildvariant_dll <avx512.dll> <avx2.dll>\n";
        return 2;
    }
    std::cout << "==== L-74a Build-Varianten-DLL-Round-Trip (echter ABI-Pull über die .dll-Grenze) ====\n";

    cea::BuildVariantDefinitionV1 a512{}, b256{};
    tr("Avx512-Variante-DLL geladen + Symbol gezogen", pull_build_variant(argv[1], a512));
    tr("Avx2-Variante-DLL geladen + Symbol gezogen", pull_build_variant(argv[2], b256));

    std::cout << "-- Avx512-Variante (über die DLL-Grenze) --\n";
    eq("simd_width_bits == 512", a512.simd_width_bits, std::uint64_t{512});
    eq("simd_avx512 == 1 (detect_avx512 via provides_avx512f)", a512.simd_avx512, std::uint64_t{1});
    eq("page_kind == 0 (DenseByte)", a512.page_kind, std::uint64_t{0});
    eq("hw_cache_line == 64", a512.hw_cache_line, std::uint64_t{64});
    eq("present_mask == 7 (alle 3 Build-Achsen)", a512.present_mask, std::uint64_t{7});

    std::cout << "-- Avx2-Variante (über die DLL-Grenze) --\n";
    eq("simd_width_bits == 256", b256.simd_width_bits, std::uint64_t{256});
    eq("simd_avx512 == 0", b256.simd_avx512, std::uint64_t{0});
    eq("page_kind == 0 (DenseByte, identisch)", b256.page_kind, std::uint64_t{0});
    eq("hw_cache_line == 64 (identisch)", b256.hw_cache_line, std::uint64_t{64});

    // KERN-BEWEIS: 2 Build-Varianten DERSELBEN page/hw, über die echte .dll-Grenze literal unterscheidbar.
    tr("a512 != b256 (Build-Varianten cross-.dll unterscheidbar)", a512 != b256);
    tr("nur simd differiert: page/hw identisch",
       a512.page_kind == b256.page_kind && a512.hw_cache_line == b256.hw_cache_line);

    std::cout << "\n==== L-74a Build-Varianten-DLL: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
