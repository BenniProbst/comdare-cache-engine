// S3 (Section 62-B, COMDARE_PRUEF_ONLY): der Load+Gate-Baustein run_so_conformance_gate laedt EINE bereits gebaute
// Tier-.so (die r5g-adhoc-Fixture, dieselbe wie test_v41_anatomy_adhoc_dll_load) und faehrt NUR das Konformitaets-Gate
// -- KEIN Bau, KEINE Messung. Belegt: gebaute .so laedt, Gate bestanden, literale Ausgabe.

#include <pruef_dock/pruef_only.hpp> // Pruefling: run_so_conformance_gate / PruefOutcome

#include <gtest/gtest.h>

#include <filesystem>
#include <iostream>
#include <string>

namespace pd = ::comdare::cache_engine::builder::pruef_dock;

namespace {
// Die eine gebaute perm-.so im Fixture-Dir finden (AnatomyModuleLoader-Konvention: Dateiname comdare_anatomy_perm_*).
[[nodiscard]] std::filesystem::path find_built_so(std::filesystem::path const& dir) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) return {};
    for (auto const& e : std::filesystem::directory_iterator{dir, ec}) {
        if (!e.is_regular_file()) continue;
        std::string const name = e.path().filename().string();
        std::string const ext  = e.path().extension().string();
        if (name.rfind("comdare_anatomy_perm_", 0) == 0 && (ext == ".so" || ext == ".dll")) return e.path();
    }
    return {};
}
} // namespace

TEST(PruefOnlyGate, LoadsBuiltSoAndGatePasses) {
    std::filesystem::path const dir{COMDARE_R5G_ADHOC_DLL_DIR};
    std::filesystem::path const so = find_built_so(dir);
    ASSERT_FALSE(so.empty()) << "keine gebaute perm-.so im Fixture-Dir gefunden: " << dir;

    pd::PruefOutcome const oc = pd::run_so_conformance_gate(so);
    std::cout << "[pruef] so=" << so.filename().string() << " loaded=" << (oc.loaded ? 1 : 0)
              << " gate=" << oc.gate.cases_passed << "/" << oc.gate.cases_total << " passed=" << (oc.passed() ? 1 : 0)
              << "\n";

    EXPECT_TRUE(oc.loaded) << "gebaute .so nicht ladbar / kein Mess-Interface: " << so;
    EXPECT_GT(oc.gate.cases_total, 0u) << "Gate hat keine Faelle gefahren (nicht pruefbar)";
    EXPECT_TRUE(oc.gate.passed()) << "Gate: " << oc.gate.cases_passed << "/" << oc.gate.cases_total
                                  << " first_fail=" << oc.gate.first_fail;
    EXPECT_TRUE(oc.passed());
}

TEST(PruefOnlyGate, MissingSoIsNotLoadedAndNotPassed) {
    // Nicht-existente .so -> loaded=false -> passed()=false (== nicht-pruefbar zaehlt als Fail; Exit!=0-Vertrag).
    pd::PruefOutcome const oc = pd::run_so_conformance_gate("/nonexistent/perm.dll");
    EXPECT_FALSE(oc.loaded);
    EXPECT_FALSE(oc.passed());
}
