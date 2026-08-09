#pragma once
// V41.F.6.1 R5.E (2026-05-29) — latency_stats: wiederverwendbare Latenz-Perzentile/Statistik
//
// @subsystem CEB (Mess-Auswertung, neben welch_t_test.hpp + multiple_comparison.hpp + result_aggregator.hpp)
// @phase_owner CEB
//
// Extrahiert aus ExecuteEngineCommand::percentile_ns (war privat + MUTIERTE die Eingabe via
// nth_element). Hier wiederverwendbar + NON-MUTIEREND (kopiert intern) — damit die Roh-Samples
// fuer Welch's t-Test (CompareEngineCommand) in Original-Reihenfolge erhalten bleiben und mehrere
// Konsumenten (ExecuteEngine, ResultAggregator, Analyse) dieselbe Perzentil-Definition teilen.
//
// Perzentil-Methode seit D5-1 (2026-08-09): Nearest-Rank, Lehrbuch-Formel k = ceil(q*n) - 1
// (0-basiert). Bis dahin rechnete percentile_ns k = min(n-1, floor(q*n)) -- siehe den KANON-Block
// unten fuer die verworfenen Alternativen und die Folge fuer alle vorher erhobenen Zahlen.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace comdare::cache_engine::builder::commands::stats {

// == D5-1 PERZENTIL-KANON (2026-08-09) =======================================================================
// SELBSTCHECK DIESES BLOCKS
//   ZUSICHERT: nearest_rank_index ist die EINZIGE Umrechnung Quantil -> Feld-Index im Auswertungs-Baum
//              (libs/cache_engine/builder + libs/cache_engine/harness). Jede Perzentil-Zahl, die dieses
//              Repo ausgibt, kommt ueber diese eine Funktion.
//   ZUSICHERT NICHT: nichts ueber die super-Werkzeuge (Code/04_csv_to_latex, Code/05_diagram_generator) --
//              die tragen eigene Kopien und haengen an einem eigenen ce-Vendor-Stand (Paket D5-2/D5-3).
//              Nichts ueber die uebrigen Median-Bauarten (eta_kalibrierung::median_t_s mittelt bei geradem n,
//              HDR-Histogramm ist ein eigenes Verfahren) -- das entscheidet D5-2/D5-5.
//
// GEWAEHLTE KONVENTION -- es gibt mehrere legitime, und der Streit entsteht spaeter genau hier:
//   NEAREST RANK (Lehrbuch; Hyndman/Fan 1996 Typ 1, inverse empirische Verteilungsfunktion).
//     ordinaler Rang   R(q,n) = ceil(q*n)          1-basiert, geklemmt auf [1, n]
//     Feld-Index       k(q,n) = R(q,n) - 1         0-basiert, in das AUFSTEIGEND sortierte Feld
//   Definierende Eigenschaft: P(q) ist der KLEINSTE Stichprobenwert v, fuer den mindestens q*n
//   Stichprobenwerte <= v sind. P(q) ist damit IMMER ein real gemessener Wert.
//
// AUSDRUECKLICH NICHT GEWAEHLT (und warum):
//   * KEINE Interpolation (Typ 7 / Excel PERCENTILE.INC): erzeugt Zahlen, die nie gemessen wurden --
//     fuer Latenz-Tails eine Erfindung.
//   * KEIN round(q*(n-1)) (bis 2026-08-09 als detail::nearest_rank_p im Baum, ERSATZLOS GELOESCHT):
//     trug den Namen "Nearest-Rank" zu Unrecht und lieferte bei 1..100 p50=51 statt 50.
//   * KEIN floor(q*n) (bis 2026-08-09 die Rechnung von percentile_ns): um genau eine Rangstelle zu
//     hoch, sobald q*n ganzzahlig ist -- bei 1..100 p95=96 statt 95, p99=100 statt 99.
//   * Der MEDIAN ist der Fall q=0.5 und KEINE eigene Bauart. Bei GERADEM n liefert er die UNTERE
//     Mitte (n=4 -> Index 1), nicht das Mittel der beiden mittleren Werte.
//
// FOLGE (bewusst, Owner-KERN "Invalidieren ist das Ziel"): ALLE vor dem 2026-08-09 erhobenen
// p50/p95/p99-Zahlen sind nach einer ANDEREN Definition gerechnet und damit ungueltig. Die Messdaten
// selbst werden NICHT geloescht -- sie werden markiert.

/// Rundungs-Schutz der Rang-Rechnung. q kommt als double; 0.95 ist binaer NICHT exakt darstellbar,
/// das Produkt q*n liegt damit minimal ueber oder unter der ganzen Zahl, die die Lehrbuch-Formel
/// meint. Ohne Schutz kippt ceil() bei einem Produkt wie 95.0000000000000018 auf 96 und das
/// Perzentil springt um eine VOLLE Rangstelle. Der Schutz ist RELATIV zum Produkt (1e-12, mindestens
/// 1e-12 absolut): um Groessenordnungen groesser als der Rundungsfehler von q*n und um
/// Groessenordnungen kleiner als der kleinste gewollte Rang-Unterschied (der ist 1).
[[nodiscard]] inline double rang_schutz(double q, std::size_t n) noexcept {
    double const roh = q * static_cast<double>(n);
    return 1e-12 * ((roh > 1.0) ? roh : 1.0);
}

