#pragma once
// D5-5 HDR-VERDRAHTUNG (2026-08-09) -- HDR-Perzentile NEBEN dem Kanon, in der AUSWERTUNG.
//
// @subsystem CEB (Mess-Auswertung, neben latency_stats.hpp + drift_detector.hpp)
// @phase_owner CEB
//
// SELBSTCHECK DIESER DATEI
//   ZUSICHERT: (1) die Toleranz zwischen HDR und Kanon ist aus significant_figures HERGELEITET
//              (kRelativeToleranz = 1/sub_bucket_half_count), nicht gesetzt; (2) der Kanon-Wert
//              kommt AUSSCHLIESSLICH aus stats::percentile_ns -- hier steht keine zweite
//              Perzentil-Formel; (3) verworfene Proben werden durchgereicht, nicht geschluckt.
//   ZUSICHERT NICHT: nichts ueber die Je-Lauf-PERSISTENZ der Histogramme (Owner-Termin 3) -- die
//              braucht einen Lager-Ort, den es heute nicht gibt (s. HALTEPUNKT unten). Nichts
//              darueber, ob eine Abweichung ausserhalb der Toleranz schlimm ist -- das ist ein
//              BEFUND fuer den Auswerter, kein Fehlschlag dieser Datei.
//
// == DIE ABGRENZUNG, DIE DIESE DATEI TRAEGT ========================================================
// Owner 09.08.: "Die Hauptauswertung misst ueber die Checkpoints STUMPF reale Daten aus. Die
// Ableitung ist eine Darstellungsform fuer LaTeX und PDF -- NICHT Teil des hardware-checkpoint-
// measure direkt." Deep Research von der anderen Seite: "HdrHistogram ist Vorbild fuer das
// ALLOKATIONSPRINZIP, nicht fuer die verlustbehaftete Bucket-Politik; die fertige
// LatencyHdrHistogram gehoert in die NACHGELAGERTE compare-Phase."
// ==> Diese Datei liegt in builder/commands (Auswertung), NICHT in builder/measure_storage
//   (Erhebung). Ein HDR-Bucket im Messpfad wuerde den Rohwert schon bei der Aufnahme quantisieren
//   und waere unumkehrbar. test_d55_hdr_verdrahtung bewacht das per Quell-Zensus.
//
// == WARUM BEIDE UND NICHT EINES VON BEIDEN ========================================================
// HDR ist die ERHEBUNGSSTRUKTUR (Buckets, konstanter Speicher, zusammenfuehrbar ueber Laeufe), der
// Kanon ist die INDEX-FORMEL (ceil(q*n)-1, Hyndman/Fan Typ 1). Sie beantworten verschiedene Fragen:
// der Kanon liefert einen REAL GEMESSENEN Wert, HDR liefert eine Bucket-Oberkante bei konstantem
// Speicher. Der Vergleich beider auf DERSELBEN Stichprobe ist die Gegenprobe gegen Ueberschaerfe --
// deshalb traegt PerzentilPaar beide Zahlen UND ihre Abweichung, statt sich fuer eine zu entscheiden.
//
// == HALTEPUNKT: JE-LAUF-PERSISTENZ IST HIER NICHT GEBAUT ==========================================
// Owner-Termin 3 (09.04.): "Je Lauf werden HDR-Histogramme persistiert." Diese Datei RECHNET die
// Histogramme, sie SCHREIBT sie nicht. Grund: das Lager (builder/lager_ablage + bestandslog) kennt
// heute genau zwei Blatt-Sorten -- xlsx (Default) und CSV (Fallback), beide ZEILEN-orientiert ueber
// eine vom Iterator vorgegebene Spaltenmenge. Ein HDR-Histogramm ist ein BINAERER Bucket-Block
// (hdr_log_writer/hdr_encode), keine Zeile. blatt_dateiname() nimmt zwar eine freie Endung entgegen
// (lager_pfad_grammatik.hpp), aber es gibt keine Artefakt-Sorte, keinen Bestandslog-Eintrag und
// keine SKIP-Regel dafuer -- und die xlsx-Doktrin ("xlsx IST die Ausgabe") sagt nicht, wie ein
// Nicht-Zeilen-Artefakt daneben gefuehrt wird. Diesen Ort zu ERFINDEN waere ein Behelfsweg; er ist
// gemeldet statt gebaut.

