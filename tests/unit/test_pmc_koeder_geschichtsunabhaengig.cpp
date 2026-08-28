// test_pmc_koeder_geschichtsunabhaengig -- E07/dtlb-Koeder (Diagnose 25.08.2026): DIE ERKENNUNG MUSS
// GESCHICHTSUNABHAENGIG BEISSEN.
//
// BEFUND (#114-Re-Run 25.08.2026, Beweisort backups-workflow/20260825-diagnose-e07/): zwei Aufrufe von
// dump_experiment_plan_facade im SELBEN Prozess lieferten Plan-Texte, die sich in genau EINEM Byte
// unterschieden -- Zeile 7 "pmc_befund=amd events=3/4" gegen "events=2/4"; kippender Koeder = dtlb_misses.
// Instrumentiert (src_diag/b10_k5_diag3_koeder.cpp): der Koeder in measurement/pmc_event_biss.hpp deklarierte
// "64 MiB, 4096 Byte Schrittweite", legte aber kSlots ELEMENTE std::size_t an = 128 KiB = 32 Seiten. Diese
// Arbeitsmenge passt in jeden DTLB; die dtlb_misses (Page-Walks) kamen NUR aus der Erstberuehrung FRISCHER
// Seiten. Nach glibc-Wiederverwendung des Chunks (dieselbe Adresse, 33/33 Seiten resident) zaehlte der
// Zaehler ehrlich 0: die Erkennung hing an der Allokator-Geschichte des Prozesses, nicht am Chase.
//
// WAS HIER GEPRUEFT WIRD (T-1, rot zuerst -- am alten Koeder auf prod1: 20/20 frische Prozesse ungleich):
//   (1) VIER Erhebungen probe_pmc_host<>() im selben Prozess tragen IDENTISCHE Biss-Vektoren, Lage und
//       Zaehler (Lead-Order 25.08.2026: "4x im selben Prozess, identische Bisse").
//   (2) Je Event ACHT direkte Koeder-Fenster (pmc_event_beisst, DIE Event-Liste) urteilen identisch --
//       die feinere Sonde: jede Erhebung ist selbst eine Folge solcher Fenster.
//   (3) GEOMETRIE-ANKER (T-3, Literale HIER eingefroren): 64 MiB, 4096 B je Kettenglied, 16384 Seiten --
//       mehr als der groesste bekannte L2-DTLB (4096 Eintraege) haelt. Wer den Koeder wieder schrumpft,
//       reisst diesen Anker, bevor die Erkennung still geschichtsabhaengig wird.
//
// FREMDER NENNER (T-3): die Event-Zahl kommt aus measurement::kPmcEventCount, nie als hier erfundene Zahl.
// EHRLICHE GRENZE: beisst auf dieser Maschine KEIN Event (kein Zugriff, Container, paranoid), ist "alle
// beissen nicht" trivial identisch und beweist nichts -- dann SKIPPT der Test ausdruecklich mit Grund
// (keine stille gruene Null). Genauso bei einer verdraengten PMU (0/N nach dem Fenster-Deckel): das ist die
// deklarierte Multiplexing-Klasse (CI 16073/382856), nicht die Geschichts-Klasse dieses Tests.
//
// NACHTRAG (CI 16260, Job 385992, prod2/GenuineIntel): dieser Test biss im Feld -- Erhebung 2 von 4 kam
// auf events=2/4 (cache_misses_l3_ll=0; dtlb_misses=0), Fenster 8 der dtlb-Sonde urteilte anders als
// Fenster 1. Ursache am Objekt: der Kern las ein TEIL-Fenster (0 < t_running < t_enabled, PMU-Rotation
// unter Belegung: 4 GP-Counter je Thread unter HT + NMI-Watchdog + CI-Nachbarn) mit Wert 0 als ehrliche
// Absage; dtlb_misses=0 ueber einen voll gemessenen 16384-Seiten-Chase ist physikalisch unmoeglich
// (groesster bekannter L2-DTLB/STLB: 4096 Eintraege) -- die 0 stammte aus nicht gemessener Fenster-Zeit.
// HEILUNG im Kern (measurement/pmc_event_biss.hpp): gepinntes Event (alles-oder-nichts-Scheduling),
// frischer fd je Fenster, die 0 urteilt NUR aus einem VOLL gelaufenen Fenster, und der Fenster-Verlauf
// steht als PmcBissFenster/fenster_vektor im Befund. Die Assertions dieses Tests sind UNVERAENDERT --
// die Wache bleibt scharf, keine Vendor-Ausnahme.
//
// ASCII-only.

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "planner/pmc_host_probe.hpp"

#include <cache_engine/measurement/pmc_event_biss.hpp>
#include <cache_engine/measurement/pmc_event_set.hpp>

namespace planner = comdare::cache_engine::planner;
namespace cme     = comdare::cache_engine::measurement;

namespace {

constexpr std::size_t kErhebungen = 4;
constexpr std::size_t kFenster    = 8;

} // namespace

