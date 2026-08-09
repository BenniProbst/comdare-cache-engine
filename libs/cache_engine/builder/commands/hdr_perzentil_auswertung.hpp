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
//              Perzentil-Formel; (3) verworfene Proben werden durchgereicht, nicht geschluckt;
//              (4) hdr_lauf_persistieren schreibt einen Lauf so, dass der NENNER (vorgelegt) und
//              alle vier Verwurf-Toepfe IM SELBEN STROM vor der Verteilung stehen.
//   ZUSICHERT NICHT: nichts ueber die Je-Lauf-Persistenz des GOLDEN-MESSPFADS -- dessen Rohwerte
//              sterben schon in harness/perm_runner.hpp (s. HALTEPUNKT unten); persistiert wird
//              heute nur, wo die Roh-Samples noch leben (compare-Phase). Nichts darueber, ob eine
//              Abweichung ausserhalb der Toleranz schlimm ist -- das ist ein BEFUND fuer den
//              Auswerter, kein Fehlschlag dieser Datei.
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
// == HALTEPUNKT: PERSISTIERT WIRD, WO DIE ROHWERTE NOCH LEBEN =====================================
// Owner-Termin 3 (09.04.): "Je Lauf werden HDR-Histogramme persistiert." GEBAUT ist das fuer die
// compare-Phase (hdr_lauf_persistieren unten, Aufrufer apps/f15_compare --hdr-out=): dort liegen die
// Roh-Samples noch vollstaendig im Speicher, und die Ausgabe-Datei ist ein CLI-gewaehltes Artefakt
// derselben Klasse wie das dort schon vorhandene --json/--csv. Das Lager wird dafuer NICHT angefasst.
//
// NICHT GEBAUT und ausdruecklich offen ist der GOLDEN-MESSPFAD. Zwei getrennte Gruende, die nicht
// verwechselt werden duerfen:
//   (1) DATENVERLUST, der eigentliche Blocker: harness/perm_runner.hpp reduziert res.insert_ns /
//       lookup_ns / erase_ns / clear_ns / scan_ns / rmw_ns noch IM Messlauf auf OpKindLatency
//       {n, p50, p99, p999} (perm_runner.hpp:331-339). Ab da existiert kein Rohwert mehr -- an der
//       CSV-Schreib-Naht (experiment_tree/cache_engine_builder_iterator.hpp, result.csv) liegt nur
//       noch das Aggregat. Ein Histogramm laesst sich dort nicht mehr bilden, egal wo man schreibt.
//   (2) LAGER-ORT, nachgelagert: das Lager (builder/lager_ablage + bestandslog) kennt zwei
//       Blatt-Sorten -- xlsx (Default) und CSV --, beide ZEILEN-orientiert. Fuer ein Nicht-Zeilen-
//       Artefakt gibt es keine Artefakt-Sorte, keinen Bestandslog-Eintrag und keine SKIP-Regel.
// (1) muss VOR (2) entschieden werden: ohne durchgereichte Rohwerte gibt es nichts abzulegen. Beides
// zusammen ist ein eigenes Paket; hier zu raten waere ein Behelfsweg.

#include "latency_stats.hpp" // DER KANON: stats::percentile_ns (ceil(q*n)-1). Keine zweite Formel.

#include <latency_hdr_histogram.hpp>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

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
    /// false = die RELATIVE Abweichung ist nicht definierbar, weil der Kanon 0 ns ist, HDR aber
    /// nicht. Dann ist abweichung_rel bedeutungslos und in_toleranz allein aus dem Vergleich
    /// kanon==hdr gefaellt. Der Fall ist NICHT exotisch -- s. paar_bauen.
    bool vergleichbar{true};
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

