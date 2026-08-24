// test_s13_03_per_binary_mappe.cpp -- S13-03 (#18/D-1, KON32-01 "nur per Binary xlsx"):
// per-Binary-Ergebnis-Mappe + RESUME-VERTRAG (result.csv+stamp AUSSERHALB des Ziel-Filters).
//
// WAS HIER BEWIESEN WIRD:
//   (Z) ZAEHL-ABNAHME (Design 4.1 woertlich): N Binaries => N Mappen + 1 Aggregat-Mappe -- am
//       Fassaden-Sink (mach_per_binary_mappe_sink) + Aggregat-Naht, gezaehlt am DATEISYSTEM.
//   (K1) KOEDER 1: ein Nur-xlsx-Lauf laesst den Resume-Vertrag INTAKT -- lazy_try_resume_binary
//        findet den Wiederanlauf, obwohl KEINE Auswerte-CSV im bin_dir liegt. Eine Fassung, die
//        result.csv unter den Filter zieht, wird hier ROT.
//   (K2/K3) KOEDER 2+3 als BYTE-WACHE: der per-Binary-Mappen-Sink laesst result.csv und
//        result.csv.stamp BYTE-IDENTISCH liegen (K3: die Auswerte-CSV -- das __S001-Kind --
//        ueberschreibt den Vertrag nie; K2: der measurement_sink-Kanal, der exakt diese Datei
//        reicht, verliert nichts).
//   (R) RESUME-POSITIVPROBE N->N+1 am Arbiter: N=2 zertifizierte Staende resumieren; der
//       abgebrochene dritte NICHT (kein Stamp); nach dem "Nachmessen" (Vertrag geschrieben)
//       resumieren alle 3 -- der Wiederanlauf setzt bei N+1 auf, literal.
//
// WARUM AM SINK UND AM ARBITER STATT AN EINEM VOLLEN MESS-LAUF: der Iterator ruft den Sink an
// der per-Binary-Synchron-Naht NUR fuer real GEMESSENE Binaries (per_binary_csv non-empty) --
// ein echter Mess-Lauf braucht ladbare Tier-DLLs und gehoert ins Mess-Fenster (CI), nicht in
// die Unit-Ebene. Sink (die S13-03-Mechanik) und Arbiter (der Vertrag) sind hier beide DIREKT
// unter Beweis; die Verdrahtung cfg.per_binary_mappe ist an BEIDEN Seams eine Zeile neben
// cfg.measurement_sink (dessen Durchreichung test_t2a_f4 fuer die Fassade belegt).
//
// T-11c-PROTOKOLL: Wegwerf-Mutationen mit literalem Rot am 2026-08-21 -- Protokoll im
// Strang-Ergebnis (20260820-w2-sofortstaffel/s13-schema-kette-ergebnis.md).

#include <gtest/gtest.h>

#include <experiment_tree/cache_engine_builder_iterator.hpp> // lazy_try_resume_binary + kLazyResumeRowsKey
#include <profile_facade/ergebnis_mappe_naht.hpp>            // MappenNaht + mach_per_binary_mappe_sink

#include <atomic>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace naht = comdare::cache_engine::lager_naht;
namespace lab  = comdare::cache_engine::builder::lager_ablage;
namespace ex   = comdare::cache_engine::builder::experiment;
namespace fs   = std::filesystem;

