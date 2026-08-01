// tests/unit/test_p2_ram_probe_chain.cpp -- Task #7, Paket P2: die RT-Erhebungs-Kette + die CT-Factory.
//
// WAS HIER BEWIESEN WIRD, und was ausdruecklich nicht: geprueft wird das VERHALTEN der Kette gegen
// einen eingespielten Testbaum -- welche Stufe liefert, welche faellt mit welchem Grund durch, und ob
// der Degradations-Pfad das ehrlich wiedergibt. NICHT geprueft werden Live-Hardware-WERTE: eine
// Erwartung wie "hier stehen 4800 MT/s" waere auf der naechsten Maschine falsch und in der CI ohnehin
// nicht reproduzierbar (Auflage A9: "CI testet decline-Pfade + eingespielte Puffer, nie Live-Hardware").
//
// FIXTURE-HERKUNFT (A9 -- ableiten, nicht behaupten):
//   - SPD-Bytes: die realen prod1-Offsets (27.07.2026, /sys/bus/i2c/devices/1-0051/eeprom, 0444). Die
//     erwartete Rate wird NICHT als 4800 behauptet, sondern im Test aus den Fixture-Bytes ueber die
//     Parser-Funktion abgeleitet -- aendert sich die Fixture, wandert die Erwartung mit.
//   - Deklarations-Erwartungen: durchgaengig aus kDeclaredMachines gezogen (Schluessel UND Zahl). Es
//     steht in dieser Datei kein einziger handkopierter Maschinen-Schluessel; eine Umbenennung in der
//     Deklaration zieht die Tests automatisch mit, statt sie stumm falsch werden zu lassen.
//   - Boot-Cache: aus den Kontrakt-Konstanten des Headers zusammengesetzt, nicht aus einem
//     abgeschriebenen Textblock -- so kann der Kontrakt nicht in Header und Test auseinanderlaufen.

#include <cache_engine/measurement/hardware_probe_factory.hpp>
#include <cache_engine/measurement/ram_probe_chain.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace meas = comdare::cache_engine::measurement;
namespace fs   = std::filesystem;