/// Baut ein PerzentilPaar und faellt das Toleranz-Urteil.
///
/// KORREKTUR (D5-5, zweiter Zug): hier stand "kanon_ns == 0 (leere Stichprobe) ==> das Urteil bleibt
/// positiv". Die Klammer war eine falsche Annahme, und sie hat das Urteil VERFAELSCHT. kanon_ns wird
/// naemlich auch bei einer VOLLEN Stichprobe 0, sobald genug 0-ns-Proben darin liegen: der Kanon
/// sortiert die ROHWERTE einschliesslich der Nullen, HDR wirft sie vorher weg (verworfen_null). Bei
/// 400 Nullen unter 780 Proben liefert der Kanon fuer p50 exakt 0 ns und HDR ~464895 ns -- die
/// beiden Verfahren sind sich also maximal UNEINIG, und die alte Zeile meldete "in Toleranz", weil
/// abweichung_rel mangels Division 0 blieb. Genau die stille Verzerrung, gegen die der 0-ns-Zaehler
/// gebaut wurde, nur eine Ebene hoeher.
///
/// DREI FAELLE statt zweier, weil es drei verschiedene Aussagen sind:
///   kanon != 0            -> relative Abweichung definiert, normales Urteil.
///   kanon == 0, hdr == 0  -> beide sagen dasselbe. Einig, vergleichbar (das ist der LEERE Fall).
///   kanon == 0, hdr != 0  -> UNEINIG, aber die relative Abweichung ist nicht definierbar. Das Urteil
///                            faellt negativ, und vergleichbar=false sagt, dass abweichung_rel hier
///                            nichts bedeutet. Ein "in Toleranz" waere hier schlicht falsch.
[[nodiscard]] inline PerzentilPaar paar_bauen(std::span<std::int64_t const>                                    proben,
                                              ::comdare::cache_engine::measurement::LatencyHdrHistogram const& hist,
                                              double                                                           q) {
    PerzentilPaar p{};
    p.kanon_ns = stats::percentile_ns(proben, q).count(); // DER KANON -- keine zweite Formel hier
    p.hdr_ns   = hist.value_at(q);
    if (p.kanon_ns != 0) {
        p.abweichung_rel = static_cast<double>(p.hdr_ns - p.kanon_ns) / static_cast<double>(p.kanon_ns);
        p.vergleichbar   = true;
        p.in_toleranz    = std::fabs(p.abweichung_rel) <= GeltendeGeometrie::kRelativeToleranz;
    } else {
        // Keine Division moeglich -- abweichung_rel bleibt 0 und traegt KEINE Aussage.
        p.abweichung_rel = 0.0;
        p.vergleichbar   = (p.hdr_ns == 0);
        p.in_toleranz    = (p.hdr_ns == 0);
    }
    return p;
}

} // namespace detail

namespace detail {

/// Fuellt die Auswertung aus einem SCHON GEBAUTEN Histogramm. Ausgelagert, damit
/// hdr_auswerten und hdr_lauf_persistieren dieselbe Rechnung benutzen statt zweier, die
/// auseinanderdriften koennen -- die persistierte Datei muss dieselben Zahlen tragen wie die Anzeige.
[[nodiscard]] inline HdrAuswertung
auswertung_bauen(std::span<std::int64_t const>                                    proben,
                 ::comdare::cache_engine::measurement::LatencyHdrHistogram const& hist) {
    HdrAuswertung a{};
    a.histogramm_bereit    = hist.bereit();
    a.aufgezeichnet        = hist.count();
    a.verworfen_null       = hist.verworfen_null();
    a.verworfen_negativ    = hist.verworfen_negativ();
    a.verworfen_ausserhalb = hist.verworfen_ausserhalb();
    a.vorgelegt            = static_cast<std::int64_t>(proben.size());

    a.p50 = paar_bauen(proben, hist, 0.50);
    a.p95 = paar_bauen(proben, hist, 0.95);
    a.p99 = paar_bauen(proben, hist, 0.99);
    return a;
}

} // namespace detail

/// Wertet eine ROH-Stichprobe nachgelagert aus: Kanon und HDR nebeneinander, plus der Verwurf.
///
/// WICHTIG: die Eingabe sind ROH-Samples aus der Erhebung. Diese Funktion quantisiert NICHTS an der
/// Quelle -- sie baut ihr Histogramm auf einer Kopie der schon vollstaendig erhobenen Werte.
[[nodiscard]] inline HdrAuswertung hdr_auswerten(std::span<std::int64_t const> proben) {
    auto const hist = ::comdare::cache_engine::measurement::LatencyHdrHistogram::from_samples(proben);
    return detail::auswertung_bauen(proben, hist);
}

// == JE-LAUF-PERSISTENZ (Owner-Termin 3, 09.04.: "JE LAUF werden HDR-Histogramme persistiert") =====
//
// WARUM DER VERWURF-KOPF NICHT WEGGELASSEN WERDEN DARF: eine nackte .hgrm-Verteilung ist GENAU die
// stille Verzerrung, gegen die der 0-ns-Zaehler gebaut wurde. hdr_percentiles_print kennt nur die
// aufgezeichneten Proben -- die 0-ns-Proben liegen nicht im Histogramm, tauchen in seiner Ausgabe
// also nirgends auf, auch nicht als Fehlbetrag. Wer die Datei spaeter liest, saehe eine geschlossen
// wirkende Verteilung ueber n=aufgezeichnet und haette keinen Anhalt, dass n < vorgelegt war.
// Deshalb traegt JEDE persistierte Datei ihren Nenner (vorgelegt) und alle vier Verwurf-Toepfe VOR
// der Verteilung, im selben Strom -- untrennbar, nicht als zweite Datei danebengelegt.
//
// WO DAS HINGEHOERT: in die AUSWERTUNG, wie die Rechnung darueber. Der Aufrufer ist heute die
// compare-Phase (apps/f15_compare, --hdr-out=), die ihre Roh-Samples noch vollstaendig im Speicher
// haelt. Die Datei ist ein CLI-GEWAEHLTES Ausgabe-Artefakt derselben Klasse wie das dort schon
// vorhandene --json/--csv -- KEINE Lager-Blattsorte. Das Lager (builder/lager_ablage + bestandslog)
// wird hier NICHT angefasst und braucht keine neue Artefakt-Sorte; die xlsx/CSV-Doktrin bleibt
// unberuehrt, weil dies kein Lager-Blatt ist. Was damit NICHT geloest ist, steht als offener Posten
// im Kopf dieser Datei: der golden-Messpfad (builder/experiment_tree -> result.csv) reduziert seine
// Roh-Vektoren schon in harness/perm_runner.hpp auf n/p50/p99/p999; dort ist je-Lauf-Persistenz erst
// moeglich, wenn die Rohwerte bis an eine Schreib-Naht durchgereicht werden. Das ist ein eigenes
// Paket, kein Nebenzug dieser Datei.

