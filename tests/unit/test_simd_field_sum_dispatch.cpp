// Phase 0.1 SIMD-Dispatch (Doc 21 §F): Amd64Isa::simd_field_sum dispatcht auf die aktive Vektor-Breite
// (__AVX512F__/__AVX2__/SSE2/Skalar) — gesteuert durch die Compiler-SIMD-Makros. Diese CI-Varianten setzen die
// -m-Flag direkt (tests/unit/CMakeLists.txt); die Kopplung 09b-Achse -> ISA-Flag ist seit GO-3 A1 (Task #5,
// 2026-07-12) gebaut: comdare_apply_simd_extension_flags (cmake/isa_features.cmake) + Kohaerenz-Guard
// (axis_09b_build_coherence.hpp), konsumiert von den real-Wrapper-DLLs; E2-Voll-Matrix-Emission = #276-Folge-Slice.
// Dieser Contract beweist: (a) der aktive Pfad ist numerisch KORREKT (== uint64-Referenzsumme) ueber alle
// Tail-Restlaengen aller Lane-Breiten; (b) BUILD-INVARIANZ unter Ueberlauf; (c) die aktive SIMD-Stufe wird ehrlich gemeldet.

#include <axes/simd/axis_09_isa_amd64.hpp>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace sd = ::comdare::cache_engine::simd;

namespace {

int g_fail = 0;

void check_eq_u64(char const* what, std::uint64_t got, std::uint64_t want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) std::cout << "  (erwartet: " << want << ")";
    std::cout << "\n";
    if (!ok) ++g_fail;
}

// Referenz: Summe der ersten n uint32-Worte als uint64 (overflow-frei durch kleine Werte).
[[nodiscard]] std::uint64_t reference_sum(std::vector<std::uint32_t> const& v, std::size_t n) {
    std::uint64_t s = 0;
    for (std::size_t i = 0; i < n && i < v.size(); ++i) s += v[i];
    return s;
}

[[nodiscard]] std::string active_path() {
#if defined(__AVX512F__)
    return "AVX-512F (512b, 16 Lanes)";
#elif defined(__AVX2__)
    return "AVX2 (256b, 8 Lanes)";
#elif defined(__x86_64__) || defined(_M_X64)
    return "SSE2 (128b, 4 Lanes)";
#else
    return "Skalar (kein x86-SIMD)";
#endif
}

} // namespace

int main() {
    std::cout << "==== Phase 0.1 SIMD-Dispatch: Amd64Isa::simd_field_sum ====\n";
    std::cout << "Aktiver Pfad (Build-SIMD-Stufe): " << active_path() << "\n";

    // Puffer mit kleinen, overflow-freien Werten (v[i] = i*7 + 3).
    std::vector<std::uint32_t> vals;
    vals.reserve(128);
    for (std::uint32_t k = 0; k < 128; ++k) vals.push_back(k * 7u + 3u);

    auto const* buf = reinterpret_cast<unsigned char const*>(vals.data());

    // Tail-Restlaengen aller Lane-Breiten (1/4/8/16) + Grenzen: 0..17 + 100 + 128.
    constexpr std::size_t kNs[] = {0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 100, 128};
    for (std::size_t n : kNs) {
        std::uint64_t const got  = sd::Amd64Isa::simd_field_sum(buf, n);
        std::uint64_t const want = reference_sum(vals, n);
        std::string const   what = "simd_field_sum(n=" + std::to_string(n) + ")";
        check_eq_u64(what.c_str(), got, want);
    }

    // Build-Invarianz unter Ueberlauf (Review wf_fd87be00, der load-bearende Kontrakt): grosse Werte
    // (0xC0000000 + k ~ 3.2e9 > 2^31), bei denen eine per-lane-32-bit-Akkumulation zwischen SSE2 (2 Werte je
    // 128b-Lane) und AVX2/AVX-512 (1 Wert je Lane) DIVERGieren wuerde — der alte SSE2-Pfad lieferte bei n=8 eine
    // andere Summe als AVX2/AVX-512 (build-abhaengige last_checksum). Die uint64-Akkumulation liefert auf JEDEM
    // Pfad die WAHRE Summe == uint64-Referenz; dieser Kontrakt haette den Alt-Pfad fallen lassen und sperrt den Fix.
    std::vector<std::uint32_t> big;
    big.reserve(128);
    for (std::uint32_t k = 0; k < 128; ++k) big.push_back(0xC0000000u + k);
    auto const* bbuf = reinterpret_cast<unsigned char const*>(big.data());
    for (std::size_t n : kNs) {
        std::uint64_t const got  = sd::Amd64Isa::simd_field_sum(bbuf, n);
        std::uint64_t const want = reference_sum(big, n);
        std::string const   what = "overflow simd_field_sum(n=" + std::to_string(n) + ")";
        check_eq_u64(what.c_str(), got, want);
    }

    // Meta-Kontrakte der Achse (unveraendert gueltig).
    check_eq_u64("is_64bit() == true (Amd64Isa)", sd::Amd64Isa::is_64bit() ? 1 : 0, 1);
    std::cout << (sd::Amd64Isa::supports_native_simd() ? "  [OK]  " : "  [ERR] ") << "supports_native_simd == true\n";
    if (!sd::Amd64Isa::supports_native_simd()) ++g_fail;

    std::cout << "\n==== Phase 0.1 SIMD-Dispatch: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