/// KANON: 0-basierter Index des q-Perzentils in einem aufsteigend sortierten Feld der Groesse n.
/// k = ceil(q*n) - 1, geklemmt auf [0, n-1]. n==0 -> 0 (der Aufrufer muss die leere Stichprobe
/// vorher abfangen; ein Index in ein leeres Feld ist immer ein Aufrufer-Fehler).
[[nodiscard]] inline std::size_t nearest_rank_index(std::size_t n, double q) noexcept {
    if (n == 0) return 0;
    if (q <= 0.0) return 0;
    if (q >= 1.0) return n - 1;
    std::size_t const rang = static_cast<std::size_t>(std::ceil(q * static_cast<double>(n) - rang_schutz(q, n)));
    if (rang <= 1) return 0;
    return (rang >= n) ? (n - 1) : (rang - 1);
}

/// Nearest-Rank-Perzentil der Latenz-Samples (non-mutierend; q wird in nearest_rank_index geklemmt).
/// Der Index kommt AUSSCHLIESSLICH aus dem Kanon -- hier steht keine zweite Formel.
[[nodiscard]] inline std::chrono::nanoseconds percentile_ns(std::span<const std::int64_t> samples, double q) {
    if (samples.empty()) return std::chrono::nanoseconds{0};
    std::vector<std::int64_t> copy(samples.begin(), samples.end());
    std::size_t const         k = nearest_rank_index(copy.size(), q);
    std::nth_element(copy.begin(), copy.begin() + static_cast<std::ptrdiff_t>(k), copy.end());
    return std::chrono::nanoseconds{copy[k]};
}

[[nodiscard]] inline std::chrono::nanoseconds latency_p50_ns(std::span<const std::int64_t> s) {
    return percentile_ns(s, 0.50);
}
[[nodiscard]] inline std::chrono::nanoseconds latency_p95_ns(std::span<const std::int64_t> s) {
    return percentile_ns(s, 0.95);
}
[[nodiscard]] inline std::chrono::nanoseconds latency_p99_ns(std::span<const std::int64_t> s) {
    return percentile_ns(s, 0.99);
}

/// Minimum (0 bei leerer Eingabe).
[[nodiscard]] inline std::int64_t latency_min_ns(std::span<const std::int64_t> s) {
    if (s.empty()) return 0;
    return *std::min_element(s.begin(), s.end());
}
/// Maximum (0 bei leerer Eingabe).
[[nodiscard]] inline std::int64_t latency_max_ns(std::span<const std::int64_t> s) {
    if (s.empty()) return 0;
    return *std::max_element(s.begin(), s.end());
}
/// arithmetisches Mittel in ns (0.0 bei leerer Eingabe; long-double-Akkumulation gegen Overflow).
[[nodiscard]] inline double latency_mean_ns(std::span<const std::int64_t> s) {
    if (s.empty()) return 0.0;
    long double sum = 0.0L;
    for (auto v : s) sum += static_cast<long double>(v);
    return static_cast<double>(sum / static_cast<long double>(s.size()));
}

// ── #165-A (P-MD9, 2026-06-20): WINSORIZED MEAN — Lehrbuch-Robust-Statistik ─────────────────────────────────
// Benanntes Muster: "Winsorized Mean" (Winsorizing nach C. P. Winsor; siehe Dixon & Tukey 1968, "Approximate
// Behavior of the Distribution of Winsorized t", Technometrics 10(1):83–98). ABGRENZUNG zum getrimmten Mittel:
// das getrimmte Mittel ENTFERNT die Extrem-Samples; das winsorisierte Mittel BEHÄLT alle n Samples, CLAMPt aber
// die unteren/oberen Ausreißer auf die jeweilige Perzentil-Grenze [P(trim_q), P(1-trim_q)] und mittelt dann über
// ALLE n geklemmten Werte. Robust gegen einzelne System-Störungs-Spitzen (Scheduler/IRQ), ohne die Stichprobengröße
// zu verkleinern → ein stabilerer zentraler Lagewert als das arithmetische Mittel für die Latenz-Auswertung.
//
// NON-MUTIEREND (kopiert intern, identisch zu percentile_ns): die Roh-Samples bleiben in Original-Reihenfolge für
// Welch's t-Test/Perzentile erhalten. trim_q wird auf [0, 0.5) geklemmt (symmetrischer Trim je Flanke); trim_q<=0
// ⇒ kein Clamping ⇒ exakt latency_mean_ns. Leere Eingabe ⇒ 0.0.
//
// Grenzen via percentile_ns (Nearest-Rank, dieselbe Single-Source-Perzentil-Definition) → keine Methoden-Drift.
[[nodiscard]] inline double winsorized_mean_ns(std::span<const std::int64_t> samples, double trim_q) {
    if (samples.empty()) return 0.0;
    if (trim_q <= 0.0) return latency_mean_ns(samples); // kein Trim → arithmetisches Mittel über alle n
    if (trim_q >= 0.5) trim_q = 0.5 - 1e-9;             // symmetrischer Trim < halbe Stichprobe (kein Kollaps)
    std::int64_t const lo = percentile_ns(samples, trim_q).count();       // untere Winsor-Grenze P(trim_q)
    std::int64_t const hi = percentile_ns(samples, 1.0 - trim_q).count(); // obere  Winsor-Grenze P(1-trim_q)
    // Robust gegen lo>hi (degenerierte/winzige Stichprobe): in geordnete [min(lo,hi), max(lo,hi)] normalisieren.
    std::int64_t const clamp_lo = (lo <= hi) ? lo : hi;
    std::int64_t const clamp_hi = (lo <= hi) ? hi : lo;
    long double        sum      = 0.0L;
    for (auto v : samples) {
        std::int64_t const w = (v < clamp_lo) ? clamp_lo : (v > clamp_hi ? clamp_hi : v); // auf [lo,hi] winsorisieren
        sum += static_cast<long double>(w);
    }
    return static_cast<double>(sum / static_cast<long double>(samples.size())); // Mittel über ALLE n (behalten!)
}

} // namespace comdare::cache_engine::builder::commands::stats