// (1) Die REALE Erkennung, viermal hintereinander im selben Prozess: derselbe Host, dieselbe Rechte-Lage,
// derselbe Koeder -- vier Antworten, die sich unterscheiden, waeren vier Meinungen ueber EINE Maschine.
TEST(PmcKoederGeschichtsunabhaengig, VierErhebungenImSelbenProzessBeissenIdentisch) {
    std::vector<planner::PmcHostBefund> b;
    b.reserve(kErhebungen);
    for (std::size_t i = 0; i < kErhebungen; ++i) b.push_back(planner::probe_pmc_host<>());
    ASSERT_EQ(b.size(), kErhebungen);
    for (std::size_t i = 0; i < b.size(); ++i) {
        ASSERT_TRUE(b[i].probe_gefahren) << "Erhebung " << (i + 1) << " lief nicht";
        ASSERT_EQ(b[i].events_geprueft, cme::kPmcEventCount) << "Erhebung " << (i + 1) << ": fremder Nenner";
        if (b[i].events_gebissen == 0) {
            GTEST_SKIP() << "Erhebung " << (i + 1) << " ohne einen einzigen Biss (" << b[i].nenner_zeile()
                         << ") -- kein Zugriff oder verdraengte PMU; Geschichtsunabhaengigkeit ist auf dieser "
                            "Maschine JETZT nicht pruefbar (ausdruecklicher SKIP, keine stille Null)";
        }
    }
    std::string sicht;
    for (std::size_t i = 0; i < b.size(); ++i)
        sicht += "\n    Erhebung " + std::to_string(i + 1) + ": " + b[i].nenner_zeile();
    for (std::size_t i = 1; i < b.size(); ++i) {
        EXPECT_EQ(b[i].biss_vektor, b[0].biss_vektor)
            << "Erhebung " << (i + 1) << " beisst anders als Erhebung 1 (Geschichtsabhaengigkeit)." << sicht;
        EXPECT_EQ(b[i].events_gebissen, b[0].events_gebissen) << "Zaehler wackelt." << sicht;
        EXPECT_EQ(b[i].lage, b[0].lage) << "Lage wackelt." << sicht;
    }
}

// (2) Die feinere Sonde: je Event acht Koeder-Fenster hintereinander. Ein Fenster, das anders urteilt als
// das erste, ist genau der Kipp, der im #114-Re-Run als "events=3/4 -> 2/4" sichtbar wurde.
TEST(PmcKoederGeschichtsunabhaengig, AchtKoederFensterJeEventUrteilenIdentisch) {
    if (cme::kPmcEventCount == 0) GTEST_SKIP() << "keine Event-Liste auf dieser Plattform (Nicht-Linux)";
    bool                                    irgendein_biss = false;
    std::string                             sicht;
    std::vector<std::array<bool, kFenster>> urteile(cme::kPmcEventCount);
    for (std::size_t e = 0; e < cme::kPmcEventCount; ++e) {
        sicht += "\n    " + std::string{cme::kPmcEvents[e].name} + "=[";
        for (std::size_t f = 0; f < kFenster; ++f) {
            urteile[e][f]  = cme::pmc_event_beisst(cme::kPmcEvents[e]);
            irgendein_biss = irgendein_biss || urteile[e][f];
            sicht += urteile[e][f] ? "1" : "0";
        }
        sicht += "]";
    }
    if (!irgendein_biss) {
        GTEST_SKIP() << "kein Event beisst in " << kFenster
                     << " Fenstern -- kein Zugriff oder verdraengte PMU; "
                        "Geschichtsunabhaengigkeit ist hier JETZT nicht pruefbar (ausdruecklicher SKIP)."
                     << sicht;
    }
    for (std::size_t e = 0; e < cme::kPmcEventCount; ++e) {
        for (std::size_t f = 1; f < kFenster; ++f) {
            EXPECT_EQ(urteile[e][f], urteile[e][0])
                << cme::kPmcEvents[e].name << ": Fenster " << (f + 1) << " urteilt anders als Fenster 1 "
                << "(Geschichtsabhaengigkeit des Koeders)." << sicht;
        }
    }
}

// (3) Die Geometrie, die (1) und (2) traegt -- als eingefrorene Literale (T-3), nicht aus dem Pruefling gezogen.
TEST(PmcKoederGeschichtsunabhaengig, GeometrieAnkerSechzigVierMiBEineSeiteJeKettenglied) {
    constexpr std::size_t kBytesSoll   = 67108864u;
    constexpr std::size_t kSchrittSoll = 4096u;
    constexpr std::size_t kSeitenSoll  = 16384u;
    constexpr std::size_t kL2DtlbMax   = 4096u;
    static_assert(cme::kPmcKoederBytes == kBytesSoll, "64 MiB");
    static_assert(cme::kPmcKoederSchritt == kSchrittSoll, "eine 4-KiB-Seite je Kettenglied");
    static_assert(cme::kPmcKoederSlots == kSeitenSoll, "16384 Kettenglieder = 16384 Seiten");
    static_assert(cme::kPmcKoederSlots > kL2DtlbMax, "mehr Seiten als der groesste bekannte L2-DTLB");
    EXPECT_EQ(cme::kPmcKoederBytes, kBytesSoll);
    EXPECT_EQ(cme::kPmcKoederSchritt, kSchrittSoll);
    EXPECT_EQ(cme::kPmcKoederSlots, kSeitenSoll);
    EXPECT_GT(cme::kPmcKoederSlots, kL2DtlbMax);
}
