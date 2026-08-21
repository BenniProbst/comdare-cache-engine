// test_lg_host_binder -- B05 (K01/F2-5, Designplan par.4 Z47 "LG-HostBinder 3 Felder", Klasse K4
// "test laeuft nie"): die BINDUNGS-SEMANTIK des Lager-Host-Binders, alle DREI Felder einzeln.
//
// GEGENSTAND (am Objekt gesucht, nicht am Konsumenten -- Bestands-Pflicht): der Host-Binder der
// Lager-/Mess-Kette ist die S-3c-Naht belege_aktive_maschinen_deklaration
// (profile_facade/maschinen_deklarations_naht.hpp): sie bindet den LIVE-Host ueber
// <machine hostname_hint=..> und belegt die aktive Maschinen-Deklaration mit dem
// Eigenschafts-Tupel (cpu_fabrication, ram_pair). DIE DREI FELDER, 3/3 benannt:
//   [1] hostname_hint    -- NUR Lookup-Schluessel (O-4: Instanz-Eigenschaft, nie Identitaet)
//   [2] cpu_fabrication  -- Tupel-Haelfte 1 der Kern-Identitaet
//   [3] ram_pair         -- Tupel-Haelfte 2 der Kern-Identitaet
// NENNER-ABGRENZUNG (macht die 3 zur MENGE, nicht zur Aufzaehlung): ram_frequency_mhz,
// cas_latency_cl und die A14/OS-U2-Felder sind NICHT bindungsrelevant -- ein Zweit-Treffer, der
// NUR dort abweicht, ist KEIN Konflikt. Ohne diese Rueckrichtung waere "3 Felder" eine Behauptung.
//
// VORBESTAND (Anrechnung statt Doppelung): test_s3_ordnung_freigabe konsumiert die Naht fuer die
// GATE-Inertheit (T-6); die BINDUNGS-Semantik selbst -- Feld fuer Feld mit Negativprobe -- hatte
// 0 Tests (Gegenprobe der BAULISTE: test_lg_skip_callback_null trifft im selben ls-Muster).
//
// T-11c-MUTATIONSANKER: die hostname_hint-Vergleichszeile der Naht lockern (leeren Hint als
// Treffer zaehlen) bricht HintFeldNegativproben literal.

#include "maschinen_deklarations_naht.hpp"         // der Pruefling (S-3c-Naht)
#include "xml_config_parser/xml_config_parser.hpp" // ExperimentMachine (<machines><machine>)

#include <cache_engine/measurement/simd_build_gate.hpp> // reset_active_machine_declaration_for_test

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

