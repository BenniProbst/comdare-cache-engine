// tests/unit/test_p3_boot_cache_dmi.cpp -- Task #7, Paket P3: der Stufe-1-REAL-Beweis (F13).
//
// WAS HIER BEWIESEN WIRD: dass BootCacheDmiProbe das ECHTE, live verifizierte Schreiber-Format des
// Infra-Kanals v1 liest -- nicht nur die idealisierte Kontrakt-Form der P2-Fixtures. Der Kanal ist
// seit 01.08.2026 auf beiden Hosts LIVE; sein Schreiber emittiert die Werte-Schluessel JE DIMM
// erneut (Kontrakt-Praezisierung in ram_probe_chain.hpp). Die P2-Tests bauten Dateien mit EINFACHEN
// Schluesseln -- diese Datei traegt (a) Fixtures in der ECHTEN je-DIMM-Block-Form und (b) den ersten
// Live-Beweis gegen den realen Boot-Cache dieses Hosts.
//
// FIXTURE-HERKUNFT (A9 -- ableiten, nie handkopieren): die Fixture wird aus den KONTRAKT-KONSTANTEN
// des Headers gebaut (Marker, Schluessel), ihre STRUKTUR (je-DIMM-Bloecke, slots_populated) aus dem
// live verifizierten Schreiber-Format. Die LIVE-Erwartung wird aus der echten Datei UNABHAENGIG
// zweitgelesen (eigener Zeilen-Leser, nicht der Produktions-Parser) -- eine gepinnte Zahl wie 5600
// stuende hier gegen den Owner-KERN "Hardware-Werte nie statisch": das BIOS-Profil des naechsten
// Boots darf die Zahl aendern, ohne dass ein Test luegt. Der Test prueft die STRUKTUR (Stufe 1
// liefert, Provenienz stimmt, Zahl == Datei-Zahl) und MELDET die Zahl literal.
//
// SKIP-GUARD: in Umgebungen ohne Infra-Schreiber (CI-Docker, 8er-Matrix) existiert die Datei nicht;
// der Live-Teil wird dann benannt uebersprungen -- die Fixture-Teile laufen ueberall.

#include <cache_engine/measurement/hardware_probe_factory.hpp>
#include <cache_engine/measurement/ram_probe_chain.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace meas = comdare::cache_engine::measurement;
namespace fs   = std::filesystem;

