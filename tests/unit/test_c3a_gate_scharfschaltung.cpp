// tests/unit/test_c3a_gate_scharfschaltung.cpp -- Lane C, Paket C-3a (Bauplan D2.3/D2.11,
// Owner-Vorab-GO Ledger §69.9, Identitaets-Auflage §70.9). UNREGISTRIERT.
//
// C-3a schaltet die DREI Gate-Hooks GEMEINSAM scharf. Dieser Test ist der Beleg, dass das
// BYTE-NEUTRAL geschieht -- und zwar nicht, weil das Gate abgeschaltet waere, sondern weil der
// dominante Schalter (die Organ-Seite) leer ist. Genau diese Unterscheidung wird hier festgenagelt.

#include <cache_engine/measurement/machine_identity.hpp>
#include <cache_engine/measurement/simd_build_gate.hpp>
#include <cache_engine/measurement/simd_organ_requirement.hpp>
#include <cache_engine/measurement/simd_sub_axis.hpp>

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

namespace meas = comdare::cache_engine::measurement;

namespace {

/// SPIEGEL der Fassaden-Naht (profile_run_facade.cpp:528-541 / :1003-1016), wie in C-3b: EIN
/// flags-String, opt zuerst, dann march, dann die Gate-Extras angehaengt. Damit ist der rsp-Zeilen-
/// Vergleich hier derselbe Text, den die Fassade baut.
[[nodiscard]] std::string rsp_zeile(std::string const& opt, std::string const& march) {
    std::string flags = opt;
    if (!march.empty()) {
        flags += ' ';
        flags += march;
    }
    for (auto const& mf : meas::gate_extra_march_flags_for_build(meas::route_of_march_flag(march))) {
        flags += ' ';
        flags += mf;
    }
    return flags;
}

struct RouteFall {
    char const* name;
    char const* march;
};
constexpr RouteFall kRouten[]{
    {"no_extension", ""},
    {"avx2", "-mavx2"},
    {"avx512", "-mavx512f"},
};

} // namespace

// =================================================================================================
// 1. DER KILL-SWITCH: ohne CEB-Belegung ist das Gate inert
// =================================================================================================
TEST(C3aScharfschaltung, OhneCebBelegungIstDasGateInert) {
    meas::reset_active_machine_declaration_for_test();
    EXPECT_EQ(meas::active_machine_verdict(), meas::MachineIdentityVerdict::UnbekannteMaschine);
    EXPECT_TRUE(meas::active_machine_signature().empty())
        << "Ohne belegte Maschinen-Deklaration darf NIE eine Signatur herausfallen.";
    for (auto const& r : kRouten)
        EXPECT_TRUE(meas::gate_extra_march_flags_for_build(meas::route_of_march_flag(r.march)).empty()) << r.name;
    EXPECT_FALSE(meas::gate_contributes_anything());
}

TEST(C3aScharfschaltung, DerNotAusIstScharfGeschaltetAberVorhanden) {
    // Default ARMED (ein Knopf, kein CMake-Schalter). Der Test haelt fest, dass der Knopf existiert
    // und wie er heute steht -- wer ihn umlegt, aendert diese Zeile bewusst.
    EXPECT_TRUE(meas::kSimdGateArmed) << "Default ist ARMED; COMDARE_SIMD_GATE_ARMED=0 ist der Not-Aus.";
}

// =================================================================================================
// 2. DER KERN: voll scharf UND trotzdem byte-neutral -- weil die Organ-Seite leer ist
// =================================================================================================
TEST(C3aScharfschaltung, ScharfMitEchterIdentitaetUndTROTZDEMKeineFlags) {
    meas::reset_active_machine_declaration_for_test();
    ASSERT_EQ(meas::set_active_machine_declaration("amd_zen5_avx512", "ddr5_2x32"),
              meas::MachineDeclarationSetResult::Gesetzt);

    if (meas::live_hostname() == "prod1") {
        // Auf prod1 loest die Identitaet WIRKLICH auf -- das Gate ist maximal scharf.
        EXPECT_EQ(meas::active_machine_verdict(), meas::MachineIdentityVerdict::Match);
        EXPECT_FALSE(meas::active_machine_signature().empty())
            << "prod1 muss seine Signatur liefern, sonst waere der Test wertlos.";
        EXPECT_EQ(meas::active_machine_signature().size(), meas::Prod1Zen5Signature::signature().size());
    }

    // UND TROTZDEM: null Gate-Flags auf ALLEN Routen. Das ist die Byte-Neutralitaets-Aussage.
    for (auto const& r : kRouten) {
        auto const flags = meas::gate_extra_march_flags_for_build(meas::route_of_march_flag(r.march));
        EXPECT_TRUE(flags.empty()) << "Route " << r.name << " liefert unerwartet " << flags.size() << " Flags.";
    }
    EXPECT_FALSE(meas::gate_contributes_anything());
}

