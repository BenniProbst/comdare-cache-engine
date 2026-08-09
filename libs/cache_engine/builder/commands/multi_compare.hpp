#pragma once
// V41.F.6.1 R6 (2026-05-29) — multi_compare: N Kompositionen gegen eine Baseline, FWER-kontrolliert
//
// @subsystem CEB (Mess-Auswertung)
// @phase_owner CEB
//
// CompareEngineCommand vergleicht GENAU ZWEI Engines (paarweise, Welch). Der eigentliche
// F15-Workflow vergleicht aber VIELE Achsen-Kompositionen gegen eine Baseline (z.B. std::map oder
// prt-art) — und braucht dann FWER-Kontrolle, sonst entstehen bei m Vergleichen Zufalls-
// "Signifikanzen". Diese Utility verbindet die drei Statistik-Bausteine zum kompletten Workflow:
//   1) welch_t_test pro Kandidat-vs-Baseline   (Effektgroesse + Roh-p)
//   2) holm_bonferroni_adjust ueber alle Roh-p  (FWER-Kontrolle, step-down)
//   3) Signifikanz-Markierung bei FWER alpha + Schneller-als-Baseline-Flag.
//
// Arbeitet direkt auf ExecutionResult (engine_name + latency_samples_ns) → maximal integriert.

#include "execution_result.hpp"
#include "welch_t_test.hpp"
#include "mann_whitney_u_test.hpp" // R5.D: robuster nicht-parametrischer Komplementaer-Test
#include "multiple_comparison.hpp"
#include "result_aggregator.hpp" // reuse detail::csv_quote + detail::json_escape (DRY)

#include <cstddef>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::commands::stats {

/// Ein Kandidat-vs-Baseline-Vergleich mit Roh- + FWER-korrigiertem p-Wert.
struct PairwiseComparison {
    std::string_view name{};                      ///< Kompositions-/Engine-Name des Kandidaten
    WelchResult      welch{};                     ///< Welch-Resultat (Kandidat = a, Baseline = b)
    double           raw_p{1.0};                  ///< unkorrigierter Welch-p-Wert (1.0 wenn ungueltig)
    double           adjusted_p{1.0};             ///< Holm-Bonferroni-korrigierter Welch-p-Wert
    bool             significant{false};          ///< adjusted_p <= alpha (parametrisch, Welch)
    bool             faster_than_baseline{false}; ///< mean_a < mean_b (kleinere Latenz)
    // R5.D — robuster nicht-parametrischer Komplementaer-Test (rang-basiert, ausreisser-robust):
    MannWhitneyResult mwu{};                           ///< Mann-Whitney-U (Kandidat = a, Baseline = b)
    double            robust_adjusted_p{1.0};          ///< Holm-korrigierter MWU-p-Wert
    bool              robust_significant{false};       ///< robust_adjusted_p <= alpha (Rang-Test)
    bool              significance_discrepancy{false}; ///< Welch- ↔ MWU-Signifikanz UNEINIG (Warnsignal)
    // D4c (2026-08-09) -- WER GEHOERT UEBERHAUPT IN DIE HYPOTHESEN-FAMILIE?
    //
    // proben_tot ist die DATEN-Ebene: Kandidat oder Baseline haben keine einzige Probe > 0. Das
    // faengt WEDER D4a NOCH D4b, weil es keine Eigenschaft des Tests ist: ein Kandidat aus lauter
    // Nullen gegen eine normal streuende Baseline hat se>0 (die Baseline traegt die Varianz),
    // laeuft glatt durch Welch, hat mean_a=0 -- also "unschlagbar schnell" -- und kommt mit einem
    // winzigen p-Wert und faster_than_baseline=true heraus. Genau so gewinnt eine praeparierte
    // Null-DLL das Ranking.
    //
    // getestet_welch / getestet_mwu sind GETRENNT, und das ist keine Doppelung: die beiden Tests
    // haben verschiedene Zustaendigkeiten. Zwei konstante Gruppen 929 und 1031 sind fuer Welch
    // nicht rechenbar (se=0) und fuer den Rang-Test sehr wohl. Jede Familie zaehlt deshalb ihre
    // EIGENEN Hypothesen -- eine gemeinsame Zahl waere fuer mindestens eine der beiden falsch.
    bool proben_tot{false};     ///< Daten-Ebene: Kandidat oder Baseline ohne verwertbare Probe
    bool getestet_welch{false}; ///< in die Welch-Familie eingegangen (zaehlt fuer m)
    bool getestet_mwu{false};   ///< in die MWU-Familie eingegangen (zaehlt fuer m)
};

