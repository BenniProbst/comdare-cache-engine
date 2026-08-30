// test_stempel2_vertragspaare -- STEMPEL TEIL 2 (B-7/RN-78 Emitter-Haelfte, WEICHE A): die C2-VERTRAGSPAARE
// JE GATTUNG ueber den ECHTEN dlopen-Weg (H-23 L3 woertlich: "je Gattung ein Vertrags-Testpaar: emittierte
// Form kompiliert+laedt / stempellos faellt mit 13").
//
// DIE 13 FIXTURES stammen aus comdare-modul-emitter (apps/modul_emitter) -- also aus der L4-Andock-Flaeche
// selbst, nicht aus Hand-Quellen -- und wurden von cmake/modul_emitter.cmake als SHARED-Module gebaut:
//   SearchAlgorithm / Set / Sequence / View / Adapter: je {gestempelt, stempellos}
//   Hybrid: {Ziel SearchAlgorithm, Ziel Set} gestempelt + {stempellos}
// Positiv-Seite: derselbe gattungs-agnostische AnatomyModuleLoader laedt das Erzeugnis mit status_ok, die
// Handle traegt die Stempel-Zeilen (7. Pflicht-Symbol), die Organ-Zeile IST die vom Emitter gespeiste
// (Registry-name()/algo_version bzw. reroute_ziel), und die Instanz meldet das erwartete Genus (Hybrid: das
// ZIEL-Genus, Weg C). Negativ-Seite: das stempellose Erzeugnis desselben Emitters faellt EXAKT auf status 13
// (version_lines_symbol_missing) -- Praezedenz test_q2_identitaets_riegel (Fixture perm_a11_ohne_stempel).
//
// DRIFT-WACHE: die Fixture-Liste lebt DREIMAL (Werkzeug, cmake/modul_emitter.cmake, diese TU). manifest.txt des
// Werkzeugs wird gegen die einkompilierte Liste gehalten -- ein Eintrag mehr oder weniger faellt hier auf.
//
// @doku ~/backups-workflow/20260826-stempel-teil2/STAND.md + H-23 TEIL B L3 + FINAL-stempel-teil2 Abschnitt 4

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>

#include <anatomy/anatomy_base.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <string_view>

namespace loader = ::comdare::cache_engine::builder::anatomy_loader;
namespace ana    = ::comdare::cache_engine::anatomy;

namespace {

struct Fixture {
    char const*       name = nullptr; ///< Fixture-Name (== Unterverzeichnis des Werkzeugs, == manifest.txt Spalte 1)
    char const*       pfad = nullptr; ///< $<TARGET_FILE:...> des gebauten SHARED-Moduls
    ana::AnatomyGenus genus;          ///< erwartetes Genus der Instanz (Hybrid: das ZIEL-Genus, Weg C)
    bool              gestempelt = false;   ///< Positiv-/Negativ-Seite des Paares
    char const*       organ_soll = nullptr; ///< ein Token, das die Organ-Zeile tragen MUSS (gespeist, nicht erfunden)
    std::size_t       organ_eintraege = 0;  ///< Zahl der Haupt-Achsen-Segmente der Organ-Zeile (Blindstellen-Wache)
};

constexpr Fixture kFixtures[] = {
    {"sa_gestempelt", COMDARE_STEMPEL2_SA_GESTEMPELT, ana::AnatomyGenus::SearchAlgorithm, true, "search_algo=array256@",
     18},
    {"sa_stempellos", COMDARE_STEMPEL2_SA_STEMPELLOS, ana::AnatomyGenus::SearchAlgorithm, false, "", 0},
    {"set_gestempelt", COMDARE_STEMPEL2_SET_GESTEMPELT, ana::AnatomyGenus::Set, true, "search_algo=array256@", 13},
    {"set_stempellos", COMDARE_STEMPEL2_SET_STEMPELLOS, ana::AnatomyGenus::Set, false, "", 0},
    {"sequence_gestempelt", COMDARE_STEMPEL2_SEQUENCE_GESTEMPELT, ana::AnatomyGenus::Sequence, true,
     "growth_policy=doubling_growth@1.0.0.c", 9},
    {"sequence_stempellos", COMDARE_STEMPEL2_SEQUENCE_STEMPELLOS, ana::AnatomyGenus::Sequence, false, "", 0},
    {"view_gestempelt", COMDARE_STEMPEL2_VIEW_GESTEMPELT, ana::AnatomyGenus::View, true,
     "extent_policy=dynamic_extent@1.0.0.c;layout_policy=layout_right@1.0.0.c;accessor_policy=default_accessor@1.0.0.c",
     5},
    {"view_stempellos", COMDARE_STEMPEL2_VIEW_STEMPELLOS, ana::AnatomyGenus::View, false, "", 0},
    {"adapter_gestempelt", COMDARE_STEMPEL2_ADAPTER_GESTEMPELT, ana::AnatomyGenus::Adapter, true,
     "inner_container=deque_inner@1.0.0.c", 11},
    {"adapter_stempellos", COMDARE_STEMPEL2_ADAPTER_STEMPELLOS, ana::AnatomyGenus::Adapter, false, "", 0},
    {"hybrid_sa_gestempelt", COMDARE_STEMPEL2_HYBRID_SA_GESTEMPELT, ana::AnatomyGenus::SearchAlgorithm, true,
     "reroute_ziel=SearchAlgorithm@1.0.0.c", 1},
    {"hybrid_set_gestempelt", COMDARE_STEMPEL2_HYBRID_SET_GESTEMPELT, ana::AnatomyGenus::Set, true,
     "reroute_ziel=Set@1.0.0.c", 1},
    {"hybrid_stempellos", COMDARE_STEMPEL2_HYBRID_STEMPELLOS, ana::AnatomyGenus::SearchAlgorithm, false, "", 0},
};
constexpr std::size_t kFixtureZahl = sizeof(kFixtures) / sizeof(kFixtures[0]);
static_assert(kFixtureZahl == 13, "5 Gattungen x 2 + Hybrid 3 = 13 Vertragspaar-Fixtures");

std::size_t zaehle(std::string_view s, char c) {
    std::size_t n = 0;
    for (char const x : s)
        if (x == c) ++n;
    return n;
}

} // namespace

