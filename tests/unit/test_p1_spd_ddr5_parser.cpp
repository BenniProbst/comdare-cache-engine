// tests/unit/test_p1_spd_ddr5_parser.cpp -- Task #7, Paket P1: der DDR5-SPD-Byte-Parser.
//
// WAS HIER BEWIESEN WIRD: dass der Parser aus echten SPD-Bytes die JEDEC-Nennrate ableitet, und --
// wichtiger -- dass er in JEDEM Nicht-Normalfall eine benannte Fehlerklasse liefert statt einer
// Zahl. Ein Byte-Parser, der bei Muell irgendetwas Plausibles zurueckgibt, ist gefaehrlicher als
// keiner: die Zahl landet sonst als "gemessen" in der Messreihe.
//
// FIXTURE-HERKUNFT (Auflage A9 -- ableiten, nicht behaupten): Die Byte-Werte stammen aus dem REALEN
// Dump von /sys/bus/i2c/devices/1-0051/eeprom auf prod1 (27.07.2026, 0444 world-readable, beide
// bestueckten DIMMs bitgleich). Uebernommen sind die Offsets, die der Parser liest; der Rest des
// 1024-Byte-Puffers bleibt 0, damit die Fixture lesbar bleibt und keine DIMM-Seriennummern ins Repo
// wandern (dieselbe Sorgfalt, die die Kernel-Sperre fuer SMBIOS Type 17 begruendet).
//
// Die erwartete Rate wird NICHT als Konstante 4800 behauptet, sondern im Test aus den Fixture-Bytes
// UNABHAENGIG nachgerechnet (zweiter Ableitungsweg) und zusaetzlich gegen den extern belegten
// JEDEC-Wert gepinnt. Faellt eine der beiden Richtungen um, ist es ein echter Befund.

#include <cache_engine/measurement/spd_ddr5_parser.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace meas = comdare::cache_engine::measurement;

namespace {

/// Die realen prod1-Bytes an den Offsets, die der Parser liest (siehe Kopf-Block).
constexpr std::uint8_t kRealKeyByte    = 18;  // Byte 2  -> DDR5
constexpr std::uint8_t kRealTckLow     = 160; // Byte 20 \_ little endian -> 416 ps
constexpr std::uint8_t kRealTckHigh    = 1;   // Byte 21 /
constexpr std::uint8_t kRealXmpMagic0  = 0x0C;
constexpr std::uint8_t kRealXmpMagic1  = 0x4A;
constexpr std::size_t  kRealDumpLaenge = 1024;

/// Baut den Fixture-Puffer. Der Bau ist die Single-Source: jeder Testfall variiert ihn, statt eine
/// zweite handgepflegte Byte-Liste zu fuehren.
std::vector<std::uint8_t> baue_spd_fixture(bool mit_xmp = true, bool mit_expo = true) {
    std::vector<std::uint8_t> b(kRealDumpLaenge, 0U);
    b[meas::kSpdKeyByteOffset]       = kRealKeyByte;
    b[meas::kSpdTckAvgMinOffset]     = kRealTckLow;
    b[meas::kSpdTckAvgMinOffset + 1] = kRealTckHigh;
    if (mit_xmp) {
        b[meas::kSpdXmpMagicOffset]     = kRealXmpMagic0;
        b[meas::kSpdXmpMagicOffset + 1] = kRealXmpMagic1;
    }
    if (mit_expo) {
        b[meas::kSpdExpoMagicOffset]     = static_cast<std::uint8_t>('E');
        b[meas::kSpdExpoMagicOffset + 1] = static_cast<std::uint8_t>('X');
        b[meas::kSpdExpoMagicOffset + 2] = static_cast<std::uint8_t>('P');
        b[meas::kSpdExpoMagicOffset + 3] = static_cast<std::uint8_t>('O');
    }
    return b;
}

/// Die Erwartung, UNABHAENGIG vom Parser aus den Fixture-Bytes abgeleitet (A9: zweiter Weg).
/// Bewusst anders formuliert als im Parser (Fliesskomma + Rundung statt Ganzzahl-Arithmetik) --
/// eine Kopie derselben Zeile wuerde nur beweisen, dass sie sich selbst gleicht.
std::uint32_t erwartete_rate_aus_bytes(std::vector<std::uint8_t> const& b) {
    double const tck_ps = static_cast<double>(b[meas::kSpdTckAvgMinOffset]) +
                          256.0 * static_cast<double>(b[meas::kSpdTckAvgMinOffset + 1]);
    double const roh    = 2000000.0 / tck_ps;
    return static_cast<std::uint32_t>((roh + 50.0) / 100.0) * 100U;
}

} // namespace

