// D5-5 HDR-VERDRAHTUNG (2026-08-09) -- die Thesis-Zusage "das HdrHistogram ERHEBT die Latenz-
// Perzentile" bekommt einen Produktions-Konsumenten in der AUSWERTUNG.
//
// SELBSTCHECK DIESER DATEI
//   ZUSICHERT: (1) die Toleranz zwischen HDR und Kanon ist aus significant_figures HERGELEITET
//              (1/sub_bucket_half_count), nicht gesetzt; (2) verworfene Proben (0 ns / negativ)
//              werden GEZAEHLT und sind abfragbar, statt still zu verschwinden; (3) die Auswertung
//              liefert Kanon UND HDR NEBENEINANDER; (4) HDR taucht im MESSPFAD nicht auf.
//   ZUSICHERT NICHT: nichts ueber die Je-Lauf-PERSISTENZ der Histogramme (Owner-Termin 3) -- fuer
//              die gibt es im Lager heute keinen Ort; das ist gemeldet, nicht erfunden. Nichts
//              ueber die super-Werkzeuge (04_csv_to_latex/05_diagram_generator, eigene Kopien).
//
// ABGRENZUNG, die dieser Test bewacht (Owner 09.08. + Deep Research): die Hauptmessung nimmt ROH
// auf. HDR ist eine DARSTELLUNGSFORM der nachgelagerten compare-Phase -- ein HDR-Bucket in
// checkpoint_measure waere ein Doktrinbruch, weil er den Rohwert schon bei der Erhebung quantisiert.

#include <builder/commands/hdr_perzentil_auswertung.hpp>
#include <builder/commands/latency_stats.hpp>
#include <latency_hdr_histogram.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace auswertung  = ::comdare::cache_engine::builder::commands::auswertung;
namespace stats       = ::comdare::cache_engine::builder::commands::stats;
namespace measurement = ::comdare::cache_engine::measurement;

namespace {

std::span<std::int64_t const> as_span(std::vector<std::int64_t> const& v) { return {v.data(), v.size()}; }

/// Stichprobe, bei der die HDR-Quantisierung WIRKLICH beisst. Werte unter 2048 liegen bei
/// significant_figures=3 im Bucket 0 mit Schrittweite 1 -- dort ist HDR exakt und ein Vergleich
/// beweist nichts. 500000..599900 liegt in den Buckets 8/9 (Schrittweite 256 bzw. 512).
std::vector<std::int64_t> stichprobe_mit_quantisierung() {
    std::vector<std::int64_t> v;
    v.reserve(1000);
    for (int i = 0; i < 1000; ++i) v.push_back(500000 + i * 100);
    return v; // n=1000 -> q*n ist fuer q=0.50/0.95/0.99 GANZZAHLIG, die Raenge stimmen ueberein
}

} // namespace

// == (1) DIE TOLERANZ IST HERGELEITET, NICHT GESETZT ===============================================
TEST(D55HdrVerdrahtung, ToleranzWirdAusSignifikantenStellenHergeleitet) {
    // Herleitung aus der vendorierten Quelle (hdr_histogram.c:406-418):
    //   largest_value_with_single_unit_resolution = 2 * 10^sf
    //   sub_bucket_count = 2^ceil(log2(...)), sub_bucket_half_count = sub_bucket_count / 2
    static_assert(auswertung::HdrGeometrie<1>::kHalbeUnterBuckets == 16);
    static_assert(auswertung::HdrGeometrie<2>::kHalbeUnterBuckets == 128);
    static_assert(auswertung::HdrGeometrie<3>::kHalbeUnterBuckets == 1024);
    static_assert(auswertung::HdrGeometrie<4>::kHalbeUnterBuckets == 16384);
    static_assert(auswertung::HdrGeometrie<5>::kHalbeUnterBuckets == 131072);

    // Die Toleranz ist 1/1024 -- und AUSDRUECKLICH nicht die frueher gesetzte 1 %.
    EXPECT_DOUBLE_EQ(auswertung::HdrGeometrie<3>::kRelativeToleranz, 1.0 / 1024.0);
    EXPECT_LT(auswertung::HdrGeometrie<3>::kRelativeToleranz, 0.01);
    EXPECT_NEAR(auswertung::HdrGeometrie<3>::kRelativeToleranz, 0.0009765625, 1e-15);
}

// == (2)+(3) VERWORFENE PROBEN WERDEN GEZAEHLT =====================================================
TEST(D55HdrVerdrahtung, NullNsProbenWerdenGezaehltStattVerschluckt) {
    std::vector<std::int64_t> v;
    for (int i = 0; i < 700; ++i) v.push_back(1000 + i);
    for (int i = 0; i < 300; ++i) v.push_back(0); // unterhalb der Uhr-Aufloesung -- ECHTE Messung

    auto const h = measurement::LatencyHdrHistogram::from_samples(as_span(v));

    EXPECT_EQ(h.count(), 700);          // das Histogramm haelt nur 700
    EXPECT_EQ(h.verworfen_null(), 300); // ... und sagt jetzt, dass 300 fehlen
    EXPECT_EQ(h.verworfen_negativ(), 0);
    EXPECT_EQ(h.vorgelegt(), 1000); // Nenner: was ueberhaupt angeboten wurde
}

