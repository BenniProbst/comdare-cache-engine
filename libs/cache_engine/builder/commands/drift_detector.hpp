#pragma once
// drift_detector.hpp — #197 chaos: Mess-Drift-Detektor + Rerun-Gate (Pipeline-Stufe 6, EPIC #186)
//
// @subsystem CEB (Mess-Auswertung, neben latency_stats.hpp + mann_whitney_u_test.hpp +
//            multi_compare.hpp + result_aggregator.hpp)
// @phase_owner CEB
//
// ZWECK (Diplomarbeit-Kern, Direktive „wissenschaftlich belastbare Messung statt driftverseuchter Werte"):
//   Eine Messung wird in N (Default 3) aufeinanderfolgenden 2-Phasen-Wiederholungen erhoben. Streuen
//   diese Wiederholungen um mehr als die Schwelle (Default 5 %, Median-relativ), gilt die Messung als
//   instabil → die GANZE Wiederholungs-Gruppe wird neu gemessen (bis max_reruns), und jedes Rerun-
//   Ereignis wird als WARNUNG geloggt. Selbstheilung statt stiller Korruption: erschöpft sich das
//   Rerun-Budget, wird die stabilste (geringste Drift) Gruppe zurückgegeben + eine finale Warnung
//   emittiert — ADVISORY, kein Abbruch (Direktive „>5 % → 3× + Warn-Log", nicht „fail").
//
// EINORDNUNG / KEINE DUPLIKATION: nutzt stats::percentile_ns aus latency_stats.hpp als Single-Source-
//   Perzentil (Nearest-Rank, non-mutierend) → KEINE Methoden-Drift gegenüber Welch-t-Test, Perzentilen
//   und winsorized_mean. Median := percentile_ns(s, 0.5); Spannweite via latency_min/max_ns. Es wird
//   KEIN synthetischer Mess-Puffer erzeugt — der Detektor bewertet reale per-Wiederholungs-Proben (ns).
//
// T-15 (2026-08-09) -- WER DIE ZAHLEN STELLT, seit dieses Gate produktiv laeuft.
//   Die Signatur-Defaults unten (reps=3, threshold=0.05, max_reruns=3) sind MECHANISMUS-Defaults fuer
//   Aufrufer ohne Konfiguration. Der PRODUKTIVE Mess-Pfad benutzt sie NICHT: er uebergibt immer
//   explizit die Werte aus harness/drift_gated_cell.hpp DriftGateConfig, die ihrerseits aus dem
//   Profil-XML (<drift_gate reps threshold_permille max_reruns/>) kommen. Dort steht auch das
//   Owner-Budget 5 (GOAL-v8 VI.5, 08.08.2026: "Beim Scheitern bis zu 5 Wiederholungen"); es steht
//   bewusst NICHT hier, weil ein zweiter Ort fuer dieselbe Owner-Zahl genau die Drift erzeugte, die
//   dieser Header sonst ueberall vermeidet -- und weil eine Aenderung dieses Signatur-Defaults von
//   keinem Test und keinem Aufrufer beobachtet wuerde (test_chaos_drift_gate uebergibt max_reruns an
//   jeder Stelle explizit): sie waere eine Aenderung, die nichts aendert.
//   BIS T-15 hatte diese Datei NULL produktive Aufrufer -- der Mechanismus war vollstaendig, getestet
//   und wirkungslos. Die Klammer sitzt in cache_engine_builder_iterator.hpp (measure_under_setting).
//
// KALIBRIERUNG: Die Schwelle (threshold) ist parametrisiert; ihre exakte Kalibrierung gegen reale
//   PMC-Läufe ist #156-gegatet (mehrtägiger Voll-Lauf). Der MECHANISMUS (Detektor + Rerun-Controller +
//   Warn-Log) ist hier vollständig und deterministisch CI-verifizierbar (Job chaos:drift /
//   test_chaos_drift_gate). Verzahnt mit Quality-Flag #165 (winsorized_mean_ns/Perzentil-Ausgabe).

