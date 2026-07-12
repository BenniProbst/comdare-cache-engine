#pragma once
// h2_score_akte.hpp — GO-5 Fork 7 (2026-07-12, Dossier 20260712-go5 A.7, Thesis-Hypothese H2
// kapitel/de/06_evaluation_methodology.tex:56-57 „Der vom Original-Quell-Repository geerbte
// Code-Qualitäts-Score korreliert messbar mit dem erreichten Durchsatz").
//
// Die EINE Single-Source fuer den maschinenlesbaren, TOOL-BERECHNETEN H2-Code-Qualitaets-Score:
//   • Score-Formel (Gewichte + Dichte pro kLOC) — geteilt zwischen Generator (apps/h2_score_akte_tool)
//     und Konsument (profile_run_entry → CSV-Endspalte h2_code_quality_score).
//   • Akten-Format (sota_h2_scores.xml, NEUE Datei neben den sota/*.profile.xml — die bestehenden
//     Profile bleiben byte-unberuehrt): je sota/*.profile.xml EIN <code_quality>-Eintrag
//     (dossier-treues Element `<code_quality method= score= tool= computed=/>`, A.7 Option (b)).
//   • parse + honest-n/a-Resolver (fehlende Akte/fehlender Eintrag ⇒ "n/a", NIE ein 0-Phantom).
//
// ANTI-FAKE-DOKTRIN (exakt das #25-Akten-Muster von compute_dataset_akte, „kein Hand-Hash"):
//   computed="tool" ist der einzige legitime Wert — der Score wird IMMER von cppcheck (CI-Bestand,
//   Ledger `:480`) ueber die vendorten ext/-Paper-Quellen berechnet; Toolname+Version+Args+Gewichte
//   stehen IN der Akte (FF4-Reproduzierbarkeit). Papers OHNE vendorte Original-Quelle
//   (Rekonstruktionen, z.B. vampir is_original=false) tragen score="n/a" + reason — ehrlich,
//   kein Pseudo-Wert (Dossier A.7 Ehrlichkeits-Regel).
//
// SCORE-SEMANTIK: gewichtete cppcheck-Befunddichte pro kLOC ueber den vendorten Original-Snapshot
// (hoeher = mehr Befunde = niedrigere geerbte Code-Qualitaet). Die H2-KORRELATION selbst ist
// DATEN-gated (#156-Durchsaetze) — diese Akte macht sie im Auswertungs-Schritt berechenbar.
//
// ⚠️ header-only, C++23; XML via den self-contained Reader (KF-1, kein tinyxml2).

