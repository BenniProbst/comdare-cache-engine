// test_begriffs_alias_registry -- M13-/I-7-Wache (#91-Vollzug W2-D, 2026-08-21): die
// Begriffs-Alias-Registry (include/cache_engine/naming/begriffs_alias_registry.hpp) gegen ihre
// FREMDEN Objekt-Quellen: den Baustein-Tag (baustein_variants.hpp), die Tooling-Registry-ids
// (mess_axes) und die Merge-Modi-Tokens (merge_plan.hpp kExperimentAxisMergeModes). Die
// Kreuz-Wachen fahren BEWUSST hier statt als Include im Querschnitt (Pin-Doktrin).
//
// GEGENEINGANG (T-4): unbekannte Begriffe und fremde Fachgebiete liefern nullptr -- ein
// Laufzeit-Konsument darf NIE still uebersetzen (R-2: Uebersetzen ist consteval-only).

#include <cache_engine/naming/begriffs_alias_registry.hpp>

#include "merge_plan.hpp"                             // kExperimentAxisMergeModes {replace, merge, union}
#include <cache_engine/abi/baustein_variants.hpp>     // NodeSparseNode4Art::tag == "SPARSE_NODE4_ART"
#include <mess_axes/measurement_tooling_registry.hpp> // wallclock/macro/micro (Kanon-Quelle)

#include <gtest/gtest.h>

#include <cstddef>
#include <string_view>

namespace nm = comdare::cache_engine::naming;
namespace ms = comdare::cache_engine::measurement;

// -- Nenner fremd: ACHT Eintraege (design91-v2 M13: node4 + w/ma/mi-Gruppe [w, compare, ma, mi] +
// Verbund-Uebergang [Phase, Achsen-Token] + E-10 disk_io/festplatten_io) -- 1 + 4 + 2 + 1 = 8 als
// eigenes Literal.
TEST(BegriffsAliasRegistry, AchtEintraege) {
    ASSERT_EQ(nm::kBegriffsAliasCount, static_cast<std::size_t>(1 + 4 + 2 + 1));
}

// -- E-10/ORG-19 (Schritt 1f): "Festplatten IO" (Owner-Wort) ist LESE-Form von disk_io -- nie
// Schreibform (KON120-02 D-07). kanon_of ist consteval: der Rueckweg bricht compile-time, nie still.
TEST(BegriffsAliasRegistry, DiskIoTraegtDenFestplattenIoAlias) {
    auto const* z = nm::begriffs_alias_zeile("organ_meta_meta_achse", "festplatten_io");
    ASSERT_NE(z, nullptr);
    EXPECT_EQ(z->kanon, std::string_view{"disk_io"});
    EXPECT_EQ(nm::kanon_of("organ_meta_meta_achse", "festplatten_io"), std::string_view{"disk_io"});
}

// -- node4 gegen den GEBAUTEN Baustein-Tag (keine Abschrift: der Vergleich laeuft gegen das
// Objekt-Symbol NodeSparseNode4Art::tag, nicht gegen ein zweites Literal).
TEST(BegriffsAliasRegistry, Node4KanonIstDerBausteinTag) {
    auto const* z = nm::begriffs_alias_zeile("organ_baustein", "node4");
    ASSERT_NE(z, nullptr);
    EXPECT_EQ(z->kanon, std::string_view{comdare::cache_engine::baustein::NodeSparseNode4Art::tag});
    EXPECT_EQ(z->art, nm::kArtAlias);
}

