// L-74a — ADHOC-BUILDVARIANT-DLL-Round-Trip: beweist, dass EINE DLL SOWOHL die 17-Slot-SearchAlgorithm-Anatomie
// ALS AUCH die Build-Identität der 3 Build-Achsen trägt (Doc 27 §0.1: die 3 Build-Achsen modifizieren DIESELBE
// 17-Slot-Binary, NICHT eine eigene Gattung). Aus DERSELBEN .dll werden host-seitig BEIDE gezogen:
//   (1) die Anatomie über den gattungs-agnostischen AnatomyModuleLoader → genus()==SearchAlgorithm, organ_count()==18 (Doc 30 §8.0);
//   (2) die Build-Variante über dll_sym("comdare_build_variant_inspect") → simd_width_bits==512 (Avx512).
// Build: siehe CMake-Registrierung (braucht den Anatomie-Umbrella + Boost + generierte Dirs). Aufruf: <adhoc_buildvariant.dll>.

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>
#include <anatomy/anatomy_base.hpp>
#include <anatomy/build_variant_definition.hpp>

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

namespace loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace ana    = ::comdare::cache_engine::anatomy;

using InspectFn = void (*)(ana::BuildVariantDefinitionV1*);

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

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: test_adhoc_buildvariant_dll <adhoc_buildvariant.dll>\n";
        return 2;
    }
    char const* dll = argv[1];
    std::cout << "==== L-74a ADHOC-BUILDVARIANT-DLL: 17-Anatomie + 3 Build-Achsen in EINER DLL ====\n";

    // (1) Die ANATOMIE über den gattungs-agnostischen Loader (die 4 ABI-Symbole) — DERSELBE Loader wie alle Module.
    std::cout << "-- (1) Anatomie über den AnatomyModuleLoader --\n";
    loader::AnatomyModuleHandle handle;
    int const                   st = loader::AnatomyModuleLoader::load(dll, handle);
    tr("load == status_ok", st == loader::status_ok);
    if (st == loader::status_ok) {
        ana::IAnatomyBase* a = handle.anatomy();
        tr("anatomy() != null", a != nullptr);
        if (a != nullptr) {
            tr("genus() == SearchAlgorithm (die 17-Slot-Anatomie)", a->genus() == ana::AnatomyGenus::SearchAlgorithm);
            eq("organ_count() == 17", a->organ_count(), std::size_t{17});
        }
    } else {
        std::cerr << "  status: " << loader::status_name(st) << "\n";
    }

    // (2) Die BUILD-VARIANTE über dll_sym aus DERSELBEN .dll (das Inspection-Symbol NEBEN den 4 ABI-Symbolen).
    std::cout << "-- (2) Build-Identität über comdare_build_variant_inspect (DIESELBE .dll) --\n";
    DllHandle h = dll_open(dll);
    tr("dll_open (zweiter Handle auf DIESELBE .dll)", h != nullptr);
    if (h != nullptr) {
        auto fn = reinterpret_cast<InspectFn>(dll_sym(h, "comdare_build_variant_inspect"));
        tr("dll_sym(comdare_build_variant_inspect) != null", fn != nullptr);
        if (fn != nullptr) {
            ana::BuildVariantDefinitionV1 v{};
            fn(&v);
            eq("simd_width_bits == 512 (Avx512, DIESELBE Binary)", v.simd_width_bits, std::uint64_t{512});
            eq("simd_avx512 == 1 (via provides_avx512f der echten Achse)", v.simd_avx512, std::uint64_t{1});
            eq("page_kind == 0 (DenseByte)", v.page_kind, std::uint64_t{0});
            eq("hw_cache_line == 64 (X86_64)", v.hw_cache_line, std::uint64_t{64});
            eq("present_mask == 7 (alle 3 Build-Achsen)", v.present_mask, std::uint64_t{7});
        }
        dll_close(h);
    }

    std::cout
        << "\nKERN-BEWEIS: dieselbe .dll trägt die 17-Slot-Anatomie (genus==SearchAlgorithm) UND die 3-Build-Achsen-\n"
           "Identität (simd 512) — die Build-Achsen sind Build-Parameter DERSELBEN Binary (Doc 27 §0.1).\n";
    std::cout << "\n==== L-74a ADHOC-BUILDVARIANT: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