namespace {

// -- Die deklarierte Referenz-Maschine: die EINZIGE Zeile mit einer erhobenen RAM-Zahl. Sie wird
//    GESUCHT statt indiziert -- ein fester Index waere genau die Sorte Handkopie, die A9 ausschliesst.
[[nodiscard]] meas::DeclaredMachine const& maschine_mit_deklarierter_frequenz() {
    for (auto const& m : meas::kDeclaredMachines)
        if (m.ram_frequency_mhz != 0U) return m;
    ADD_FAILURE() << "Keine Maschine in kDeclaredMachines traegt eine RAM-Frequenz -- die Fixture-Basis "
                     "dieses Tests ist weggefallen.";
    return meas::kDeclaredMachines[0];
}

/// Die Maschine OHNE erhobene RAM-Zahl (0-Marke). Ihr Tupel ist der benannte Stufe-3-Ausfall-Fall.
[[nodiscard]] meas::DeclaredMachine const& maschine_ohne_deklarierte_frequenz() {
    for (auto const& m : meas::kDeclaredMachines)
        if (m.ram_frequency_mhz == 0U) return m;
    ADD_FAILURE() << "Alle Maschinen tragen inzwischen eine RAM-Frequenz -- der 0-Marken-Fall dieses "
                     "Tests existiert nicht mehr und gehoert angepasst, nicht entfernt.";
    return meas::kDeclaredMachines[0];
}

// -- Der Testbaum ---------------------------------------------------------------------------------
/// Ein Wegwerf-Verzeichnis je Testfall. Der Zeitstempel im Namen haelt parallele Laeufe auseinander
/// (die CI faehrt Tests nebenlaeufig); der Destruktor raeumt auf, damit kein Lauf den naechsten sieht.
class Testbaum {
public:
    explicit Testbaum(std::string const& name)
        : wurzel_(fs::temp_directory_path() /
                  ("comdare_p2_" + name + "_" +
                   std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {
        std::error_code ec;
        fs::create_directories(wurzel_, ec);
    }
    Testbaum(Testbaum const&)            = delete;
    Testbaum& operator=(Testbaum const&) = delete;
    ~Testbaum() {
        std::error_code ec;
        fs::remove_all(wurzel_, ec);
    }

    [[nodiscard]] fs::path const& wurzel() const noexcept { return wurzel_; }

    /// Legt <wurzel>/<geraet>/eeprom mit den gegebenen Bytes an (Nachbau des i2c-sysfs-Layouts).
    [[nodiscard]] fs::path lege_spd_knoten(std::string const& geraet, std::vector<std::uint8_t> const& bytes) {
        std::error_code ec;
        fs::create_directories(wurzel_ / geraet, ec);
        fs::path const p = wurzel_ / geraet / std::string{meas::kSpdEepromLeafName};
        std::ofstream  f(p, std::ios::binary);
        f.write(reinterpret_cast<char const*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        return p;
    }

    [[nodiscard]] fs::path schreibe_datei(std::string const& name, std::string const& inhalt) {
        fs::path const p = wurzel_ / name;
        std::ofstream  f(p, std::ios::binary);
        f << inhalt;
        return p;
    }

private:
    fs::path wurzel_;
};

// -- SPD-Fixture (dieselbe Bau-Logik wie in test_p1: der BAU ist die Single-Source) -----------------
constexpr std::uint8_t kRealKeyByte    = 18;  // Byte 2 -> DDR5 (realer prod1-Dump)
constexpr std::uint8_t kRealTckLow     = 160; // Byte 20 \_ little endian -> 416 ps
constexpr std::uint8_t kRealTckHigh    = 1;   // Byte 21 /
constexpr std::size_t  kRealDumpLaenge = 1024;

[[nodiscard]] std::vector<std::uint8_t> baue_spd_fixture() {
    std::vector<std::uint8_t> b(kRealDumpLaenge, 0U);
    b[meas::kSpdKeyByteOffset]       = kRealKeyByte;
    b[meas::kSpdTckAvgMinOffset]     = kRealTckLow;
    b[meas::kSpdTckAvgMinOffset + 1] = kRealTckHigh;
    return b;
}

/// Die Erwartung aus der Fixture ABGELEITET (A9), nicht als Konstante behauptet.
[[nodiscard]] std::uint32_t erwartete_spd_rate() {
    auto const bytes = baue_spd_fixture();
    return meas::spd_mts_from_tck_ps(meas::spd_read_u16_le(bytes, meas::kSpdTckAvgMinOffset));
}

// -- Boot-Cache-Fixture, aus den KONTRAKT-KONSTANTEN gebaut ----------------------------------------
[[nodiscard]] std::string baue_boot_cache(std::string const& boot_id, std::uint32_t mts, bool mit_marker = true,
                                          bool mit_boot_id = true) {
    std::string s;
    if (mit_marker) s += std::string{meas::kBootCacheMarker} + "\n";
    if (mit_boot_id) s += std::string{meas::kBootCacheKeyBootId} + "=" + boot_id + "\n";
    s += std::string{meas::kBootCacheKeyConfiguredSpeed} + "=" + std::to_string(mts) + "\n";
    s += std::string{meas::kBootCacheKeyMemoryType} + "=DDR5\n";
    return s;
}

constexpr char const* kFixtureBootId = "11111111-2222-3333-4444-555555555555";
constexpr char const* kFremdeBootId  = "99999999-8888-7777-6666-555555555555";

/// Ein Kontext, der auf NICHTS zeigt -- der Docker-Fall in Reinform.
[[nodiscard]] meas::RamProbeContext leerer_kontext(meas::DeclaredMachine const& m) {
    meas::RamProbeContext ctx{};
    ctx.cpu_fabrication_key = m.cpu_fabrication;
    ctx.ram_pair_key        = m.ram_pair;
    ctx.injected_now_unix_s = 1'700'000'000ULL; // feste Uhr: sonst ist jede Zeit-Erwartung unpruefbar
    return ctx;
}

using LinuxZelle = meas::HardwareProbeDevice<meas::DefaultTargetIsaComplex, meas::LinuxOperatingSystem>;

} // namespace

// =================================================================================================
// 1. DER DOCKER-FALL: leerer Baum -> Stufe 3, und der Pfad sagt WARUM
// =================================================================================================
TEST(P2RamProbeChain, LeererBaumFaelltEhrlichAufDieDeklarationDurch) {
    auto const& m   = maschine_mit_deklarierter_frequenz();
    auto const  ctx = leerer_kontext(m);
    auto const  r   = meas::probe_ram_frequency<LinuxZelle>(ctx);

    EXPECT_EQ(r.provenance, meas::RamFrequencyProvenance::DeclaredNotMeasured);
    EXPECT_TRUE(meas::has_frequency(r));
    // Die Zahl wird AUS der Deklaration gezogen, nicht behauptet (A9).
    EXPECT_EQ(r.mts, m.ram_frequency_mhz);

    // Und der eigentliche Punkt: der Pfad weist beide uebersprungenen Stufen als DURCHGEFALLEN aus,
    // mit dem erwarteten Normalgrund -- nicht als "nicht gefragt".
    EXPECT_EQ(meas::stage_outcome(r.trail, meas::RamFrequencyProvenance::ConfiguredMeasured),
              meas::ProbeStageOutcome::Durchgefallen);
    EXPECT_EQ(meas::stage_reason(r.trail, meas::RamFrequencyProvenance::ConfiguredMeasured),
              meas::HardwareProbeErrorClass::QuelleFehlt);
    EXPECT_EQ(meas::stage_outcome(r.trail, meas::RamFrequencyProvenance::SpdJedecBase),
              meas::ProbeStageOutcome::Durchgefallen);
    EXPECT_EQ(meas::stage_reason(r.trail, meas::RamFrequencyProvenance::SpdJedecBase),
              meas::HardwareProbeErrorClass::QuelleFehlt);
    EXPECT_EQ(meas::stage_outcome(r.trail, meas::RamFrequencyProvenance::DeclaredNotMeasured),
              meas::ProbeStageOutcome::Geliefert);
    EXPECT_FALSE(meas::stage_reason(r.trail, meas::RamFrequencyProvenance::DeclaredNotMeasured).has_value())
        << "Eine gelieferte Stufe hat KEINEN Grund -- der Default QuelleFehlt darf nicht durchschlagen.";
}

TEST(P2RamProbeChain, OhneDeklarationUndOhneQuellenBleibtAllesLeer) {
    // Ein Tupel, das in KEINER Deklaration steht: jetzt faellt auch das Terminal-Glied durch.
    meas::RamProbeContext ctx{};
    ctx.cpu_fabrication_key = "gibt_es_nicht";
    ctx.ram_pair_key        = "auch_nicht";
    auto const r            = meas::probe_ram_frequency<LinuxZelle>(ctx);

    EXPECT_FALSE(meas::has_frequency(r)) << "Ohne jede Quelle wird NICHTS geraten.";
    EXPECT_EQ(r.state, meas::RamReadingState::NichtErhoben);
    EXPECT_EQ(r.mts, 0U);
    // Der Pfad ist trotzdem vollstaendig -- gerade im Totalausfall ist er die einzige Auskunft.
    for (std::size_t i = 0; i < meas::kRamFrequencyProvenanceCount; ++i) {
        auto const s = static_cast<meas::RamFrequencyProvenance>(i);
        EXPECT_EQ(meas::stage_outcome(r.trail, s), meas::ProbeStageOutcome::Durchgefallen)
            << "Stufe " << i << " muesste gefragt worden und durchgefallen sein.";
    }
}

TEST(P2RamProbeChain, DieNullMarkeIstKeinWert) {
    // Die Maschine ohne erhobene Frequenz traegt 0. Das Terminal-Glied darf daraus KEINEN erhobenen
    // Wert machen -- sonst stuenden 0 MT/s als Messung in der Reihe.
    auto const& m = maschine_ohne_deklarierte_frequenz();
    auto const  r = meas::probe_ram_frequency<LinuxZelle>(leerer_kontext(m));

    EXPECT_FALSE(meas::has_frequency(r));
    EXPECT_EQ(meas::stage_outcome(r.trail, meas::RamFrequencyProvenance::DeclaredNotMeasured),
              meas::ProbeStageOutcome::Durchgefallen);
    EXPECT_EQ(meas::stage_reason(r.trail, meas::RamFrequencyProvenance::DeclaredNotMeasured),
              meas::HardwareProbeErrorClass::QuelleFehlt);
}

// =================================================================================================
// 2. STUFE 2 -- der SPD-Weg gegen einen eingespielten Baum
// =================================================================================================
TEST(P2RamProbeChain, EingespielterSpdKnotenLiefertStufeZwei) {
    Testbaum baum("spd_ok");
    (void)baum.lege_spd_knoten("1-0051", baue_spd_fixture());

    auto const&           m      = maschine_mit_deklarierter_frequenz();
    std::string const     wurzel = baum.wurzel().string();
    meas::RamProbeContext ctx    = leerer_kontext(m);
    ctx.spd_device_root          = wurzel;
    auto const r                 = meas::probe_ram_frequency<LinuxZelle>(ctx);

    EXPECT_EQ(r.provenance, meas::RamFrequencyProvenance::SpdJedecBase);
    EXPECT_EQ(r.mts, erwartete_spd_rate()) << "Die Rate folgt den Fixture-Bytes, nicht einer Konstante.";
    EXPECT_EQ(r.collected_at_unix_s, *ctx.injected_now_unix_s);
    // Stufe 3 wurde NICHT gefragt -- und dieser Unterschied zu "durchgefallen" ist die Aussage.
    EXPECT_EQ(meas::stage_outcome(r.trail, meas::RamFrequencyProvenance::SpdJedecBase),
              meas::ProbeStageOutcome::Geliefert);
    EXPECT_EQ(meas::stage_outcome(r.trail, meas::RamFrequencyProvenance::DeclaredNotMeasured),
              meas::ProbeStageOutcome::NichtGefragt);
}

TEST(P2RamProbeChain, KorrupteSpdQuelleDegradiertNichtStill) {
    // Der Kern von Auflage A4: die Quelle IST da, sie ist nur kaputt. Die Kette faellt weiter auf die
    // Deklaration durch -- aber der Pfad traegt ein ANDERES Etikett als bei einer fehlenden Quelle.
    Testbaum                  baum("spd_korrupt");
    std::vector<std::uint8_t> kaputt      = baue_spd_fixture();
    kaputt[meas::kSpdTckAvgMinOffset]     = 0; // genullte Taktzeit: der halb gelesene sysfs-Knoten
    kaputt[meas::kSpdTckAvgMinOffset + 1] = 0;
    (void)baum.lege_spd_knoten("1-0051", kaputt);

    auto const&           m   = maschine_mit_deklarierter_frequenz();
    std::string const     w   = baum.wurzel().string();
    meas::RamProbeContext ctx = leerer_kontext(m);
    ctx.spd_device_root       = w;
    auto const r              = meas::probe_ram_frequency<LinuxZelle>(ctx);

    EXPECT_EQ(r.provenance, meas::RamFrequencyProvenance::DeclaredNotMeasured) << "Die Kette laeuft weiter.";
    EXPECT_EQ(meas::stage_reason(r.trail, meas::RamFrequencyProvenance::SpdJedecBase),
              meas::HardwareProbeErrorClass::QuelleKorrupt)
        << "Eine kaputte Quelle ist ein BEFUND und darf NIE als fehlende Quelle erscheinen (A4).";
    EXPECT_NE(meas::stage_reason(r.trail, meas::RamFrequencyProvenance::SpdJedecBase),
              meas::HardwareProbeErrorClass::QuelleFehlt);
}

TEST(P2RamProbeChain, FremdesSpdFormatDisqualifiziertDenKnotenNichtDieStufe) {
    // An einem i2c-Bus haengt mehr als Speicher. Ein fremder Knoten darf die Stufe nicht kippen,
    // solange ein anderer traegt -- und die Sortierung stellt sicher, dass das reproduzierbar ist.
    Testbaum                  baum("spd_gemischt");
    std::vector<std::uint8_t> fremd = baue_spd_fixture();
    fremd[meas::kSpdKeyByteOffset]  = 12;        // DDR4-Key -> FormatUnbekannt
    (void)baum.lege_spd_knoten("0-0050", fremd); // sortiert VOR dem guten Knoten
    (void)baum.lege_spd_knoten("1-0051", baue_spd_fixture());

    auto const&           m   = maschine_mit_deklarierter_frequenz();
    std::string const     w   = baum.wurzel().string();
    meas::RamProbeContext ctx = leerer_kontext(m);
    ctx.spd_device_root       = w;
    auto const r              = meas::probe_ram_frequency<LinuxZelle>(ctx);

    EXPECT_EQ(r.provenance, meas::RamFrequencyProvenance::SpdJedecBase);
    EXPECT_EQ(r.mts, erwartete_spd_rate());
}

TEST(P2RamProbeChain, NurFremdeKnotenLassenDenLetztenGrundStehen) {
    Testbaum                  baum("spd_nur_fremd");
    std::vector<std::uint8_t> fremd = baue_spd_fixture();
    fremd[meas::kSpdKeyByteOffset]  = 12;
    (void)baum.lege_spd_knoten("1-0051", fremd);

    auto const&           m   = maschine_mit_deklarierter_frequenz();
    std::string const     w   = baum.wurzel().string();
    meas::RamProbeContext ctx = leerer_kontext(m);
    ctx.spd_device_root       = w;
    auto const r              = meas::probe_ram_frequency<LinuxZelle>(ctx);

    EXPECT_EQ(meas::stage_reason(r.trail, meas::RamFrequencyProvenance::SpdJedecBase),
              meas::HardwareProbeErrorClass::FormatUnbekannt)
        << "\"Da war etwas, es war nur nicht unseres\" darf nicht als \"da war nichts\" erscheinen.";
}

// =================================================================================================
// 3. STUFE 1 -- der Boot-Cache und seine Frische-Wache
// =================================================================================================
TEST(P2RamProbeChain, FrischerBootCacheLiefertStufeEins) {
    Testbaum                baum("bootcache_frisch");
    constexpr std::uint32_t kFixtureMts = 5600; // bewusst NICHT die JEDEC-Basis: so ist die Stufe unterscheidbar
    std::string const bc = baum.schreibe_datei("dmi_ram.cache", baue_boot_cache(kFixtureBootId, kFixtureMts)).string();
    std::string const bi = baum.schreibe_datei("boot_id", std::string{kFixtureBootId} + "\n").string();

    auto const&           m   = maschine_mit_deklarierter_frequenz();
    meas::RamProbeContext ctx = leerer_kontext(m);
    ctx.boot_cache_path       = bc;
    ctx.boot_id_path          = bi;
    auto const r              = meas::probe_ram_frequency<LinuxZelle>(ctx);

    EXPECT_EQ(r.provenance, meas::RamFrequencyProvenance::ConfiguredMeasured);
    EXPECT_EQ(r.mts, kFixtureMts);
    EXPECT_NE(r.mts, m.ram_frequency_mhz) << "Die Fixture unterscheidet sich absichtlich von der "
                                             "Deklaration -- sonst waere nicht zu sehen, welche Stufe lieferte.";
    EXPECT_EQ(meas::stage_outcome(r.trail, meas::RamFrequencyProvenance::SpdJedecBase),
              meas::ProbeStageOutcome::NichtGefragt);
}

TEST(P2RamProbeChain, StaleBootCacheFaelltDurchStattEinenAltenTaktAuszugeben) {
    Testbaum          baum("bootcache_stale");
    std::string const bc = baum.schreibe_datei("dmi_ram.cache", baue_boot_cache(kFremdeBootId, 5600)).string();
    std::string const bi = baum.schreibe_datei("boot_id", std::string{kFixtureBootId} + "\n").string();

    auto const&           m   = maschine_mit_deklarierter_frequenz();
    meas::RamProbeContext ctx = leerer_kontext(m);
    ctx.boot_cache_path       = bc;
    ctx.boot_id_path          = bi;
    auto const r              = meas::probe_ram_frequency<LinuxZelle>(ctx);

    EXPECT_NE(r.provenance, meas::RamFrequencyProvenance::ConfiguredMeasured)
        << "Eine Datei aus einem FRUEHEREN Boot beschreibt moeglicherweise ein anderes BIOS-Profil.";
    EXPECT_EQ(meas::stage_reason(r.trail, meas::RamFrequencyProvenance::ConfiguredMeasured),
              meas::HardwareProbeErrorClass::QuelleKorrupt);
    EXPECT_EQ(r.mts, m.ram_frequency_mhz) << "Die Kette laeuft ehrlich bis zur Deklaration durch.";
}

TEST(P2RamProbeChain, BootCacheOhneMarkerWirdNichtInterpretiert) {
    Testbaum          baum("bootcache_fremd");
    std::string const bc =
        baum.schreibe_datei("fremd.cache", baue_boot_cache(kFixtureBootId, 5600, /*mit_marker=*/false)).string();
    std::string const bi = baum.schreibe_datei("boot_id", std::string{kFixtureBootId} + "\n").string();

    auto const&           m   = maschine_mit_deklarierter_frequenz();
    meas::RamProbeContext ctx = leerer_kontext(m);
    ctx.boot_cache_path       = bc;
    ctx.boot_id_path          = bi;
    auto const r              = meas::probe_ram_frequency<LinuxZelle>(ctx);

    EXPECT_EQ(meas::stage_reason(r.trail, meas::RamFrequencyProvenance::ConfiguredMeasured),
              meas::HardwareProbeErrorClass::FormatUnbekannt)
        << "Eine fremde Datei ist NICHT korrupt -- sie ist in Ordnung, nur nicht unsere.";
}

TEST(P2RamProbeChain, BootCacheOhneBootIdIstUnvollstaendig) {
    Testbaum          baum("bootcache_ohne_id");
    std::string const bc =
        baum.schreibe_datei("dmi_ram.cache", baue_boot_cache(kFixtureBootId, 5600, true, /*mit_boot_id=*/false))
            .string();
    std::string const bi = baum.schreibe_datei("boot_id", std::string{kFixtureBootId} + "\n").string();

    auto const&           m   = maschine_mit_deklarierter_frequenz();
    meas::RamProbeContext ctx = leerer_kontext(m);
    ctx.boot_cache_path       = bc;
    ctx.boot_id_path          = bi;
    auto const r              = meas::probe_ram_frequency<LinuxZelle>(ctx);

    EXPECT_EQ(meas::stage_reason(r.trail, meas::RamFrequencyProvenance::ConfiguredMeasured),
              meas::HardwareProbeErrorClass::QuelleKorrupt)
        << "Ohne boot_id laesst sich die Frische nicht beweisen -- dann wird nicht geglaubt.";
}

TEST(P2RamProbeChain, DerNennwertWirdNIEALSISTTAKTGELESEN) {
    // Die teuerste Verwechslung des ganzen Pakets: `speed_mts` (Nennwert) darf NICHT als
    // `configured_speed_mts` (Ist-Takt) durchgehen. Eine Datei, die NUR den Nennwert traegt, ist
    // unvollstaendig -- nicht etwa eine mit Ist-Takt.
    Testbaum    baum("bootcache_nur_nennwert");
    std::string inhalt   = std::string{meas::kBootCacheMarker} + "\n" + std::string{meas::kBootCacheKeyBootId} + "=" +
                           kFixtureBootId + "\nspeed_mts=5600\n";
    std::string const bc = baum.schreibe_datei("dmi_ram.cache", inhalt).string();
    std::string const bi = baum.schreibe_datei("boot_id", std::string{kFixtureBootId} + "\n").string();

    auto const&           m   = maschine_mit_deklarierter_frequenz();
    meas::RamProbeContext ctx = leerer_kontext(m);
    ctx.boot_cache_path       = bc;
    ctx.boot_id_path          = bi;
    auto const r              = meas::probe_ram_frequency<LinuxZelle>(ctx);

    EXPECT_NE(r.provenance, meas::RamFrequencyProvenance::ConfiguredMeasured);
    EXPECT_EQ(meas::stage_reason(r.trail, meas::RamFrequencyProvenance::ConfiguredMeasured),
              meas::HardwareProbeErrorClass::QuelleKorrupt);
    EXPECT_NE(r.mts, 5600U) << "Der Nennwert darf auf KEINEM Weg als Frequenz herausfallen.";
}

TEST(P2RamProbeChain, DieVollstaendigeOrdnungBootCacheSchlaegtSpd) {
    // Beide Quellen vorhanden, verschiedene Werte: die Kette MUSS die hoehere Stufe nehmen. Das ist
    // die Ordnungs-Aussage der CoR, hier zur Laufzeit belegt (compile-time steht sie im Header).
    Testbaum baum("ordnung");
    (void)baum.lege_spd_knoten("1-0051", baue_spd_fixture());
    constexpr std::uint32_t kBootMts = 5600;
    std::string const bc = baum.schreibe_datei("dmi_ram.cache", baue_boot_cache(kFixtureBootId, kBootMts)).string();
    std::string const bi = baum.schreibe_datei("boot_id", std::string{kFixtureBootId} + "\n").string();

    auto const&           m   = maschine_mit_deklarierter_frequenz();
    std::string const     w   = baum.wurzel().string();
    meas::RamProbeContext ctx = leerer_kontext(m);
    ctx.boot_cache_path       = bc;
    ctx.boot_id_path          = bi;
    ctx.spd_device_root       = w;
    auto const r              = meas::probe_ram_frequency<LinuxZelle>(ctx);

    ASSERT_NE(kBootMts, erwartete_spd_rate()) << "Die Fixture-Werte muessen sich unterscheiden, sonst "
                                                 "beweist dieser Test nichts.";
    EXPECT_EQ(r.provenance, meas::RamFrequencyProvenance::ConfiguredMeasured);
    EXPECT_EQ(r.mts, kBootMts);
}

// =================================================================================================
// 4. DIE FACTORY: Zellen, Totalitaet und die ehrliche Nicht-Implementierung
// =================================================================================================
TEST(P2HardwareProbeFactory, DeclaredOnlyZellenFallenBENANNTDurchStattZuSchweigen) {
    using WindowsZelle = meas::HardwareProbeDevice<meas::DefaultTargetIsaComplex, meas::WindowsOperatingSystem>;
    auto const& m      = maschine_mit_deklarierter_frequenz();
    auto const  r      = meas::probe_ram_frequency<WindowsZelle>(leerer_kontext(m));

    EXPECT_EQ(r.provenance, meas::RamFrequencyProvenance::DeclaredNotMeasured);
    EXPECT_EQ(r.mts, m.ram_frequency_mhz);
    // Der eigentliche Punkt von A8: die beiden oberen Stufen wurden GEFRAGT und sind durchgefallen.
    // Waeren sie schlicht weggelassen, stuenden sie als NichtGefragt da -- also als "eine bessere
    // Stufe hat geliefert", was hier nachweislich falsch ist.
    EXPECT_EQ(meas::stage_outcome(r.trail, meas::RamFrequencyProvenance::ConfiguredMeasured),
              meas::ProbeStageOutcome::Durchgefallen);
    EXPECT_EQ(meas::stage_outcome(r.trail, meas::RamFrequencyProvenance::SpdJedecBase),
              meas::ProbeStageOutcome::Durchgefallen);
    EXPECT_FALSE(WindowsZelle::has_native_probe());
    EXPECT_FALSE(WindowsZelle::not_implemented_reason().empty())
        << "Eine nicht implementierte Zelle MUSS den Grund benennen (A8).";
}

TEST(P2HardwareProbeFactory, EineDeclaredOnlyZelleLiestKeineQuelleAuchWennEineDaLiegt) {
    // Gegenprobe zur Zell-Wahl: derselbe Testbaum, zwei Zellen. Die Linux-Zelle liest ihn, die
    // Windows-Zelle nicht -- der Unterschied liegt AUSSCHLIESSLICH im Typ.
    Testbaum baum("zellwahl");
    (void)baum.lege_spd_knoten("1-0051", baue_spd_fixture());

    auto const&           m   = maschine_mit_deklarierter_frequenz();
    std::string const     w   = baum.wurzel().string();
    meas::RamProbeContext ctx = leerer_kontext(m);
    ctx.spd_device_root       = w;

    using WindowsZelle = meas::HardwareProbeDevice<meas::DefaultTargetIsaComplex, meas::WindowsOperatingSystem>;
    auto const linux_r = meas::probe_ram_frequency<LinuxZelle>(ctx);
    auto const win_r   = meas::probe_ram_frequency<WindowsZelle>(ctx);

    EXPECT_EQ(linux_r.provenance, meas::RamFrequencyProvenance::SpdJedecBase);
    EXPECT_EQ(win_r.provenance, meas::RamFrequencyProvenance::DeclaredNotMeasured)
        << "Die declared-only-Zelle darf die Quelle NICHT lesen -- sonst waere die Zell-Wahl folgenlos.";
}

TEST(P2HardwareProbeFactory, DieTotalitaetsWacheIstNichtLeer) {
    // Ohne diese Gegenprobe waere die Totalitaets-Wache eine leere Zusicherung: sie koennte fuer JEDE
    // Zelle true liefern und niemandem faellt es auf. Eine erfundene Achsen-Auspraegung ohne
    // Spezialisierung MUSS als unentschieden erkannt werden.
    struct ErfundenesOs final : meas::OperatingSystemAxis<ErfundenesOs> {
        [[nodiscard]] static constexpr std::string_view do_os_family_id() noexcept { return "erfunden"; }
        [[nodiscard]] static constexpr bool             do_is_posix_family() noexcept { return false; }
    };
    static_assert(meas::HasProbeDeviceCell<meas::DefaultTargetIsaComplex, meas::LinuxOperatingSystem>);
    static_assert(meas::HasProbeDeviceCell<meas::DefaultTargetIsaComplex, meas::WindowsOperatingSystem>);
    static_assert(meas::HasProbeDeviceCell<meas::DefaultTargetIsaComplex, meas::MacosOperatingSystem>);
    static_assert(!meas::HasProbeDeviceCell<meas::DefaultTargetIsaComplex, ErfundenesOs>,
                  "Die Totalitaets-Wache haelt eine unentschiedene Zelle fuer entschieden -- sie ist damit "
                  "wirkungslos, und jede neue Achsen-Auspraegung wuerde still durchrutschen (K5).");
    SUCCEED() << "Gegenprobe compile-time bestanden: unentschiedene Zellen werden als solche erkannt.";
}

TEST(P2HardwareProbeFactory, JedeZelleDerVollenMatrixTraegtEinGeraet) {
    // Die Laufzeit-Sicht auf K3: alle sechs Zellen sind instanziierbar und liefern ein Etikett. Die
    // compile-time-Wache steht im Header; diese hier faellt auf, wenn jemand sie dort entfernt.
    EXPECT_FALSE(
        (meas::HardwareProbeDevice<meas::Prod1Zen5TargetIsa, meas::LinuxOperatingSystem>::device_id()).empty());
    EXPECT_FALSE(
        (meas::HardwareProbeDevice<meas::Prod1Zen5TargetIsa, meas::WindowsOperatingSystem>::device_id()).empty());
    EXPECT_FALSE(
        (meas::HardwareProbeDevice<meas::Prod1Zen5TargetIsa, meas::MacosOperatingSystem>::device_id()).empty());
    EXPECT_FALSE(
        (meas::HardwareProbeDevice<meas::Prod2RaptorLakeTargetIsa, meas::LinuxOperatingSystem>::device_id()).empty());
    EXPECT_FALSE(
        (meas::HardwareProbeDevice<meas::Prod2RaptorLakeTargetIsa, meas::WindowsOperatingSystem>::device_id()).empty());
    EXPECT_FALSE(
        (meas::HardwareProbeDevice<meas::Prod2RaptorLakeTargetIsa, meas::MacosOperatingSystem>::device_id()).empty());
}

// =================================================================================================
// 5. DIE FREIGABE-NAHT: einmal erheben, danach benannt memoisieren
// =================================================================================================
TEST(P2AcquireNaht, ZweiterAufrufIstMemoisiertUndSagtEs) {
    meas::reset_ram_acquisition_for_test();
    auto const& m = maschine_mit_deklarierter_frequenz();

    auto const erst = meas::acquire_ram_frequency<LinuxZelle>(m.cpu_fabrication, m.ram_pair);
    EXPECT_EQ(erst.state, meas::RamAcquisitionState::FrischErhoben);

    auto const zweit = meas::acquire_ram_frequency<LinuxZelle>(m.cpu_fabrication, m.ram_pair);
    EXPECT_EQ(zweit.state, meas::RamAcquisitionState::Memoisiert);
    EXPECT_EQ(zweit.reading.mts, erst.reading.mts);
    EXPECT_EQ(zweit.reading.collected_at_unix_s, erst.reading.collected_at_unix_s)
        << "Der Zeitstempel darf sich NICHT aendern -- sonst waere es eine zweite Erhebung.";

    meas::reset_ram_acquisition_for_test();
}

TEST(P2AcquireNaht, FremderSchluesselWirdBENANNTStattStillBeantwortet) {
    // Das ist der Grund, warum hier kein blosser Meyers-Static mit den Parametern steht: der wuerde
    // den zweiten Aufruf stillschweigend mit dem alten Ergebnis beantworten.
    meas::reset_ram_acquisition_for_test();
    auto const& m = maschine_mit_deklarierter_frequenz();

    auto const erst = meas::acquire_ram_frequency<LinuxZelle>(m.cpu_fabrication, m.ram_pair);
    ASSERT_EQ(erst.state, meas::RamAcquisitionState::FrischErhoben);

    auto const fremd = meas::acquire_ram_frequency<LinuxZelle>("ein_anderes_tupel", "und_ein_anderes_ram");
    EXPECT_EQ(fremd.state, meas::RamAcquisitionState::MemoisiertFremderSchluessel)
        << "Ein zweiter Aufruf mit abweichendem Schluessel MUSS als solcher gemeldet werden.";
    EXPECT_EQ(fremd.reading.mts, erst.reading.mts) << "Der Altwert bleibt -- benannt, nicht heimlich.";

    meas::reset_ram_acquisition_for_test();
}

TEST(P2AcquireNaht, VorDerErstenErhebungBehauptetDieNahtNichts) {
    meas::reset_ram_acquisition_for_test();
    meas::RamAcquisitionResult const leer{};
    EXPECT_FALSE(meas::has_frequency(leer.reading));
    EXPECT_EQ(leer.reading.collected_at_unix_s, 0U);
}

// =================================================================================================
// 6. LIVE-GEGENPROBE (nur wo die Hardware-Quelle wirklich lesbar ist)
// =================================================================================================
TEST(P2RamProbeChain, AufEchterHardwareBleibtDieAbleitungKonsistent) {
    // Dieselbe Zurueckhaltung wie im P1-Live-Test: geprueft wird die INVARIANTE, nie ein Wert. Auf
    // prod2 ist der Ausgang bis zum Infra-Check offen (E-8) -- ohne SPD-Knoten ist Stufe 3 das
    // ERWARTETE Ergebnis und kein Fehlschlag. Genau deshalb pinnt dieser Test keine Stufe.
    auto const& m   = maschine_mit_deklarierter_frequenz();
    auto const  ctx = meas::make_cell_context<LinuxZelle>(m.cpu_fabrication, m.ram_pair);
    auto const  r   = meas::probe_ram_frequency<LinuxZelle>(ctx);

    if (!meas::has_frequency(r)) {
        GTEST_SKIP() << "Keine Quelle und keine Deklaration auf dieser Maschine -- Live-Abgleich uebersprungen.";
    }
    // Was IMMER gelten muss, egal welche Stufe lieferte:
    EXPECT_EQ(meas::stage_outcome(r.trail, r.provenance), meas::ProbeStageOutcome::Geliefert)
        << "Die ausgewiesene Provenienz MUSS die Stufe sein, die im Pfad als liefernd steht.";
    EXPECT_FALSE(meas::stage_reason(r.trail, r.provenance).has_value());
    EXPECT_GT(r.collected_at_unix_s, 0U) << "Eine echte Erhebung traegt einen echten Zeitpunkt.";
    // Keine Stufe OBERHALB der liefernden darf als geliefert markiert sein -- sonst haette die Kette
    // die bessere Quelle gefunden und trotzdem die schlechtere genommen.
    for (std::size_t i = 0; i < static_cast<std::size_t>(r.provenance); ++i) {
        EXPECT_EQ(meas::stage_outcome(r.trail, static_cast<meas::RamFrequencyProvenance>(i)),
                  meas::ProbeStageOutcome::Durchgefallen);
    }
}
