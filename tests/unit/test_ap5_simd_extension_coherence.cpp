// GO-3 A1 (Task #5 Hebel-A-Rest, 2026-07-12) — Kohaerenz-Beweis "Etikett == Realitaet" ueber die echte
// .dll-Grenze: laedt die 2 real-Wrapper-Build-Varianten-DLLs (perm_buildvariant_real_avx512/_real_avx2 —
// jetzt MIT echten ISA-Flags + CHECKED-Makro gebaut, s. tests/unit/CMakeLists.txt) und prueft je DLL:
//   (1) ETIKETT:   POD BuildVariantDefinitionV1.simd_width_bits (512 bzw. 256) via comdare_build_variant_inspect,
//   (2) REALITAET: die im DLL-Build ausgewertete zentrale consteval-Kaskade via comdare_build_isa_width_probe
//                  (axis_09b_build_coherence.hpp::actual_build_simd_width_bits, in JENER Uebersetzungseinheit),
//   (3) KOHAERENZ: Etikett == Realitaet — der GO-3-Kern (Mess-Etikett == Mess-Gegenstand; Dossier GO3 §2.3).
// Der Compile-Beweis steckt zusaetzlich im CHECKED-static_assert der DLLs selbst + in der konfigurationszeitigen
// Negativ-Probe (CHECKED-Form ohne Flag kompiliert NICHT — check_cxx_source_compiles, s. CMakeLists.txt).
// Registrierung host-gated (COMDARE_HOST_RUNS_AVX2/AVX512F, Muster test_simd_field_sum_dispatch): die DLLs
// enthalten echten AVX2-/AVX-512-Maschinencode und werden nur auf faehigen Hosts ausgefuehrt.
// Aufruf: test_ap5_simd_extension_coherence <real_avx512.dll> <real_avx2.dll>.

#include "anatomy/build_variant_definition.hpp"

// Portables DLL-Shim (Muster test_buildvariant_dll.cpp): identischer Symbol-Roundtrip auf Windows UND Linux.
#if defined(_WIN32)
#include <windows.h>
using DllHandle = HMODULE;
static DllHandle dll_open(char const* p) { return LoadLibraryA(p); }
static void*     dll_sym(DllHandle h, char const* s) { return reinterpret_cast<void*>(GetProcAddress(h, s)); }
static void      dll_close(DllHandle h) { FreeLibrary(h); }
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
using ProbeFn   = std::uint64_t (*)();

namespace {

int g_fail = 0;

void eq_u64(char const* what, std::uint64_t got, std::uint64_t want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << " (erwartet " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

/// Laedt eine real-Wrapper-DLL, zieht Etikett (POD) UND Realitaet (Build-ISA-Probe) und prueft die Kohaerenz
/// gegen die erwartete Stufe. errno-style: false = Lade-/Symbol-Fehler (bereits als [ERR] gezaehlt).
bool check_dll_coherence(char const* dll_path, std::uint64_t expected_width_bits) {
    DllHandle h = dll_open(dll_path);
    if (h == nullptr) {
        std::cout << "  [ERR] dll_open fehlgeschlagen: " << dll_path << "\n";
        ++g_fail;
        return false;
    }
    auto inspect = reinterpret_cast<InspectFn>(dll_sym(h, "comdare_build_variant_inspect"));
    auto probe   = reinterpret_cast<ProbeFn>(dll_sym(h, "comdare_build_isa_width_probe"));
    if (inspect == nullptr || probe == nullptr) {
        std::cout << "  [ERR] dll_sym(comdare_build_variant_inspect/comdare_build_isa_width_probe) == null: "
                  << dll_path << "\n";
        ++g_fail;
        dll_close(h);
        return false;
    }
    cea::BuildVariantDefinitionV1 pod{};
    inspect(&pod);                            // (1) Etikett ueber die echte .dll-Grenze
    std::uint64_t const real_width = probe(); // (2) Realitaet: Build-ISA-Kaskade JENER Uebersetzungseinheit
    eq_u64("ETIKETT   POD.simd_width_bits", pod.simd_width_bits, expected_width_bits);
    eq_u64("REALITAET build_isa_width_probe", real_width, expected_width_bits);
    eq_u64("KOHAERENZ Etikett == Realitaet", pod.simd_width_bits, real_width);
    dll_close(h);
    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: test_ap5_simd_extension_coherence <real_avx512.dll> <real_avx2.dll>\n";
        return 2;
    }
    std::cout << "==== GO-3 A1 simd_extension-Kohaerenz (Etikett == Realitaet ueber die .dll-Grenze) ====\n";

    std::cout << "-- real-Avx512-Variante (" << argv[1] << ") --\n";
    check_dll_coherence(argv[1], 512);

    std::cout << "-- real-Avx2-Variante (" << argv[2] << ") --\n";
    check_dll_coherence(argv[2], 256);

    std::cout << "\n==== GO-3 A1 Kohaerenz: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
