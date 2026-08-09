#pragma once
// V33.A.3 (2026-05-21) - Welch's unequal-variance t-test
//
// @subsystem CEB (Statistik-Utility)
// @phase_owner CEB
//
// Echter Welch's t-Test (Welch 1947 "The generalization of Student's problem
// when several different population variances are involved"). Verwendet
// Welch-Satterthwaite-Approximation fuer Freiheitsgrade + zweiseitigen
// Studentschen t-Verteilung CDF via incomplete beta function fuer P-Value.
//
// Referenz fuer betacf/betai: Numerical Recipes in C, 2nd ed. §6.4
// (BSD-Style fork erlaubt da nur die mathematische Formel verwendet wird).

#include <cmath>
#include <cstdint>
#include <span>
#include <vector>

namespace comdare::cache_engine::builder::commands::stats {

struct WelchResult {
    double      t_statistic{0.0};
    double      degrees_of_freedom{0.0};
    double      p_value{1.0};
    double      mean_a{0.0};
    double      mean_b{0.0};
    double      var_a{0.0};
    double      var_b{0.0};
    std::size_t n_a{0};
    std::size_t n_b{0};
    bool        valid{false};
    // D4a (2026-08-09) -- DIE NULL ALS DATEN-AUSSAGE, NICHT NUR ALS DIVISIONS-GEFAHR.
    //
    // `valid` und `degeneriert` sagen ZWEI VERSCHIEDENE Dinge, und die Trennung ist tragend:
    //   valid       == "es wurde ueberhaupt ein Ergebnis GERECHNET" (n>=2 auf beiden Seiten)
    //   degeneriert == "das Ergebnis traegt KEINE Aussage" -- der Zahlenwert ist eine Rettung
    //                  vor der Division, kein Messbefund.
    // Sie sind NICHT komplementaer: se<=0 liefert valid=true UND degeneriert=true (gerechnet,
    // aber leer), n<2 liefert valid=false UND degeneriert=true (nicht einmal gerechnet).
    //
    // WARUM DAS EIN EIGENES FELD BRAUCHT und nicht durch p_value==1.0 ausdrueckbar ist: ein
    // ECHT gemessenes p von 1.0 (zwei identisch streuende Gruppen) und die Rettung bei
    // Varianz 0 auf beiden Seiten sind am Zahlenwert nicht unterscheidbar. Genau diese
    // Ununterscheidbarkeit ist der Befund -- ein Konsument (multi_compare, D4c) muss den
    // degenerierten Fall aus der Hypothesen-Familie NEHMEN koennen, den echten nicht.
    bool degeneriert{false};
};

namespace detail {

/// Continued-fraction Auswertung fuer die incomplete beta function.
/// Lentz-Algorithm. Mathematisch korrekt, konvergiert in <100 Iterationen
/// fuer sinnvolle Parameter.
inline double betacf(double a, double b, double x) {
    constexpr int    kMaxIter = 200;
    constexpr double kEps     = 3.0e-7;
    constexpr double kFpMin   = 1.0e-30;

    const double qab = a + b;
    const double qap = a + 1.0;
    const double qam = a - 1.0;
    double       c   = 1.0;
    double       d   = 1.0 - qab * x / qap;
    if (std::fabs(d) < kFpMin) d = kFpMin;
    d        = 1.0 / d;
    double h = d;

    for (int m = 1; m <= kMaxIter; ++m) {
        const int m2 = 2 * m;
        double    aa = static_cast<double>(m) * (b - m) * x / ((qam + m2) * (a + m2));
        d            = 1.0 + aa * d;
        if (std::fabs(d) < kFpMin) d = kFpMin;
        c = 1.0 + aa / c;
        if (std::fabs(c) < kFpMin) c = kFpMin;
        d = 1.0 / d;
        h *= d * c;
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2));
        d  = 1.0 + aa * d;
        if (std::fabs(d) < kFpMin) d = kFpMin;
        c = 1.0 + aa / c;
        if (std::fabs(c) < kFpMin) c = kFpMin;
        d                = 1.0 / d;
        const double del = d * c;
        h *= del;
        if (std::fabs(del - 1.0) < kEps) break;
    }
    return h;
}

/// Regularized incomplete beta function I_x(a, b).
inline double betai(double a, double b, double x) {
    if (x < 0.0 || x > 1.0) return std::nan("");
    if (x == 0.0 || x == 1.0) return x;
    const double bt =
        std::exp(std::lgamma(a + b) - std::lgamma(a) - std::lgamma(b) + a * std::log(x) + b * std::log(1.0 - x));
    if (x < (a + 1.0) / (a + b + 2.0)) { return bt * betacf(a, b, x) / a; }
    return 1.0 - bt * betacf(b, a, 1.0 - x) / b;
}

/// Zweiseitiger P-Value einer t-Statistik mit df Freiheitsgraden via
/// Student-t-Verteilung-CDF: p = I_{df/(df+t^2)}(df/2, 1/2).
inline double student_t_p_value_two_tailed(double t, double df) {
    if (df <= 0.0) return std::nan("");
    const double x = df / (df + t * t);
    return betai(df / 2.0, 0.5, x);
}