#include "xml_config_parser/xml_reader.hpp" // comdare::common::xml::parse_document / XmlNode

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace comdare::cache_engine::thesis_lazy {

namespace cxml = ::comdare::common::xml;

// ── Die GEWICHTE der cppcheck-Severity-Klassen (Dossier A.7: „Warnungs-Klassen gewichtet"). ──
// Single-Source: Generator UND Akte (weights-Attribut) UND Tests speisen sich hier. Konvention:
// error dominiert (3x), warning 2x, performance/portability je 1x, style 0.5x — dokumentiert,
// deterministisch, in der Akte selbst festgeschrieben (aendern ⇒ Akte neu generieren, nie Hand-Edit).
inline constexpr double kH2WeightError       = 3.0;
inline constexpr double kH2WeightWarning     = 2.0;
inline constexpr double kH2WeightPerformance = 1.0;
inline constexpr double kH2WeightPortability = 1.0;
inline constexpr double kH2WeightStyle       = 0.5;

/// Die Gewichts-Signatur, wie sie WOERTLICH im weights-Attribut der Akte steht (Single-Source-Abgleich).
[[nodiscard]] inline std::string h2_weights_signature() {
    auto one = [](char const* name, double w) {
        char buf[48];
        // %g: "3" statt "3.000000" — kompakt + eindeutig fuer die hier verwendeten Konstanten.
        int const n = std::snprintf(buf, sizeof(buf), "%s=%g", name, w);
        return std::string(buf, (n > 0) ? static_cast<std::size_t>(n) : 0);
    };
    return one("error", kH2WeightError) + ";" + one("warning", kH2WeightWarning) + ";" +
           one("performance", kH2WeightPerformance) + ";" + one("portability", kH2WeightPortability) + ";" +
           one("style", kH2WeightStyle);
}

// ── Die Befund-Zaehler EINER SOTA-Quelle (cppcheck-Severity-Klassen des gefahrenen Enable-Sets). ──
struct H2FindingCounts {
    std::uint64_t error       = 0;
    std::uint64_t warning     = 0;
    std::uint64_t style       = 0;
    std::uint64_t performance = 0;
    std::uint64_t portability = 0;
};

/// compute_h2_score — DIE Score-Formel: gewichtete Befunde pro kLOC, "%.3f"-formatiert.
/// loc==0 ⇒ "n/a" (ehrlich, kein Div-0-Phantom; eine leere Quelle traegt keinen Score).
[[nodiscard]] inline std::string compute_h2_score(H2FindingCounts const& f, std::uint64_t loc) {
    if (loc == 0) return "n/a";
    double const weighted =
        kH2WeightError * static_cast<double>(f.error) + kH2WeightWarning * static_cast<double>(f.warning) +
        kH2WeightPerformance * static_cast<double>(f.performance) +
        kH2WeightPortability * static_cast<double>(f.portability) + kH2WeightStyle * static_cast<double>(f.style);
    double const per_kloc = weighted / (static_cast<double>(loc) / 1000.0);
    char         buf[64];
    int const    n = std::snprintf(buf, sizeof(buf), "%.3f", per_kloc);
    return std::string(buf, (n > 0) ? static_cast<std::size_t>(n) : 0);
}

// ── EIN Akten-Eintrag = EIN sota/*.profile.xml (Dossier A.7: „je sota/*.profile.xml"). ──
struct H2CodeQualityEntry {
    std::string     profile_id; // Dateistamm des sota-Profils (== <sota_series lebewesen=..> fuer die 6 SOTA)
    std::string     paper_ref;  // paper_ref-Attribut des Profils (P01..P33)
    std::string     source;     // vendorte Original-Quelle relativ zur ce-Wurzel (leer bei n/a)
    std::string     method;     // "cppcheck_weighted_finding_density_per_kloc"
    std::string     computed;   // IMMER "tool" (Anti-Fake: es gibt keinen Hand-Wert-Pfad)
    std::string     score;      // "%.3f"-Dichte ODER "n/a" (Rekonstruktion ohne vendorte Quelle)
    std::string     reason;     // nur bei score=="n/a": z.B. "no_vendored_original_source"
    H2FindingCounts findings{};
    std::uint64_t   loc   = 0; // analysierte Zeilen (loc_metric der Akte, physical_lines)
    std::uint64_t   files = 0; // analysierte Dateien
};

// ── Die ganze Akte (Kopf = Tool-Provenienz, FF4; Eintraege in deterministischer profile_id-Reihenfolge). ──
struct H2ScoreAkte {
    int                             schema_version = 1;
    std::string                     tool;         // "cppcheck"
    std::string                     tool_version; // z.B. "2.21.0" (aus `cppcheck --version`)
    std::string                     tool_args;    // die gepinnten Analyse-Argumente WOERTLICH
    std::string                     loc_metric;   // "physical_lines"
    std::string                     weights;      // h2_weights_signature()
    std::string                     exclude_dirs; // beim Datei-Sammeln uebersprungene Verzeichnisnamen
    std::vector<H2CodeQualityEntry> entries;

    [[nodiscard]] H2CodeQualityEntry const* entry(std::string_view profile_id) const {
        for (auto const& e : entries)
            if (e.profile_id == profile_id) return &e;
        return nullptr;
    }
};

/// serialize_h2_score_akte — deterministische XML-Form der Akte (Generator-Output; byte-stabil bei
/// identischen Quellen+Tool — bewusst OHNE Zeitstempel, Reproduzierbarkeit vor Protokollierung).
[[nodiscard]] inline std::string serialize_h2_score_akte(H2ScoreAkte const& a) {
    std::ostringstream os;
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    os << "<!-- AUTO-GENERATED by apps/h2_score_akte_tool (GO-5 Fork 7) - DO NOT HAND-EDIT.\n"
          "     TOOL-BERECHNETE H2-Code-Qualitaets-Akte (Dossier 20260712-go5 A.7 Option (b)):\n"
          "     gewichtete cppcheck-Befunddichte pro kLOC ueber die vendorten ext/-Paper-Quellen.\n"
          "     score=\"n/a\" = Rekonstruktion ohne vendorte Original-Quelle (ehrlich, kein Pseudo-Wert).\n"
          "     Regenerieren: comdare-h2-score-akte --ce-root <ce> [--cppcheck <exe>] -->\n";
    os << "<comdare_h2_quality_akte schema_version=\"" << a.schema_version << "\" tool=\"" << a.tool
       << "\" tool_version=\"" << a.tool_version << "\"\n    tool_args=\"" << a.tool_args << "\"\n    loc_metric=\""
       << a.loc_metric << "\" weights=\"" << a.weights << "\" exclude_dirs=\"" << a.exclude_dirs << "\">\n";
    for (auto const& e : a.entries) {
        os << "  <code_quality profile=\"" << e.profile_id << "\" paper_ref=\"" << e.paper_ref << "\" method=\""
           << e.method << "\" computed=\"" << e.computed << "\"\n      score=\"" << e.score << "\"";
        if (e.score == "n/a") {
            os << " reason=\"" << e.reason << "\"/>\n";
        } else {
            os << " source=\"" << e.source << "\" loc=\"" << e.loc << "\" files=\"" << e.files
               << "\"\n      findings_error=\"" << e.findings.error << "\" findings_warning=\"" << e.findings.warning
               << "\" findings_style=\"" << e.findings.style << "\" findings_performance=\"" << e.findings.performance
               << "\" findings_portability=\"" << e.findings.portability << "\"/>\n";
        }
    }
    os << "</comdare_h2_quality_akte>\n";
    return os.str();
}

namespace h2_detail {
[[nodiscard]] inline std::uint64_t attr_u64(cxml::XmlNode const& n, std::string_view key) noexcept {
    try {
        std::string const v = n.attr(key);
        return v.empty() ? 0u : static_cast<std::uint64_t>(std::stoull(v));
    } catch (...) { return 0u; }
}
} // namespace h2_detail

/// parse_h2_score_akte — liest die Akte rein-lesend. nullopt bei fehlender Datei/falschem Wurzel-Tag
/// (der Konsument macht daraus honest-n/a, KEIN Abbruch — die Akte ist Metadaten, kein Mess-Gate).
[[nodiscard]] inline std::optional<H2ScoreAkte> parse_h2_score_akte(std::filesystem::path const& path) {
    std::ifstream in{path, std::ios::binary};
    if (!in) return std::nullopt;
    std::ostringstream ss;
    ss << in.rdbuf();
    auto root = cxml::parse_document(ss.str());
    if (!root.has_value() || root->tag != "comdare_h2_quality_akte") return std::nullopt;

    H2ScoreAkte a;
    try {
        a.schema_version = std::stoi(root->attr("schema_version", "1"));
    } catch (...) { a.schema_version = 1; }
    a.tool         = root->attr("tool");
    a.tool_version = root->attr("tool_version");
    a.tool_args    = root->attr("tool_args");
    a.loc_metric   = root->attr("loc_metric");
    a.weights      = root->attr("weights");
    a.exclude_dirs = root->attr("exclude_dirs");
    for (auto const* c : root->children_named("code_quality")) {
        H2CodeQualityEntry e;
        e.profile_id           = c->attr("profile");
        e.paper_ref            = c->attr("paper_ref");
        e.source               = c->attr("source");
        e.method               = c->attr("method");
        e.computed             = c->attr("computed");
        e.score                = c->attr("score");
        e.reason               = c->attr("reason");
        e.loc                  = h2_detail::attr_u64(*c, "loc");
        e.files                = h2_detail::attr_u64(*c, "files");
        e.findings.error       = h2_detail::attr_u64(*c, "findings_error");
        e.findings.warning     = h2_detail::attr_u64(*c, "findings_warning");
        e.findings.style       = h2_detail::attr_u64(*c, "findings_style");
        e.findings.performance = h2_detail::attr_u64(*c, "findings_performance");
        e.findings.portability = h2_detail::attr_u64(*c, "findings_portability");
        a.entries.push_back(std::move(e));
    }
    return a;
}

/// h2_score_for — der honest-n/a-Resolver des Treibers: Score eines SOTA-Lebewesens fuer die
/// CSV-Endspalte h2_code_quality_score. Fuer die 6 SOTA ist der Profil-Lebewesen-Name identisch
/// zum sota-Profil-Dateistamm (art/hot/masstree/surf/start/wormhole); prt_art (Prüfling, KEIN
/// Original-Quell-Repository) hat bewusst KEINEN Akten-Eintrag. Fehlende Akte, fehlender Eintrag
/// oder leerer Score ⇒ "n/a" — NIE 0, NIE ein erfundener Wert.
[[nodiscard]] inline std::string h2_score_for(std::optional<H2ScoreAkte> const& akte, std::string const& lebewesen) {
    if (!akte.has_value()) return "n/a";
    H2CodeQualityEntry const* e = akte->entry(lebewesen);
    if (e == nullptr || e->score.empty()) return "n/a";
    return e->score;
}

} // namespace comdare::cache_engine::thesis_lazy
