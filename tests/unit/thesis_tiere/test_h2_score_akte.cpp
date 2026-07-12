// test_h2_score_akte — GO-5 Fork 7 (2026-07-12, Dossier 20260712-go5 A.7, Thesis-Hypothese H2):
// ctest-Gate fuer die TOOL-BERECHNETE H2-Code-Qualitaets-Akte (sota_h2_scores.xml) + die additive
// CSV-Endspalte h2_code_quality_score.
//
// BEWEIST LITERAL:
//   (1) AKTE-PARSE + VOLLABDECKUNG: die committete Akte ist parsbar, traegt die Tool-Provenienz
//       (tool/tool_version/tool_args/weights, FF4) und hat GENAU EINEN <code_quality>-Eintrag je
//       sota/*.profile.xml (dossier-treu „je sota/*.profile.xml"; kein Phantom-, kein Fehl-Eintrag).
//   (2) TOOL-BERECHNET vs. HONEST-n/a: die vendorten Paper-Quellen tragen numerische Scores mit
//       computed="tool" + loc/files > 0; Rekonstruktionen ohne vendorte Quelle tragen score="n/a"
//       + reason (Dossier-A.7-Ehrlichkeits-Regel — NIE ein Hand-/Pseudo-Wert).
//   (3) HONEST-n/a-RESOLVER: fehlende Akte / fehlender Eintrag (prt_art = Prüfling OHNE
//       Original-Quell-Repository) ⇒ "n/a" — NIE 0, NIE ein erfundener Wert.
//   (4) FORMEL-SINGLE-SOURCE: compute_h2_score rechnet exakt die dokumentierten Gewichte pro kLOC;
//       die weights-Signatur der committeten Akte matcht die Code-Konstanten (kein Drift).
//   (5) CSV-EXPORT-SYMMETRIE: h2_code_quality_score ist die LETZTE Header-Spalte, Header==Row-
//       Spaltenzahl, Default "-", honest "n/a" wird durchgereicht; der Resume-Stamp bleibt
//       resume-v5 OHNE h2-Segment (Score = abgeleitete Lebewesen-Eigenschaft, KEIN
//       Lauf-Konfigurations-Freiheitsgrad — dokumentierte Entscheidung, kein Versehen).
//
// TABU-Wache: bestehende sota/*.profile.xml werden NUR GELESEN (byte-unberuehrt; die Akte ist
// eine NEUE Schwester-Datei). Der Generator selbst (apps/h2_score_akte_tool) laeuft hier NICHT
// (cppcheck-Lauf = teuer/tool-gebunden) — die committete Akte ist die Fixture; der Generator
// wird separat smoke-gefahren (Task-DoD: woertlicher Generator-Lauf).

#include "h2_score_akte.hpp" // parse_h2_score_akte / h2_score_for / compute_h2_score / h2_weights_signature

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // lazy_csv_header / format_csv_row /
                                                                     // LazyRunConfig / lazy_resume_stamp_prefix

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#ifndef COMDARE_SOTA_PROFILES_DIR
#error "COMDARE_SOTA_PROFILES_DIR must point to libs/cache_engine/algorithm_profiles/sota"
#endif