// -- w/ma/mi gegen die Tooling-Registry: jeder mess_ebene-KANON der Registry ist eine id der
// Mess-Tooling-Achse (FREMDE Quelle; Uebersetzung der Kuerzel landet exakt auf den ids).
TEST(BegriffsAliasRegistry, MessEbenenKanonsSindDieToolingIds) {
    constexpr std::string_view kuerzel[] = {"w", "ma", "mi"};
    for (std::size_t i = 0; i < 3; ++i) {
        auto const* z = nm::begriffs_alias_zeile("mess_ebene", kuerzel[i]);
        ASSERT_NE(z, nullptr) << "Kuerzel fehlt: " << kuerzel[i];
        EXPECT_EQ(z->kanon, ms::kMeasurementToolingRegistry[i].id)
            << "Kuerzel " << kuerzel[i] << " muss auf die Registry-id der Position " << i << " uebersetzen.";
    }
    // Die Ebene-0-Dualitaet (DESIGN-90 3.3): compare ist der zweite Name der wallclock-Ebene.
    auto const* comp = nm::begriffs_alias_zeile("mess_ebene", "compare");
    ASSERT_NE(comp, nullptr);
    EXPECT_EQ(comp->kanon, ms::kMeasurementToolingRegistry[0].id);
}

// -- Verbund-Uebergang gegen die GEBAUTE Merge-Naht: der Kanon "union" steht in
// kExperimentAxisMergeModes, der Alt-Name "fulljoin" steht dort NICHT (V-11R vollzogen).
TEST(BegriffsAliasRegistry, VerbundUebergangDecktDieMergeModi) {
    namespace tl  = comdare::cache_engine::thesis_lazy;
    auto in_modes = [](std::string_view t) {
        for (auto const* m : tl::kExperimentAxisMergeModes)
            if (std::string_view{m} == t) return true;
        return false;
    };
    auto const* ax = nm::begriffs_alias_zeile("verbund_achsen_token", "fulljoin");
    ASSERT_NE(ax, nullptr);
    EXPECT_EQ(ax->art, nm::kArtUebergang);
    EXPECT_TRUE(in_modes(ax->kanon)) << "Kanon '" << ax->kanon << "' muss ein gebauter Merge-Modus sein.";
    EXPECT_FALSE(in_modes(ax->alias)) << "Alt-Token 'fulljoin' darf KEIN gebauter Merge-Modus mehr sein.";
    auto const* ph = nm::begriffs_alias_zeile("verbund_phase", "Stufe1_CeOnly");
    ASSERT_NE(ph, nullptr);
    EXPECT_EQ(ph->kanon, std::string_view{"Verbund1_CeOnly"});
    EXPECT_EQ(ph->art, nm::kArtUebergang);
    EXPECT_NE(ph->quelle.find("V-11R"), std::string_view::npos) << "Uebergaenge tragen die Owner-Quelle V-11R.";
}

// -- I-7-Invarianten am Bestand: jede Zeile vollstaendig, kanon != alias, jeder Uebergang traegt
// ein Owner-Wort in der Quelle (Alias VOR Rename; Renames nur owner-gesetzt).
TEST(BegriffsAliasRegistry, I7InvariantenJederZeile) {
    for (auto const& z : nm::kBegriffsAliasRegistry) {
        EXPECT_FALSE(z.kanon.empty());
        EXPECT_FALSE(z.alias.empty());
        EXPECT_NE(z.kanon, z.alias);
        EXPECT_TRUE(z.art == nm::kArtAlias || z.art == nm::kArtUebergang);
        if (z.art == nm::kArtUebergang) {
            EXPECT_NE(z.quelle.find("V-11R"), std::string_view::npos)
                << "Uebergang ohne Owner-Quelle: " << z.alias << " -> " << z.kanon;
        }
    }
}

// -- Gegeneingang (T-4): unbekannter Begriff und fremdes Fachgebiet liefern nullptr; das
// work_mode-Fachgebiet ist NICHT registriert (compare uebersetzt nur als mess_ebene).
TEST(BegriffsAliasRegistry, GegeneingangUnbekanntesUndFremdesFach) {
    EXPECT_EQ(nm::begriffs_alias_zeile("mess_ebene", "profiler"), nullptr);
    EXPECT_EQ(nm::begriffs_alias_zeile("work_mode", "compare"), nullptr);
    EXPECT_EQ(nm::begriffs_alias_zeile("", "ma"), nullptr);
    EXPECT_EQ(nm::begriffs_alias_zeile("mess_ebene", ""), nullptr);
}