TEST(D55HdrVerdrahtung, NegativeProbenSindEineEIGENEFehlerklasse) {
    // 0 ns = reale Messung unterhalb der Uhr-Aufloesung. Negative ns = unmoegliche Dauer, also ein
    // DEFEKT der Zeitnahme. Zwei Diagnosen, die sich keinen Zaehler teilen duerfen.
    std::vector<std::int64_t> const v{5, 0, -1, 7, 0, -42};

    auto const h = measurement::LatencyHdrHistogram::from_samples(as_span(v));

    EXPECT_EQ(h.count(), 2);
    EXPECT_EQ(h.verworfen_null(), 2);
    EXPECT_EQ(h.verworfen_negativ(), 2);
    EXPECT_EQ(h.vorgelegt(), 6);
}

TEST(D55HdrVerdrahtung, WerteUeberDerHistogrammSpanneWerdenGezaehlt) {
    // Der Rueckgabewert von hdr_record_value war bis D5-5 mit (void) weggeworfen. Damit war eine zu
    // klein gewaehlte Histogramm-Spanne die einzige Verlustart, die man nicht einmal nachtraeglich
    // aus den Zahlen erschliessen konnte -- count() war einfach kleiner, ohne Grund.
    // Vorgabe-Obergrenze ist 3'600'000'000'000 ns (= 3600 s); die beiden grossen Werte liegen darueber.
    std::vector<std::int64_t> const v{100, 4'000'000'000'000LL, 200, 9'000'000'000'000LL};

    auto const h = measurement::LatencyHdrHistogram::from_samples(as_span(v));

    EXPECT_TRUE(h.bereit());
    EXPECT_EQ(h.count(), 2);
    EXPECT_EQ(h.verworfen_ausserhalb(), 2);
    EXPECT_EQ(h.verworfen_null(), 0);
    EXPECT_EQ(h.verworfen_negativ(), 0);
    EXPECT_EQ(h.vorgelegt(), 4); // der Nenner bleibt vollstaendig, obwohl die Haelfte nicht passte
}

// == (4) DIE AUSWERTUNG LIEFERT KANON UND HDR NEBENEINANDER ========================================
TEST(D55HdrVerdrahtung, AuswertungLiefertKanonUndHdrNebeneinander) {
    auto const v = stichprobe_mit_quantisierung();

    auswertung::HdrAuswertung const a = auswertung::hdr_auswerten(as_span(v));

    EXPECT_EQ(a.vorgelegt, 1000);
    EXPECT_EQ(a.aufgezeichnet, 1000);
    EXPECT_EQ(a.verworfen_null, 0);

    // Der Kanon-Wert ist EXAKT der aus latency_stats -- die Auswertung baut keine zweite Formel.
    EXPECT_EQ(a.p50.kanon_ns, stats::percentile_ns(as_span(v), 0.50).count());
    EXPECT_EQ(a.p95.kanon_ns, stats::percentile_ns(as_span(v), 0.95).count());
    EXPECT_EQ(a.p99.kanon_ns, stats::percentile_ns(as_span(v), 0.99).count());

    // Und der HDR-Wert steht DANEBEN, nicht anstelle.
    EXPECT_GT(a.p50.hdr_ns, 0);
    EXPECT_GT(a.p95.hdr_ns, 0);
    EXPECT_GT(a.p99.hdr_ns, 0);
}

// == (5) GEGENPROBE GEGEN UEBERSCHAERFE: beide Zahlen + die Abweichung =============================
TEST(D55HdrVerdrahtung, HdrUndKanonLiegenInnerhalbDerHergeleitetenToleranz) {
    auto const v = stichprobe_mit_quantisierung();

    auswertung::HdrAuswertung const a = auswertung::hdr_auswerten(as_span(v));

    for (auto const& [name, paar] : {std::pair{"p50", a.p50}, std::pair{"p95", a.p95}, std::pair{"p99", a.p99}}) {
        // Beide Zahlen und die Abweichung werden GEDRUCKT -- ein Befund ausserhalb der Toleranz
        // soll lesbar sein, nicht nur rot.
        std::cout << "  " << name << ": kanon=" << paar.kanon_ns << " ns  hdr=" << paar.hdr_ns
                  << " ns  abweichung=" << (paar.abweichung_rel * 100.0) << " %  toleranz=" << (a.toleranz_rel * 100.0)
                  << " %\n";
        EXPECT_TRUE(paar.in_toleranz) << name << ": HDR weicht um " << (paar.abweichung_rel * 100.0)
                                      << " % ab, erlaubt sind " << (a.toleranz_rel * 100.0) << " %";
        // HDR rundet auf die BUCKET-OBERKANTE -- die Abweichung ist einseitig nach oben.
        EXPECT_GE(paar.hdr_ns, paar.kanon_ns) << name << ": HDR darf nie UNTER dem Kanon liegen";
    }
}