#include <builder/commands/latency_stats.hpp> // stats::percentile_ns / latency_min_ns / latency_max_ns

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::commands {

// ── Drift-Verdikt einer Wiederholungs-Gruppe ──────────────────────────────────────────────────
// relative_drift = (max-min)/median : die Median-relative Spannweite der Wiederholungen. Robust über
// percentile_ns definiert (gleiche Perzentil-Methode wie der Rest der Auswertung). „unstable" gdw.
// relative_drift die Schwelle überschreitet.
struct DriftVerdict {
    std::int64_t median_ns      = 0;     // stats::percentile_ns(samples, 0.5)
    std::int64_t min_ns         = 0;     // stats::latency_min_ns
    std::int64_t max_ns         = 0;     // stats::latency_max_ns
    double       relative_drift = 0.0;   // (max-min)/median; NUR gueltig wenn bestimmbar == true
    double       threshold      = 0.05;  // >5 % Default (#156-kalibriert)
    bool         unstable       = false; // bestimmbar && relative_drift > threshold
    std::size_t  samples        = 0;     // Anzahl Wiederholungen in der Gruppe
    // D4 (2026-08-08): DER ZUSTAND, DER GEFEHLT HAT.
    // Bis heute konnte ein Konsument "Drift 0, weil wirklich stabil" nicht von "Drift 0, weil kein
    // Nenner" unterscheiden -- beide sahen als (relative_drift == 0.0, unstable == false) identisch
    // aus. Bei median_ns <= 0 (alle Proben 0, weggeoptimierte Operation, kaputte Uhr) hiess das:
    // GRUEN MIT NENNER NULL. run_with_drift_gate nahm eine solche Gruppe beim ERSTEN Versuch an,
    // ohne einen Rerun und ohne eine einzige Warnzeile -- das Gate schwieg genau dann, wenn gar
    // nichts gemessen wurde. Dieselbe Klasse wie der m3v2_pmc_smoke-Befund ("misst ein leeres
    // Fenster"). Fuer reps == 0 hatte derselbe Code den Fall erkannt und stable = false gesetzt
    // (s.u.); fuer median <= 0 nicht.
    // bestimmbar == false heisst: es liegt KEINE Drift-Aussage vor. Nicht "stabil", nicht
    // "instabil" -- keine Aussage. Wer darauf eine Messung stuetzt, stuetzt sie auf nichts.
    bool bestimmbar = false;
};

// Bewertet die Drift einer Wiederholungs-Gruppe. Crash-frei an den Raendern, aber NIE stillschweigend
// zustimmend -- jeder Rand liefert bestimmbar == false statt einer erfundenen Null:
//   - 0 Samples            -> bestimmbar = false (keine Aussage ohne Daten)
//   - 1 Sample             -> bestimmbar = false (eine einzelne Probe hat keine Wiederholungs-Streuung;
//                             median=min=max wird trotzdem gefuellt, damit der Wert sichtbar bleibt)
//   - median_ns <= 0       -> bestimmbar = false (kein Nenner; degenerierte/Null-Messung)
// Nur im verbleibenden Fall gilt relative_drift = (max-min)/median, bestimmbar = true und
// unstable = relative_drift > threshold.
[[nodiscard]] inline DriftVerdict assess_drift(std::span<const std::int64_t> samples, double threshold = 0.05) {
    DriftVerdict v;
    v.threshold = threshold;
    v.samples   = samples.size();
    if (samples.empty()) return v; // 0 Samples -> keine Aussage
    if (samples.size() == 1) {     // 1 Sample -> keine Streuung messbar, also keine Aussage
        v.median_ns = v.min_ns = v.max_ns = samples[0];
        return v;
    }
    v.median_ns = stats::percentile_ns(samples, 0.5).count();
    v.min_ns    = stats::latency_min_ns(samples);
    v.max_ns    = stats::latency_max_ns(samples);
    if (v.median_ns > 0) { // Division-durch-0-Schutz UND Bestimmbarkeits-Kriterium in einem
        v.relative_drift = static_cast<double>(v.max_ns - v.min_ns) / static_cast<double>(v.median_ns);
        v.unstable       = v.relative_drift > v.threshold;
        v.bestimmbar     = true;
    }
    return v;
}

// ── Ergebnis des Rerun-Gates ──────────────────────────────────────────────────────────────────
struct DriftGateResult {
    DriftVerdict              verdict;           // Verdikt der GEWÄHLTEN Gruppe (stabil → diese; sonst stabilste)
    std::vector<std::int64_t> samples;           // Proben der gewählten Gruppe
    std::size_t               attempts  = 0;     // durchgeführte Mess-Gruppen insgesamt (1 .. max_reruns+1)
    std::size_t               reruns    = 0;     // ausgelöste Reruns (= attempts-1 bei Stabilität, sonst max_reruns)
    bool                      stable    = false; // gewählte Gruppe stabil (Drift <= threshold)?
    bool                      exhausted = false; // Rerun-Budget erschöpft und nie stabil geworden?
};

// Führt die Wiederholungs-Gruppe (reps Proben über measure_one) aus; ist sie instabil, wird die GANZE
// Gruppe neu gemessen, bis sie stabil ist ODER max_reruns erschöpft sind. Jeder Rerun wird als Warnung
// nach `warn` geloggt (nullptr = stumm). Erschöpft + nie stabil → stabilste Gruppe + finale Warnung
// (advisory; KEIN Fehler, KEIN throw). measure_one() liefert EINE 2-Phasen-Mess-Probe in ns
// (Aufrufer kapselt save-all → warmup → rollback-all → measure; hier wird NICHT interpoliert).
template <class MeasureOne>
[[nodiscard]] DriftGateResult
run_with_drift_gate(MeasureOne&& measure_one, std::size_t reps = 3, double threshold = 0.05, std::size_t max_reruns = 3,
                    std::ostream* warn = nullptr, std::string const& label = "measurement") {
    DriftGateResult result;

    // Defensive (Codex-Review #197): reps==0 = keine Wiederholung konfiguriert → keine Messung möglich.
    // Ehrliches Ergebnis (NICHT „stabil"): nichts gemessen, nicht stabil, 0 Versuche, measure_one wird
    // nie aufgerufen. (N/reps kommt später aus der Profil-Config — kein stilles „stabil" bei Fehlkonfig.)
    //
    // D4 (2026-08-08): reps == 1 gehoert in DENSELBEN Zweig und stand bisher nicht darin. Eine einzelne
    // Probe je Gruppe hat keine Wiederholungs-Streuung; die Drift ist grundsaetzlich unbestimmbar, und
    // ein Rerun aendert daran nichts -- jede Wiederholung liefert wieder eine Ein-Proben-Gruppe. Der
    // alte Code nahm sie beim ersten Versuch als "stabil" an. Ein Lauf mit reps == 1 kann keine
    // Drift-Freiheit bezeugen; er darf sie deshalb auch nicht behaupten. Sofortiger ehrlicher Ausgang
    // statt max_attempts sinnloser Messgruppen.
    if (reps < 2) {
        result.verdict  = assess_drift(std::span<const std::int64_t>{}, threshold);
        result.stable   = false;
        result.attempts = 0;
        if (warn != nullptr && reps == 1) {
            (*warn) << "[chaos:drift] WARN " << label
                    << ": reps=1 -- eine einzelne Probe je Gruppe hat keine Wiederholungs-Streuung, "
                    << "die Drift ist unbestimmbar. Kein Rerun (er wuerde dasselbe liefern). "
                    << "Die Messung gilt als NICHT drift-geprueft.\n";
        }
        return result;
    }

    DriftVerdict              best_verdict;
    std::vector<std::int64_t> best_samples;
    bool                      have_best    = false;
    std::size_t const         max_attempts = max_reruns + 1; // 1 initiale Gruppe + max_reruns Reruns

    for (std::size_t attempt = 1; attempt <= max_attempts; ++attempt) {
        result.attempts = attempt;

        std::vector<std::int64_t> group;
        group.reserve(reps);
        for (std::size_t r = 0; r < reps; ++r) group.push_back(static_cast<std::int64_t>(measure_one()));

        DriftVerdict const v = assess_drift(group, threshold);

        // Stabilste Gruppe bisher merken (für den advisory-Fall „nie stabil").
        // D4: eine UNBESTIMMBARE Gruppe traegt relative_drift == 0.0 und haette den alten Vergleich
        // immer gewonnen -- die schlechteste denkbare Gruppe waere als "beste" zurueckgegeben worden.
        // Bestimmbarkeit schlaegt darum den Zahlenwert; erst unter bestimmbaren Gruppen entscheidet
        // die kleinere Drift.
        bool const besser = !have_best || (v.bestimmbar && !best_verdict.bestimmbar) ||
                            (v.bestimmbar && best_verdict.bestimmbar && v.relative_drift < best_verdict.relative_drift);
        if (besser) {
            best_verdict = v;
            best_samples = group;
            have_best    = true;
        }

        // D4: Annahme verlangt eine AUSSAGE, nicht nur die Abwesenheit einer Klage. Vorher stand hier
        // `if (!v.unstable)` -- und unstable ist bei unbestimmbarer Drift ebenfalls false. Genau darueber
        // ist eine Null-Messung als "stabil" durchgelaufen.
        if (v.bestimmbar && !v.unstable) { // bestimmbar UND nicht instabil -> akzeptiert, fertig
            result.verdict = v;
            result.samples = std::move(group);
            result.stable  = true;
            result.reruns  = attempt - 1; // attempt 1 ⇒ 0 Reruns
            return result;
        }

        if (warn != nullptr) {
            bool const        will_retry = attempt < max_attempts;
            std::string const nachsatz   = will_retry
                                               ? ("Rerun " + std::to_string(attempt) + "/" + std::to_string(max_reruns))
                                               : std::string("Rerun-Budget erschoepft");
            if (!v.bestimmbar) {
                // D4: der Fall, in dem das Gate bisher SCHWIEG. Er ist nicht milder als eine zu hohe
                // Drift, sondern haerter -- bei zu hoher Drift wurde wenigstens etwas gemessen.
                (*warn) << "[chaos:drift] WARN " << label << ": Drift UNBESTIMMBAR "
                        << "(median=" << v.median_ns << "ns min=" << v.min_ns << " max=" << v.max_ns
                        << " n=" << v.samples << ") -- ohne positiven Median gibt es keinen Nenner. "
                        << "Die Gruppe wird NICHT als stabil angenommen. " << nachsatz << "\n";
            } else {
                (*warn) << "[chaos:drift] WARN " << label << ": Wiederholungs-Drift " << (v.relative_drift * 100.0)
                        << "% > " << (v.threshold * 100.0) << "% "
                        << "(median=" << v.median_ns << "ns min=" << v.min_ns << " max=" << v.max_ns
                        << " n=" << v.samples << ") -- " << nachsatz << "\n";
            }
        }
    }

    // Rerun-Budget erschöpft, nie stabil → stabilste Gruppe zurückgeben (advisory).
    result.verdict   = best_verdict;
    result.samples   = std::move(best_samples);
    result.stable    = false;
    result.exhausted = true;
    result.reruns    = max_reruns;
    if (warn != nullptr) {
        if (!best_verdict.bestimmbar) {
            // D4: keine einzige Gruppe lieferte einen positiven Median. Das ist kein Drift-Problem,
            // sondern ein Mess-Problem -- und es muss anders heissen, sonst sucht jemand am falschen
            // Ort. Die zurueckgegebenen Proben sind Zahlen ohne Aussagekraft.
            (*warn) << "[chaos:drift] WARN " << label << ": nach " << max_reruns
                    << " Reruns war die Drift in KEINER Gruppe bestimmbar (nie ein positiver Median, "
                    << "n=" << best_verdict.samples << "). Es liegt keine Drift-Aussage vor -- die "
                    << "Messung ist nicht drift-geprueft, nicht etwa geprueft-und-instabil "
                    << "(advisory, kein Abbruch).\n";
        } else {
            (*warn) << "[chaos:drift] WARN " << label << ": nach " << max_reruns
                    << " Reruns weiterhin instabil (beste Drift " << (best_verdict.relative_drift * 100.0) << "% > "
                    << (threshold * 100.0) << "%) -- Messung als unzuverlaessig markiert (advisory, kein Abbruch).\n";
        }
    }
    return result;
}

} // namespace comdare::cache_engine::builder::commands
