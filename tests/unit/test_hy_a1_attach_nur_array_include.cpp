// test_hy_a1_attach_nur_array_include -- A2.5-FUND F-9 (G4/L21): attach MUSS mit dem Array-Header
// allein LINKBAR sein.
//
// ============================================================================================
// WAS DIESE TU BEWEIST -- UND WARUM SIE EIN EIGENES BINARY SEIN MUSS
// ============================================================================================
// DER FUND (F-9, Register A2.5): DockArray<Policy>::attach war in hybrid_dock_array.hpp nur
// DEKLARIERT; die Definition stand in hybrid_dock_factory.hpp. Wer nur das Array inkludierte
// und attach rief, lief am LINKER auf, nicht am Compiler -- die unmarkierte Naht, die der
// Design-Nachtrag fuer offene Punkte verbietet.
//
// DIESE TU IST DER DECKUNGS-TEST DES FIXES: sie inkludiert AUSSCHLIESSLICH den Array-Header
// (die eine Include-Zeile unten ist der Testgegenstand -- wer hier die Factory ergaenzt,
// loescht den Beweis) und ruft attach auf BEIDEN Policies. Vor dem Fix bricht der Link
// (undefined reference auf beide attach-Spezialisierungen); nach dem Fix traegt der
// Array-Header die Definition ueber die Kette array -> factory -> attach-Header mit.
//
// WARUM EIN EIGENES BINARY UND KEIN FALL IN test_hy_a1_dock_contract: dort inkludiert die TU
// die Factory, und der Linker faende die attach-Instanziierung aus DERSELBEN Uebersetzungs-
// einheit -- die Probe waere vor dem Fix falsch-gruen gewesen. Der Include-Satz IST der
// Gegenstand; er ist nur je Binary pruefbar (T-7: Registrierung ist Teil des Tests).
//
// @fund   FINDINGS A2.5 F-9 (Marker stand in hybrid_dock_array.hpp:225-238, aufgeloest in G4)
// @design docs/architecture/20260802-hybrid_tier_stufe_soll_design.md Abschnitt 3.1 (e)

#include "hybrid/hybrid_dock_array.hpp" // GENAU EIN Include des Hauses -- der Testgegenstand

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

namespace cea = ::comdare::cache_engine::anatomy;
namespace hy  = ::comdare::cache_engine::hybrid;

namespace {

// Der Standard-Deskriptor beider Faelle: der einzige Vertrag mit gebautem Dock-Typ (F8-Minimal-
// DoD). AnatomyGenus kommt TRANSITIV ueber den Array-Header (hybrid_dock_contract.hpp) -- ein
// direkter Include waere hier eine zweite Include-Zeile und damit Gegenstands-Verwaesserung.
[[nodiscard]] hy::DockContractDescriptor standard_deskriptor() {
    return {static_cast<std::uint8_t>(hy::HybridDockContract::Standard), cea::AnatomyGenus::SearchAlgorithm};
}

// ============================================================================================
// (1) RUNTIME-POLICY: attach linkt und arbeitet ueber den Nur-Array-Include
// ============================================================================================

TEST(HyA1AttachNurArrayInclude, RuntimePolicyAttachLinktUeberDenArrayHeaderAllein) {
    hy::DockArray<hy::RuntimeDockArrayPolicy> arr;
    hy::DockContractDescriptor const          d = standard_deskriptor();

    // DIE ZEILE, AN DER F-9 BISS: vor dem Fix war genau dieser Aufruf ein Linker-Bruch
    // (attach nur deklariert). Der Wert-Check dahinter ist die Gegenprobe, dass die ueber die
    // Header-Kette gezogene Definition die ECHTE Factory-Strecke faehrt, nicht ein Stub.
    EXPECT_EQ(arr.attach(d), 0);
    EXPECT_EQ(arr.size(), std::size_t{1});
    EXPECT_EQ(arr.detach(0), hy::hybrid_status_ok);
    EXPECT_EQ(arr.size(), std::size_t{0});
}

// ============================================================================================
// (2) STATISCHE POLICY: die zweite attach-Spezialisierung, derselbe Beweis
// ============================================================================================

TEST(HyA1AttachNurArrayInclude, StatischePolicyAttachLinktUeberDenArrayHeaderAllein) {
    hy::DockArray<hy::StatischeDockArrayPolicy<2>> arr;
    hy::DockContractDescriptor const               d = standard_deskriptor();

    EXPECT_EQ(arr.attach(d), 0);
    EXPECT_EQ(arr.attach(d), 1);
    // Der Fehlerpfad laeuft ueber DIESELBE Definition -- ein Stub, der nur 0 liefert, faellt hier.
    EXPECT_EQ(arr.attach(d), -hy::hybrid_status_array_voll);
    EXPECT_EQ(arr.detach(0), hy::hybrid_status_ok);
    EXPECT_EQ(arr.attach(d), 0) << "freier Slot 0 muss wiederverwendet werden -- echte Definition";
}

} // namespace