namespace {

struct TempDir {
    fs::path path;
    TempDir() {
        static std::atomic<unsigned> counter{0};
        path = fs::temp_directory_path() /
               ("s13_03_perbin_" + std::to_string(counter.fetch_add(1)) + "_" + std::to_string(::time(nullptr)));
        fs::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

std::string read_file(fs::path const& p) {
    std::ifstream is(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
}

void write_file(fs::path const& p, std::string const& inhalt) {
    std::ofstream os(p, std::ios::trunc | std::ios::binary);
    os << inhalt;
}

/// Rekursive Zaehlung der Dateien mit Endung -- die MENGE ist die Zusicherung (Zaehl-Abnahme).
std::size_t zaehl_endung_rekursiv(fs::path const& wurzel, std::string const& endung) {
    std::size_t n = 0;
    for (auto const& e : fs::recursive_directory_iterator(wurzel)) {
        if (!e.is_regular_file()) continue;
        auto const name = e.path().filename().string();
        if (name.size() >= endung.size() && name.compare(name.size() - endung.size(), endung.size(), endung) == 0) ++n;
    }
    return n;
}

/// Schreibt den RESUME-VERTRAG eines bin_dir exakt in der Form des Iterators (result.csv =
/// lazy_csv_header + N Zeilen; result.csv.stamp = <prefix>|rows=<N>). Der Header kommt aus
/// DERSELBEN Quelle wie beim Arbiter (lazy_csv_header) -- Header-Identitaet ist Teil des Vertrags.
void schreibe_resume_vertrag(fs::path const& bin_dir, std::string const& stamp_prefix,
                             std::vector<std::string> const& zeilen) {
    fs::create_directories(bin_dir);
    std::string csv = ex::lazy_csv_header();
    for (auto const& z : zeilen) csv += z;
    write_file(bin_dir / "result.csv", csv);
    write_file(bin_dir / "result.csv.stamp",
               stamp_prefix + std::string{ex::kLazyResumeRowsKey} + std::to_string(zeilen.size()) + "\n");
}

constexpr char kKopf[] = "binary_id;setting;op_lat_ns\n";

} // namespace

// (Z) -- die Zaehl-Abnahme: N=3 per-Binary-Sinks + 1 Aggregat-Naht => 4 Mappen im Baum.
TEST(S1303PerBinary, DreiBinariesErgebenDreiMappenPlusEinAggregat) {
    TempDir const tmp;

    auto sink = naht::mach_per_binary_mappe_sink({"xlsx"}, "testhost");
    for (int i = 0; i < 3; ++i) {
        fs::path const bin_dir = tmp.path / ("bin" + std::to_string(i));
        fs::create_directories(bin_dir);
        sink(bin_dir, "bid" + std::to_string(i), kKopf,
             "b" + std::to_string(i) + ";s;100\n"); // EIN Blob je Binary, wie der Iterator ihn reicht
    }

    // Das Aggregat: dieselbe Naht, die die Seams fahren (Stamm im Lauf-Ordner).
    naht::MappenNaht aggregat;
    aggregat.oeffnen(tmp.path / "measurements.csv", {"xlsx"});
    ASSERT_TRUE(aggregat.scharf()) << aggregat.diagnose();
    aggregat.kopf_aus_csv(kKopf);
    aggregat.blob_aus_csv("b0;s;100\nb1;s;100\nb2;s;100\n");
    ASSERT_TRUE(aggregat.schliessen(lab::MaschinenSysinfo{}, {}, {})) << aggregat.diagnose();

    EXPECT_EQ(zaehl_endung_rekursiv(tmp.path, ".xlsx"), 4u)
        << "N Binaries muessen N Mappen + 1 Aggregat-Mappe ergeben (KON32-01) -- N=3.";
}

// (K1) -- Koeder 1: der Resume-Vertrag ueberlebt den Nur-xlsx-Lauf.
TEST(S1303PerBinary, ResumeArbiterFindetDenVertragAuchOhneAuswerteCsv) {
    TempDir const     tmp;
    fs::path const    bin_dir = tmp.path / "bin0";
    std::string const prefix  = "resume-v6|test-koeder-1";
    schreibe_resume_vertrag(bin_dir, prefix, {"b0;s1;100\n", "b0;s2;200\n"});

    // Der Nur-xlsx-Sink laeuft ueber demselben bin_dir -- er darf den Vertrag nicht anfassen.
    auto sink = naht::mach_per_binary_mappe_sink({"xlsx"}, "testhost");
    sink(bin_dir, "bid0", kKopf, "b0;s1;100\nb0;s2;200\n");

    EXPECT_EQ(zaehl_endung_rekursiv(bin_dir, "__S001.csv"), 0u)
        << "Nur-xlsx: im bin_dir darf KEINE Auswerte-CSV liegen (der Filter gilt auch per Binary).";
    EXPECT_EQ(zaehl_endung_rekursiv(bin_dir, ".xlsx"), 1u) << "Die per-Binary-Mappe muss liegen.";

    std::string rows;
    EXPECT_TRUE(ex::lazy_try_resume_binary(bin_dir, prefix, &rows))
        << "KOEDER 1 (Design-Abnahme woertlich): ein Nur-xlsx-Profil, nach dem lazy_try_resume_binary "
           "keinen Wiederanlauf mehr findet, ist ROT -- result.csv+stamp sind RESUME-VERTRAG und "
           "stehen AUSSERHALB des Ziel-Filters.";
    EXPECT_EQ(rows, "b0;s1;100\nb0;s2;200\n") << "Der Arbiter muss die Alt-Zeilen unversehrt liefern.";
}

// (K2/K3) -- die Byte-Wache ueber dem Vertrag: der Sink laesst result.csv+stamp byte-identisch.
TEST(S1303PerBinary, DerSinkLaesstResultCsvUndStampByteIdentisch) {
    TempDir const     tmp;
    fs::path const    bin_dir = tmp.path / "bin0";
    std::string const prefix  = "resume-v6|test-koeder-2-3";
    schreibe_resume_vertrag(bin_dir, prefix, {"b0;s1;100\n"});
    std::string const csv_vorher   = read_file(bin_dir / "result.csv");
    std::string const stamp_vorher = read_file(bin_dir / "result.csv.stamp");
    ASSERT_FALSE(csv_vorher.empty());

    // BEIDE Formate gewaehlt: das __S001-Kind (die per-Binary-AUSWERTE-CSV) entsteht im SELBEN
    // Ordner wie der Vertrag -- genau die Kollisionslage, gegen die Koeder 3 gerichtet ist.
    auto sink = naht::mach_per_binary_mappe_sink({"csv", "xlsx"}, "testhost");
    sink(bin_dir, "bid0", kKopf, "b0;s1;100\n");

    EXPECT_EQ(zaehl_endung_rekursiv(bin_dir, "__S001.csv"), 1u) << "Die Auswerte-CSV (Kind) muss liegen.";
    EXPECT_EQ(read_file(bin_dir / "result.csv"), csv_vorher)
        << "KOEDER 3: die Auswerte-CSV darf den Resume-Vertrag NIE ueberschreiben (Namens-Trennung).";
    EXPECT_EQ(read_file(bin_dir / "result.csv.stamp"), stamp_vorher)
        << "Auch der Stamp ist Vertragsbestandteil und bleibt unberuehrt.";

    // KOEDER 2 in seiner pruefbaren Form: der measure-drop-Kanal reicht exakt result.csv weiter
    // (Iterator: cfg.measurement_sink(rcsv, ...) hinter einem exists()-Gate). Liegt die Datei
    // byte-identisch da, hat der Kanal nichts verloren -- ein Sink, der sie entfernte/ersetzte,
    // waere oben bereits ROT geworden.
    std::string rows;
    EXPECT_TRUE(ex::lazy_try_resume_binary(bin_dir, prefix, &rows));
}

// (R) -- die Positivprobe N->N+1 am Arbiter: Abbruch nach N, Wiederanlauf misst NUR N+1 nach.
TEST(S1303PerBinary, WiederanlaufSetztBeiNPlusEinsAuf) {
    TempDir const     tmp;
    std::string const prefix = "resume-v6|test-n-plus-1";

    // Lauf 1 (abgebrochen): bin0+bin1 zertifiziert, bin2 ANGEFANGEN (result.csv ohne Stamp --
    // exakt der Zustand, den der Iterator bei nicht zertifizierbarem Stand hinterlaesst).
    schreibe_resume_vertrag(tmp.path / "bin0", prefix, {"b0;s1;100\n"});
    schreibe_resume_vertrag(tmp.path / "bin1", prefix, {"b1;s1;110\n"});
    fs::create_directories(tmp.path / "bin2");
    write_file(tmp.path / "bin2" / "result.csv", ex::lazy_csv_header() + "b2;s1;120\n");

    std::string rows;
    EXPECT_TRUE(ex::lazy_try_resume_binary(tmp.path / "bin0", prefix, &rows)) << "bin0 muss resumieren (N).";
    EXPECT_TRUE(ex::lazy_try_resume_binary(tmp.path / "bin1", prefix, &rows)) << "bin1 muss resumieren (N).";
    EXPECT_FALSE(ex::lazy_try_resume_binary(tmp.path / "bin2", prefix, &rows))
        << "bin2 hat KEINEN Stamp -- ein Resume waere die Uebernahme eines unzertifizierten Standes.";

    // Wiederanlauf: NUR bin2 wird nachgemessen (der Vertrag wird geschrieben) -- danach N+1 komplett.
    schreibe_resume_vertrag(tmp.path / "bin2", prefix, {"b2;s1;120\n"});
    std::size_t resumierbar = 0;
    for (int i = 0; i < 3; ++i)
        if (ex::lazy_try_resume_binary(tmp.path / ("bin" + std::to_string(i)), prefix, &rows)) ++resumierbar;
    EXPECT_EQ(resumierbar, 3u) << "Nach dem Nachmessen von N+1 muessen ALLE N+1 Staende resumieren.";

    // Und der FALSCHE Lauf (anderer Config-Praefix) darf NICHTS davon uebernehmen -- Gegeneingang.
    EXPECT_FALSE(ex::lazy_try_resume_binary(tmp.path / "bin0", "resume-v6|ANDERE-config", &rows))
        << "Ein abweichender Stempel-Praefix heisst: ehrliche Neu-Messung, kein staler Uebertrag.";
}

// ---------------------------------------------------------------------------------------------
// #137 / N-12 (par.27.1.J EP-1, 24.08.2026) -- .stale-RETTUNG AUF DEM ERFOLGS-PFAD.
// ROT-ZUERST: dieser Test wurde VOR dem Helfer kompiliert (Rot-Protokoll
// fixlogs/fix6-ep1-rot.log: 'lazy_stale_rettung_vor_write' undeklariert); GRUEN erst mit
// Helfer + Verdrahtung. Die .stale-Mechanik existierte NUR im Fehlzweig (:2807-Muster);
// auf dem Erfolgs-Pfad ueberschrieb ios::trunc einen liegenden Alt-Stand ersatzlos.
// ---------------------------------------------------------------------------------------------
TEST(S1303PerBinary, StaleRettungAufDemErfolgspfadRettetAltstandByteGleich) {
    TempDir    tmp;
    auto const bin_dir = tmp.path / "bin0";
    fs::create_directories(bin_dir);
    std::string const alt = ex::lazy_csv_header() + "alt;riss;42\n";
    write_file(bin_dir / "result.csv", alt);
    // (1) Alt-Stand liegt -> Rettung: Zielplatz frei, .stale traegt den Alt-Stand BYTE-GLEICH.
    EXPECT_TRUE(ex::lazy_stale_rettung_vor_write(bin_dir / "result.csv"));
    EXPECT_FALSE(fs::exists(bin_dir / "result.csv")) << "Zielplatz muss nach der Rettung frei sein.";
    EXPECT_EQ(read_file(bin_dir / "result.csv.stale"), alt)
        << "Rohdatum: gerettet wird byte-gleich, nie geloescht (Messdaten-Dauerregel).";
    // (2) Kein Alt-Stand -> true, und es entsteht NICHTS (die .stale von (1) bleibt unberuehrt).
    EXPECT_TRUE(ex::lazy_stale_rettung_vor_write(bin_dir / "result.csv"));
    EXPECT_EQ(read_file(bin_dir / "result.csv.stale"), alt);
}

// VERDRAHTUNGS-WACHE (t15-Muster: die Quelle wird gelesen): die Rettung muss auf dem
// ERFOLGS-Pfad AUFGERUFEN sein -- die blosse Definition (1 Fundstelle des Aufruf-Tokens)
// traegt nicht. Biss-Probe: Aufruf im Iterator entfernen -> dieser Test faellt.
TEST(S1303PerBinary, StaleRettungIstAufDemErfolgspfadVerdrahtet) {
    std::ifstream quelle{COMDARE_S1303_ITERATOR_QUELLE};
    ASSERT_TRUE(quelle.is_open()) << "Iterator-Quelle nicht lesbar: " COMDARE_S1303_ITERATOR_QUELLE;
    std::string const text{std::istreambuf_iterator<char>(quelle), std::istreambuf_iterator<char>()};
    std::size_t       n = 0;
    for (std::size_t pos = 0; (pos = text.find("lazy_stale_rettung_vor_write(", pos)) != std::string::npos; ++pos) ++n;
    EXPECT_GE(n, 2u) << "#137: Definition + mindestens ein Aufruf auf dem Erfolgs-Pfad erwartet.";
}