TEST(C3aScharfschaltung, DerGrundIstDieLEEREOrganSeite) {
    // Der Beleg, WARUM es byte-neutral ist -- nicht Abschaltung, sondern der dominante Schalter.
    EXPECT_FALSE(meas::any_organ_declares_required())
        << "simd_organ_requirement.hpp:88 ist der Anker: alle 9 Organ-Klassen tragen kRequiredNone.";
    EXPECT_TRUE(meas::active_organ_required().empty());
    // Gegenprobe, dass NICHT einfach alles leer gestubbt wurde: die Sinnhaftigkeits-Vereinigung ist ECHT.
    EXPECT_FALSE(meas::active_organ_meaningful().empty())
        << "Hook 2 ist wirklich gefuellt (Vereinigung der Matrix), nicht gestubbt.";
    EXPECT_EQ(meas::kSensibilityUnion.size(), meas::active_organ_meaningful().size());
    // Und pruef_dock kehrt trotz echter Signatur und echter Obergrenze bei leerem required sofort um.
    auto const r = meas::pruef_dock(meas::active_organ_required(), meas::active_organ_meaningful(),
                                    meas::Prod1Zen5Signature::signature(), meas::SimdRoute::Avx512);
    EXPECT_EQ(r.state, meas::SimdGateState::NotApplicable);
    EXPECT_EQ(r.effective_count, 0u);
}

// =================================================================================================
// 3. DER RSP-ZEILEN-DIFF JE ROUTE (D2.11)
// =================================================================================================
TEST(C3aScharfschaltung, RspZeilenSindMitScharfemGateZeichengleich) {
    // Referenz: die Zeile OHNE jeden Gate-Beitrag (opt + optional march).
    meas::reset_active_machine_declaration_for_test();
    std::vector<std::string> ohne;
    for (auto const& r : kRouten) ohne.emplace_back(rsp_zeile("-O3", r.march));

    // Jetzt scharf schalten und dieselben Zeilen erneut bauen.
    ASSERT_EQ(meas::set_active_machine_declaration("amd_zen5_avx512", "ddr5_2x32"),
              meas::MachineDeclarationSetResult::Gesetzt);
    for (std::size_t i = 0; i < std::size(kRouten); ++i) {
        std::string const mit = rsp_zeile("-O3", kRouten[i].march);
        EXPECT_EQ(mit, ohne[i]) << "rsp-Zeile driftet auf Route " << kRouten[i].name;
    }
    // Und die erwarteten Literale, damit der Test auch bei doppeltem Fehler nicht still gruen wird.
    EXPECT_EQ(ohne[0], std::string{"-O3"});
    EXPECT_EQ(ohne[1], std::string{"-O3 -mavx2"});
    EXPECT_EQ(ohne[2], std::string{"-O3 -mavx512f"});
}

