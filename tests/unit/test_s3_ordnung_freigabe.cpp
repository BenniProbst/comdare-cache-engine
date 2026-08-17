// test_s3_ordnung_freigabe -- S-3 / Paket P1 (Task #4), Teil S-3c: die FREIGEBENDE SEITE SCHARF.
//
// GEGENSTAND: belege_aktive_maschinen_deklaration (profile_facade/maschinen_deklarations_naht.hpp) --
// DIE EINE Naht, ueber die die Fassade (run_experiment_profile_facade, NACH Parse+validate, VOR der
// Perm-Schleife) live_hostname() gegen <machines><machine hostname_hint=..> haelt und bei Treffer
// set_active_machine_declaration(cpu_fabrication, ram_pair) EINMAL belegt. Hostname ist NUR
// Lookup-Schluessel (O-4: Instanz-Eigenschaft), die Identitaet ist das Tupel.
//
// WAS DIESER TEST BEWEIST:
//   (1) Treffer-Fall: Belegung latcht; auf der ECHTEN Maschine (live_hostname()==hint) ist
//       active_machine_verdict()==Match und active_machine_signature() nicht leer.
//   (2) Kein Treffer / keine Menge / leerer Hostname: NICHTS wird belegt (natuerlicher Kill-Switch).
//   (3) AbgelehntAbweichend (zwei <machine> mit gleichem hint, verschiedenem Tupel): klassifizierter
//       Fehler-Log (konfig_xml_parse), KEIN Abbruch, Erstbelegung bleibt.
//   (4) C-3a-SPIEGEL: die rsp-Zeilen sind BYTE-GLEICH zu vorher -- scharf ist kein Byte-Ereignis,
//       weil die Organ-required-Seite leer ist (C-3a-Tripwire simd_build_gate.hpp:272-278).
//   (5) T-6 SCHWESTERSTELLEN: ALLE 9 Organ-Klassen, BEIDE Dialekte (Gpp/Clang), BEIDE deklarierten
//       Maschinen-Klassen (prod1/prod2) plus ein nicht deklarierter Hostname.
//
// Die <machines>-Menge kommt aus der ce-Naht-Fixture experiment_kern_seam_fixture.xml (NUR GELESEN) --
// kein nachgebautes Fixture, dieselbe Quelle wie test_experiment_kern_seam. Ihre <machines>-Werte sind
// byte-gleich zur kanonischen super-Instanz Code/test_data_xml/experiment_golden_kern.xml; die Fixture
// hiess bis zur Weg-a-Umbenennung am 17.08.2026 genauso wie diese.
//
// ASCII-only (Leitplanke). Zeilen <= 120 (Diff-Hygiene-Wache).

#include "maschinen_deklarations_naht.hpp"         // der Prueflig (S-3c)
#include "xml_config_parser/xml_config_parser.hpp" // XmlConfigParser / ExperimentProfile (<machines>)

#include <cache_engine/measurement/machine_identity.hpp>
#include <cache_engine/measurement/simd_build_gate.hpp>
#include <cache_engine/measurement/simd_organ_requirement.hpp>
#include <cache_engine/measurement/simd_organ_sensibility.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifndef COMDARE_EXPERIMENT_KERN_SEAM_FIXTURE
#error "COMDARE_EXPERIMENT_KERN_SEAM_FIXTURE must point to tests/unit/thesis_tiere/experiment_kern_seam_fixture.xml"
#endif

