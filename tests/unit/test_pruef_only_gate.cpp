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

    // M-1/D-2: run_so_conformance_gate fordert seit dem Mess-Vertrag eine ERWARTUNG. Diese TU prueft das
    // FUNKTIONS-Gate; die Fixture traegt bewusst keinen Versionierungs-Stempel (kein
    // COMDARE_ANATOMY_VERSION_STAMP in auto_emitted_perm_module.cpp), ihr Mess-Vertrag ist also per
    // Konstruktion offen. Deshalb wird hier auf oc.gate geprueft und NICHT auf oc.passed() -- und die
    // Mess-Seite wird an ihrer eigenen Stelle bewiesen (test_d2_mess_konsistenz_gate, Fall 5).
    pd::PruefOutcome const oc = pd::run_so_conformance_gate(so, /*erwartete_mess_zeile=*/"");
    std::cout << "[pruef] so=" << so.filename().string() << " loaded=" << (oc.loaded ? 1 : 0)
              << " gate=" << oc.gate.cases_passed << "/" << oc.gate.cases_total
              << " mess_status=" << pd::mess_konsistenz_status_name(oc.mess.status) << "\n";

    EXPECT_TRUE(oc.loaded) << "gebaute .so nicht ladbar / kein Mess-Interface: " << so;
    EXPECT_GT(oc.gate.cases_total, 0u) << "Gate hat keine Faelle gefahren (nicht pruefbar)";
    EXPECT_TRUE(oc.gate.passed()) << "Gate: " << oc.gate.cases_passed << "/" << oc.gate.cases_total
                                  << " first_fail=" << oc.gate.first_fail;
    // Der Mess-Vertrag ist fail-closed: ohne Erwartung ist NICHTS bestanden. Das ist keine Schwaeche
    // dieser TU, sondern die Zusage -- hier festgehalten, damit ein spaeterer stiller Durchlass auffaellt.
    EXPECT_FALSE(oc.passed()) << "eine leere Mess-Erwartung darf das Gesamt-Ergebnis NIE bestehen lassen";
}

TEST(PruefOnlyGate, MissingSoIsNotLoadedAndNotPassed) {
    // Nicht-existente .so -> loaded=false -> passed()=false (== nicht-pruefbar zaehlt als Fail; Exit!=0-Vertrag).
    pd::PruefOutcome const oc = pd::run_so_conformance_gate("/nonexistent/perm.dll", /*erwartete_mess_zeile=*/"");
    EXPECT_FALSE(oc.loaded);
    EXPECT_FALSE(oc.passed());
}