// =================================================================================================
// 4. EINMAL-BELEGUNG (Manager-Auflage): kein stilles Ueberschreiben in der Perm-Schleife
// =================================================================================================
TEST(C3aScharfschaltung, EinmalBelegungMitBenanntemVerhalten) {
    meas::reset_active_machine_declaration_for_test();
    EXPECT_EQ(meas::set_active_machine_declaration("amd_zen5_avx512", "ddr5_2x32"),
              meas::MachineDeclarationSetResult::Gesetzt);
    // Zweite, IDENTISCHE Belegung: geduldet, kein Effekt.
    EXPECT_EQ(meas::set_active_machine_declaration("amd_zen5_avx512", "ddr5_2x32"),
              meas::MachineDeclarationSetResult::BereitsGesetztIdentisch);
    // Zweite, ABWEICHENDE Belegung: ABGELEHNT -- ein Wechsel mitten im Lauf waere ein
    // Determinismus-Bruch (halbe Perm-Schleife mit anderer Freigabe).
    EXPECT_EQ(meas::set_active_machine_declaration("intel_avx2", "ddr4_2x32"),
              meas::MachineDeclarationSetResult::AbgelehntAbweichend);
    // Der ALTWERT haelt -- die Ablehnung ist wirksam, nicht bloss eine Meldung.
    if (meas::live_hostname() == "prod1")
        EXPECT_EQ(meas::active_machine_verdict(), meas::MachineIdentityVerdict::Match)
            << "Nach der Ablehnung muss prod1s Deklaration weiter gelten.";
}

// =================================================================================================
// 5. §70.9 -- SICHTBARKEIT DES GATE-BEITRAGS IN DER IDENTITAET
// =================================================================================================
TEST(C3aScharfschaltung, IdentitaetsTextIstHeuteLEER) {
    meas::reset_active_machine_declaration_for_test();
    ASSERT_EQ(meas::set_active_machine_declaration("amd_zen5_avx512", "ddr5_2x32"),
              meas::MachineDeclarationSetResult::Gesetzt);
    for (auto const& r : kRouten)
        EXPECT_TRUE(meas::gate_contribution_identity_text(meas::route_of_march_flag(r.march)).empty())
            << "Route " << r.name << ": kein Beitrag => kein Segment.";
}

TEST(C3aScharfschaltung, IdentitaetsTextIstSORTIERTUndDamitStempelTauglich) {
    // Die Stabilitaets-Zusage: die Ausgabe darf NICHT von der Reihenfolge der Eingabe abhaengen,
    // sonst wackelt eine Stempel-Variable, sobald jemand eine Tabellen-Zeile verschiebt.
    std::vector<std::string> const a{"-mavx512vpopcntdq", "-mgfni", "-mavx512bitalg"};
    std::vector<std::string> const b{"-mgfni", "-mavx512bitalg", "-mavx512vpopcntdq"};
    EXPECT_EQ(meas::format_gate_contribution(a), meas::format_gate_contribution(b));
    EXPECT_EQ(meas::format_gate_contribution(a), std::string{"gate=[-mavx512bitalg,-mavx512vpopcntdq,-mgfni]"});
    // Leerer Beitrag => leerer Text (kein "gate=[]"-Rauschen im Stempel).
    EXPECT_TRUE(meas::format_gate_contribution({}).empty());
}

TEST(C3aScharfschaltung, WARUM70Punkt9NoetigIstDisjunktheitBleibtBestehen) {
    // Der Grund der Auflage (C-3b-Fund): wuerde das Gate je beitragen, waere seine Menge DISJUNKT
    // zum Codegen-Kanal -- und die Sub-Feature-Flags implizieren AVX512F. Der Test haelt die
    // Groessenordnung fest, damit die Auflage nicht in Vergessenheit geraet.
    static constexpr std::array<meas::SimdFeatureFlag, 1> kDemo{{meas::kAvx512Vpopcntdq}};
    auto const scharf = meas::pruef_dock(kDemo, meas::kSensibilityFilterFlags, meas::Prod1Zen5Signature::signature(),
                                         meas::SimdRoute::Avx512);
    ASSERT_EQ(scharf.state, meas::SimdGateState::Freigegeben);
    auto const gate_flags = meas::effective_march_flags(scharf);
    EXPECT_GT(gate_flags.size(), 1u);
    for (auto const& f : gate_flags)
        EXPECT_NE(f, std::string{meas::SimdAvx512Option::gcc_march_flag()})
            << "Gate- und Codegen-Kanal duerfen sich nicht ueberschneiden (C-3b-Befund).";
    // Und so saehe das Identitaets-Segment aus, sobald es soweit ist -- sortiert und stabil.
    EXPECT_FALSE(meas::format_gate_contribution(gate_flags).empty());
    EXPECT_TRUE(meas::format_gate_contribution(gate_flags).starts_with("gate=["));
}
