// test_registry_gen_out_pflicht -- B12 (Z04/F7): --out ist PFLICHT-Argument aller drei
// ce-Registry-Generatoren -- keine stille CWD-Schreibung mehr.
//
// BEFUND (am Objekt, 2026-08-21): alle drei Generatoren trugen einen CWD-Default
// (system_axis_registry_gen main.cpp:625 "system_axis_registry.xml";
// measurement_...:118; axis_...:188). Das Root-Duplikat von F7 war weg (ls-tree rc=1), aber die
// URSACHE lebte: jeder Aufruf ohne --out legte die Datei im Arbeitsverzeichnis neu an -- die
// F7-Latenz. EINE Fehlerklasse, EIN Zug (BAULISTE): alle drei gleich behandelt.
// AUFRUFER-BELEG (vor dem Fix erhoben): der einzige maschinelle Aufrufer ist
// tests/unit/registry_roundtrip.cmake:44 mit explizitem --out; die produktiven Registry-XMLs
// sind eingecheckt (profile_facade konsumiert Source-Pfade als Defines); die J-1-Treppe BAUT die
// Tools nur. prt-art-Gen: eigenes Repo, eigener Posten (B14-Flaeche).
//
// WAS DIESER TEST ZUSICHERT (je Tool, 3/3 benannt):
//   (1) OHNE --out: Exit != 0, Fehlermeldung nennt --out, und im frischen CWD entsteht KEINE
//       Datei (die stille Schreibung ist der Defekt, nicht der Exit-Code allein).
//   (2) MIT --out: Exit 0 und die Datei entsteht GENAU am genannten Pfad (der Fix darf den
//       Gutfall nicht beschaedigen; XML-Bytes selbst deckt registry_roundtrip).
//
// T-11c-MUTATIONSANKER: den Pflicht-Check in EINEM Generator wegmutieren -> (1) bricht dort
// literal (Exit 0 + CWD-Datei).

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

namespace {

namespace fs = std::filesystem;

std::vector<std::string> g_tools; // argv[1..3]: die drei Generator-Binaries
// STATISCH, nicht aus main(): die INSTANTIATE-Namens-Funktion laeuft bei der STATISCHEN
// Registrierung (vor main) -- ein main()-gefuellter vector waere dort leer (Segfault, gemessen).
constexpr char const* kToolName[3] = {"axis", "system", "measurement"};

struct Lauf {
    int         rc = -1;
    std::string ausgabe; ///< stdout+stderr (2>&1) -- die Pflicht-Meldung ist der Gegenstand
};

[[nodiscard]] Lauf fahre_in(fs::path const& cwd, std::string const& binary, std::string const& args) {
    Lauf        l{};
    std::string cmd = "cd \"" + cwd.string() + "\" && \"" + binary + "\" " + args + " 2>&1";
    std::FILE*  p   = ::popen(cmd.c_str(), "r");
    if (p == nullptr) return l;
    char puffer[512];
    while (std::fgets(puffer, sizeof(puffer), p) != nullptr) l.ausgabe += puffer;
    int const status = ::pclose(p);
    l.rc             = (status >= 0 && WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
    return l;
}

[[nodiscard]] std::size_t dateien_in(fs::path const& dir) {
    std::size_t     n = 0;
    std::error_code ec;
    for (auto const& e : fs::directory_iterator{dir, ec}) {
        (void)e;
        ++n;
    }
    return n;
}

class RegistryGenOutPflicht : public ::testing::TestWithParam<std::size_t> {};

TEST_P(RegistryGenOutPflicht, OhneOutFailLoudUndKeineCwdSchreibung) {
    auto const      i   = GetParam();
    fs::path const  cwd = fs::temp_directory_path() / (std::string{"comdare_b12_"} + kToolName[i] + "_ohne");
    std::error_code ec;
    fs::remove_all(cwd, ec);
    fs::create_directories(cwd, ec);

    Lauf const l = fahre_in(cwd, g_tools[i], "");
    EXPECT_NE(l.rc, 0) << kToolName[i] << ": ohne --out muss der Lauf fail-loud enden\n" << l.ausgabe;
    EXPECT_NE(l.ausgabe.find("--out"), std::string::npos)
        << kToolName[i] << ": die Meldung muss das fehlende --out nennen\n"
        << l.ausgabe;
    EXPECT_EQ(dateien_in(cwd), 0u) << kToolName[i]
                                   << ": im frischen CWD darf KEINE Datei entstehen (stille "
                                      "CWD-Schreibung ist der F7-Defekt)";
    fs::remove_all(cwd, ec);
}

TEST_P(RegistryGenOutPflicht, MitOutSchreibtGenauDenGenanntenPfad) {
    auto const      i   = GetParam();
    fs::path const  cwd = fs::temp_directory_path() / (std::string{"comdare_b12_"} + kToolName[i] + "_mit");
    std::error_code ec;
    fs::remove_all(cwd, ec);
    fs::create_directories(cwd, ec);
    fs::path const ziel = cwd / "ziel.xml";

    Lauf const l = fahre_in(cwd, g_tools[i], "--out \"" + ziel.string() + "\"");
    EXPECT_EQ(l.rc, 0) << kToolName[i] << ": der Gutfall darf nicht brechen\n" << l.ausgabe;
    EXPECT_TRUE(fs::exists(ziel)) << kToolName[i] << ": die Datei muss am genannten Pfad liegen";
    EXPECT_GT(fs::file_size(ziel, ec), 0u) << kToolName[i] << ": und Inhalt tragen";
    EXPECT_EQ(dateien_in(cwd), 1u) << kToolName[i] << ": GENAU die eine genannte Datei, nichts daneben";
    fs::remove_all(cwd, ec);
}

INSTANTIATE_TEST_SUITE_P(AlleDreiGeneratoren, RegistryGenOutPflicht, ::testing::Values(0u, 1u, 2u),
                         [](::testing::TestParamInfo<std::size_t> const& param_info) {
                             return std::string{kToolName[param_info.param]};
                         });

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 4) {
        std::fprintf(stderr, "Aufruf: %s <axis_gen> <system_gen> <measurement_gen>\n", argv[0]);
        return 2;
    }
    g_tools = {argv[1], argv[2], argv[3]};
    return RUN_ALL_TESTS();
}
