// tests/unit/test_vo31_global_o2_flags.cpp -- VO3-1(b) T-A (Owner-KERN X4 25.08.2026: "(b) global O2 und
// nur auf XML Eingangswunsch O3 mit bauen"): die Release-Flag-Strings der Haus-Bauwelt kommen aus der
// EINEN CMake-Quelle (cmake/compiler_flags.cmake, ${_COMDARE_release_opt} per CACHE-FORCE) und sind
// GLOBAL -- Haus, Vendor ext/, FetchContent, super-Embed. Dieser Test friert die SOLL-Literale ein
// (T-3: der Nenner ist FREMD -- die IST-Werte kommen als Configure-Defines herein, die SOLL-Literale
// stehen HIER im Test und nie im Pruefling).
//
// T-1-KOEDER (rot zuerst, 26.08.2026, Beweisort backups-workflow/20260825-vo3-1-global-o2/bau/rot/):
// am IST-Stand d3b5a393 traegt CMAKE_CXX_FLAGS_RELEASE den CMake-Default "-O3 -DNDEBUG" -> die
// EXPECT_EQ unten beissen literal; gruen erst durch den B3-FORCE-Dreh. Der Test laeuft in ALLEN
// 4 Zellen (die *_FLAGS_RELEASE-Cache-Variablen existieren in jedem Build-Typ).
#include <gtest/gtest.h>

#include <string_view>
#include <vector>

#ifndef COMDARE_VO31_CXX_RELEASE_FLAGS
#error "VO3-1 T-A: COMDARE_VO31_CXX_RELEASE_FLAGS fehlt (Registrierung tests/unit/CMakeLists.txt)"
#endif
#ifndef COMDARE_VO31_C_RELEASE_FLAGS
#error "VO3-1 T-A: COMDARE_VO31_C_RELEASE_FLAGS fehlt (Registrierung tests/unit/CMakeLists.txt)"
#endif
#ifndef COMDARE_VO31_OPT_O3
#error "VO3-1 T-A: COMDARE_VO31_OPT_O3 fehlt (Registrierung tests/unit/CMakeLists.txt)"
#endif

namespace {

// Whitespace-Tokenisierung des Flag-Strings (genau die Grammatik, in der der Compiler ihn liest).
[[nodiscard]] std::vector<std::string_view> flag_tokens(std::string_view s) {
    std::vector<std::string_view> out;
    std::size_t                   pos = 0;
    while (pos < s.size()) {
        std::size_t const      ws = s.find(' ', pos);
        std::string_view const t  = s.substr(pos, ws == std::string_view::npos ? std::string_view::npos : ws - pos);
        if (!t.empty()) out.push_back(t);
        if (ws == std::string_view::npos) break;
        pos = ws + 1;
    }
    return out;
}

[[nodiscard]] int opt_token_count(std::string_view s) {
    int n = 0;
    for (std::string_view const t : flag_tokens(s)) {
        if (t.starts_with("-O")) ++n;
    }
    return n;
}

constexpr std::string_view kIstCxx = COMDARE_VO31_CXX_RELEASE_FLAGS;
constexpr std::string_view kIstC   = COMDARE_VO31_C_RELEASE_FLAGS;
constexpr bool             kOptO3  = (COMDARE_VO31_OPT_O3 != 0);
// SOLL-Literal IM TEST eingefroren (nie aus dem Pruefling gezogen): O2-Standard, O3 nur als
// COMDARE_OPT_O3-Opt-in unter Configure-WARNING (Owner-Kette 21.08. -> X4 25.08.).
constexpr std::string_view kSoll = kOptO3 ? "-O3 -DNDEBUG" : "-O2 -DNDEBUG";

} // namespace

TEST(Vo31GlobalO2Flags, ReleaseFlagStringsTragenDenHausStandard) {
    EXPECT_EQ(kIstCxx, kSoll) << "CMAKE_CXX_FLAGS_RELEASE weicht vom VO3-1(b)-Haus-Standard ab "
                              << "(COMDARE_OPT_O3=" << (kOptO3 ? "ON" : "OFF") << ")";
    EXPECT_EQ(kIstC, kSoll) << "CMAKE_C_FLAGS_RELEASE weicht vom VO3-1(b)-Haus-Standard ab "
                            << "(COMDARE_OPT_O3=" << (kOptO3 ? "ON" : "OFF") << ")";
}

// Gegeneingang: GENAU EIN "-O"-Token je String. Zwei Stufen im selben String ("-O2 -O3") hiessen:
// die letzte gewinnt still -- exakt die Klasse Divergenz, die (b) beseitigt.
TEST(Vo31GlobalO2Flags, GenauEinOptTokenJeString) {
    EXPECT_EQ(opt_token_count(kIstCxx), 1) << "cxx='" << kIstCxx << "'";
    EXPECT_EQ(opt_token_count(kIstC), 1) << "c='" << kIstC << "'";
}
