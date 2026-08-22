// test_s13_01_csv_kind_projektion.cpp -- S13-01 (#18/D-1, Owner 09.08. 16:31): die offizielle CSV
// ist KIND der Mappe -- eine PROJEKTION des Stamms, kein zweiter Schreibweg.
//
// WAS HIER BEWIESEN WIRD (an der Naht selbst; die Seam-Wirkung beweist
// test_s13_02_zielfilter_vier_faelle am echten run_profile):
//   (A) BYTE-IDENTITAET: die Projektion formt aus Header/Blob/Zeilen EXAKT die Bytes, die der
//       entfernte rohe Strom (csv << header; csv << blob; csv << row...) geschrieben haette --
//       das ist die Zusatz-Probe "golden-CSV byte-identisch vor/nach dem Umbau" des Designs,
//       heruntergebrochen auf ihr Orakel (die Zeilen aendern sich nicht, nur ihr Weg).
//   (B) ZIEL-FILTER: ohne csv-Token entsteht die Datei NICHT (projektion_oeffnen = No-op true).
//   (C) DESIGN-KOEDER (Abnahme S13-01 woertlich): "eine Fassung, die bei ErgebnisSchreibFehler
//       trotzdem eine CSV zuruecklaesst, wird ROT" -- der Stamm-Bruch (LXW_STR_MAX-Ausloeser aus
//       A9S5Ablehnung, Vendor-Vertrag) ENTFERNT die bereits angefangene Datei und macht
//       projektion_ok() UND schliessen() false.
//   (D) SCOPE-FESTLEGUNG (S13-02 Folgewirkung 2): die Projektion traegt GENAU 1 Kopf +
//       zeilen() angenommene Ergebnis-Zeilen -- eine kuenftige Profil-Blatt-Speisung
//       (MessEbenenSchluessel, S13-09) briche diese Zaehlung und wuerde ROT.
//
// T-11c-PROTOKOLL: je Naht-Eigenschaft eine Wegwerf-Mutation mit literalem Rot -- gefahren am
// 2026-08-21, Protokoll im Strang-Ergebnis (20260820-w2-sofortstaffel/s13-schema-kette-ergebnis.md).

#include <gtest/gtest.h>

#include <profile_facade/ergebnis_mappe_naht.hpp>

#include <atomic>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace naht = comdare::cache_engine::lager_naht;
namespace lab  = comdare::cache_engine::builder::lager_ablage;
namespace fs   = std::filesystem;