TEST(Stempel2Vertragspaare, ManifestDesWerkzeugsDecktDieEinkompilierteListe) {
    std::filesystem::path const manifest = std::filesystem::path{COMDARE_STEMPEL2_FIXTURE_DIR} / "manifest.txt";
    std::ifstream               in(manifest);
    ASSERT_TRUE(in.good()) << "manifest.txt fehlt: " << manifest;
    std::set<std::string> im_manifest;
    std::size_t           zeilen = 0;
    for (std::string zeile; std::getline(in, zeile);) {
        if (zeile.empty()) continue;
        ++zeilen;
        im_manifest.insert(zeile.substr(0, zeile.find('\t')));
        // Spalte 3 ist die Klasse; sie muss zur einkompilierten Erwartung passen.
        std::size_t const t1     = zeile.find('\t');
        std::size_t const t2     = zeile.find('\t', t1 + 1);
        std::size_t const t3     = zeile.find('\t', t2 + 1);
        std::string const name   = zeile.substr(0, t1);
        std::string const klasse = zeile.substr(t2 + 1, t3 - t2 - 1);
        for (Fixture const& f : kFixtures) {
            if (name == f.name) { EXPECT_EQ(klasse, f.gestempelt ? "gestempelt" : "stempellos") << name; }
        }
    }
    EXPECT_EQ(zeilen, kFixtureZahl) << "das Werkzeug emittiert exakt die Vertragspaar-Menge";
    std::set<std::string> erwartet;
    for (Fixture const& f : kFixtures) erwartet.insert(f.name);
    EXPECT_EQ(im_manifest, erwartet) << "Drift zwischen Werkzeug-Liste und Test-Liste";
}

TEST(Stempel2Vertragspaare, GestempelteErzeugnisseLadenMitStatusOkUndTragenDieGespeistenZeilen) {
    std::size_t geladen = 0;
    for (Fixture const& f : kFixtures) {
        if (!f.gestempelt) continue;
        loader::AnatomyModuleHandle h;
        int const                   st = loader::AnatomyModuleLoader::load(f.pfad, h);
        ASSERT_EQ(st, loader::status_ok) << f.name << ": " << loader::status_name(st) << " (" << f.pfad << ")";
        ASSERT_TRUE(h.valid()) << f.name;
        ASSERT_NE(h.anatomy(), nullptr) << f.name;
        EXPECT_EQ(h.anatomy()->genus(), f.genus) << f.name << " (Hybrid meldet das ZIEL-Genus, Weg C)";
        ASSERT_NE(h.version_lines(), nullptr) << f.name << ": ab status_ok garantiert non-null (A-11)";
        std::string_view const organ{h.version_lines()->organ_line,
                                     static_cast<std::size_t>(h.version_lines()->organ_len)};
        std::string_view const system{h.version_lines()->system_line,
                                      static_cast<std::size_t>(h.version_lines()->system_len)};
        EXPECT_NE(organ.find(f.organ_soll), std::string_view::npos)
            << f.name << ": Organ-Zeile traegt nicht das gespeiste Token '" << f.organ_soll << "' -- ist: " << organ;
        EXPECT_EQ(zaehle(organ, ';') + 1, f.organ_eintraege)
            << f.name
            << ": Zahl der Organ-Segmente != Makro-Aritaet der Gattung (Stempel-Blindstelle) -- ist: " << organ;
        EXPECT_NE(system.find("=code@"), std::string_view::npos) << f.name << ": System-Zeile ist: " << system;
        std::cout << "  POSITIV OK: " << f.name << " -> " << loader::status_name(st)
                  << " genus=" << ana::genus_name(h.anatomy()->genus()) << " organ=" << organ << "\n";
        ++geladen;
    }
    EXPECT_EQ(geladen, 7u) << "5 Gattungen + 2 Hybrid-Ziele";
}

TEST(Stempel2Vertragspaare, StempelloseErzeugnisseFallenExaktAufStatus13) {
    std::size_t abgewiesen = 0;
    for (Fixture const& f : kFixtures) {
        if (f.gestempelt) continue;
        loader::AnatomyModuleHandle h;
        int const                   st = loader::AnatomyModuleLoader::load(f.pfad, h);
        EXPECT_EQ(st, loader::status_version_lines_symbol_missing)
            << f.name << ": erwartet version_lines_symbol_missing (13), bekommen " << loader::status_name(st)
            << " -- die stempellose Emission desselben Emitters muss GENAU am siebten Pflicht-Symbol fallen";
        EXPECT_EQ(st, 13) << f.name << ": der Status ist literal 13 (A-11/golden-102)";
        EXPECT_FALSE(h.valid()) << f.name << ": abgelehntes Modul darf keine gueltige Handle tragen";
        std::cout << "  NEGATIV OK: " << f.name << " -> " << loader::status_name(st) << "\n";
        ++abgewiesen;
    }
    EXPECT_EQ(abgewiesen, 6u) << "5 Gattungen + Hybrid (als sechste Klasse einmal)";
}