namespace {

// -- Wegwerf-Baum (Muster test_p2_ram_probe_chain) ------------------------------------------------
class Testbaum {
public:
    explicit Testbaum(std::string const& name)
        : wurzel_(fs::temp_directory_path() /
                  ("comdare_p3_" + name + "_" +
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

    [[nodiscard]] fs::path schreibe_datei(std::string const& name, std::string const& inhalt) {
        fs::path const p = wurzel_ / name;
        std::ofstream  f(p, std::ios::binary);
        f << inhalt;
        return p;
    }

private:
    fs::path wurzel_;
};

/// Die Boot-Cache-Datei in der ECHTEN Schreiber-Form: je bestuecktem DIMM ein eigener Werte-Block
/// (configured_speed_mts/speed_mts/memory_type), boot_id und slots_populated einmal. Marker und
/// Schluessel kommen aus den Kontrakt-Konstanten -- Header und Test koennen nicht auseinanderlaufen.
/// `speed_mts` hat bewusst KEINE Header-Konstante (der Nennwert darf nie gelesen werden) und steht
/// deshalb wie im Kontrakt-Kommentar als Literal.
[[nodiscard]] std::string baue_je_dimm_datei(std::string const&                boot_id,
                                             std::vector<std::uint32_t> const& je_dimm_configured) {
    std::string s;
    s += std::string{meas::kBootCacheMarker} + "\n";
    s += std::string{meas::kBootCacheKeyBootId} + "=" + boot_id + "\n";
    for (auto const mts : je_dimm_configured) {
        s += std::string{meas::kBootCacheKeyConfiguredSpeed} + "=" + std::to_string(mts) + "\n";
        s += "speed_mts=" + std::to_string(mts) + "\n";
        s += std::string{meas::kBootCacheKeyMemoryType} + "=DDR5\n";
    }
    s += "slots_populated=" + std::to_string(je_dimm_configured.size()) + "\n";
    return s;
}

constexpr char const* kFixtureBootId = "11111111-2222-3333-4444-555555555555";

/// UNABHAENGIGE Zweitlesung eines Schluessels: eigener Zeilen-Leser statt des Produktions-Parsers,
/// damit Erwartung und Prueferling nicht derselbe Code sind. Erste Treffer-Zeile, wie der Kontrakt
/// es dem Leser zusichert.
[[nodiscard]] std::optional<std::uint32_t> zweitlesung_erster_wert(fs::path const& datei, std::string_view key) {
    std::ifstream f(datei);
    if (!f.is_open()) return std::nullopt;
    std::string const praefix = std::string{key} + "=";
    std::string       zeile;
    while (std::getline(f, zeile)) {
        if (zeile.rfind(praefix, 0) != 0) continue;
        std::string const rest = zeile.substr(praefix.size());
        if (rest.empty()) return std::nullopt;
        std::uint32_t wert = 0;
        for (char const c : rest) {
            if (c < '0' || c > '9') return std::nullopt;
            wert = wert * 10U + static_cast<std::uint32_t>(c - '0');
        }
        return wert;
    }
    return std::nullopt;
}

using LinuxZelle = meas::HardwareProbeDevice<meas::DefaultTargetIsaComplex, meas::LinuxOperatingSystem>;

} // namespace

// =================================================================================================
// 1. DAS ECHTE SCHREIBER-FORMAT (je-DIMM-Bloecke) GEGEN DEN LESER
// =================================================================================================
TEST(P3BootCacheDmi, JeDimmWiederholteSchluesselLiefernStufeEins) {
    // Die P2-Fixtures trugen jeden Schluessel EINMAL; der echte Schreiber wiederholt die
    // Werte-Schluessel je DIMM. Dieser Test schliesst genau die Luecke zwischen Kontrakt-Ideal und
    // Schreiber-Wirklichkeit (die Lehre der O-8-Fixture-Falle: Fixtures vom IST ableiten).
    Testbaum                baum("je_dimm_homogen");
    constexpr std::uint32_t kFixtureMts = 6000; // Fixture-EINGABE, kein Hardware-Pin
    std::string const       bc =
        baum.schreibe_datei("dmi_ram.cache", baue_je_dimm_datei(kFixtureBootId, {kFixtureMts, kFixtureMts})).string();
    std::string const bi = baum.schreibe_datei("boot_id", std::string{kFixtureBootId} + "\n").string();

    meas::RamProbeContext ctx{};
    ctx.boot_cache_path     = bc;
    ctx.boot_id_path        = bi;
    ctx.injected_now_unix_s = 1'700'000'000ULL;
    auto const r            = meas::probe_ram_frequency<LinuxZelle>(ctx);

    EXPECT_EQ(r.provenance, meas::RamFrequencyProvenance::ConfiguredMeasured);
    EXPECT_EQ(r.mts, kFixtureMts) << "Die Erwartung folgt der Fixture-Eingabe, nicht einer Hardware-Zahl.";
    EXPECT_EQ(meas::stage_outcome(r.trail, meas::RamFrequencyProvenance::ConfiguredMeasured),
              meas::ProbeStageOutcome::Geliefert);
}

TEST(P3BootCacheDmi, HeterogeneDimmBloeckeNimmtDieErsteZeile) {
    // Befund-Notiz P3 (Kontrakt-Praezisierung im Header): bei heterogener Bestueckung gewinnt die
    // ERSTE Zeile. Dieser Test pinnt die heutige v1-Leser-Semantik, damit eine spaetere
    // Aggregations-Regel als BEWUSSTE Kontrakt-Erweiterung eintritt statt als stille Drift.
    Testbaum                baum("je_dimm_heterogen");
    constexpr std::uint32_t kErsterDimm  = 6000;
    constexpr std::uint32_t kZweiterDimm = 6400;
    std::string const       bc =
        baum.schreibe_datei("dmi_ram.cache", baue_je_dimm_datei(kFixtureBootId, {kErsterDimm, kZweiterDimm})).string();
    std::string const bi = baum.schreibe_datei("boot_id", std::string{kFixtureBootId} + "\n").string();

    meas::RamProbeContext ctx{};
    ctx.boot_cache_path = bc;
    ctx.boot_id_path    = bi;
    auto const r        = meas::probe_ram_frequency<LinuxZelle>(ctx);

    EXPECT_EQ(r.provenance, meas::RamFrequencyProvenance::ConfiguredMeasured);
    EXPECT_EQ(r.mts, kErsterDimm) << "v1-Leser-Semantik: erste Treffer-Zeile gewinnt (Befund-Notiz P3).";
    EXPECT_NE(r.mts, kZweiterDimm);
}

// =================================================================================================
// 2. DER LIVE-BEWEIS (F13: erst prod1 testen) -- Skip-Guard fuer Umgebungen ohne Infra-Schreiber
// =================================================================================================
TEST(P3BootCacheLive, DerEchteBootCacheDiesesHostsLiefertStufeEins) {
    std::error_code ec;
    fs::path const  live_datei{std::string{meas::kDefaultBootCachePath}};
    if (!fs::exists(live_datei, ec) || ec) {
        GTEST_SKIP() << "Kein Boot-Cache unter " << meas::kDefaultBootCachePath
                     << " -- Umgebung ohne Infra-Schreiber (CI-Docker); Live-Beweis uebersprungen.";
    }

    // Erwartung UNABHAENGIG aus der Live-Datei zweitgelesen -- nie eine gepinnte Hardware-Zahl.
    auto const erwartet = zweitlesung_erster_wert(live_datei, meas::kBootCacheKeyConfiguredSpeed);
    ASSERT_TRUE(erwartet.has_value()) << "Die Live-Datei traegt keinen lesbaren " << meas::kBootCacheKeyConfiguredSpeed
                                      << " -- Kontrakt-Bruch.";

    // (a) Die Stufe SOLO gegen die echten Default-Handles der Linux-Zelle.
    meas::RamProbeContext ctx{};
    ctx.boot_cache_path = meas::kDefaultBootCachePath;
    ctx.boot_id_path    = meas::kDefaultBootIdPath;
    meas::BootCacheDmiProbe const stufe1{};
    auto const                    solo = stufe1.probe(ctx);
    ASSERT_TRUE(solo.has_value()) << "Stufe 1 fiel LIVE durch (" << meas::hardware_probe_label(solo.error())
                                  << ") -- bei vorhandener Datei ist das ein Befund, kein Skip.";
    EXPECT_EQ(solo->provenance, meas::RamFrequencyProvenance::ConfiguredMeasured);
    EXPECT_EQ(solo->mts, *erwartet) << "Parser und unabhaengige Zweitlesung muessen dieselbe Zahl sehen.";
    EXPECT_GE(solo->mts, meas::kBootCacheMtsMin);
    EXPECT_LE(solo->mts, meas::kBootCacheMtsMax);

    // (b) Die GANZE Kette: mit lieferfaehiger Stufe 1 darf keine schlechtere Quelle gewinnen.
    auto const kette = meas::probe_ram_frequency<LinuxZelle>(meas::make_cell_context<LinuxZelle>("", ""));
    EXPECT_EQ(kette.provenance, meas::RamFrequencyProvenance::ConfiguredMeasured)
        << "Stufe 1 ist live lieferfaehig -- die Kette MUSS sie nehmen (Vertrauens-Ordnung).";
    EXPECT_EQ(kette.mts, *erwartet);
    EXPECT_EQ(meas::stage_outcome(kette.trail, meas::RamFrequencyProvenance::SpdJedecBase),
              meas::ProbeStageOutcome::NichtGefragt);

    // Der literale Live-Beleg fuer das Protokoll (erster Stufe-1-Live-Beweis).
    std::cout << "[P3-LIVE] Stufe-1 configured_measured = " << solo->mts << " MT/s aus " << meas::kDefaultBootCachePath
              << "\n";
}
