// test_ph89_paper_prueflinge -- P-H/#89 (Ledger-#44/PV-4, KON110-05 R-1..R-5, KON112-08..10):
// Paper->Prueflinge-Uebersetzungs-Registry (CT, Begriffs-Alias, KEIN Uebersetzer) + PV-4
// profile_ref-Dereferenzierung (R-4: Fehlziel = harter Planer-Fehler 'UNERFUELLBARES XML-ZIEL
// "ERROR"') + I-5 Stempel-"Farben" (Achsen-Tokens in der Organ-Zeile, KEINE Merge-Zeile) +
// M14-Ranking-Grammatik + Markierungs-/Ranking-Ausgabe + 33 Paper-Experiment-XMLs (F1a:
// ein Paper = genau ein Experiment-XML; Owner-GO 08.08.2026).
//
// NENNER-DOKTRIN (T-2, Nenner fremd): der 33er-Nenner kommt NICHT aus der Registry selbst,
// sondern aus dem XML-BESTAND libs/cache_engine/algorithm_profiles/sota/*.profile.xml
// (COMDARE_CE_ALGORITHM_PROFILES_DIR, CMake-seitig gereicht) -- Registry und Bestand werden
// GEGENEINANDER geprueft (beide Richtungen), kein Selbst-Beleg.

#include <gtest/gtest.h>

#include "profile_facade/paper_pruefling_registry.hpp"
#include "profile_facade/planner/markierung_ranking.hpp"
#include "profile_facade/pruefling_stempel_farben.hpp"
#include "profile_facade/validate_profile.hpp"

#include "comdare_test_tmp.hpp"
#include "xml_config_parser/xml_config_parser.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