inline std::pair<double, double> mean_and_unbiased_variance(std::span<const std::int64_t> samples) {
    if (samples.size() < 2) return {0.0, 0.0};
    long double sum = 0.0L;
    for (auto s : samples) sum += static_cast<long double>(s);
    const double mean = static_cast<double>(sum / static_cast<long double>(samples.size()));
    long double  sq   = 0.0L;
    for (auto s : samples) {
        const long double diff = static_cast<long double>(s) - mean;
        sq += diff * diff;
    }
    const double var = static_cast<double>(sq / static_cast<long double>(samples.size() - 1));
    return {mean, var};
}

} // namespace detail

/// D4a -- DIE EINE Stelle, an der ueber "degeneriert" entschieden wird.
///
/// Zwei Ausloeser, und sie sind NICHT derselbe Fall:
///   (1) se <= 0  -- beide Stichproben haben Varianz 0. Die t-Statistik ist dann (m_a - m_b)/0,
///       also 0/0 bzw. unendlich: UNDEFINIERT. Der Zaehler kann dabei beliebig gross sein --
///       zwei konstante Gruppen 929 und 1031 liegen 11 % auseinander und sind trotzdem nicht
///       t-testbar. Genau deshalb ist das kein "kein Unterschied", sondern "keine Aussage".
///   (2) p nicht endlich -- NaN/inf aus der Verteilungsfunktion (student_t_p_value_two_tailed
///       liefert NaN bei df<=0). Aus std::int64_t-Eingaben ist das HEUTE nicht erreichbar
///       (denom>0 folgt aus se>0, gemessen ueber 2000 zufaellige Laeufe in
///       F15WelchDegeneriert.NichtEndlichesPFeuertUeberDemGanzenNennerNie: 0 von 2000). Der
///       Zweig steht trotzdem und ist fail-closed: ein unerkennbarer NaN, der als p=NaN in die
///       Bonferroni-Familie liefe, waere schlimmer als ein nie feuernder Guard.
///
/// FAIL-CLOSED-FORM: `!(se > 0.0)` statt `se <= 0.0` -- damit faengt der Guard auch ein NaN-se,
/// bei dem BEIDE Vergleiche falsch waeren.
[[nodiscard]] inline bool welch_ergebnis_ist_degeneriert(double standardfehler, double p_wert) noexcept {
    return !(standardfehler > 0.0) || !std::isfinite(p_wert);
}

/// Welch's t-test fuer zwei unabhaengige Stichproben mit ungleichen Varianzen.
/// Liefert t-Statistik, Welch-Satterthwaite-df + zweiseitigen P-Value.
/// Bei < 2 Samples pro Gruppe ist .valid = false UND .degeneriert = true (nichts gerechnet).
inline WelchResult welch_t_test(std::span<const std::int64_t> samples_a, std::span<const std::int64_t> samples_b) {
    WelchResult r{};
    r.n_a = samples_a.size();
    r.n_b = samples_b.size();
    if (r.n_a < 2 || r.n_b < 2) {
        // Nicht gerechnet (valid bleibt false) UND ohne Aussage (degeneriert). Beide Felder
        // werden gesetzt, weil ein Konsument beides getrennt braucht: multi_compare schliesst
        // ueber `degeneriert` aus der Familie aus, compare_engine_command liest `valid`.
        r.degeneriert = true;
        return r;
    }

    const auto [mean_a, var_a] = detail::mean_and_unbiased_variance(samples_a);
    const auto [mean_b, var_b] = detail::mean_and_unbiased_variance(samples_b);
    r.mean_a                   = mean_a;
    r.mean_b                   = mean_b;
    r.var_a                    = var_a;
    r.var_b                    = var_b;

    const double se_a_sq = var_a / static_cast<double>(r.n_a);
    const double se_b_sq = var_b / static_cast<double>(r.n_b);
    const double se      = std::sqrt(se_a_sq + se_b_sq);

    if (welch_ergebnis_ist_degeneriert(se, 1.0)) {
        // se <= 0 (oder NaN): beide Varianzen sind 0. Der zurueckgegebene p-Wert von 1.0 bleibt
        // stehen, damit sich am Zahlen-Verhalten bestehender Konsumenten NICHTS aendert -- die
        // AUSSAGE steckt jetzt in degeneriert. Kein Vorzeichen und keine Signifikanz werden hier
        // erfunden, auch wenn mean_a != mean_b ist: der Test hat nicht gerechnet.
        r.t_statistic        = 0.0;
        r.degrees_of_freedom = static_cast<double>(r.n_a + r.n_b - 2);
        r.p_value            = 1.0;
        r.valid              = true;
        r.degeneriert        = true;
        return r;
    }

    r.t_statistic = (mean_a - mean_b) / se;

    const double num = (se_a_sq + se_b_sq) * (se_a_sq + se_b_sq);
    const double denom =
        (se_a_sq * se_a_sq) / static_cast<double>(r.n_a - 1) + (se_b_sq * se_b_sq) / static_cast<double>(r.n_b - 1);
    r.degrees_of_freedom = (denom > 0.0) ? (num / denom) : 0.0;

    r.p_value = detail::student_t_p_value_two_tailed(r.t_statistic, r.degrees_of_freedom);
    r.valid   = true;
    // Zweiter Ausloeser: ein nicht endlicher p-Wert waere ein gerechnetes Nichts.
    r.degeneriert = welch_ergebnis_ist_degeneriert(se, r.p_value);
    return r;
}

} // namespace comdare::cache_engine::builder::commands::stats
