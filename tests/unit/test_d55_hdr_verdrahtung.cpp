// D5-5 HDR-VERDRAHTUNG (2026-08-09) -- die Thesis-Zusage "das HdrHistogram ERHEBT die Latenz-
// Perzentile" bekommt einen Produktions-Konsumenten in der AUSWERTUNG.
//
// SELBSTCHECK DIESER DATEI
//   ZUSICHERT: (1) die Toleranz zwischen HDR und Kanon ist aus significant_figures HERGELEITET
//              (1/sub_bucket_half_count), nicht gesetzt; (2) verworfene Proben (0 ns / negativ)
//              werden GEZAEHLT und sind abfragbar, statt still zu verschwinden; (3) die Auswertung
//              liefert Kanon UND HDR NEBENEINANDER; (4) HDR taucht im MESSPFAD nicht auf.
//              (5) die JE-LAUF PERSISTIERTE Datei traegt ihren Nenner und alle vier Verwurf-Toepfe,
//              und ein nicht anlegbares Ziel wird als Fehlschlag GEMELDET, nicht verschwiegen.
//   ZUSICHERT NICHT: nichts ueber die Je-Lauf-Persistenz des GOLDEN-Messpfads -- dessen Rohwerte
//              sterben schon in harness/perm_runner.hpp (Reduktion auf n/p50/p99/p999); das ist ein
//              eigenes Paket und im Kopf von hdr_perzentil_auswertung.hpp als offen ausgewiesen.
//              Nichts ueber die super-Werkzeuge (04_csv_to_latex/05_diagram_generator, eigene Kopien).
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
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
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