// == (5b) DER BEFUND-FALL: die Toleranz-Wache MUSS auch NEIN sagen koennen =========================
TEST(D55HdrVerdrahtung, RangDivergenzErzeugtEinenBefundAusserhalbDerToleranz) {
    // WARUM ES DIESEN TEST GIBT: laege in_toleranz IMMER auf true, waere die Wache aus (5) nicht
    // beobachtbar -- sie wuerde auch dann gruen bleiben, wenn jemand sie festnagelt. Hier wird der
    // NEIN-Zweig erzwungen.
    //
    // ZWEITE FEHLERQUELLE, die die Bucket-Schranke NICHT abdeckt: die beiden Verfahren zaehlen
    // Raenge verschieden.
    //   Kanon: R = ceil(q*n)                    (latency_stats.hpp)
    //   HDR:   R = floor(q*n + 0.5)             (hdr_histogram.c, hdr_value_at_percentile)
    // Sie fallen auseinander, sobald frac(q*n) in (0, 0.5) liegt. Fuer n=1012 und q=0.95 ist
    // q*n = 961.4 -> Kanon nimmt Rang 962 (Index 961), HDR nimmt Rang 961 (Index 960). Liegt an
    // GENAU dieser Stelle ein Sprung in der Verteilung, ist die Abweichung beliebig gross -- ganz
    // unabhaengig von der Bucket-Breite.
    std::vector<std::int64_t> v;
    v.reserve(1012);
    for (int i = 0; i < 961; ++i) v.push_back(500000);     // Index 0..960
    for (int i = 961; i < 1012; ++i) v.push_back(5000000); // Index 961..1011 -- der Sprung, Faktor 10

    auswertung::HdrAuswertung const a = auswertung::hdr_auswerten(as_span(v));

    std::cout << "  BEFUND p95: kanon=" << a.p95.kanon_ns << " ns  hdr=" << a.p95.hdr_ns
              << " ns  abweichung=" << (a.p95.abweichung_rel * 100.0) << " %  toleranz=" << (a.toleranz_rel * 100.0)
              << " %\n";

    EXPECT_EQ(a.p95.kanon_ns, 5000000) << "Kanon muss Rang ceil(0.95*1012)=962 nehmen, also Index 961";
    EXPECT_LT(a.p95.hdr_ns, 1000000) << "HDR muss Rang 961 nehmen, also Index 960 -- den Wert VOR dem Sprung";
    EXPECT_FALSE(a.p95.in_toleranz) << "eine Rang-Divergenz an einem Sprung MUSS als Befund auffallen";
    EXPECT_LT(a.p95.abweichung_rel, 0.0) << "hier liegt HDR ausnahmsweise UNTER dem Kanon (Rang, nicht Bucket)";

    // Die Gegenprobe dazu: p50 hat bei n=1012 KEINE Rang-Divergenz (0.5*1012 = 506 ist ganzzahlig)
    // und bleibt deshalb in der Toleranz -- der Befund oben ist also stellenbezogen, kein Dauerrot.
    EXPECT_TRUE(a.p50.in_toleranz) << "p50 hat hier keine Rang-Divergenz und darf nicht mitrot werden";
}

// == (6) ABGRENZUNG: HDR GEHOERT NICHT IN DEN MESSPFAD =============================================
TEST(D55HdrVerdrahtung, HdrTauchtImMesspfadNichtAuf) {
    // Zensus statt Behauptung: der Erhebungs-Pfad (checkpoint_measure = measure_storage) darf den
    // HDR-Header NICHT einbinden. Sonst quantisiert die Hauptmessung schon bei der Aufnahme.
    std::filesystem::path const messpfad{COMDARE_D55_MESSPFAD_ROOT};
    ASSERT_TRUE(std::filesystem::exists(messpfad)) << "Messpfad-Wurzel fehlt: " << messpfad;

    std::size_t geprueft = 0;
    std::size_t treffer  = 0;
    for (auto const& e : std::filesystem::recursive_directory_iterator(messpfad)) {
        if (!e.is_regular_file()) continue;
        auto const ext = e.path().extension().string();
        if (ext != ".hpp" && ext != ".cpp" && ext != ".h") continue;
        ++geprueft;
        std::ifstream in(e.path());
        std::string   zeile;
        while (std::getline(in, zeile)) {
            if (zeile.find("latency_hdr_histogram") != std::string::npos) {
                ++treffer;
                ADD_FAILURE() << "HDR im MESSPFAD: " << e.path().string() << " -- " << zeile;
            }
        }
    }
    // GEGENPROBE zum Nichtfund: der Zensus muss ueberhaupt Dateien gesehen haben.
    EXPECT_GT(geprueft, 0u) << "Zensus hat KEINE Quelldatei gelesen -- der Nichtfund waere wertlos";
    EXPECT_EQ(treffer, 0u);
}