// =================================================================================================
// 1. DER NORMALFALL
// =================================================================================================
TEST(P1SpdDdr5Parser, RealerProd1DumpLiefertJedecBasisrate) {
    auto const bytes = baue_spd_fixture();
    auto const r     = meas::parse_spd_ddr5(bytes);
    ASSERT_TRUE(r.has_value()) << "gueltige DDR5-Bytes muessen parsen";
    EXPECT_EQ(r->tck_ps, 416U) << "Byte 20/21 little endian: 160 + 256*1";
    // (a) unabhaengig aus den Bytes abgeleitet, (b) gegen den extern belegten JEDEC-Wert gepinnt.
    EXPECT_EQ(r->jedec_mts, erwartete_rate_aus_bytes(bytes));
    EXPECT_EQ(r->jedec_mts, 4800U) << "416 ps ist die JEDEC-Stufe DDR5-4800 (dmidecode-Beleg prod2, "
                                      "Recherche-Doc prod1)";
}

TEST(P1SpdDdr5Parser, ReadingTraegtStufeZweiUndNiemalsDasAngebot) {
    auto const r = meas::parse_spd_ddr5(baue_spd_fixture());
    ASSERT_TRUE(r.has_value());
    auto const reading = meas::reading_from_spd(*r);
    EXPECT_EQ(reading.provenance, meas::RamFrequencyProvenance::SpdJedecBase)
        << "SPD liefert NIE den konfigurierten Ist-Takt -- nur die Nennrate";
    EXPECT_EQ(reading.state, meas::RamReadingState::Erhoben);
    EXPECT_TRUE(meas::has_frequency(reading));
    EXPECT_EQ(reading.mts, 4800U);
    // Der Kern von Auflage A7: das Riegel-Angebot darf NIE als Frequenz erscheinen. Der Parser
    // erkennt es (unten), aber es gibt keinen Weg, es in mts zu bekommen.
    EXPECT_EQ(reading.xmp_expo_offer_mts, 0U);
    EXPECT_FALSE(meas::has_profile_offer(reading));
}

// =================================================================================================
// 2. DIE ANGEBOTS-ERKENNUNG (Praesenz, bewusst ohne Frequenz -- siehe Parser-Kopf)
// =================================================================================================
TEST(P1SpdDdr5Parser, XmpUndExpoWerdenErkanntUndFehlenWirdErkannt) {
    auto const beide = meas::parse_spd_ddr5(baue_spd_fixture(true, true));
    ASSERT_TRUE(beide.has_value());
    EXPECT_TRUE(beide->xmp_present) << "XMP-3.0-Magic 0x0C 0x4A bei Byte 640 (prod1-Ist)";
    EXPECT_TRUE(beide->expo_present) << "EXPO-ASCII bei Byte 832 (prod1-Ist)";

    auto const keins = meas::parse_spd_ddr5(baue_spd_fixture(false, false));
    ASSERT_TRUE(keins.has_value()) << "fehlende Profile sind KEIN Fehler -- die Riegel bieten eben keins";
    EXPECT_FALSE(keins->xmp_present);
    EXPECT_FALSE(keins->expo_present);
    EXPECT_EQ(keins->jedec_mts, 4800U) << "die JEDEC-Rate haengt nicht an den Profil-Bloecken";

    auto const nur_xmp = meas::parse_spd_ddr5(baue_spd_fixture(true, false));
    ASSERT_TRUE(nur_xmp.has_value());
    EXPECT_TRUE(nur_xmp->xmp_present);
    EXPECT_FALSE(nur_xmp->expo_present) << "die beiden Magics duerfen sich nicht gegenseitig setzen";
}

// =================================================================================================
// 3. DIE FEHLERFAELLE -- hier entscheidet sich, ob der Parser ehrlich ist
// =================================================================================================
TEST(P1SpdDdr5Parser, FremderDramTypWirdNIEInterpretiert) {
    auto bytes                     = baue_spd_fixture();
    bytes[meas::kSpdKeyByteOffset] = 12; // DDR4-Key
    auto const r                   = meas::parse_spd_ddr5(bytes);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), meas::HardwareProbeErrorClass::FormatUnbekannt)
        << "Ein fremdes Format ist NICHT korrupt -- die Quelle ist in Ordnung, nur nicht unsere. "
           "Die tCK-Bytes stehen bei DDR4 woanders; sie hier zu lesen ergaebe eine plausible Zahl "
           "aus fremden Feldern.";
}

TEST(P1SpdDdr5Parser, AbgeschnittenerPufferIstKorruptNichtLeer) {
    auto const voll = baue_spd_fixture();
    // (a) zu kurz fuer die Rate, aber Key noch lesbar
    std::vector<std::uint8_t> const kurz(voll.begin(), voll.begin() + meas::kSpdMinBytesForRate - 1);
    auto const                      r_kurz = meas::parse_spd_ddr5(kurz);
    ASSERT_FALSE(r_kurz.has_value());
    EXPECT_EQ(r_kurz.error(), meas::HardwareProbeErrorClass::QuelleKorrupt);
    // (b) so kurz, dass nicht einmal der Key dasteht
    std::vector<std::uint8_t> const winzig(voll.begin(), voll.begin() + 1);
    auto const                      r_winzig = meas::parse_spd_ddr5(winzig);
    ASSERT_FALSE(r_winzig.has_value());
    EXPECT_EQ(r_winzig.error(), meas::HardwareProbeErrorClass::QuelleKorrupt);
    // (c) leerer Puffer -- der Fall, den ein fehlgeschlagener Lesevorgang liefert
    auto const r_leer = meas::parse_spd_ddr5({});
    ASSERT_FALSE(r_leer.has_value());
    EXPECT_EQ(r_leer.error(), meas::HardwareProbeErrorClass::QuelleKorrupt);
}

