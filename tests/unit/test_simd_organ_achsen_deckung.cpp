// test_simd_organ_achsen_deckung -- K06/#44-Rest (B03): der NENNER der SIMD-Organ-Tabellen.
//
// BEFUND (BAULISTE 2026-08-21, am Objekt verifiziert): kSimdOrganRequirement und
// kSimdOrganSensibility fuehrten NEUN Organ-Klassen, davon "scoring" KEINE der 18
// kCompositionAxisNames; zehn echte Achsen fehlten (path_compression, node_type, allocator,
// concurrency, serialization, io_dispatch, migration_policy, queuing_q1, queuing_q2,
// persistence_target). Die INERTHEIT war ehrlich deklariert ("Default LEER -> Gate
// NotApplicable"), der NENNER war es nicht: required_of("node_type") war nicht "leer, weil
// nichts deklariert", sondern "leer, weil die Achse gar nicht gefuehrt wurde" -- zwei
// verschiedene Aussagen, die die Tabelle nicht unterscheiden konnte.
//
// WAS DIESER TEST ZUSICHERT (Wache am VERBRAUCH; die Header koennen kCompositionAxisNames aus
// Schicht-Gruenden nicht sehen -- measurement darf nicht auf builder zeigen):
//   (1) DECKUNG 18/18 + KEINE GEISTER, beide Tabellen: Position i traegt EXAKT
//       kCompositionAxisNames[i] (Positions-Pin macht Hin- und Rueckrichtung zu EINER Aussage:
//       jede Registry-Achse gedeckt UND kein Name ausserhalb der Registry).
//   (2) INERTHEIT UNVERAENDERT: any_organ_declares_required() == false (das Bau-Gate bleibt
//       NotApplicable/byte-neutral -- die Erweiterung ist ein Nenner-Fix, kein Verhaltens-Bau).
//   (3) LEERE MENGEN je neuer Achse: die 10 nachgezogenen Achsen tragen required LEER und
//       sensibility LEER (der ehrliche heutige Stand; spekulative Zuordnungen waeren neue
//       Behauptungen und gehoeren nicht in einen Nenner-Fix).
//   (4) "scoring" ist RAUS: kein Eintrag beider Tabellen traegt einen Namen ausserhalb der 18
//       (folgt aus (1)); die VNNI/BF16-Historie ist im Sensibility-Header dokumentiert.
//
// T-11c-MUTATIONSANKER: einen Tabellen-Namen mutieren (z.B. "node_type" -> "node_typ") bricht
// (1) literal im static_assert.

#include "experiment_tree/axis_path_serialization.hpp" // experiment::kCompositionAxisNames (18, Registry)

#include <cache_engine/measurement/simd_organ_requirement.hpp>
#include <cache_engine/measurement/simd_organ_sensibility.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <string_view>

namespace {

namespace meas = ::comdare::cache_engine::measurement;
namespace exp  = ::comdare::cache_engine::builder::experiment;

// (1) DECKUNG + KEINE GEISTER als EIN Positions-Pin je Tabelle, compile time:
static_assert(meas::kSimdOrganRequirement.size() == exp::kCompositionAxisNames.size(),
              "B03: kSimdOrganRequirement muss ALLE 18 Kompositions-Achsen fuehren -- ein kleinerer "
              "Nenner macht 'leer weil nichts deklariert' und 'leer weil nicht gefuehrt' ununterscheidbar.");
static_assert(meas::kSimdOrganSensibility.size() == exp::kCompositionAxisNames.size(),
              "B03: kSimdOrganSensibility muss ALLE 18 Kompositions-Achsen fuehren.");
static_assert(
    [] {
        for (std::size_t i = 0; i < exp::kCompositionAxisNames.size(); ++i)
            if (meas::kSimdOrganRequirement[i].organ_class != exp::kCompositionAxisNames[i]) return false;
        return true;
    }(),
    "B03: kSimdOrganRequirement[i].organ_class != kCompositionAxisNames[i] -- Deckungs-/Geister-/"
    "Reihenfolge-Drift (der Positions-Pin traegt alle drei Zusagen).");
static_assert(
    [] {
        for (std::size_t i = 0; i < exp::kCompositionAxisNames.size(); ++i)
            if (meas::kSimdOrganSensibility[i].organ_class != exp::kCompositionAxisNames[i]) return false;
        return true;
    }(),
    "B03: kSimdOrganSensibility[i].organ_class != kCompositionAxisNames[i] -- Deckungs-/Geister-/"
    "Reihenfolge-Drift.");

// (2) Das Gate bleibt inert -- dieselbe Zusicherung, die der Header selbst traegt, hier als
// Verbrauchs-Wache benannt (Nenner-Fix, kein Verhaltens-Bau):
static_assert(!meas::any_organ_declares_required(),
              "B03: die 18er-Ausrichtung darf KEINE required-Deklaration einfuehren -- das Gate muss "
              "NotApplicable/byte-neutral bleiben (Aktivierung ist ein eigener, deklarierter Bau).");

TEST(SimdOrganAchsenDeckung, ZehnNachgezogeneAchsenTragenLeereMengen) {
    // (3): die 10 am 2026-08-21 nachgezogenen Achsen, namentlich -- je required UND sensibility leer.
    constexpr std::string_view nachgezogen[] = {
        "path_compression", "node_type",        "allocator",  "concurrency", "serialization",
        "io_dispatch",      "migration_policy", "queuing_q1", "queuing_q2",  "persistence_target"};
    for (std::string_view achse : nachgezogen) {
        EXPECT_TRUE(meas::required_of(achse).empty()) << achse << ": required muss leer sein (Nenner-Fix)";
        EXPECT_TRUE(meas::sensibility_of(achse).empty())
            << achse << ": sensibility muss leer sein (keine spekulative Zuordnung erfinden)";
    }
    // Nenner der Schleife selbst: 10 benannte + 8 vorbestandene = 18 (kein stiller Rest).
    EXPECT_EQ(std::size(nachgezogen) + 8u, exp::kCompositionAxisNames.size());
}

TEST(SimdOrganAchsenDeckung, VorbestandeneZuordnungenSindUnveraendert) {
    // Byte-/Verhaltens-Neutralitaet der 8 vorbestandenen echten Achsen: dieselben Flag-Mengen wie
    // vor dem Umbau (Stichproben je Klasse; die Mengen selbst sind im Header static_assert-gedeckt).
    EXPECT_EQ(meas::sensibility_of("filter").size(), 3u);      // vpopcntdq/bitalg/gfni
    EXPECT_EQ(meas::sensibility_of("search_algo").size(), 4u); // bw/dq/f/vl
    EXPECT_EQ(meas::sensibility_of("prefetch").size(), 0u);    // ehrlich: KEIN SIMD-Flag
    EXPECT_TRUE(meas::required_of("filter").empty());
    EXPECT_TRUE(meas::required_of("search_algo").empty());
    EXPECT_TRUE(meas::required_of("nicht_existente_klasse").empty()) << "unbekannt bleibt leer (kein Wurf)";
}

} // namespace