/// Ergebnis eines Multi-Vergleichs gegen eine Baseline.
struct MultiCompareReport {
    std::vector<PairwiseComparison> comparisons{};
    std::size_t                     significant_count{0};
    double                          alpha{0.05};
};

/// Vergleicht jede Kandidaten-ExecutionResult gegen die Baseline (Welch ueber latency_samples_ns),
/// korrigiert alle p-Werte per Holm-Bonferroni und markiert Signifikanz bei FWER alpha.
/// Reihenfolge der comparisons == Reihenfolge der candidates.
[[nodiscard]] inline MultiCompareReport multi_compare_against_baseline(ExecutionResult const&           baseline,
                                                                       std::span<const ExecutionResult> candidates,
                                                                       double                           alpha = 0.05) {
    MultiCompareReport rep;
    rep.alpha = alpha;
    rep.comparisons.reserve(candidates.size());

    // D4c: NUR die getesteten Hypothesen gehen in die Korrektur. Die Indizes werden mitgefuehrt,
    // damit die Ergebnisse danach exakt auf die Original-Kandidaten zurueckgestreut werden koennen
    // (die Reihenfolge von rep.comparisons bleibt die Reihenfolge von candidates).
    std::vector<double>      raw_p_getestet, raw_p_mwu_getestet;
    std::vector<std::size_t> idx_welch, idx_mwu;

    std::span<const std::int64_t> const base_samples{baseline.latency_samples_ns};
    bool const                          baseline_tot = proben_sind_tot(base_samples);
    for (auto const& c : candidates) {
        PairwiseComparison pc;
        pc.name = c.engine_name;
        std::span<const std::int64_t> const cand_samples{c.latency_samples_ns};
        // Daten-Ebene ZUERST: sie entscheidet unabhaengig davon, was die Tests spaeter sagen.
        pc.proben_tot = baseline_tot || proben_sind_tot(cand_samples);
        pc.welch      = welch_t_test(cand_samples, base_samples);
        pc.mwu        = mann_whitney_u_test(cand_samples, base_samples); // robuster Rang-Test

        pc.getestet_welch = !pc.proben_tot && pc.welch.valid && !pc.welch.degeneriert;
        pc.getestet_mwu   = !pc.proben_tot && pc.mwu.valid && !pc.mwu.degeneriert;

        // raw_p bleibt der Platzhalter 1.0 fuer alles Ungetestete -- aber dieser Platzhalter geht
        // jetzt NICHT mehr in die Familie ein, er ist nur noch die Anzeige "kein Befund".
        pc.raw_p                = pc.getestet_welch ? pc.welch.p_value : 1.0;
        pc.faster_than_baseline = pc.getestet_welch && (pc.welch.mean_a < pc.welch.mean_b);

        if (pc.getestet_welch) {
            idx_welch.push_back(rep.comparisons.size());
            raw_p_getestet.push_back(pc.welch.p_value);
        }
        if (pc.getestet_mwu) {
            idx_mwu.push_back(rep.comparisons.size());
            raw_p_mwu_getestet.push_back(pc.mwu.p_value);
        }
        rep.comparisons.push_back(pc);
    }

    // Zwei GETRENNTE Familien mit i.A. verschiedenem m -- siehe die Begruendung an getestet_mwu.
    auto const adj     = holm_bonferroni_adjust(std::span<const double>{raw_p_getestet});
    auto const adj_mwu = holm_bonferroni_adjust(std::span<const double>{raw_p_mwu_getestet});
    for (std::size_t k = 0; k < idx_welch.size(); ++k) {
        auto& pc       = rep.comparisons[idx_welch[k]];
        pc.adjusted_p  = adj[k];
        pc.significant = (adj[k] <= alpha);
        if (pc.significant) ++rep.significant_count;
    }
    for (std::size_t k = 0; k < idx_mwu.size(); ++k) {
        auto& pc              = rep.comparisons[idx_mwu[k]];
        pc.robust_adjusted_p  = adj_mwu[k];
        pc.robust_significant = (adj_mwu[k] <= alpha);
    }
    // Ungetestete behalten ihre Defaults: adjusted_p = 1.0, significant = false. Das ist kein
    // "nicht signifikant gemessen", sondern "nicht gemessen" -- die Unterscheidung steht in
    // getestet_welch/getestet_mwu und wird von summarize() als eigene Kategorie gefuehrt.
    for (auto& pc : rep.comparisons) {
        // Diskrepanz: parametrischer und Rang-Test sind UNEINIG → Warnsignal (oft ausreisser-getrieben).
        // D4b: das gilt NUR, wenn BEIDE Tests ueberhaupt etwas gesagt haben. Ist einer degeneriert,
        // ist die "Uneinigkeit" ein Artefakt des Platzhalter-p von 1.0 und kein Ausreisser-Hinweis --
        // genau der Kandidat 929-vs-1031 loeste vorher eine Diskrepanz-Warnung aus, weil Welch dort
        // nicht rechnen kann und der Rang-Test schon.
        pc.significance_discrepancy = pc.getestet_welch && pc.getestet_mwu && (pc.significant != pc.robust_significant);
    }
    return rep;
}