namespace {

namespace pf   = ::comdare::cache_engine::profile_facade;
namespace meas = ::comdare::cache_engine::measurement;
using ::comdare::builder::xml::ExperimentMachine;

ExperimentMachine maschine(std::string id, std::string fab, std::string ram, std::string hint) {
    ExperimentMachine m{};
    m.id              = std::move(id);
    m.cpu_fabrication = std::move(fab);
    m.ram_pair        = std::move(ram);
    m.hostname_hint   = std::move(hint);
    return m;
}

class LgHostBinder : public ::testing::Test {
protected:
    void SetUp() override { meas::reset_active_machine_declaration_for_test(); }
    void TearDown() override { meas::reset_active_machine_declaration_for_test(); }
};

TEST_F(LgHostBinder, FeldEinsHintBindetUndDreiNegativproben) {
    // [1] POSITIV: der Hint trifft -> Bindung steht (treffer 1/1, belegt, nichts abgelehnt, Log leer).
    std::vector<ExperimentMachine> machines{maschine("m1", "zen5", "ddr5-2x32", "prod1")};
    std::ostringstream             log;
    auto const                     b1 = pf::belege_aktive_maschinen_deklaration("prod1", machines, log);
    EXPECT_EQ(b1.treffer, 1u);
    EXPECT_TRUE(b1.belegt);
    EXPECT_EQ(b1.abgelehnt, 0u);
    EXPECT_TRUE(log.str().empty());

    // NEGATIV (a): leerer live_host -> NIE ein Treffer, nichts belegt, Log bleibt leer.
    meas::reset_active_machine_declaration_for_test();
    auto const ba = pf::belege_aktive_maschinen_deklaration("", machines, log);
    EXPECT_EQ(ba.treffer, 0u);
    EXPECT_FALSE(ba.belegt);
    EXPECT_TRUE(log.str().empty());

    // NEGATIV (b): leerer hostname_hint zaehlt NIE als Treffer -- auch nicht fuer irgendeinen Host.
    std::vector<ExperimentMachine> ohne_hint{maschine("m2", "zen5", "ddr5-2x32", "")};
    auto const                     bb = pf::belege_aktive_maschinen_deklaration("prod1", ohne_hint, log);
    EXPECT_EQ(bb.treffer, 0u);
    EXPECT_FALSE(bb.belegt);

    // NEGATIV (c): fremder Hint -> kein Treffer (der Schluessel VERGLEICHT, er raet nicht).
    auto const bc = pf::belege_aktive_maschinen_deklaration("prod2", machines, log);
    EXPECT_EQ(bc.treffer, 0u);
    EXPECT_FALSE(bc.belegt);
    EXPECT_TRUE(log.str().empty());
}

TEST_F(LgHostBinder, FeldZweiCpuFabricationKonfliktWirdAbgelehnt) {
    // [2] NEGATIV: Zweit-Treffer desselben Hosts mit ABWEICHENDER cpu_fabrication -> abgelehnt,
    // Erstbelegung bleibt, der Fehler-Log traegt Klasse + id (D1, kein Abbruch).
    std::vector<ExperimentMachine> machines{maschine("m1", "zen5", "ddr5-2x32", "prod1"),
                                            maschine("m1b", "zen4", "ddr5-2x32", "prod1")};
    std::ostringstream             log;
    auto const                     b = pf::belege_aktive_maschinen_deklaration("prod1", machines, log);
    EXPECT_EQ(b.treffer, 2u) << "beide Zeilen treffen den Hint -- der Konflikt liegt im Tupel";
    EXPECT_TRUE(b.belegt) << "die ERSTbelegung bleibt bestehen";
    EXPECT_EQ(b.abgelehnt, 1u);
    EXPECT_NE(log.str().find("konfig_xml_parse"), std::string::npos) << log.str();
    EXPECT_NE(log.str().find("cpu_fabrication=\"zen4\""), std::string::npos) << log.str();
    EXPECT_NE(log.str().find("id=\"m1b\""), std::string::npos) << log.str();
}

TEST_F(LgHostBinder, FeldDreiRamPairKonfliktWirdAbgelehnt) {
    // [3] NEGATIV: Zweit-Treffer mit ABWEICHENDEM ram_pair -- dieselbe Konflikt-Klasse.
    std::vector<ExperimentMachine> machines{maschine("m1", "zen5", "ddr5-2x32", "prod1"),
                                            maschine("m1c", "zen5", "ddr4-2x16", "prod1")};
    std::ostringstream             log;
    auto const                     b = pf::belege_aktive_maschinen_deklaration("prod1", machines, log);
    EXPECT_EQ(b.treffer, 2u);
    EXPECT_TRUE(b.belegt);
    EXPECT_EQ(b.abgelehnt, 1u);
    EXPECT_NE(log.str().find("ram_pair=\"ddr4-2x16\""), std::string::npos) << log.str();
}

TEST_F(LgHostBinder, NennerAbgrenzungNurDieDreiFelderBinden) {
    // Die RUECKRICHTUNG des 3/3-Nenners: ein Zweit-Treffer, der NUR in nicht-bindenden Feldern
    // abweicht (ram_frequency_mhz, cas_latency_cl, os_version), ist ein IDENTISCHER Treffer --
    // kein Konflikt, kein Log. Ohne diese Probe waere "3 Felder" nur eine Aufzaehlung.
    ExperimentMachine zweit = maschine("m1d", "zen5", "ddr5-2x32", "prod1");
    zweit.ram_frequency_mhz = 6000;
    zweit.cas_latency_cl    = 30;
    zweit.os_version        = "ubuntu-24.04";
    std::vector<ExperimentMachine> machines{maschine("m1", "zen5", "ddr5-2x32", "prod1"), zweit};
    std::ostringstream             log;
    auto const                     b = pf::belege_aktive_maschinen_deklaration("prod1", machines, log);
    EXPECT_EQ(b.treffer, 2u);
    EXPECT_TRUE(b.belegt);
    EXPECT_EQ(b.abgelehnt, 0u) << "nicht-bindende Felder duerfen keinen Tupel-Konflikt ausloesen";
    EXPECT_TRUE(log.str().empty()) << log.str();
}

TEST_F(LgHostBinder, IdentischerZweittrefferIstKeinKonflikt) {
    // BereitsGesetztIdentisch: dieselben drei Felder zweimal -> beide zaehlen als Treffer, beide
    // belegen, nichts wird abgelehnt (Wiederholungs-Stabilitaet der Bindung).
    std::vector<ExperimentMachine> machines{maschine("m1", "zen5", "ddr5-2x32", "prod1"),
                                            maschine("m1e", "zen5", "ddr5-2x32", "prod1")};
    std::ostringstream             log;
    auto const                     b = pf::belege_aktive_maschinen_deklaration("prod1", machines, log);
    EXPECT_EQ(b.treffer, 2u);
    EXPECT_TRUE(b.belegt);
    EXPECT_EQ(b.abgelehnt, 0u);
    EXPECT_TRUE(log.str().empty());
}

} // namespace