namespace {

namespace tlz = ::comdare::cache_engine::thesis_lazy;
namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace cx  = ::comdare::builder::xml;
namespace fs  = std::filesystem;

[[nodiscard]] fs::path algorithm_profiles_dir() { return fs::path{COMDARE_CE_ALGORITHM_PROFILES_DIR}; }

// ---------------------------------------------------------------------------------------------
// T-A -- Uebersetzungs-Registry <-> XML-Bestand, BEIDE Richtungen (KON112-08 (a): alle 33 Paper
// werden FOERMLICH Prueflinge; L6: P08/P09/P33 abstrakt).
// ---------------------------------------------------------------------------------------------
TEST(Ph89PaperPruefling, RegistryDecktXmlBestandBeideRichtungen) {
    fs::path const sota = algorithm_profiles_dir() / "sota";
    ASSERT_TRUE(fs::exists(sota)) << sota;

    cx::XmlConfigParser const parser;
    std::set<std::string>     seen_paper_refs;
    std::size_t               profile_count = 0;
    for (auto const& e : fs::directory_iterator{sota}) {
        // NUR die Paper-Akten *.profile.xml (die Schwester-Akte sota_h2_scores.xml ist KEIN Profil).
        if (!e.path().filename().string().ends_with(".profile.xml")) continue;
        auto const prof = parser.parse_profile(e.path());
        ASSERT_FALSE(prof.id.empty()) << e.path();
        ++profile_count;
        // Richtung 1: jeder XML-Bestands-Eintrag ist in der Registry, mit identischer Identitaet.
        auto const* reg = tlz::find_paper(prof.paper_ref);
        ASSERT_NE(reg, nullptr) << "paper_ref fehlt in der Registry: " << prof.paper_ref;
        EXPECT_EQ(reg->profil_id, prof.id) << prof.paper_ref;
        std::string const xml_typ = prof.pruefling_type.empty() ? "full" : prof.pruefling_type;
        EXPECT_EQ(reg->pruefling_typ, xml_typ) << prof.paper_ref;
        seen_paper_refs.insert(prof.paper_ref);
    }
    // Nenner aus dem BESTAND (fremd zur Registry): 33 Paper-Profile.
    EXPECT_EQ(profile_count, std::size_t{33});
    // Richtung 2: jeder Registry-Eintrag existiert im Bestand.
    for (auto const& e : tlz::kPaperPrueflingRegistry)
        EXPECT_TRUE(seen_paper_refs.count(std::string{e.paper_ref})) << e.paper_ref;
    EXPECT_EQ(seen_paper_refs.size(), tlz::kPaperPrueflingRegistry.size());
    // L6-Anker: genau die 3 abstrakten Marker-Prueflinge.
    std::size_t abstrakt = 0;
    for (auto const& e : tlz::kPaperPrueflingRegistry)
        if (e.pruefling_typ == "abstract") ++abstrakt;
    EXPECT_EQ(abstrakt, std::size_t{3});
    EXPECT_EQ(tlz::find_paper("P08")->pruefling_typ, "abstract");
    EXPECT_EQ(tlz::find_paper("P09")->pruefling_typ, "abstract");
    EXPECT_EQ(tlz::find_paper("P33")->pruefling_typ, "abstract");
}

// ---------------------------------------------------------------------------------------------
// T-B -- Begriffs-Alias (R-2/KON112-09): deklarierte Gleichheit compile-time; KEIN Uebersetzer.
// Gegeneingang: NICHT deklarierte Paare sind NICHT dasselbe.
// ---------------------------------------------------------------------------------------------
TEST(Ph89PaperPruefling, BegriffsAliasDeklarierteGleichheit) {
    // Anwendungsfall (1): Ledger-#44/F1-Vokabular-Naht.
    static_assert(tlz::same_begriff("SPARSE_NODE4_ART", "node4"));
    static_assert(tlz::begriff_kanonisch("SPARSE_NODE4_ART") == "node4");
    // Anwendungsfall (2): KON112-01c compare/macro/micro == w/ma/mi.
    static_assert(tlz::same_begriff("compare", "w"));
    static_assert(tlz::same_begriff("macro", "ma"));
    static_assert(tlz::same_begriff("micro", "mi"));
    // Anwendungsfall (3): B-2/V-11R Stufe*->Verbund*.
    static_assert(tlz::same_begriff("Stufe1_CeOnly", "Verbund1_CeOnly"));
    static_assert(tlz::same_begriff("Stufe2_PrueflingReplace", "Verbund2_Replace"));
    static_assert(tlz::same_begriff("Stufe3_FullJoin", "Verbund3_Union"));
    // Identitaet gilt immer; Unbekanntes bleibt es selbst (kein Uebersetzer, keine Transformation).
    static_assert(tlz::same_begriff("irgendwas", "irgendwas"));
    static_assert(tlz::begriff_kanonisch("unbekannt") == "unbekannt");
    // GEGENEINGANG: nicht deklarierte Paare sind NICHT dasselbe (Registry uebersetzt nicht quer).
    static_assert(!tlz::same_begriff("node4", "w"));
    static_assert(!tlz::same_begriff("SPARSE_NODE4_ART", "Verbund1_CeOnly"));
    static_assert(!tlz::same_begriff("compare", "ma"));
    SUCCEED();
}

// ---------------------------------------------------------------------------------------------
// T-C -- I-5 Stempel-"Farben": die Pruefling-Achswerte erscheinen als ACHSEN-TOKENS in der
// ORGAN-Zeile ("achse=wert@X.Y.Z", compose_organ_stamp_line), KEINE Merge-Zeile. Die Versionen
// kommen aus der REALEN Versions-Tabelle (Registry-Wrapper, CT-Pflicht (e)) -- der 0.0.0-Sentinel
// ist VERBOTEN (er hiesse: der deklarierte Farb-Token existiert nicht als realer Achsen-Wrapper).
// ---------------------------------------------------------------------------------------------
TEST(Ph89PaperPruefling, FarbTokensErscheinenAlsAchsenTokensMitRealerVersion) {
    // IDENTITAETS-Form (Lager-Identitaet, REGISTRIERTE Population): alle 3 deklarierten
    // Farb-Tokens loesen mit REALER Version auf -- der 0.0.0-Sentinel ist VERBOTEN (er hiesse:
    // der Token benennt keinen registrierten Wrapper). vampir_nfp ist Default-OFF und beweist
    // damit genau die registered-statt-enabled-Entscheidung (Befund 21.08., Header-Doku).
    std::string const p08 = tlz::paper_organ_stempel_zeile("P08");
    EXPECT_NE(p08.find("concurrency=olc_optimistic@"), std::string::npos) << p08;
    EXPECT_EQ(p08.find("@0.0.0"), std::string::npos) << p08;

    std::string const p09 = tlz::paper_organ_stempel_zeile("P09");
    EXPECT_NE(p09.find("memory_layout=memory_layout_packed_bitmap@"), std::string::npos) << p09;
    EXPECT_EQ(p09.find("@0.0.0"), std::string::npos) << p09;

    std::string const p33 = tlz::paper_organ_stempel_zeile("P33");
    EXPECT_NE(p33.find("allocator=vampir_nfp@"), std::string::npos) << p33;
    EXPECT_EQ(p33.find("@0.0.0"), std::string::npos) << p33;

    // EMISSIONS-Form ((paper_ref, table)-Overload, Tabelle des KONKRETEN Baus): der immer
    // freigeschaltete P08-Marker loest auch hier real auf (dieselben geteilten Helfer).
    auto const        table   = ex::build_axis_variant_version_table();
    std::string const p08_bau = tlz::paper_organ_stempel_zeile("P08", table);
    EXPECT_NE(p08_bau.find("concurrency=olc_optimistic@"), std::string::npos) << p08_bau;
    EXPECT_EQ(p08_bau.find("@0.0.0"), std::string::npos) << p08_bau;

    // KEINE Merge-Zeile: die produzierten Stempel-Zeilen sind das 3-Zeilen-Trio (SotaStampLines);
    // eine vierte merge-Zeile existiert im Typ nicht (A13-M3/C3 Owner-E2 bleibt bindend).
    auto const lines = tlz::paper_stamp_lines("P08", table, "mess=combo");
    EXPECT_FALSE(lines.organ.empty());
    EXPECT_FALSE(lines.system.empty());
    EXPECT_EQ(lines.measurement, "mess=combo");

    // Volle Prueflinge: Farben = volle Komposition -> Organ-Zeile hier EHRLICH leer, solange der
    // SOTA-METADATEN-BLOCKER (K-3-REST) steht; KEINE zweite Zeilen-Ableitung in diesem Strang.
    EXPECT_TRUE(tlz::paper_organ_stempel_zeile("P01").empty());
    // Unbekanntes Paper: leer (kein Phantom-Token).
    EXPECT_TRUE(tlz::paper_organ_stempel_zeile("P99").empty());
}

// ---------------------------------------------------------------------------------------------
// T-D -- PV-4/#44: profile_ref-DEREFERENZIERUNG. Bestand (m3v2_study, 7 base_tiers) dereferenziert
// sauber; Fehlziel (fehlende Datei / P-Nummern-Mismatch) = harter Fehler mit dem Owner-Literal.
// ---------------------------------------------------------------------------------------------
TEST(Ph89PaperPruefling, ProfileRefDereferenzierungBestandGruen) {
    fs::path const profil = algorithm_profiles_dir() / "thesis_profiles" / "m3v2_study.profile.xml";
    ASSERT_TRUE(fs::exists(profil)) << profil;
    cx::XmlConfigParser const parser;
    auto const                tp = parser.parse_thesis_profile(profil);
    ASSERT_TRUE(tp.has_value());
    auto const r = tlz::dereference_base_tier_profile_refs(*tp, profil);
    EXPECT_TRUE(r.ok) << (r.errors.empty() ? std::string{} : r.errors.front());
    EXPECT_EQ(r.tiers_checked, tp->base_tiers.size());
    EXPECT_EQ(r.dereferenced, tp->base_tiers.size());
    EXPECT_TRUE(r.errors.empty());
}

TEST(Ph89PaperPruefling, ProfileRefFehlzielIstHarterFehlerMitOwnerLiteral) {
    fs::path const dir = comdare::test::user_tmp_dir() / "ph89_deref";
    fs::create_directories(dir);

    // (a) Fehlziel: Datei existiert nicht.
    {
        cx::ThesisProfile tp;
        tp.base_tiers.push_back(cx::ThesisTier{"geist", "../sota/gibt_es_nicht.profile.xml", "P01"});
        auto const r = tlz::dereference_base_tier_profile_refs(tp, dir / "fixture.profile.xml");
        EXPECT_FALSE(r.ok);
        ASSERT_EQ(r.errors.size(), std::size_t{1});
        EXPECT_NE(r.errors.front().find("UNERFUELLBARES XML-ZIEL \"ERROR\""), std::string::npos) << r.errors.front();
    }
    // (b) Fehlziel: Ziel ist kein comdare_algorithm_profile (id-Sentinel leer).
    {
        fs::path const bogus = dir / "kein_profil.xml";
        std::ofstream{bogus} << "<?xml version=\"1.0\"?><wurzel/>\n";
        cx::ThesisProfile tp;
        tp.base_tiers.push_back(cx::ThesisTier{"leer", "kein_profil.xml", "P01"});
        auto const r = tlz::dereference_base_tier_profile_refs(tp, dir / "fixture.profile.xml");
        EXPECT_FALSE(r.ok);
        ASSERT_EQ(r.errors.size(), std::size_t{1});
        EXPECT_NE(r.errors.front().find("UNERFUELLBARES XML-ZIEL \"ERROR\""), std::string::npos);
    }
    // (c) Fehlziel: P-Nummern-paper_ref des Tiers stimmt nicht mit dem Ziel-Profil ueberein.
    {
        fs::path const    echt = algorithm_profiles_dir() / "sota" / "art.profile.xml"; // paper_ref=P01
        cx::ThesisProfile tp;
        tp.base_tiers.push_back(cx::ThesisTier{"falsch", echt.string(), "P02"});
        auto const r = tlz::dereference_base_tier_profile_refs(tp, dir / "fixture.profile.xml");
        EXPECT_FALSE(r.ok);
        ASSERT_EQ(r.errors.size(), std::size_t{1});
        EXPECT_NE(r.errors.front().find("UNERFUELLBARES XML-ZIEL \"ERROR\""), std::string::npos);
    }
    // (d) Host-Referenz ausserhalb des P-Namensraums (PRT am prt_art-Tier, Bestand): KEIN Fehler.
    {
        fs::path const    echt = algorithm_profiles_dir() / "sota" / "art.profile.xml";
        cx::ThesisProfile tp;
        tp.base_tiers.push_back(cx::ThesisTier{"prt_art", echt.string(), "PRT"});
        auto const r = tlz::dereference_base_tier_profile_refs(tp, dir / "fixture.profile.xml");
        EXPECT_TRUE(r.ok);
        EXPECT_EQ(r.dereferenced, std::size_t{1});
    }
    // (e) Leerer profile_ref: kein deklariertes Ziel -> Warnung, kein harter Fehler.
    {
        cx::ThesisProfile tp;
        tp.base_tiers.push_back(cx::ThesisTier{"ohne_ref", "", "P05"});
        auto const r = tlz::dereference_base_tier_profile_refs(tp, dir / "fixture.profile.xml");
        EXPECT_TRUE(r.ok);
        EXPECT_EQ(r.dereferenced, std::size_t{0});
        EXPECT_FALSE(r.warnings.empty());
    }
}

// ---------------------------------------------------------------------------------------------
// T-E -- R-4 am Experiment-Kanal: <template ref> auf ein NICHT registriertes Paper-Template ist
// jetzt HART (die Registry existiert; der alte tolerant-Fallback war der #44/PV-4-Altstand U-8-(3)).
// ---------------------------------------------------------------------------------------------
TEST(Ph89PaperPruefling, ExperimentTemplateRefFehlzielHart) {
    cx::ExperimentProfile ep;
    ep.id = "t";
    ep.engines.push_back(cx::ExperimentEngine{"ee_ce", "CacheEngineExecutionEngineAdapter", "r.xml"});
    ep.engines.push_back(cx::ExperimentEngine{"ee_prt", "PrtArtExecutionEngineAdapter", "r2.xml"});
    ep.lebewesen.push_back("art");
    ep.op_types.push_back("OP-1");

    ep.templ.ref  = "P99"; // Fehlziel: kein registriertes Paper-Template
    ep.templ.mode = "full";
    auto const r1 = tlz::validate_experiment_profile(ep);
    EXPECT_FALSE(r1.ok);
    bool found = false;
    for (auto const& e : r1.errors)
        if (e.find("UNERFUELLBARES XML-ZIEL \"ERROR\"") != std::string::npos) found = true;
    EXPECT_TRUE(found);

    ep.templ.ref  = "P01"; // registriert -> kein Template-Fehler
    auto const r2 = tlz::validate_experiment_profile(ep);
    for (auto const& e : r2.errors) EXPECT_EQ(e.find("UNERFUELLBARES XML-ZIEL"), std::string::npos) << e;

    ep.templ.ref  = ""; // Abwesenheit bleibt byte-identisch tolerant (mode-getrieben)
    auto const r3 = tlz::validate_experiment_profile(ep);
    for (auto const& e : r3.errors) EXPECT_EQ(e.find("UNERFUELLBARES XML-ZIEL"), std::string::npos) << e;
}

// ---------------------------------------------------------------------------------------------
// T-F -- F1a/Owner-GO 08.08.: EIN Paper = GENAU EIN Experiment-XML. 33 Instanzen, jede parsebar,
// validierbar, template-ref == paper_ref des Registry-Eintrags des EINEN lebewesen-Tiers.
// ---------------------------------------------------------------------------------------------
TEST(Ph89PaperPruefling, PaperExperimentXmlJePaper) {
    fs::path const dir = algorithm_profiles_dir() / "paper_experiments";
    ASSERT_TRUE(fs::exists(dir)) << dir;

    cx::XmlConfigParser const parser;
    std::set<std::string>     refs;
    std::size_t               count = 0;
    for (auto const& e : fs::directory_iterator{dir}) {
        if (e.path().extension() != ".xml") continue;
        ++count;
        auto const ep = parser.parse_experiment_profile(e.path());
        ASSERT_TRUE(ep.has_value()) << e.path();
        ASSERT_EQ(ep->lebewesen.size(), std::size_t{1}) << e.path();
        auto const* reg = tlz::paper_by_profil_id(ep->lebewesen.front());
        ASSERT_NE(reg, nullptr) << e.path();
        EXPECT_EQ(ep->templ.ref, reg->paper_ref) << e.path();
        EXPECT_EQ(ep->templ.mode, "full") << e.path();
        // KEINE <phases>: Abwesenheit => KERN-A leitet die 3 Verbund-Stufen ab (F1c Voll-Auspermutation).
        EXPECT_TRUE(ep->phases.empty()) << e.path();
        auto const vr = tlz::validate_experiment_profile(*ep);
        EXPECT_TRUE(vr.ok) << e.path() << (vr.errors.empty() ? std::string{} : (": " + vr.errors.front()));
        refs.insert(ep->templ.ref);
    }
    EXPECT_EQ(count, std::size_t{33});
    EXPECT_EQ(refs.size(), std::size_t{33}); // genau EIN XML je Paper, keine Dopplung
}

// ---------------------------------------------------------------------------------------------
// T-G -- M14-Ranking-Grammatik + Markierungs-/Ranking-Ausgabe (R-1 Forscher-Workflow): wo stehen
// die MARKIERTEN Achsen je Parameter im Ranking. Deterministisch, ASCII-Zeilen-Grammatik.
// ---------------------------------------------------------------------------------------------
TEST(Ph89PaperPruefling, RankingGrammatikDeterministischUndMarkiert) {
    using namespace ::comdare::cache_engine::thesis_lazy::ranking;

    MarkierungsSatz mark;
    mark.achsen.push_back(MarkierteAchse{"concurrency", "olc_optimistic"});
    EXPECT_TRUE(ist_markiert(mark, "sota_tier=x/concurrency=olc_optimistic/prefetch=none"));
    EXPECT_FALSE(ist_markiert(mark, "sota_tier=x/concurrency=mutex/prefetch=none"));
    mark.kompositionen.push_back("sota::A::PrtArtComposition");
    EXPECT_TRUE(ist_markiert(mark, "sota::A::PrtArtComposition"));

    std::vector<RankingKandidat> kand;
    kand.push_back(RankingKandidat{"b/concurrency=mutex", 10.0});
    kand.push_back(RankingKandidat{"a/concurrency=olc_optimistic", 12.5});
    kand.push_back(RankingKandidat{"c/concurrency=spin", 12.5}); // Tie -> binary_id lexikalisch

    auto const oben = rank_parameter("THROUGHPUT", RankingRichtung::GroesserIstBesser, kand);
    ASSERT_EQ(oben.plaetze.size(), std::size_t{3});
    EXPECT_EQ(oben.plaetze[0].binary_id, "a/concurrency=olc_optimistic");
    EXPECT_EQ(oben.plaetze[1].binary_id, "c/concurrency=spin");
    EXPECT_EQ(oben.plaetze[2].binary_id, "b/concurrency=mutex");

    auto const unten = rank_parameter("LATENCY_P99", RankingRichtung::KleinerIstBesser, kand);
    EXPECT_EQ(unten.plaetze[0].binary_id, "b/concurrency=mutex");

    MarkierungsRankingBericht bericht;
    bericht.parameter.push_back(oben);
    bericht.parameter.push_back(unten);
    std::string const txt = render_markierungs_ranking(bericht, mark);
    EXPECT_NE(txt.find("RANKING parameter=THROUGHPUT richtung=groesser_ist_besser plaetze=3"), std::string::npos)
        << txt;
    EXPECT_NE(txt.find("PLATZ 1/3 parameter=THROUGHPUT binary=a/concurrency=olc_optimistic"), std::string::npos) << txt;
    EXPECT_NE(txt.find("markiert=ja"), std::string::npos) << txt;
    EXPECT_NE(txt.find("markiert=nein"), std::string::npos) << txt;
    // Die Planer-Schluss-Ausgabe: je markiertem Kandidaten je Parameter eine MARKIERUNG-Zeile.
    EXPECT_NE(txt.find("MARKIERUNG parameter=THROUGHPUT binary=a/concurrency=olc_optimistic platz=1/3"),
              std::string::npos)
        << txt;
    // Determinismus: zweite Renderung byte-identisch.
    EXPECT_EQ(txt, render_markierungs_ranking(bericht, mark));
}

} // namespace