namespace {

namespace ex  = comdare::cache_engine::builder::experiment;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace fs  = std::filesystem;

fs::path sota_dir() { return fs::path{COMDARE_SOTA_PROFILES_DIR}; }
fs::path akte_path() { return sota_dir() / "sota_h2_scores.xml"; }

std::vector<std::string> sota_profile_ids() {
    std::vector<std::string> ids;
    for (auto const& de : fs::directory_iterator{sota_dir()}) {
        std::string const name = de.path().filename().string();
        if (de.is_regular_file() && name.size() > 12 && name.ends_with(".profile.xml"))
            ids.push_back(name.substr(0, name.size() - std::string{".profile.xml"}.size()));
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

bool is_numeric_score(std::string const& s) {
    if (s.empty() || s == "n/a") return false;
    bool dot = false;
    for (char const c : s) {
        if (c == '.') {
            if (dot) return false;
            dot = true;
        } else if (c < '0' || c > '9') {
            return false;
        }
    }
    return dot; // "%.3f"-Format traegt immer einen Punkt
}

std::size_t count_char(std::string const& s, char c) {
    return static_cast<std::size_t>(std::count(s.begin(), s.end(), c));
}

} // namespace

// (1) AKTE-PARSE + VOLLABDECKUNG — genau ein Eintrag je sota/*.profile.xml, Tool-Provenienz vorhanden.
TEST(H2ScoreAkte, ParseCommittedAkteCoversEverySotaProfile) {
    auto const akte = tlz::parse_h2_score_akte(akte_path());
    ASSERT_TRUE(akte.has_value()) << "committete Akte nicht lesbar: " << akte_path().string();

    EXPECT_EQ(akte->tool, "cppcheck") << "offizielles Tooling (Dossier A.7: cppcheck ist CI-Bestand)";
    EXPECT_FALSE(akte->tool_version.empty()) << "Toolversion MUSS in der Akte stehen (FF4-Reproduzierbarkeit)";
    EXPECT_FALSE(akte->tool_args.empty()) << "gepinnte Analyse-Argumente MUESSEN in der Akte stehen";
    EXPECT_EQ(akte->loc_metric, "physical_lines");
    EXPECT_EQ(akte->weights, tlz::h2_weights_signature())
        << "weights-Signatur der Akte muss die Code-Konstanten matchen (Single-Source, kein Drift)";

    std::vector<std::string> const ids = sota_profile_ids();
    ASSERT_FALSE(ids.empty());
    EXPECT_EQ(akte->entries.size(), ids.size()) << "GENAU ein <code_quality>-Eintrag je sota/*.profile.xml";
    for (auto const& id : ids) {
        tlz::H2CodeQualityEntry const* e = akte->entry(id);
        ASSERT_NE(e, nullptr) << "Akten-Eintrag fehlt fuer sota-Profil: " << id;
        EXPECT_EQ(e->computed, "tool") << id << ": es gibt KEINEN Hand-Wert-Pfad (Anti-Fake)";
        EXPECT_FALSE(e->method.empty()) << id;
        EXPECT_FALSE(e->paper_ref.empty()) << id;
    }
}

// (2) TOOL-BERECHNET vs. HONEST-n/a — vendorte Quellen numerisch, Rekonstruktionen n/a + reason.
TEST(H2ScoreAkte, VendoredSourcesScoredReconstructionsHonestNa) {
    auto const akte = tlz::parse_h2_score_akte(akte_path());
    ASSERT_TRUE(akte.has_value());

    // Die 6 SOTA-Lebewesen der Mess-Reihen (Profil-Name == sota-Profil-Dateistamm) + die uebrigen
    // vendorten Paper-Snapshots (ext/traversal/REPOS_OVERVIEW.md) tragen echte Tool-Scores.
    for (char const* id : {"art", "hot", "masstree", "surf", "start", "wormhole", "coco_trie", "b2tree",
                           "btreesareback", "mahling", "rcu", "hazard_pointers"}) {
        tlz::H2CodeQualityEntry const* e = akte->entry(id);
        ASSERT_NE(e, nullptr) << id;
        EXPECT_TRUE(is_numeric_score(e->score)) << id << ": vendorte Quelle MUSS tool-berechneten Score tragen, "
                                                << "ist aber '" << e->score << "'";
        EXPECT_GT(e->loc, 0u) << id;
        EXPECT_GT(e->files, 0u) << id;
        EXPECT_FALSE(e->source.empty()) << id;
        // Formel-Nachrechnung aus den in der Akte stehenden Roh-Zaehlern (Single-Source-Beweis).
        EXPECT_EQ(e->score, tlz::compute_h2_score(e->findings, e->loc)) << id;
    }

    // Rekonstruktionen ohne vendorte Original-Quelle: honest n/a + reason (NIE ein Pseudo-Wert).
    for (char const* id : {"vampir", "olc", "css_tree", "csb_tree", "graefe"}) {
        tlz::H2CodeQualityEntry const* e = akte->entry(id);
        ASSERT_NE(e, nullptr) << id;
        EXPECT_EQ(e->score, "n/a") << id;
        EXPECT_EQ(e->reason, "no_vendored_original_source") << id;
    }
}

// (3) HONEST-n/a-RESOLVER — fehlende Akte / fehlender Eintrag (prt_art) ⇒ "n/a", nie 0.
TEST(H2ScoreAkte, ResolverIsHonestOnMissingAkteAndMissingEntry) {
    // Fehlende Akte (Datei existiert nicht) ⇒ parse nullopt ⇒ Resolver "n/a".
    auto const missing = tlz::parse_h2_score_akte(sota_dir() / "does_not_exist_h2.xml");
    EXPECT_FALSE(missing.has_value());
    EXPECT_EQ(tlz::h2_score_for(missing, "art"), "n/a");

    // prt_art = Prüfling (KEIN Original-Quell-Repository, KEIN sota-Profil) ⇒ kein Eintrag ⇒ "n/a".
    auto const akte = tlz::parse_h2_score_akte(akte_path());
    ASSERT_TRUE(akte.has_value());
    EXPECT_EQ(akte->entry("prt_art"), nullptr) << "prt_art darf KEINEN Akten-Eintrag haben (Prüfling)";
    EXPECT_EQ(tlz::h2_score_for(akte, "prt_art"), "n/a");

    // Vorhandener Eintrag wird 1:1 durchgereicht (kein Umrechnen im Resolver).
    tlz::H2CodeQualityEntry const* art = akte->entry("art");
    ASSERT_NE(art, nullptr);
    EXPECT_EQ(tlz::h2_score_for(akte, "art"), art->score);
    EXPECT_TRUE(is_numeric_score(tlz::h2_score_for(akte, "art")));
}

// (4) FORMEL-SINGLE-SOURCE — dokumentierte Gewichte pro kLOC; loc==0 ⇒ "n/a" (kein Div-0-Phantom).
TEST(H2ScoreAkte, ScoreFormulaMatchesDocumentedWeights) {
    tlz::H2FindingCounts f;
    f.error       = 2; // x3.0 = 6.0
    f.warning     = 1; // x2.0 = 2.0
    f.performance = 1; // x1.0 = 1.0
    f.portability = 0; // x1.0 = 0.0
    f.style       = 4; // x0.5 = 2.0  → gewichtet 11.0; bei 2000 LOC = 11.0/2 kLOC = 5.500
    EXPECT_EQ(tlz::compute_h2_score(f, 2000u), "5.500");
    EXPECT_EQ(tlz::compute_h2_score(f, 0u), "n/a") << "leere Quelle traegt keinen Score (ehrlich)";
    EXPECT_EQ(tlz::compute_h2_score(tlz::H2FindingCounts{}, 1000u), "0.000")
        << "0 Befunde bei echter Quelle sind ein ECHTER Tool-Wert (0.000), kein n/a";
    EXPECT_EQ(tlz::h2_weights_signature(), "error=3;warning=2;performance=1;portability=1;style=0.5");
}

// (5) CSV-EXPORT-SYMMETRIE + STAMP-INVARIANZ — letzte Spalte, Header==Row, honest n/a, resume-v5 ohne h2.
TEST(H2ScoreAkte, CsvColumnSymmetryHonestNaAndStampInvariance) {
    std::string const header = ex::lazy_csv_header();
    ASSERT_GE(header.size(), 23u);
    EXPECT_EQ(header.substr(header.size() - 23), ";h2_code_quality_score\n");

    ex::LazyMeasuredRow row;
    row.binary_id         = "sota_tier=h2smoke";
    std::string const def = ex::format_csv_row(row);
    EXPECT_EQ(def.substr(def.size() - 3), ";-\n") << "Basis/Sweep-Default ist '-'";
    row.h2_score         = "n/a";
    std::string const na = ex::format_csv_row(row);
    EXPECT_EQ(na.substr(na.size() - 5), ";n/a\n") << "honest n/a wird 1:1 exportiert (kein 0-Phantom)";
    row.h2_score          = "5.500";
    std::string const num = ex::format_csv_row(row);
    EXPECT_EQ(num.substr(num.size() - 7), ";5.500\n");
    EXPECT_EQ(count_char(def, ';'), count_char(header, ';'));
    EXPECT_EQ(count_char(na, ';'), count_char(header, ';'));
    EXPECT_EQ(count_char(num, ';'), count_char(header, ';'));

    // Resume-Stamp bleibt resume-v5 OHNE h2-Segment: der Score ist abgeleitete Lebewesen-Metadaten
    // (dieselbe binary_id kann nie mit zwei Scores kollidieren); Schema-Drift faengt der Header-
    // Identitaets-Vergleich der per-Binary-CSV — alte Staende werden ehrlich neu gemessen.
    ex::LazyRunConfig cfg;
    cfg.row_h2_score        = "5.500";
    std::string const stamp = ex::lazy_resume_stamp_prefix(cfg, {});
    EXPECT_EQ(stamp.rfind("resume-v5|", 0), 0u) << stamp;
    EXPECT_EQ(stamp.find("h2="), std::string::npos) << stamp;
    ex::LazyRunConfig const cfg_default;
    EXPECT_EQ(stamp, ex::lazy_resume_stamp_prefix(cfg_default, {}))
        << "row_h2_score darf den Stamp NICHT beeinflussen (kein Lauf-Freiheitsgrad)";
}