#include "latency_stats.hpp" // DER KANON: stats::percentile_ns (ceil(q*n)-1). Keine zweite Formel.

#include <latency_hdr_histogram.hpp>

#include <cmath>
#include <cstdint>
#include <span>

namespace comdare::cache_engine::builder::commands::auswertung {

namespace detail {

/// 10^n als ganze Zahl, compile-time. std::pow ist nicht constexpr und rundet ausserdem.
[[nodiscard]] consteval std::int64_t zehn_hoch(int n) noexcept {
    std::int64_t p = 1;
    for (int i = 0; i < n; ++i) p *= 10;
    return p;
}

/// ceil(log2(x)) fuer x >= 1, compile-time und ohne Gleitkomma. Die vendorierte Quelle rechnet
/// ceil(log(x)/log(2)) in double; fuer die hier auftretenden x (20 .. 200000) sind beide Wege
/// deckungsgleich -- der ganzzahlige ist aber nicht von Rundung abhaengig.
[[nodiscard]] consteval int aufrunden_log2(std::int64_t x) noexcept {
    int          m = 0;
    std::int64_t p = 1;
    while (p < x) {
        p <<= 1;
        ++m;
    }
    return m;
}

} // namespace detail

/// Die Bucket-Geometrie von HdrHistogram_c, HERGELEITET aus significant_figures.
///
/// Quelle der Rechnung: vendor/hdr_histogram.c, hdr_calculate_bucket_config:
///     largest_value_with_single_unit_resolution = 2 * 10^sf
///     sub_bucket_count_magnitude                = ceil(log2(largest_...))
///     sub_bucket_half_count_magnitude           = sub_bucket_count_magnitude - 1
///     sub_bucket_count                          = 2^(half_count_magnitude + 1)
///     sub_bucket_half_count                     = sub_bucket_count / 2
///
/// WORAUS DIE TOLERANZ FOLGT: ein Wert v liegt in Bucket b mit Schrittweite w = 2^(unit_mag + b),
/// und sein Unter-Bucket-Index liegt in [half_count, 2*half_count). Also gilt v >= half_count * w,
/// mithin w/v <= 1/half_count. hdr_value_at_percentile gibt die BUCKET-OBERKANTE zurueck
/// (highest_equivalent_value = lowest_equivalent + w - 1), die Abweichung ist damit EINSEITIG nach
/// oben und relativ durch 1/half_count beschraenkt.
///     sf=3  ->  2*10^3 = 2000  ->  ceil(log2(2000)) = 11  ->  half_count = 2^10 = 1024
///           ->  Toleranz = 1/1024 = 0.0009765625 = 0.09765625 %
/// Die frueher in test_ap8_hdr_histogram gesetzte 1 % war damit um den Faktor 10.24 zu weit -- sie
/// haette eine echte Bucket-Fehlwahl nicht bemerkt.
template <int kSignifikanteStellen>
    requires(kSignifikanteStellen >= 1 && kSignifikanteStellen <= 5) // hdr_init lehnt alles andere ab
struct HdrGeometrie {
    static constexpr std::int64_t kEinheitsAufloesung   = 2 * detail::zehn_hoch(kSignifikanteStellen);
    static constexpr int          kUnterBucketMagnitude = detail::aufrunden_log2(kEinheitsAufloesung);
    static constexpr std::int64_t kHalbeUnterBuckets    = std::int64_t{1} << (kUnterBucketMagnitude - 1);
    static constexpr double       kRelativeToleranz     = 1.0 / static_cast<double>(kHalbeUnterBuckets);
};

/// Die Geometrie, die zu dem Histogramm gehoert, das diese Auswertung tatsaechlich baut. Der Bezug
/// laeuft ueber LatencyHdrHistogram::kSignifikanteStellen -- eine zweite Konstante hier koennte von
/// der des Wrappers abdriften, ohne dass es jemand merkt.
using GeltendeGeometrie = HdrGeometrie<::comdare::cache_engine::measurement::LatencyHdrHistogram::kSignifikanteStellen>;

/// Ein Perzentil, von BEIDEN Verfahren gerechnet. Beide Zahlen bleiben stehen -- wer sie vergleichen
/// will, braucht sie einzeln, nicht als schon verrechnete Differenz.
struct PerzentilPaar {
    std::int64_t kanon_ns{0};         ///< stats::percentile_ns -- ein REAL gemessener Wert
    std::int64_t hdr_ns{0};           ///< hdr_value_at_percentile -- die Bucket-Oberkante
    double       abweichung_rel{0.0}; ///< (hdr - kanon) / kanon; VORZEICHENBEHAFTET
    bool         in_toleranz{true};   ///< |abweichung_rel| <= kRelativeToleranz
};

/// Das Ergebnis der nachgelagerten HDR-Auswertung einer Roh-Stichprobe.
struct HdrAuswertung {
    PerzentilPaar p50{};
    PerzentilPaar p95{};
    PerzentilPaar p99{};