// == (7) JE-LAUF-PERSISTENZ (Owner-Termin 3, 09.04.) ===============================================
//
// DIE STICHPROBE IST GEWUERFELT, nicht gewaehlt: 400 Null-Proben, 372 echte, 8 negative, Basis
// 464626 ns -- alle vier Zahlen frisch aus /dev/urandom (K13), damit kein Wert versehentlich der
// eine ist, den die Implementierung ohnehin trifft. Sie stehen hier als Literale, weil ein Test mit
// Zufall ZUR LAUFZEIT nicht reproduzierbar rot wird.
namespace {

constexpr std::int64_t kProbenNull    = 400; // gewuerfelt
constexpr std::int64_t kProbenEcht    = 372; // gewuerfelt
constexpr std::int64_t kProbenNegativ = 8;   // gewuerfelt
constexpr std::int64_t kBasisNs       = 464626;
constexpr std::int64_t kVorgelegt     = kProbenNull + kProbenEcht + kProbenNegativ; // 780

std::vector<std::int64_t> gewuerfelte_stichprobe() {
    std::vector<std::int64_t> v;
    v.reserve(static_cast<std::size_t>(kVorgelegt));
    for (std::int64_t i = 0; i < kProbenEcht; ++i) v.push_back(kBasisNs + i);
    for (std::int64_t i = 0; i < kProbenNull; ++i) v.push_back(0);
    for (std::int64_t i = 0; i < kProbenNegativ; ++i) v.push_back(-(i + 1));
    return v;
}

std::string datei_lesen(std::filesystem::path const& p) {
    std::ifstream     in(p, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

/// Liest EINEN Kopf-Wert der Form "#[schluessel=<zahl>]". Gibt false zurueck, wenn der Schluessel
/// fehlt -- ein fehlender Nenner ist ein FEHLER, kein stiller 0-Wert.
bool kopf_zahl(std::string const& inhalt, std::string_view schluessel, std::int64_t& aus) {
    std::string const muster = "#[" + std::string{schluessel} + "=";
    auto const        pos    = inhalt.find(muster);
    if (pos == std::string::npos) return false;
    auto const start = pos + muster.size();
    auto const ende  = inhalt.find(']', start);
    if (ende == std::string::npos) return false;
    aus = std::stoll(inhalt.substr(start, ende - start));
    return true;
}

/// Die "Total count"-Zahl aus dem Fuss, den hdr_percentiles_print selbst schreibt. Sie ist die
/// UNABHAENGIGE Gegenquelle (T-5): sie kommt aus dem vendorierten C-Kern, nicht aus unserem Kopf.
bool verteilung_gesamtzahl(std::string const& inhalt, std::int64_t& aus) {
    auto const pos = inhalt.find("Total count");
    if (pos == std::string::npos) return false;
    auto const gleich = inhalt.find('=', pos);
    if (gleich == std::string::npos) return false;
    std::size_t i = gleich + 1;
    while (i < inhalt.size() && (inhalt[i] == ' ')) ++i;
    std::size_t const start = i;
    while (i < inhalt.size() && inhalt[i] >= '0' && inhalt[i] <= '9') ++i;
    if (i == start) return false;
    aus = std::stoll(inhalt.substr(start, i - start));
    return true;
}

std::filesystem::path frisches_testverzeichnis(std::string_view name) {
    auto const p = std::filesystem::temp_directory_path() / ("comdare_d55_" + std::string{name});
    std::filesystem::remove_all(p);
    return p;
}

} // namespace

TEST(D55HdrPersistenz, PersistierteDateiTraegtNennerUndAlleVierVerwurfToepfe) {
    auto const                  v    = gewuerfelte_stichprobe();
    auto const                  dir  = frisches_testverzeichnis("nenner");
    std::filesystem::path const ziel = dir / "lauf.hdr.hgrm";

    ASSERT_TRUE(auswertung::hdr_lauf_persistieren(as_span(v), ziel, "gewuerfelter-lauf"));
    ASSERT_TRUE(std::filesystem::exists(ziel));

    std::string const inhalt = datei_lesen(ziel);

    // T-2: NICHT "die Datei existiert", sondern "die Datei sagt die richtigen Zahlen".
    std::int64_t vorgelegt = -1, aufgezeichnet = -1, v_null = -1, v_negativ = -1, v_ausserhalb = -1;
    ASSERT_TRUE(kopf_zahl(inhalt, "vorgelegt", vorgelegt)) << "der NENNER fehlt in der persistierten Datei";
    ASSERT_TRUE(kopf_zahl(inhalt, "aufgezeichnet", aufgezeichnet));
    ASSERT_TRUE(kopf_zahl(inhalt, "verworfen_null", v_null)) << "der 0-ns-Zaehler fehlt -- genau die stille Verzerrung";
    ASSERT_TRUE(kopf_zahl(inhalt, "verworfen_negativ", v_negativ));
    ASSERT_TRUE(kopf_zahl(inhalt, "verworfen_ausserhalb", v_ausserhalb));

    std::cout << "  persistiert: vorgelegt=" << vorgelegt << " aufgezeichnet=" << aufgezeichnet
              << " verworfen_null=" << v_null << " verworfen_negativ=" << v_negativ << "\n";

    EXPECT_EQ(vorgelegt, kVorgelegt);
    EXPECT_EQ(aufgezeichnet, kProbenEcht);
    EXPECT_EQ(v_null, kProbenNull);
    EXPECT_EQ(v_negativ, kProbenNegativ);
    EXPECT_EQ(v_ausserhalb, 0);
    // Die Topf-Invariante gilt auch in der DATEI, nicht nur im Speicher.
    EXPECT_EQ(vorgelegt, aufgezeichnet + v_null + v_negativ + v_ausserhalb);

    std::filesystem::remove_all(dir);
}

TEST(D55HdrPersistenz, DieVerteilungALLEINWaereIrrefuehrendDeshalbDerKopf) {
    // DAS IST DER GRUND FUER DEN KOPF, als Test formuliert: hdr_percentiles_print kennt nur die
    // AUFGEZEICHNETEN Proben. Sein eigener "Total count"-Fuss meldet 372 -- wer nur ihn liest,
    // haelt 372 fuer die Grundgesamtheit, obwohl 780 vorgelegt wurden. Die Luecke ist mehr als die
    // Haelfte. Sie ist NUR aus dem Kopf erkennbar.
    auto const                  v    = gewuerfelte_stichprobe();
    auto const                  dir  = frisches_testverzeichnis("verteilung");
    std::filesystem::path const ziel = dir / "lauf.hdr.hgrm";

    ASSERT_TRUE(auswertung::hdr_lauf_persistieren(as_span(v), ziel, "gewuerfelter-lauf"));
    std::string const inhalt = datei_lesen(ziel);

    std::int64_t verteilung_n = -1, kopf_vorgelegt = -1;
    ASSERT_TRUE(verteilung_gesamtzahl(inhalt, verteilung_n)) << "die Verteilung selbst fehlt in der Datei";
    ASSERT_TRUE(kopf_zahl(inhalt, "vorgelegt", kopf_vorgelegt));

    std::cout << "  Verteilung zaehlt " << verteilung_n << ", vorgelegt waren " << kopf_vorgelegt << " -- Differenz "
              << (kopf_vorgelegt - verteilung_n) << " (nur aus dem Kopf sichtbar)\n";

    EXPECT_EQ(verteilung_n, kProbenEcht) << "der vendorierte Fuss zaehlt die aufgezeichneten Proben";
    EXPECT_EQ(kopf_vorgelegt, kVorgelegt);
    EXPECT_GT(kopf_vorgelegt, verteilung_n) << "ohne diese Differenz waere der Test blind -- die Stichprobe muss "
                                               "ueberhaupt Proben verlieren, sonst beweist er nichts";

    std::filesystem::remove_all(dir);
}

TEST(D55HdrPersistenz, PersistenzMeldetFehlschlagStattStillZuScheitern) {
    // T-4 GEGENEINGANG zur Zusicherung "gibt true zurueck": ein Ziel, das NICHT anlegbar ist. Der
    // Elternpfad ist hier eine REGULAERE DATEI -- create_directories bricht mit ENOTDIR ab. Meldete
    // die Funktion hier true, waere jede spaetere "persistiert"-Zeile wertlos.
    auto const dir = frisches_testverzeichnis("fehlschlag");
    std::filesystem::create_directories(dir);
    std::filesystem::path const blocker = dir / "ich_bin_eine_datei";
    {
        std::ofstream os(blocker);
        os << "keine Verzeichnis\n";
    }
    ASSERT_TRUE(std::filesystem::is_regular_file(blocker));

    auto const v = gewuerfelte_stichprobe();
    EXPECT_FALSE(auswertung::hdr_lauf_persistieren(as_span(v), blocker / "unter" / "lauf.hdr.hgrm", "unmoeglich"));

    // GEGENPROBE (beide Richtungen, K13): derselbe Aufruf mit gueltigem Ziel MUSS true liefern --
    // sonst koennte die Zusicherung konstant false sein und der Test oben waere ein Scheinbeweis.
    std::filesystem::path const gut = dir / "geht" / "lauf.hdr.hgrm";
    EXPECT_TRUE(auswertung::hdr_lauf_persistieren(as_span(v), gut, "moeglich"));
    EXPECT_TRUE(std::filesystem::exists(gut));

    std::filesystem::remove_all(dir);
}

// == (8) DER FALL, IN DEM DER KANON SELBST 0 ns IST ================================================
TEST(D55HdrToleranzUrteil, KanonNullUndHdrNichtNullIstKEINEToleranz) {
    // GEFUNDEN BEIM BAU, nicht vermutet: bei der gewuerfelten Stichprobe (400 Nullen unter 780) ist
    // der Kanon-p50 exakt 0 ns -- er sortiert die ROHWERTE mitsamt der Nullen. HDR wirft die Nullen
    // vorher weg und liefert ~464895 ns. Die beiden sind sich also maximal uneinig.
    //
    // Bis zu diesem Test meldete das Urteil hier "in Toleranz", weil abweichung_rel mangels
    // Division 0 blieb und 0 <= 0.0009765625 gilt. Das ist die stille Verzerrung eine Ebene ueber
    // dem 0-ns-Zaehler: das Messgeraet war richtig, es zeigte nur auf nichts.
    auto const v = gewuerfelte_stichprobe();

    auswertung::HdrAuswertung const a = auswertung::hdr_auswerten(as_span(v));

    std::cout << "  p50: kanon=" << a.p50.kanon_ns << " ns  hdr=" << a.p50.hdr_ns
              << " ns  vergleichbar=" << (a.p50.vergleichbar ? 1 : 0) << " in_toleranz=" << (a.p50.in_toleranz ? 1 : 0)
              << "  (von " << a.vorgelegt << " Proben waren " << a.verworfen_null << " null)\n";

    ASSERT_EQ(a.p50.kanon_ns, 0) << "die Stichprobe muss den Kanon ueberhaupt auf 0 druecken, sonst prueft der Test "
                                    "etwas anderes als er behauptet";
    ASSERT_GT(a.p50.hdr_ns, 0) << "und HDR muss ungleich 0 liefern, sonst waeren sie zu Recht einig";

    EXPECT_FALSE(a.p50.vergleichbar) << "ohne Division ist die relative Abweichung nicht definiert";
    EXPECT_FALSE(a.p50.in_toleranz) << "0 ns gegen " << a.p50.hdr_ns << " ns darf NIE als 'in Toleranz' durchgehen";

    // GEGENPROBE 1 (K13, andere Richtung): p95 liegt oberhalb der Nullen, ist reguler vergleichbar
    // und MUSS in der Toleranz bleiben -- sonst waere die Wache konstant rot statt stellenbezogen.
    EXPECT_TRUE(a.p95.vergleichbar) << "p95 liegt ueber den Nullen und ist normal vergleichbar";
    EXPECT_TRUE(a.p95.in_toleranz) << "p95 darf nicht mitrot werden";
}

TEST(D55HdrToleranzUrteil, LEERE_StichprobeBleibtEinigUndVergleichbar) {
    // GEGENPROBE 2: der Fall, den der alte Kommentar gemeint hat. Kanon 0 UND HDR 0 -- beide sagen
    // dasselbe. Hier waere ein Befund ein FEHLALARM, und der Test haelt fest, dass keiner kommt.
    std::vector<std::int64_t> const leer;

    auswertung::HdrAuswertung const a = auswertung::hdr_auswerten(as_span(leer));

    EXPECT_EQ(a.vorgelegt, 0);
    EXPECT_EQ(a.p50.kanon_ns, 0);
    EXPECT_EQ(a.p50.hdr_ns, 0);
    EXPECT_TRUE(a.p50.vergleichbar) << "0 gegen 0 ist eine Uebereinstimmung, kein undefinierter Fall";
    EXPECT_TRUE(a.p50.in_toleranz) << "eine leere Stichprobe darf keinen Befund erzeugen";
}

TEST(D55HdrPersistenz, PersistierteKanonUndHdrZahlenStimmenMitDerAuswertungUeberein) {
    // T-5: das Orakel ist NICHT die Datei, sondern hdr_auswerten auf derselben Stichprobe. Driftete
    // der Schreiber von der Rechnung ab, faellt genau das hier auf.
    auto const                  v    = gewuerfelte_stichprobe();
    auto const                  dir  = frisches_testverzeichnis("zahlen");
    std::filesystem::path const ziel = dir / "lauf.hdr.hgrm";

    ASSERT_TRUE(auswertung::hdr_lauf_persistieren(as_span(v), ziel, "gewuerfelter-lauf"));
    std::string const                 inhalt = datei_lesen(ziel);
    auswertung::HdrAuswertung const   a      = auswertung::hdr_auswerten(as_span(v));

    for (auto const& [name, paar] : {std::pair{"p50", a.p50}, std::pair{"p95", a.p95}, std::pair{"p99", a.p99}}) {
        std::string const marke = "#[" + std::string{name} + " kanon_ns=" + std::to_string(paar.kanon_ns) +
                                  " hdr_ns=" + std::to_string(paar.hdr_ns) + " ";
        EXPECT_NE(inhalt.find(marke), std::string::npos)
            << name << ": die Datei traegt nicht dieselben Zahlen wie die Auswertung; erwartet '" << marke << "'";
    }

    std::filesystem::remove_all(dir);
}