// ─── F15-Headline-Metrik: quantitative Kernaussage des Multi-Vergleichs ───────────────────────

/// Aggregierte F15-Kennzahlen ueber einen MultiCompareReport.
struct MultiCompareSummary {
    std::size_t total{0};              ///< GRUNDGESAMTHEIT: Zahl der vorgelegten Kandidaten
    std::size_t significant_faster{0}; ///< signifikant UND schneller als Baseline (= CE bringt Wert)
    std::size_t significant_slower{0}; ///< signifikant ABER langsamer
    std::size_t not_significant{0};    ///< GETESTET, aber kein signifikanter Unterschied (FWER-korrigiert)
    double      win_rate{0.0};         ///< significant_faster / getestet -- die F15-Headline-Zahl
    std::size_t robust_significant{0}; ///< R5.D: signifikant auch im robusten Rang-Test (MWU)
    std::size_t discrepancies{0};      ///< R5.D: Welch ↔ MWU uneinig (ausreisser-Warnsignal)
    // D4c: der NENNER der Headline-Zahl, und er steht neben ihr statt hinter ihr.
    //
    // Die vier Kategorien significant_faster + significant_slower + not_significant + degeneriert
    // decken `total` VOLLSTAENDIG ab. Vorher wurden die degenerierten unter not_significant
    // gefuehrt -- damit war "kein Unterschied gemessen" von "nie gemessen" nicht zu trennen, und
    // die win_rate teilte durch eine Grundgesamtheit, in der nie gemessene Zellen mitzaehlten.
    std::size_t getestet{0};        ///< Nenner der win_rate: Kandidaten in der Welch-Familie
    std::size_t degeneriert{0};     ///< total - getestet (nicht testbar, KEIN Messbefund)
    std::size_t getestet_mwu{0};    ///< Nenner der robusten Quote (eigene Familie, eigenes m)
    std::size_t degeneriert_mwu{0}; ///< total - getestet_mwu
};

/// D4c -- die Headline-Zahl mit ihrer Grundgesamtheit in EINER Zeile. Sie steht hier und nicht
/// beim Aufrufer, damit CLI, CSV-Kopf und Bericht dieselbe Formulierung tragen und nicht an drei
/// Stellen driften. Eine Quote ohne Nenner ist von einem Freispruch nicht zu unterscheiden.
[[nodiscard]] inline std::string win_rate_zeile(MultiCompareSummary const& s) {
    std::ostringstream os;
    os << s.win_rate << " (" << s.significant_faster << " von " << s.getestet << " getestet, " << s.degeneriert
       << " degeneriert, Grundgesamtheit " << s.total << ")";
    return os.str();
}

/// Verdichtet den Report zur F15-Kernaussage: welcher Anteil der Kompositionen schlaegt die Baseline
/// FWER-korrigiert signifikant? (win_rate). Beantwortet "bringt die CacheEngine messbaren Wert?".
[[nodiscard]] inline MultiCompareSummary summarize(MultiCompareReport const& rep) {
    MultiCompareSummary s;
    s.total = rep.comparisons.size();
    for (auto const& c : rep.comparisons) {
        // D4c: die Degeneration ist eine EIGENE Kategorie und faellt nicht mehr in
        // not_significant. "Nie gemessen" ist kein Messbefund "kein Unterschied".
        if (!c.getestet_welch) {
            ++s.degeneriert;
        } else if (c.significant) {
            ++s.getestet;
            if (c.faster_than_baseline)
                ++s.significant_faster;
            else
                ++s.significant_slower;
        } else {
            ++s.getestet;
            ++s.not_significant;
        }
        if (c.getestet_mwu)
            ++s.getestet_mwu;
        else
            ++s.degeneriert_mwu;
        if (c.robust_significant) ++s.robust_significant;
        if (c.significance_discrepancy) ++s.discrepancies;
    }
    // Der Nenner ist die GETESTETE Menge. Bleibt sie leer, ist die Quote 0 -- und die Zeile sagt
    // dann "0 von 0 getestet", was etwas voellig anderes ist als "0 von 40 haben gewonnen".
    s.win_rate = (s.getestet > 0) ? static_cast<double>(s.significant_faster) / static_cast<double>(s.getestet) : 0.0;
    return s;
}