    std::int64_t vorgelegt{0};            ///< NENNER: alle Proben, die vorgelegt wurden
    std::int64_t aufgezeichnet{0};        ///< im Histogramm gelandet
    std::int64_t verworfen_null{0};       ///< 0 ns -- unterhalb der Uhr-Aufloesung
    std::int64_t verworfen_negativ{0};    ///< < 0 ns -- Defekt der Zeitnahme
    std::int64_t verworfen_ausserhalb{0}; ///< ueber der Histogramm-Obergrenze
    bool         histogramm_bereit{true}; ///< false -> hdr_init fehlgeschlagen, ALLES verloren

    double toleranz_rel{GeltendeGeometrie::kRelativeToleranz};
};

namespace detail {

/// Baut ein PerzentilPaar und faellt das Toleranz-Urteil. kanon_ns == 0 (leere Stichprobe) ==> keine
/// relative Abweichung definierbar; dann bleibt sie 0 und das Urteil positiv, statt durch 0 zu teilen.
[[nodiscard]] inline PerzentilPaar paar_bauen(std::span<std::int64_t const>                                    proben,
                                              ::comdare::cache_engine::measurement::LatencyHdrHistogram const& hist,
                                              double                                                           q) {
    PerzentilPaar p{};
    p.kanon_ns = stats::percentile_ns(proben, q).count(); // DER KANON -- keine zweite Formel hier
    p.hdr_ns   = hist.value_at(q);
    if (p.kanon_ns != 0) {
        p.abweichung_rel = static_cast<double>(p.hdr_ns - p.kanon_ns) / static_cast<double>(p.kanon_ns);
    }
    p.in_toleranz = std::fabs(p.abweichung_rel) <= GeltendeGeometrie::kRelativeToleranz;
    return p;
}

} // namespace detail

/// Wertet eine ROH-Stichprobe nachgelagert aus: Kanon und HDR nebeneinander, plus der Verwurf.
///
/// WICHTIG: die Eingabe sind ROH-Samples aus der Erhebung. Diese Funktion quantisiert NICHTS an der
/// Quelle -- sie baut ihr Histogramm auf einer Kopie der schon vollstaendig erhobenen Werte.
[[nodiscard]] inline HdrAuswertung hdr_auswerten(std::span<std::int64_t const> proben) {
    auto const hist = ::comdare::cache_engine::measurement::LatencyHdrHistogram::from_samples(proben);

    HdrAuswertung a{};
    a.histogramm_bereit    = hist.bereit();
    a.aufgezeichnet        = hist.count();
    a.verworfen_null       = hist.verworfen_null();
    a.verworfen_negativ    = hist.verworfen_negativ();
    a.verworfen_ausserhalb = hist.verworfen_ausserhalb();
    a.vorgelegt            = static_cast<std::int64_t>(proben.size());

    a.p50 = detail::paar_bauen(proben, hist, 0.50);
    a.p95 = detail::paar_bauen(proben, hist, 0.95);
    a.p99 = detail::paar_bauen(proben, hist, 0.99);
    return a;
}

} // namespace comdare::cache_engine::builder::commands::auswertung