/// Persistiert EINEN Lauf: Verwurf-Kopf (mit Nenner) + klassische HdrHistogram-Verteilung, ein Strom.
///
/// Fehlende Elternverzeichnisse werden angelegt. false = Zielverzeichnis nicht anlegbar, Datei nicht
/// zu oeffnen, oder ein Schreibfehler -- in allen drei Faellen ist die Datei NICHT gueltig, und der
/// Aufrufer muss das melden statt sie fuer geschrieben zu halten.
[[nodiscard]] inline bool hdr_lauf_persistieren(std::span<std::int64_t const> proben, std::filesystem::path const& ziel,
                                                std::string_view lauf_kennung) {
    auto const          hist = ::comdare::cache_engine::measurement::LatencyHdrHistogram::from_samples(proben);
    HdrAuswertung const a    = detail::auswertung_bauen(proben, hist);

    if (auto const eltern = ziel.parent_path(); !eltern.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(eltern, ec);
        if (ec && !std::filesystem::is_directory(eltern)) return false;
    }

    std::FILE* strom = std::fopen(ziel.string().c_str(), "w");
    if (strom == nullptr) return false;

    std::string const kennung{lauf_kennung};
    // Der Kopf ist bewusst '#'-kommentiert: hdr_percentiles_print haengt seinen eigenen '#'-Fuss an,
    // die Datei bleibt so durchgehend im selben Kommentar-Stil und Zeile-fuer-Zeile greppbar.
    int rc =
        std::fprintf(strom,
                     "#[comdare D5-5 HDR-Histogramm, je Lauf persistiert]\n"
                     "#[lauf=%s]\n"
                     "#[significant_figures=%d]\n"
                     "#[toleranz_rel=%.10f]\n"
                     "#[histogramm_bereit=%d]\n"
                     "#[vorgelegt=%lld]\n"
                     "#[aufgezeichnet=%lld]\n"
                     "#[verworfen_null=%lld]\n"
                     "#[verworfen_negativ=%lld]\n"
                     "#[verworfen_ausserhalb=%lld]\n",
                     kennung.c_str(), ::comdare::cache_engine::measurement::LatencyHdrHistogram::kSignifikanteStellen,
                     a.toleranz_rel, a.histogramm_bereit ? 1 : 0, static_cast<long long>(a.vorgelegt),
                     static_cast<long long>(a.aufgezeichnet), static_cast<long long>(a.verworfen_null),
                     static_cast<long long>(a.verworfen_negativ), static_cast<long long>(a.verworfen_ausserhalb));

    // Beide Perzentil-Zahlen wandern MIT in die Datei: wer sie spaeter liest, soll den Kanon nicht
    // aus der Bucket-Verteilung zurueckrechnen muessen (er stuende dort gar nicht drin).
    for (auto const& [name, paar] : {std::pair{"p50", a.p50}, std::pair{"p95", a.p95}, std::pair{"p99", a.p99}}) {
        if (rc < 0) break;
        // vergleichbar=0 heisst: abweichung_rel in DIESER Zeile ist bedeutungslos (Kanon 0 ns, HDR
        // nicht). Wer die Datei auswertet, darf die Zahl dann nicht mitteln oder auftragen.
        rc =
            std::fprintf(strom, "#[%s kanon_ns=%lld hdr_ns=%lld abweichung_rel=%.10f in_toleranz=%d vergleichbar=%d]\n",
                         name, static_cast<long long>(paar.kanon_ns), static_cast<long long>(paar.hdr_ns),
                         paar.abweichung_rel, paar.in_toleranz ? 1 : 0, paar.vergleichbar ? 1 : 0);
    }

    bool const verteilung_ok = (rc >= 0) && hist.verteilung_schreiben(strom);
    // fclose meldet gepufferte Schreibfehler, die fprintf noch nicht gesehen hat -- sein Rueckgabewert
    // ist die letzte Stelle, an der ein volles Dateisystem ueberhaupt auffallen kann.
    bool const geschlossen_ok = (std::fclose(strom) == 0);
    return verteilung_ok && geschlossen_ok;
}

} // namespace comdare::cache_engine::builder::commands::auswertung
