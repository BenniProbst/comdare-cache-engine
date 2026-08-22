// test_pmc_raw_event_katalog -- #82 (I-PMC-3, Owner-GO KON103 17.08.2026): der modell-gebundene RAW-Katalog.
//
// GEGENSTAND: measurement/pmc_raw_event_katalog.hpp. Der Katalog traegt die am Objekt KREUZGEPROBTEN
// PERF_TYPE_RAW-Kodierungen fuer L2/coherence (heute Zen 5) und behauptet fuer alles andere NICHTS
// (vorhanden=false -> die Quelle oeffnet nichts Rohes, die Zellen lesen "n/a").
//
// FREMDER NENNER (T-3): die Zen-5-configs werden hier NICHT vom Katalog abgeschrieben, sondern aus der
// FREMDEN Quelle unabhaengig NACHGERECHNET -- dem AMD-RAW-config-Layout aus sysfs
// (/sys/bus/event_source/devices/cpu/format/: event=config:0-7,32-35; umask=config:8-15) mit den
// perf-JSON-Werten (l2_cache_req_stat.ic_dc_miss_in_l2 = event 0x64, umask 0x09;
// ls_dmnd_fills_from_sys.remote_cache = event 0x43, umask 0x14). Stimmen Katalog und Rechnung nicht
// ueberein, hat einer von beiden gedriftet -- genau das soll hier brechen.

#include <gtest/gtest.h>

#include <cstdint>

#include <cache_engine/measurement/pmc_raw_event_katalog.hpp>

namespace cme = comdare::cache_engine::measurement;

namespace {

/// Die FREMDE Rechnung: AMD-Core-PMU-RAW-config aus event+umask (sysfs-Format-Layout, s. Kopf).
/// Bewusst eine EIGENE Implementierung, keine Weiterleitung -- sie ist die Gegenprobe zum Katalog.
[[nodiscard]] constexpr std::uint64_t amd_raw_config(std::uint64_t event, std::uint64_t umask) noexcept {
    // event bits 0-7 (die hier verwendeten Events passen in 8 bit; bits 32-35 blieben 0), umask bits 8-15.
    return (umask << 8) | (event & 0xFFu);
}

} // namespace

#if defined(__linux__)

TEST(PmcRawEventKatalog, Zen5TraegtDieKreuzgeprobtenKodierungen) {
    auto const b = cme::pmc_raw_belegung_fuer("AuthenticAMD", 26);
    ASSERT_TRUE(b.vorhanden) << "Zen 5 (family 26) ist der am Objekt bewiesene Eintrag (#82)";

    // Die fremde Rechnung gegen den Katalog (perf-JSON-Werte, s. Kopf): 0x64/0x09 und 0x43/0x14.
    EXPECT_EQ(b.l2.config, amd_raw_config(0x64, 0x09))
        << "L2 = l2_cache_req_stat.ic_dc_miss_in_l2 (Kreuzprobe r964, 17.314.023 == 17.314.023)";
    EXPECT_EQ(b.coherence.config, amd_raw_config(0x43, 0x14))
        << "coherence = ls_dmnd_fills_from_sys.remote_cache (Kreuzprobe r1443, 21.503 == 21.503)";
    EXPECT_EQ(b.l2.type, static_cast<std::uint32_t>(PERF_TYPE_RAW)) << "I-PMC-3 ist der RAW-Weg";
    EXPECT_EQ(b.coherence.type, static_cast<std::uint32_t>(PERF_TYPE_RAW));

    // Diagnose-Identitaet: beide Namen nicht leer und verschieden (ein [PMC-DIAG]-Log ohne
    // unterscheidbare Namen klassifiziert nichts).
    EXPECT_FALSE(b.l2.name.empty());
    EXPECT_FALSE(b.coherence.name.empty());
    EXPECT_NE(b.l2.name, b.coherence.name);
}

TEST(PmcRawEventKatalog, UngeprobteModelleBleibenLeer) {
    // KEIN Rateversuch (Kernel-Falle: unbekannte Kodierungen zaehlen still etwas anderes) -- jedes nicht
    // kreuzgeprobte Modell liefert vorhanden=false, in JEDER Richtung des Schluessels:
    EXPECT_FALSE(cme::pmc_raw_belegung_fuer("AuthenticAMD", 25).vorhanden) << "Zen 3/4: nicht kreuzgeprobt";
    EXPECT_FALSE(cme::pmc_raw_belegung_fuer("AuthenticAMD", 27).vorhanden) << "Zukunfts-Family: unbekannt";
    EXPECT_FALSE(cme::pmc_raw_belegung_fuer("GenuineIntel", 6).vorhanden) << "Intel: prod1 ist AMD";
    EXPECT_FALSE(cme::pmc_raw_belegung_fuer("GenuineIntel", 26).vorhanden)
        << "die Family allein genuegt NICHT -- der Vendor gehoert zum Schluessel";
    EXPECT_FALSE(cme::pmc_raw_belegung_fuer("", 0).vorhanden) << "unbekannt => leer (fail-closed)";
    EXPECT_FALSE(cme::pmc_raw_belegung_fuer("authenticamd", 26).vorhanden)
        << "cpuid-Vendor-Strings sind exakt (keine stille Normalisierung, die etwas anderes matcht)";
}

#else // !__linux__

TEST(PmcRawEventKatalog, NichtLinuxIstStrukturellLeer) {
    EXPECT_FALSE(cme::pmc_raw_belegung_fuer("AuthenticAMD", 26).vorhanden)
        << "ohne perf_event_open gibt es keinen RAW-Weg (fail-closed)";
}

#endif // __linux__
