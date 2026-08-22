// test_parameter_filter_registry_skelett -- M6-Skelett-Wache (#91-Vollzug W2-D, 2026-08-21):
// das Katalog-Skelett der Parameter-Filter-Registry (traeger/ceb/parameter_filter_registry.hpp,
// erste Kopf-Datei der eigenen Include-Wurzel <traeger/ceb/...>, Design #29 R5).
//
// Der Test ist zugleich der BAUWEG-Beweis der neuen Include-Wurzel: er inkludiert den Kopf
// AUSSCHLIESSLICH ueber das INTERFACE-Ziel comdare_ceb (LIBRARIES-Zeile in der CMakeLists) --
// keine PRIVATE-Include-Kruecke auf den traeger-Pfad.
//
// GEGENEINGANG (T-4): unbekannte Filter-Tokens liefern nullptr (nie still ein Default-Filter).

#include <traeger/ceb/parameter_filter_registry.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <string_view>

namespace tc = comdare::traeger::ceb;

// -- Nenner fremd: das Skelett traegt GENAU EINEN mechanisch entschiedenen Filter (M6-SOLL:
// "Filter 1 = kuerzeste Gesamtzeit"); die Fuellung ist #88 nach Trigger.
TEST(ParameterFilterRegistrySkelett, GenauEinEintragImSkelett) {
    ASSERT_EQ(tc::kParameterFilterCount, static_cast<std::size_t>(1));
}

// -- Filter 1 verbatim: Token, Rang 1, Zielgroesse nicht leer (Rueckverfolgung).
TEST(ParameterFilterRegistrySkelett, Filter1KuerzesteGesamtzeitAnRang1) {
    auto const* f = tc::parameter_filter_of("kuerzeste_gesamtzeit");
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->impact_rang, static_cast<std::size_t>(1));
    EXPECT_FALSE(f->zielgroesse.empty());
    EXPECT_EQ(&tc::kParameterFilterRegistry[0], f) << "Filter 1 steht an Katalog-Position 0 (impact-sortiert).";
}

// -- Impact-Sortierung als Vertrag: Rang == Index + 1, lueckenlos (bricht laut, sobald die
// #88-Fuellung unsortiert einreiht).
TEST(ParameterFilterRegistrySkelett, ImpactSortierungLueckenlos) {
    for (std::size_t i = 0; i < tc::kParameterFilterCount; ++i) {
        EXPECT_EQ(tc::kParameterFilterRegistry[i].impact_rang, i + 1);
    }
}

// -- Gegeneingang (T-4): unbekannte Tokens (auch der Falsch-Freund der ORGAN-Such-Filter-Achse)
// liefern nullptr.
TEST(ParameterFilterRegistrySkelett, GegeneingangUnbekannteTokens) {
    EXPECT_EQ(tc::parameter_filter_of("axis_filter"), nullptr); // ORGAN-Falsch-Freund, andere Sache
    EXPECT_EQ(tc::parameter_filter_of("gesamtzeit"), nullptr);
    EXPECT_EQ(tc::parameter_filter_of(""), nullptr);
}