// ─── Export des Multi-Vergleichs-Reports (Diplomarbeit-Tabellen / CI-Artefakte) ───────────────

/// MultiCompareReport → CSV (Header + eine Zeile pro Vergleich, RFC-4180 fuer name).
[[nodiscard]] inline std::string report_to_csv(MultiCompareReport const& rep) {
    std::ostringstream os;
    // D4c: die fuenf neuen Spalten haengen HINTEN an -- ein Leser, der die alten zehn per Position
    // liest, bricht dadurch nicht. Ohne sie waere aus dem Export nicht rekonstruierbar, ueber
    // welcher Grundgesamtheit die adjusted_p gerechnet wurden.
    os << "name,raw_p,adjusted_p,significant,faster_than_baseline,welch_t,welch_df,mean_a,mean_b,welch_valid,"
          "welch_degeneriert,mwu_degeneriert,proben_tot,getestet_welch,getestet_mwu\n";
    for (auto const& c : rep.comparisons) {
        os << ::comdare::cache_engine::builder::commands::detail::csv_quote(c.name) << ',' << c.raw_p << ','
           << c.adjusted_p << ',' << (c.significant ? 1 : 0) << ',' << (c.faster_than_baseline ? 1 : 0) << ','
           << c.welch.t_statistic << ',' << c.welch.degrees_of_freedom << ',' << c.welch.mean_a << ',' << c.welch.mean_b
           << ',' << (c.welch.valid ? 1 : 0) << ',' << (c.welch.degeneriert ? 1 : 0) << ','
           << (c.mwu.degeneriert ? 1 : 0) << ',' << (c.proben_tot ? 1 : 0) << ',' << (c.getestet_welch ? 1 : 0) << ','
           << (c.getestet_mwu ? 1 : 0) << '\n';
    }
    return os.str();
}

/// MultiCompareReport → JSON-Objekt {alpha, significant_count, comparisons:[...]}.
[[nodiscard]] inline std::string report_to_json(MultiCompareReport const& rep) {
    std::ostringstream os;
    // D4c: der Kopf traegt jetzt die GRUNDGESAMTHEIT. significant_count allein war eine Zahl ohne
    // Nenner -- aus ihr liess sich nicht ablesen, ueber wie vielen Hypothesen korrigiert wurde.
    auto const s = summarize(rep);
    os << "{\"alpha\":" << rep.alpha << ",\"significant_count\":" << rep.significant_count << ",\"total\":" << s.total
       << ",\"getestet_welch\":" << s.getestet << ",\"degeneriert\":" << s.degeneriert
       << ",\"getestet_mwu\":" << s.getestet_mwu << ",\"win_rate\":" << s.win_rate << ",\"comparisons\":[";
    bool first = true;
    for (auto const& c : rep.comparisons) {
        if (!first) os << ',';
        os << '{' << "\"name\":\"" << ::comdare::cache_engine::builder::commands::detail::json_escape(c.name) << "\","
           << "\"raw_p\":" << c.raw_p << ',' << "\"adjusted_p\":" << c.adjusted_p << ','
           << "\"significant\":" << (c.significant ? "true" : "false") << ','
           << "\"faster_than_baseline\":" << (c.faster_than_baseline ? "true" : "false") << ','
           << "\"welch_t\":" << c.welch.t_statistic << ',' << "\"welch_df\":" << c.welch.degrees_of_freedom << ','
           << "\"mean_a\":" << c.welch.mean_a << ',' << "\"mean_b\":" << c.welch.mean_b << ','
           << "\"welch_valid\":" << (c.welch.valid ? "true" : "false") << ','
           << "\"welch_degeneriert\":" << (c.welch.degeneriert ? "true" : "false") << ','
           << "\"mwu_degeneriert\":" << (c.mwu.degeneriert ? "true" : "false") << ','
           << "\"proben_tot\":" << (c.proben_tot ? "true" : "false") << ','
           << "\"getestet_welch\":" << (c.getestet_welch ? "true" : "false") << ','
           << "\"getestet_mwu\":" << (c.getestet_mwu ? "true" : "false") << '}';
        first = false;
    }
    os << "]}";
    return os.str();
}

} // namespace comdare::cache_engine::builder::commands::stats