TEST(P1SpdDdr5Parser, UnplausibleTaktzeitWirdAbgewiesenStattGerechnet) {
    // Genullte Bytes: der klassische Ausgang eines halb gelesenen sysfs-Knotens.
    auto null_tck                           = baue_spd_fixture();
    null_tck[meas::kSpdTckAvgMinOffset]     = 0;
    null_tck[meas::kSpdTckAvgMinOffset + 1] = 0;
    auto const r_null                       = meas::parse_spd_ddr5(null_tck);
    ASSERT_FALSE(r_null.has_value()) << "0 ps waere eine Division durch Null bzw. unendlich schnell";
    EXPECT_EQ(r_null.error(), meas::HardwareProbeErrorClass::QuelleKorrupt);

    // Geloeschtes EEPROM: 0xFFFF ps entspraechen rund 30 MT/s -- eine Zahl, die der Parser NIE
    // ausgeben darf, obwohl sie rechnerisch herauskaeme.
    auto leer_eeprom                           = baue_spd_fixture();
    leer_eeprom[meas::kSpdTckAvgMinOffset]     = 0xFF;
    leer_eeprom[meas::kSpdTckAvgMinOffset + 1] = 0xFF;
    auto const r_ff                            = meas::parse_spd_ddr5(leer_eeprom);
    ASSERT_FALSE(r_ff.has_value());
    EXPECT_EQ(r_ff.error(), meas::HardwareProbeErrorClass::QuelleKorrupt);
}

TEST(P1SpdDdr5Parser, GueltigerPufferOhneProfilBloeckeIstKeinFehler) {
    // Ein Puffer, der nach der Rate endet: SPD-Varianten mit 128/256 Byte gibt es real. Die
    // Angebots-Erkennung darf daran nicht scheitern, sondern meldet schlicht "kein Angebot".
    auto const                      voll = baue_spd_fixture();
    std::vector<std::uint8_t> const kurz(voll.begin(), voll.begin() + 128);
    auto const                      r = meas::parse_spd_ddr5(kurz);
    ASSERT_TRUE(r.has_value()) << "genug Bytes fuer die Rate = gueltig, auch ohne Profil-Bloecke";
    EXPECT_EQ(r->jedec_mts, 4800U);
    EXPECT_FALSE(r->xmp_present);
    EXPECT_FALSE(r->expo_present);
}

// =================================================================================================
// 4. LIVE-GEGENPROBE (nur wo die Hardware-Quelle wirklich lesbar ist)
// =================================================================================================
TEST(P1SpdDdr5Parser, AufEchtemSpdKnotenStimmtDieAbleitung) {
    // Diese Probe laeuft NUR, wenn der unprivilegierte SPD-Knoten existiert (prod1: ja, 0444). Auf
    // jeder anderen Maschine wird sie uebersprungen statt falsch behauptet -- dasselbe Muster wie
    // der prod1-Live-Abgleich in test_o4_machine_identity.
    char const* const pfad = "/sys/bus/i2c/devices/1-0051/eeprom";
    std::ifstream     f(pfad, std::ios::binary);
    if (!f) { GTEST_SKIP() << "Kein lesbarer SPD-Knoten unter " << pfad << " -- Live-Abgleich uebersprungen."; }
    std::vector<std::uint8_t> const bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    ASSERT_GE(bytes.size(), meas::kSpdMinBytesForRate) << "SPD-Knoten liefert zu wenig Bytes";

    auto const r = meas::parse_spd_ddr5(bytes);
    ASSERT_TRUE(r.has_value()) << "echter SPD-Knoten muss parsen; Fehler: "
                               << (r.has_value() ? "" : meas::hardware_probe_label(r.error()));
    // Der Wert wird NICHT auf 4800 gepinnt: auf einer anderen Maschine waere das schlicht falsch.
    // Geprueft wird die INVARIANTE -- die Rate folgt den gelesenen Bytes.
    EXPECT_EQ(r->jedec_mts, meas::spd_mts_from_tck_ps(r->tck_ps));
    EXPECT_GE(r->tck_ps, meas::kSpdTckPsMin);
    EXPECT_LE(r->tck_ps, meas::kSpdTckPsMax);
    EXPECT_GT(r->jedec_mts, 0U);
}