namespace {

struct TempDir {
    fs::path path;
    TempDir() {
        static std::atomic<unsigned> counter{0};
        path = fs::temp_directory_path() /
               ("s13_01_proj_" + std::to_string(counter.fetch_add(1)) + "_" + std::to_string(::time(nullptr)));
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

std::size_t zeilen_in(std::string const& s) {
    std::size_t n = 0;
    for (char const c : s)
        if (c == '\n') ++n;
    return n;
}

// Der Kopf und die Zeilen dieses Tests tragen bewusst LEERE Felder und ein Feld mit fuehrenden/
// abschliessenden Leerzeichen -- genau die Formen, an denen ein nachlaessiger Split/Join die Bytes
// verschieben wuerde (die Wache im Feld-Adapter der Naht nennt das den Stellvertreter-Fehler).
constexpr char kKopf[] = "binary_id;setting;op_lat_ns\n";

} // namespace

// (A) -- das Byte-Orakel der golden-Gleichheit: derselbe Eingang, EXAKT dieselben Bytes.
TEST(S1301Projektion, ByteIdentischZumEntferntenRohenStrom) {
    TempDir const  tmp;
    fs::path const ziel = tmp.path / "measurements.csv";

    // Der Eingang: Header + resumierter Blob (2 Zeilen, je genau EIN '\n', keine Leerzeile --
    // die Form, die lazy_try_resume_binary garantiert) + 2 frische Zeilen.
    std::string const blob     = "b0;s1;100\nb1; s2 ;200\n";
    std::string const zeile3   = "b2;;300\n"; // LEERES Mittelfeld -- muss Byte fuer Byte erhalten bleiben
    std::string const zeile4   = "b3;s4;\n";  // LEERES Schlussfeld -- dito
    std::string const erwartet = std::string{kKopf} + blob + zeile3 + zeile4;

    naht::MappenNaht n;
    n.oeffnen(ziel, {"csv"});
    ASSERT_TRUE(n.scharf()) << n.diagnose();
    ASSERT_TRUE(n.projektion_oeffnen(ziel)) << n.diagnose();
    n.kopf_aus_csv(kKopf);
    n.blob_aus_csv(blob);
    n.zeile_aus_csv(zeile3);
    n.zeile_aus_csv(zeile4);
    ASSERT_TRUE(n.schliessen(lab::MaschinenSysinfo{}, {}, {})) << n.diagnose();
    ASSERT_TRUE(n.projektion_ok()) << n.diagnose();

    ASSERT_TRUE(fs::exists(ziel)) << "Die offizielle CSV fehlt: " << n.diagnose();
    EXPECT_EQ(read_file(ziel), erwartet)
        << "Die Projektion muss BYTE-IDENTISCH zum entfernten rohen Strom sein -- sonst ist die "
           "golden-Zusage des Umbaus gebrochen.";
}

// (B) -- der Ziel-Filter an der offiziellen CSV selbst (S13-02-Haelfte der Naht).
TEST(S1301Projektion, NurXlsxLegtKeineOffizielleCsvAn) {
    TempDir const  tmp;
    fs::path const ziel = tmp.path / "measurements.csv";

    naht::MappenNaht n;
    n.oeffnen(ziel, {"xlsx"});
    ASSERT_TRUE(n.scharf()) << n.diagnose();
    EXPECT_TRUE(n.projektion_oeffnen(ziel)) << "Ohne csv-Token ist der Aufruf ein No-op mit true.";
    n.kopf_aus_csv(kKopf);
    n.zeile_aus_csv("b0;s1;100\n");
    ASSERT_TRUE(n.schliessen(lab::MaschinenSysinfo{}, {}, {})) << n.diagnose();
    EXPECT_TRUE(n.projektion_ok()) << "Nichts verlangt heisst: nichts fehlgeschlagen.";
    EXPECT_FALSE(fs::exists(ziel)) << "S13-02-Koeder: die NICHT deklarierte offizielle CSV darf nie entstehen.";
}

// (C) -- der benannte Design-Koeder: Stamm-Bruch laesst KEINE CSV zurueck, und der Fehlschlag ist
// an ALLEN drei Auskuenften ablesbar (projektion_ok, schliessen, Datei-Abwesenheit).
TEST(S1301Projektion, StammBruchEntferntDieBegonneneCsvUndMeldetFalse) {
    TempDir const  tmp;
    fs::path const ziel = tmp.path / "measurements.csv";

    naht::MappenNaht n;
    n.oeffnen(ziel, {"csv"});
    ASSERT_TRUE(n.scharf()) << n.diagnose();
    ASSERT_TRUE(n.projektion_oeffnen(ziel)) << n.diagnose();
    n.kopf_aus_csv(kKopf);
    n.zeile_aus_csv("b0;s1;100\n"); // wird angenommen UND projiziert

    // Der ECHTE Vendor-Ausloeser (Muster A9S5Ablehnung): Spalte A > LXW_STR_MAX (32767) bringt den
    // Stamm zu Fall -- ab hier ist jede Projektion eine Luege.
    std::string const zu_lang(32768, 'a');
    n.zeile_aus_csv(zu_lang + ";s2;200\n");
    n.zeile_aus_csv("b2;s3;300\n"); // trifft auf eine Mappe, die nicht mehr annimmt -> verworfen

    EXPECT_FALSE(n.scharf());
    EXPECT_EQ(n.verworfen(), 2u);
    EXPECT_FALSE(fs::exists(ziel))
        << "DER KOEDER: eine Fassung, die bei ErgebnisSchreibFehler trotzdem eine CSV zuruecklaesst, "
           "ist ROT (D-2(2): bricht die xlsx-Erzeugung, ist auch die csv ungueltig).";

    EXPECT_FALSE(n.schliessen(lab::MaschinenSysinfo{}, {}, {}))
        << "schliessen() muss den Verlust der VERLANGTEN offiziellen CSV als false melden.";
    EXPECT_FALSE(n.projektion_ok());
    EXPECT_FALSE(fs::exists(ziel)) << "Auch nach schliessen() darf keine Datei liegen.";
    EXPECT_NE(n.diagnose().find("offizielle CSV"), std::string::npos)
        << "Der Grund muss BENANNT sein, war: " << n.diagnose();
}

// (D) -- die Scope-Zaehl-Wache: Projektion == 1 Kopf + genau die ANGENOMMENEN Ergebnis-Zeilen.
TEST(S1301Projektion, ScopeGenauKopfPlusAngenommeneErgebniszeilen) {
    TempDir const  tmp;
    fs::path const ziel = tmp.path / "measurements.csv";

    naht::MappenNaht n;
    n.oeffnen(ziel, {"csv", "xlsx"});
    ASSERT_TRUE(n.scharf()) << n.diagnose();
    ASSERT_TRUE(n.projektion_oeffnen(ziel)) << n.diagnose();
    n.kopf_aus_csv(kKopf);
    n.blob_aus_csv("b0;s1;100\nb1;s2;200\nb2;s3;300\n");
    ASSERT_TRUE(n.schliessen(lab::MaschinenSysinfo{}, {}, {})) << n.diagnose();

    std::string const inhalt = read_file(ziel);
    EXPECT_EQ(n.zeilen(), 3u);
    EXPECT_EQ(zeilen_in(inhalt), 1u + n.zeilen())
        << "SCOPE-KOEDER (S13-02 Folgewirkung 2): die Projektion speist sich AUSSCHLIESSLICH aus dem "
           "Ergebnis-Blatt -- jede fremde Zeile (kuenftige Profil-Blaetter, S13-09-Drain) briche diese "
           "Zaehlung. Inhalt war:\n"
        << inhalt;
}
