// test_kanal_id_schema -- I-2-Festschreibungs-Wache (#91-Vollzug W2-D, 2026-08-21): das
// Kanal-ID-Schema (include/cache_engine/measurement/kanal_id_schema.hpp) gegen seine FREMDEN
// Quellen -- die Mess-Tooling-Registry (Kanon-Tokens) und die gebaute Arena (MessEbene-Ordinale,
// deskriptor_ix-Typ). Genau diese Kreuz-Wachen fahren BEWUSST hier statt als Include im
// Querschnitt (Pin-Doktrin; Zielbild Design #29: kein Querschnitts-Kopf kennt Traeger-Inneres).
//
// GEGENEINGANG (T-4): Permutationen, Duplikate und fremde Tokens muessen UNGUELTIG sein --
// V-13: "alles andere ist syntaktisch falsch", keine stille Normalisierung.

#include <cache_engine/measurement/kanal_id_schema.hpp>

#include "measure_storage/mess_arena.hpp"             // MessEbene {Compare=0,Macro=1,Micro=2,Reserviert=3} + Zeile
#include <mess_axes/measurement_tooling_registry.hpp> // kMeasurementToolingRegistry (ids = Kanon-Quelle)

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace ms = comdare::cache_engine::measurement;
namespace ar = comdare::cache_engine::builder::measure_storage;

// -- Kanon == Registry-ids (FREMDE Quelle: mess_axes/measurement_tooling_registry.hpp; der
// Nenner 3 ist das Literal der Tooling-Achse, nicht kEbenenKanonZahl nachgeplappert).
TEST(KanalIdSchema, KanonDecktDieToolingRegistryIds) {
    ASSERT_EQ(ms::kEbenenKanonZahl, static_cast<std::size_t>(3));
    ASSERT_EQ(ms::kMeasurementToolingCount, static_cast<std::size_t>(3));
    for (std::size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(ms::kEbenenKanon[i], ms::kMeasurementToolingRegistry[i].id)
            << "Kanon-Position " << i << " weicht von der Tooling-Registry ab (V-13-Quelle).";
    }
}

// -- MessEbene-Ordinale der gebauten Arena decken die Schema-Ordinale: 0/1/2 IST w-vor-ma-vor-mi
// (DESIGN-90 3.2 Objekt-Beleg); Reserviert=3 bleibt reserviert (LESEFALLE HY-0).
TEST(KanalIdSchema, MessEbeneOrdinaleDeckenDieArena) {
    static_assert(static_cast<std::uint8_t>(ar::MessEbene::Compare) == ms::kMessEbeneOrdinalWallclock &&
                      static_cast<std::uint8_t>(ar::MessEbene::Macro) == ms::kMessEbeneOrdinalMacro &&
                      static_cast<std::uint8_t>(ar::MessEbene::Micro) == ms::kMessEbeneOrdinalMicro &&
                      static_cast<std::uint8_t>(ar::MessEbene::Reserviert) == ms::kMessEbeneOrdinalReserviert,
                  "mess_arena.hpp MessEbene und kanal_id_schema-Ordinale muessen dieselbe Ordnung tragen.");
    SUCCEED();
}

// -- Tag-/Deskriptor-Bindung: der Kanal reist als deskriptor_ix; Typgleichheit mit der gebauten
// 32-Byte-Zeile (kein zweiter Index-Typ neben der Arena).
TEST(KanalIdSchema, DeskriptorIxTypDecktDieMessZeile) {
    static_assert(std::is_same_v<ms::KanalDeskriptorIx, decltype(ar::MessCheckpointZeile{}.deskriptor_ix)>,
                  "KanalDeskriptorIx muss der deskriptor_ix-Typ der MessCheckpointZeile sein (R1-Bindung).");
    SUCCEED();
}

// -- Hierarchie (KON110-02): drei Stufen Achse -> Genus -> Kategorie.
TEST(KanalIdSchema, HierarchieDreiStufen) {
    ASSERT_EQ(ms::kKanalHierarchieTiefe, static_cast<std::size_t>(3));
    EXPECT_EQ(ms::kKanalHierarchie[0], std::string_view{"achse"});
    EXPECT_EQ(ms::kKanalHierarchie[1], std::string_view{"genus"});
    EXPECT_EQ(ms::kKanalHierarchie[2], std::string_view{"kategorie"});
}

// -- Der Kanon-Pruefer, positive Seite: der Kanon selbst und alle Teilfolgen sind gueltig
// (B-19-Subset-Form; die leere Liste ist die leere Teilmenge).
TEST(KanalIdSchema, PrueferAkzeptiertKanonUndTeilfolgen) {
    std::array<std::string_view, 3> const voll{"wallclock", "macro", "micro"};
    std::array<std::string_view, 2> const kopf{"wallclock", "macro"};
    std::array<std::string_view, 2> const klammer{"wallclock", "micro"};
    std::array<std::string_view, 1> const einzel{"macro"};
    EXPECT_TRUE(ms::ist_kanon_reihenfolge(voll));
    EXPECT_TRUE(ms::ist_kanon_reihenfolge(kopf));
    EXPECT_TRUE(ms::ist_kanon_reihenfolge(klammer));
    EXPECT_TRUE(ms::ist_kanon_reihenfolge(einzel));
    EXPECT_TRUE(ms::ist_kanon_reihenfolge(std::span<std::string_view const>{}));
}

// -- Gegeneingang (T-4): JEDE Permutation, jedes Duplikat, jedes fremde Token ist syntaktisch
// falsch (V-13 verbatim: "alles andere ist syntaktisch falsch") -- der Aufrufer wirft, der
// Pruefer normalisiert NIE still.
TEST(KanalIdSchema, GegeneingangPermutationDuplikatFremdtoken) {
    std::array<std::string_view, 3> const gedreht{"micro", "macro", "wallclock"};
    std::array<std::string_view, 2> const getauscht{"macro", "wallclock"};
    std::array<std::string_view, 2> const doppelt{"micro", "micro"};
    std::array<std::string_view, 1> const fremd{"profiler"};
    std::array<std::string_view, 2> const enumname{"Compare", "macro"}; // Enum-NAME ist kein Token
    EXPECT_FALSE(ms::ist_kanon_reihenfolge(gedreht));
    EXPECT_FALSE(ms::ist_kanon_reihenfolge(getauscht));
    EXPECT_FALSE(ms::ist_kanon_reihenfolge(doppelt));
    EXPECT_FALSE(ms::ist_kanon_reihenfolge(fremd));
    EXPECT_FALSE(ms::ist_kanon_reihenfolge(enumname));
    EXPECT_EQ(ms::ebenen_kanon_position("wallclock"), static_cast<std::size_t>(0));
    EXPECT_EQ(ms::ebenen_kanon_position("profiler"), ms::kEbenenKanonZahl); // unbekannt => Zahl
}