namespace {

namespace cx   = ::comdare::builder::xml;
namespace meas = ::comdare::cache_engine::measurement;
namespace pf   = ::comdare::cache_engine::profile_facade;

[[nodiscard]] std::vector<cx::ExperimentMachine> kern_seam_machines() {
    cx::XmlConfigParser const parser;
    auto const ep = parser.parse_experiment_profile(std::filesystem::path{COMDARE_EXPERIMENT_KERN_SEAM_FIXTURE});
    if (!ep) return {};
    return ep->machines;
}

/// SPIEGEL der Fassaden-rsp-Zeile (Muster test_c3a_gate_scharfschaltung.cpp:27-37): opt zuerst, dann
/// march, dann die Gate-Extras. Byte-Gleichheit HIER ist Byte-Gleichheit der Fassaden-Zeile.
[[nodiscard]] std::string rsp_zeile(std::string const& opt, std::string const& march, meas::SimdDialect d) {
    std::string flags = opt;
    if (!march.empty()) {
        flags += ' ';
        flags += march;
    }
    for (auto const& mf : meas::gate_extra_march_flags_for_build(meas::route_of_march_flag(march), d)) {
        flags += ' ';
        flags += mf;
    }
    return flags;
}

struct RouteFall {
    char const* name;
    char const* march;
};
constexpr RouteFall         kRouten[]{{"no_extension", ""}, {"avx2", "-mavx2"}, {"avx512", "-mavx512f"}};
constexpr meas::SimdDialect kDialekte[]{meas::SimdDialect::Gpp, meas::SimdDialect::Clang};

// =================================================================================================
// (1)+(2) Treffer und Kill-Switch -- an der Maschinen-Menge der ce-Naht-Fixture (absichtlich divergent
// zum super-Master, s. Divergenz-Tabelle im Fixture-Kopf)
// =================================================================================================

TEST(S3OrdnungFreigabe, FixtureTraegtDieZweiDeklariertenMaschinen) {
    auto const machines = kern_seam_machines();
    ASSERT_EQ(machines.size(), 2u) << "experiment_kern_seam_fixture.xml fuehrt prod1 und prod2.";
    EXPECT_EQ(machines[0].hostname_hint, "prod1");
    EXPECT_EQ(machines[1].hostname_hint, "prod2");
}

TEST(S3OrdnungFreigabe, TrefferBelegtGenauEinmalUndVerdictFolgtDerEchtenCpu) {
    auto const machines = kern_seam_machines();
    ASSERT_FALSE(machines.empty());
    for (auto const& mc : machines) {
        meas::reset_active_machine_declaration_for_test();
        std::ostringstream log;
        auto const         befund = pf::belege_aktive_maschinen_deklaration(mc.hostname_hint, machines, log);
        EXPECT_EQ(befund.treffer, 1u) << "genau EIN <machine> traegt den hint '" << mc.hostname_hint << "'";
        EXPECT_TRUE(befund.belegt);
        EXPECT_EQ(befund.abgelehnt, 0u);
        EXPECT_TRUE(log.str().empty()) << "ohne Konflikt kein Fehler-Log: " << log.str();
        // Das Urteil faellt IMMER gegen die echte Host-CPU (Drift-Gegenprobe O-4). Eine Signatur
        // gibt es AUSSCHLIESSLICH bei Match -- auf jedem anderen Host bleibt sie leer (fail-closed).
        auto const verdict = meas::active_machine_verdict();
        EXPECT_EQ(meas::active_machine_signature().empty(), verdict != meas::MachineIdentityVerdict::Match)
            << "Signatur genau dann, wenn Verdict Match (hint=" << mc.hostname_hint << ")";
        if (meas::live_hostname() == mc.hostname_hint) {
            // Der Lauf auf der ECHTEN deklarierten Maschine: Belegung + Drift-Gegenprobe == Match.
            EXPECT_EQ(verdict, meas::MachineIdentityVerdict::Match);
            EXPECT_FALSE(meas::active_machine_signature().empty());
        }
    }
    meas::reset_active_machine_declaration_for_test();
}

TEST(S3OrdnungFreigabe, KeinTrefferKeineMengeLeererHostnameBelegenNichts) {
    auto const machines = kern_seam_machines();
    ASSERT_FALSE(machines.empty());
    // (a) nicht deklarierter Hostname -- der T-6-Pflichtfall.
    meas::reset_active_machine_declaration_for_test();
    std::ostringstream log;
    auto               befund = pf::belege_aktive_maschinen_deklaration("keine-solche-kiste", machines, log);
    EXPECT_EQ(befund.treffer, 0u);
    EXPECT_FALSE(befund.belegt);
    EXPECT_EQ(meas::active_machine_verdict(), meas::MachineIdentityVerdict::UnbekannteMaschine);
    EXPECT_TRUE(meas::active_machine_signature().empty()) << "kein Treffer => NICHTS belegen (Kill-Switch).";
    // (b) leere Maschinen-Menge.
    befund = pf::belege_aktive_maschinen_deklaration("prod1", {}, log);
    EXPECT_EQ(befund.treffer, 0u);
    EXPECT_FALSE(befund.belegt);
    EXPECT_TRUE(meas::active_machine_signature().empty());
    // (c) leerer Hostname (live_hostname() nicht ermittelbar) -- ausdruecklich KEIN Raten.
    befund = pf::belege_aktive_maschinen_deklaration("", machines, log);
    EXPECT_EQ(befund.treffer, 0u);
    EXPECT_FALSE(befund.belegt);
    EXPECT_TRUE(meas::active_machine_signature().empty());
    EXPECT_TRUE(log.str().empty());
    meas::reset_active_machine_declaration_for_test();
}

// =================================================================================================
// (3) Der Konflikt-Fall: klassifizierter Fehler-Log, KEIN Abbruch, Erstbelegung bleibt
// =================================================================================================

TEST(S3OrdnungFreigabe, AbweichenderZweittrefferWirdKlassifiziertAbgelehnt) {
    meas::reset_active_machine_declaration_for_test();
    std::vector<cx::ExperimentMachine> machines = kern_seam_machines();
    ASSERT_EQ(machines.size(), 2u);
    // Zwei <machine> mit DEMSELBEN hint, aber verschiedenem Tupel: der zweite MUSS abgelehnt werden
    // (Einmal-Belegung, Determinismus-Schutz von set_active_machine_declaration).
    machines[1].hostname_hint = machines[0].hostname_hint;
    std::ostringstream log;
    auto const         befund = pf::belege_aktive_maschinen_deklaration(machines[0].hostname_hint, machines, log);
    EXPECT_EQ(befund.treffer, 2u);
    EXPECT_TRUE(befund.belegt) << "die ERSTE Belegung bleibt bestehen.";
    EXPECT_EQ(befund.abgelehnt, 1u) << "der abweichende Zweit-Treffer wird abgelehnt, nicht uebernommen.";
    EXPECT_NE(log.str().find(meas::error_class_label(meas::CompilerCompilerErrorClass::KonfigXmlParse)),
              std::string::npos)
        << "der Konflikt ist ein KLASSIFIZIERTER Fehler-Log (konfig_xml_parse), kein stiller Skip: " << log.str();
    // Duldung der IDENTISCHEN Zweitbelegung (BereitsGesetztIdentisch): gleicher hint, gleiches Tupel.
    std::ostringstream log2;
    auto const befund2 = pf::belege_aktive_maschinen_deklaration(machines[0].hostname_hint, {machines[0]}, log2);
    EXPECT_TRUE(befund2.belegt);
    EXPECT_EQ(befund2.abgelehnt, 0u);
    EXPECT_TRUE(log2.str().empty());
    meas::reset_active_machine_declaration_for_test();
}

// =================================================================================================
// (4)+(5) C-3a-Spiegel + T-6: scharf ist KEIN Byte-Ereignis -- ueber alle Schwesterstellen
// =================================================================================================

TEST(S3OrdnungFreigabe, RspZeilenBleibenByteGleichUeberAlleKlassenRoutenDialekte) {
    auto const machines = kern_seam_machines();
    ASSERT_FALSE(machines.empty());
    // VORHER: ohne Belegung, je Route x Dialekt.
    meas::reset_active_machine_declaration_for_test();
    std::vector<std::string> vorher;
    for (auto const d : kDialekte)
        for (auto const& r : kRouten) vorher.push_back(rsp_zeile("-O2", r.march, d));
    // Die drei Hostname-Faelle: beide deklarierten Maschinen-Klassen + ein nicht deklarierter.
    std::vector<std::string> hosts;
    for (auto const& mc : machines) hosts.push_back(mc.hostname_hint);
    hosts.emplace_back("keine-solche-kiste");
    for (auto const& host : hosts) {
        meas::reset_active_machine_declaration_for_test();
        std::ostringstream log;
        (void)pf::belege_aktive_maschinen_deklaration(host, machines, log);
        std::size_t i = 0;
        for (auto const d : kDialekte)
            for (auto const& r : kRouten) {
                EXPECT_EQ(rsp_zeile("-O2", r.march, d), vorher[i])
                    << "rsp-Zeile aendert sich unter host=" << host << " route=" << r.name
                    << " dialekt=" << (d == meas::SimdDialect::Gpp ? "gpp" : "clang")
                    << " -- scharf MUSS byte-neutral sein (C-3a).";
                ++i;
            }
        EXPECT_EQ(meas::gate_contribution_identity_text(meas::SimdRoute::Avx512), std::string{})
            << "leeres gate-Segment ist die wahre Aussage (host=" << host << ").";
    }
    meas::reset_active_machine_declaration_for_test();
}

TEST(S3OrdnungFreigabe, Alle9OrganKlassenBleibenNotApplicableAufBeidenDialekten) {
    auto const machines = kern_seam_machines();
    ASSERT_FALSE(machines.empty());
    // Mit prod1-Belegung UND mit prod2-Belegung: das Gate bleibt je Organ-Klasse NotApplicable,
    // weil die required-Menge leer ist (C-3a-Tripwire haelt sie compile-hart leer).
    for (auto const& mc : machines) {
        meas::reset_active_machine_declaration_for_test();
        std::ostringstream log;
        (void)pf::belege_aktive_maschinen_deklaration(mc.hostname_hint, machines, log);
        ASSERT_EQ(meas::kSimdOrganRequirement.size(), 9u) << "T-6: ALLE 9 Organ-Klassen, nicht ein Beispiel.";
        for (auto const& organ : meas::kSimdOrganRequirement) {
            EXPECT_TRUE(organ.required.empty()) << organ.organ_class;
            for (auto const d : kDialekte)
                for (auto const& r : kRouten) {
                    auto const dock =
                        meas::pruef_dock(meas::required_of(organ.organ_class), meas::sensibility_of(organ.organ_class),
                                         meas::active_machine_signature(), meas::route_of_march_flag(r.march), d);
                    EXPECT_EQ(dock.state, meas::SimdGateState::NotApplicable)
                        << "organ=" << organ.organ_class << " host=" << mc.hostname_hint << " route=" << r.name;
                    EXPECT_EQ(dock.effective_count, 0u);
                }
        }
    }
    meas::reset_active_machine_declaration_for_test();
}

} // namespace
