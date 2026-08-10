// test_w0b_durchstich_kette -- ##25 DURCHSTICH: die kleinste EHRLICHE Kette, als Google Test.
//                                                                            (2026-08-10)
// =================================================================================================
// WARUM ES DIESE DATEI GIBT
//
// Owner 09.08.2026 14:34 (Rohtranskript, promptSource=typed): "Ich sehe einen Haufen shells statt
// vernuenftiger google tests, was soll das? ... Skripte sagen gar nichts." Die Durchstich-Deckung
// lag bis heute ausschliesslich in Shell (ci/durchstich_wache.sh + ci/durchstich_bissprobe.sh im
// super-Repo). Diese Datei ist die C++-Form fuer den Teil der Kette, den das ce BESITZT:
//
//     LazyMeasuredRow -> format_csv_row() -> MappenNaht -> .xlsx UND .csv auf Platte
//                     -> LaTeX-Tabelle -> pdflatex -> PDF
//
// DER KOEDER (K13) IST EIN LAUF-TOKEN, NICHT EINE KONSTANTE: er wird je Lauf aus /dev/urandom
// gewuerfelt und WOERTLICH am Ende der Kette zurueckgefordert. Eine Kette ohne wiedergefundenen
// Token ist eine Behauptung. Der Gegenkoeder (ein um EIN Zeichen veraenderter Token) darf NIRGENDS
// gefunden werden -- sonst misst die Suche nicht den Transport, sondern sich selbst.
//
// WARUM DER TOKEN NUR AUS GROSSBUCHSTABEN BESTEHT -- am Objekt gemessen, nicht vermutet:
// im PDF ueberlebt der Token die Setzerei NICHT woertlich. Gemessen 10.08.2026 mit pdflatex
// (TeX Live 2026) und dem kerning-schweren Token "DSTKNAVAWLTPATAVAYAWO":
//     /usr/bin/grep -c -F "$TOK" mini2.pdf          -> 0     (TeX zerlegt an jedem Kern-Paar)
//     tr -cd 'A-Za-z' < mini2.pdf | grep -c -F      -> 1     (nach Normalisierung wiedergefunden)
// Eine naive Literal-Suche im PDF waere also ein FALSCH-ROT gewesen. Deshalb normalisiert
// pdf_nur_buchstaben() die PDF-Bytes auf [A-Za-z], bevor gesucht wird; und deshalb enthaelt der
// Token keine Ziffern (die Kern-Zahlen zwischen den Textstuecken sind Ziffern und wuerden nach der
// Normalisierung mit einem Ziffern-Token verschmelzen) und keine Kleinbuchstaben (ff/fi/fl/ffi/ffl
// sind Ligaturen -- ein Hex-Token wie "...ff..." wuerde zu EINEM Glyph und waere unauffindbar).
//
// DIE VIER GLIEDER, jedes mit Nenner (T-3, aus einer FREMDEN Quelle) und Gegeneingang (T-4):
//
//   G1 PROFIL-INVENTAR   Nenner: die getrackten *.profile.xml unter algorithm_profiles/thesis_profiles
//                        (Dateisystem, nicht der Pruefling). Zusicherung: jedes Profil, das FORMAT-
//                        Token deklariert, hat xlsx darunter. Gegeneingang: eine Liste {"csv"} MUSS
//                        xlsx=false liefern -- die Wache kann also nein sagen.
//   G2 SCHEMA-NAHT       Nenner: die Spaltenzahl aus lazy_csv_header() -- ein ANDERER Erzeuger als
//                        format_csv_row(). Zusicherung: Kopf und Zeile haben dieselbe Zellzahl, und
//                        MappenNaht::feld_abweichungen() ist 0. Gegeneingang: eine Zeile mit
//                        gestohlenem Feld MUSS die Abweichung hochzaehlen.
//   G3 FORMAT -> PLATTE  Nenner: FormatWahl::ausgaben() (1 oder 2, nie 0) gegen die Zahl der wirklich
//                        entstandenen Dateien. Zusicherung: der Token steht WOERTLICH in der
//                        entpackten sharedStrings/sheet-XML der .xlsx UND in der Sheet-CSV, und beide
//                        tragen denselben Stamm. Gegeneingang: die UNGEHEILTE csv-only-Liste erzeugt
//                        KEINE .xlsx -- das ist der stehende Regressionswaechter der Bruchstelle.
//   G4 TEX -> PDF        Nenner: Zeilen rein == Zeilen raus (latex_anhang::parse_csv), und
//                        pdflatex-rc gegen die erwartete 0. Zusicherung: der Token steht im PDF.
//                        Gegeneingang: eine syntaktisch kaputte .tex MUSS rc != 0 liefern, und der
//                        Gegenkoeder darf im PDF NICHT auftauchen.
//
// V-8 (Gegenstand statt Ankuendigung): keine Zusicherung dieser Datei lautet "die Datei existiert"
// oder "der Aufruf wirft nicht". Geprueft werden BYTES auf der Platte (ZIP-Zentralverzeichnis,
// entpackter XML-Inhalt, PDF-Bytes), Mengen (Dateizahl je Endung, Feldzahl) und Rueckgabewerte.
//
// FAIL-CLOSED beim PDF-Glied: fehlt pdflatex, ist das GRUEN NUR, wenn niemand behauptet hat, es sei
// da. COMDARE_DURCHSTICH_PDFLATEX wird von CMake per find_program gesetzt; ist es NICHT leer und die
// Binary trotzdem unbrauchbar, ist das ROT. Ist es leer (Bau-Image ohne TeX), meldet der Test das MIT
// Nenner und ueberspringt -- und COMDARE_DURCHSTICH_PDF_PFLICHT=1 macht daraus ein hartes Rot, ohne
// dass jemand diese Datei anfassen muss.
//
// SELBSTCHECK: ASCII-only, Zeilen <= 120 Byte, deutsche Kommentare, keine Shell-Probe.
// =================================================================================================

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>

#include "ergebnis_mappe_naht.hpp"
#include "latex_anhang.hpp"
#include "xml_config_parser.hpp"

#include <zlib.h>

#include <unistd.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ex   = comdare::cache_engine::builder::experiment;
namespace naht = comdare::cache_engine::lager_naht;
namespace lab  = comdare::cache_engine::builder::lager_ablage;
namespace cx   = comdare::builder::xml;

namespace {

// -------------------------------------------------------------------------------------------------
// 0. WERKZEUG -- Wuerfel, Dateien, ZIP
// -------------------------------------------------------------------------------------------------

/// Der Lauf-Token: "DSTKN" + n Grossbuchstaben aus /dev/urandom. NIE abgeschrieben (K13).
/// Grossbuchstaben-only ist keine Kosmetik, sondern das Ergebnis der PDF-Messung im Kopf dieser Datei.
[[nodiscard]] std::string wuerfle_token(std::size_t n_buchstaben = 18) {
    std::ifstream urandom{"/dev/urandom", std::ios::binary};
    EXPECT_TRUE(urandom.good()) << "/dev/urandom nicht lesbar -- ohne Wuerfel kein Koeder (K13)";
    std::string t = "DSTKN";
    while (t.size() < n_buchstaben + 5) {
        unsigned char b = 0;
        urandom.read(reinterpret_cast<char*>(&b), 1);
        if (!urandom) break;
        t += static_cast<char>('A' + (b % 26));
    }
    return t;
}

/// Der GEGENKOEDER: derselbe Token mit EINEM veraenderten Buchstaben. Er wird nirgends eingespeist
/// und darf folglich nirgends gefunden werden -- das trennt "transportiert" von "die Suche findet
/// alles" (K13 beidseitig).
[[nodiscard]] std::string gegen_token(std::string t) {
    if (t.empty()) return t;
    char& c = t[t.size() / 2];
    c       = (c == 'Z') ? 'A' : static_cast<char>(c + 1);
    return t;
}

struct TempDir {
    std::filesystem::path pfad;
    TempDir() {
        static std::atomic<unsigned> zaehler{0};
        pfad = std::filesystem::temp_directory_path() /
               ("w0b_durchstich_" + std::to_string(zaehler.fetch_add(1)) + "_" + std::to_string(::getpid()) + "_" +
                std::to_string(static_cast<long long>(::time(nullptr))));
        std::filesystem::create_directories(pfad);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(pfad, ec);
    }
    TempDir(TempDir const&)            = delete;
    TempDir& operator=(TempDir const&) = delete;
};

[[nodiscard]] std::string lies_datei(std::filesystem::path const& p) {
    std::ifstream is(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
}

/// Alle regulaeren Dateien mit dieser Endung -- die MENGE ist der Nenner, nicht die Anwesenheit einer
/// einzelnen Datei.
[[nodiscard]] std::vector<std::filesystem::path> dateien_mit_endung(std::filesystem::path const& dir,
                                                                    std::string_view             endung) {
    std::vector<std::filesystem::path> out;
    std::error_code                    ec;
    for (auto const& e : std::filesystem::directory_iterator(dir, ec)) {
        if (!e.is_regular_file()) continue;
        if (e.path().extension().string() == endung) out.push_back(e.path());
    }
    std::sort(out.begin(), out.end());
    return out;
}

[[nodiscard]] std::uint32_t le32(std::string const& b, std::size_t off) {
    if (off + 4 > b.size()) return 0;
    return static_cast<std::uint32_t>(static_cast<unsigned char>(b[off])) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(b[off + 1])) << 8) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(b[off + 2])) << 16) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(b[off + 3])) << 24);
}

[[nodiscard]] std::uint16_t le16(std::string const& b, std::size_t off) {
    if (off + 2 > b.size()) return 0;
    return static_cast<std::uint16_t>(static_cast<unsigned char>(b[off]) |
                                      (static_cast<unsigned char>(b[off + 1]) << 8));
}

/// Roher DEFLATE-Strom (ohne zlib-Kopf) -> Klartext. -MAX_WBITS ist genau der raw-Modus, den ZIP
/// verwendet. Rueckgabe leer = nicht entpackbar (der Aufrufer macht daraus ein ROT, kein Schweigen).
[[nodiscard]] std::string inflate_raw(std::string const& komprimiert, std::size_t erwartet) {
    if (komprimiert.empty()) return {};
    std::string aus;
    aus.resize(erwartet == 0 ? (komprimiert.size() * 8 + 1024) : erwartet);
    z_stream zs{};
    if (inflateInit2(&zs, -MAX_WBITS) != Z_OK) return {};
    zs.next_in                    = reinterpret_cast<Bytef*>(const_cast<char*>(komprimiert.data()));
    zs.avail_in                   = static_cast<uInt>(komprimiert.size());
    zs.next_out                   = reinterpret_cast<Bytef*>(aus.data());
    zs.avail_out                  = static_cast<uInt>(aus.size());
    int const         rc          = inflate(&zs, Z_FINISH);
    std::size_t const geschrieben = aus.size() - zs.avail_out;
    inflateEnd(&zs);
    if (rc != Z_STREAM_END && rc != Z_OK && rc != Z_BUF_ERROR) return {};
    aus.resize(geschrieben);
    return aus;
}

/// Der GESAMTE Klartext-Inhalt eines ZIP (und eine .xlsx IST ein ZIP): ueber das ZENTRALVERZEICHNIS,
/// nicht ueber die lokalen Kopfsaetze -- letztere duerfen bei Data-Descriptor-Eintraegen Nullen in
/// den Groessenfeldern tragen. Rueckgabe: Verkettung aller entpackten Eintraege; leer = kein ZIP.
[[nodiscard]] std::string zip_klartext(std::string const& roh, std::size_t& eintraege) {
    eintraege = 0;
    if (roh.size() < 22) return {};
    // EOCD "PK\5\6" von hinten suchen (Kommentar am Dateiende ist erlaubt, daher rueckwaerts).
    std::size_t eocd = std::string::npos;
    for (std::size_t i = roh.size() - 22 + 1; i-- > 0;) {
        if (roh.compare(i, 4, "PK\x05\x06", 4) == 0) {
            eocd = i;
            break;
        }
    }
    if (eocd == std::string::npos) return {};
    std::uint16_t const n_cd  = le16(roh, eocd + 10);
    std::uint32_t const cd_at = le32(roh, eocd + 16);
    std::string         alles;
    std::size_t         p = cd_at;
    for (std::uint16_t i = 0; i < n_cd; ++i) {
        if (p + 46 > roh.size() || roh.compare(p, 4, "PK\x01\x02", 4) != 0) break;
        std::uint16_t const methode  = le16(roh, p + 10);
        std::uint32_t const gr_komp  = le32(roh, p + 20);
        std::uint32_t const gr_klar  = le32(roh, p + 24);
        std::uint16_t const len_name = le16(roh, p + 28);
        std::uint16_t const len_ext  = le16(roh, p + 30);
        std::uint16_t const len_kom  = le16(roh, p + 32);
        std::uint32_t const lfh      = le32(roh, p + 42);
        p += 46u + len_name + len_ext + len_kom;
        if (lfh + 30 > roh.size() || roh.compare(lfh, 4, "PK\x03\x04", 4) != 0) continue;
        std::uint16_t const l_name = le16(roh, lfh + 26);
        std::uint16_t const l_ext  = le16(roh, lfh + 28);
        std::size_t const   daten  = lfh + 30u + l_name + l_ext;
        if (daten + gr_komp > roh.size()) continue;
        std::string const nutz = roh.substr(daten, gr_komp);
        ++eintraege;
        if (methode == 0) {
            alles += nutz; // gespeichert
        } else if (methode == 8) {
            alles += inflate_raw(nutz, gr_klar);
        }
    }
    return alles;
}

/// PDF-Bytes auf [A-Za-z] eindampfen. Begruendung siehe Kopf: TeX zerlegt Text an Kern-Paaren in
/// mehrere Textstuecke; erst nach dieser Normalisierung steht ein gesetzter Grossbuchstaben-Token
/// wieder zusammenhaengend da.
[[nodiscard]] std::string pdf_nur_buchstaben(std::string const& roh) {
    std::string s;
    s.reserve(roh.size() / 2 + 16);
    for (char const c : roh) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) s += c;
    }
    return s;
}

[[nodiscard]] std::size_t zaehle_vorkommen(std::string const& heu, std::string const& nadel) {
    if (nadel.empty()) return 0;
    std::size_t n = 0;
    for (std::size_t p = heu.find(nadel); p != std::string::npos; p = heu.find(nadel, p + 1)) ++n;
    return n;
}

[[nodiscard]] std::vector<std::string> semikolon_felder(std::string_view z) { return naht::csv_zeile_in_felder(z); }

/// Der Pfad des getrackten Profil-Verzeichnisses -- von CMake gesetzt, damit der Test nicht raet.
[[nodiscard]] std::filesystem::path profil_verzeichnis() {
#ifdef COMDARE_DURCHSTICH_PROFIL_DIR
    return std::filesystem::path{COMDARE_DURCHSTICH_PROFIL_DIR};
#else
    return {};
#endif
}

/// ZWEITES Profil-Verzeichnis, wenn es eines gibt: das ce wird vom super als Unterprojekt gebaut, und
/// dort liegt unter Code/experiment_config/thesis_profiles das F1-Durchstich-Profil -- die Datei, die
/// den Mini-Lauf der F1-Lieferung tatsaechlich konfiguriert. CMake setzt das Define NUR, wenn das
/// Verzeichnis zur Configure-Zeit existiert (V-8: der Gegenstand entscheidet, nicht ein Schalter).
/// Leer bedeutet: das ce wird allein gebaut, dieses Inventar existiert hier nicht.
[[nodiscard]] std::filesystem::path profil_verzeichnis_2() {
#ifdef COMDARE_DURCHSTICH_PROFIL_DIR2
    return std::filesystem::path{COMDARE_DURCHSTICH_PROFIL_DIR2};
#else
    return {};
#endif
}

[[nodiscard]] std::string pdflatex_pfad() {
#ifdef COMDARE_DURCHSTICH_PDFLATEX
    return std::string{COMDARE_DURCHSTICH_PDFLATEX};
#else
    return {};
#endif
}

/// Eine EINZELNE Mess-Zeile, die den Token in der Identitaets-Spalte binary_id traegt. Sie geht durch
/// den PRODUKTIONS-Renderer format_csv_row() -- der Test baut die Zeile NICHT selbst zusammen.
[[nodiscard]] ex::LazyMeasuredRow mess_zeile_mit_token(std::string const& token, std::uint64_t n_ops) {
    ex::LazyMeasuredRow row;
    row.binary_id     = token;
    row.setting_label = "durchstich.rep=1";
    row.setting_id    = token + "#durchstich.rep=1";
    row.n_ops         = n_ops;
    row.timed_ops     = n_ops;
    row.total_ns      = static_cast<std::int64_t>(n_ops) * 7;
    row.profile_name  = "f1_durchstich";
    return row;
}

} // namespace

// =================================================================================================
// G1 -- PROFIL-INVENTAR: die Bruchstelle. xlsx ist Standard, also darf kein Profil ihn wegdeklarieren.
// =================================================================================================
//
// DER NENNER IST FREMD (T-3): er kommt aus dem Dateisystem (alle getrackten *.profile.xml unter
// libs/cache_engine/algorithm_profiles/thesis_profiles) und aus dem PRODUKTIONS-Parser
// XmlConfigParser::parse_thesis_profile -- nicht aus waehle_ergebnis_format(), dem Pruefling.
//
// OWNER-KERN, mehrfach belegt: "xlsx ist der Standard und CSV ist waehlbar" / "Es existiert kein CSV
// Lager. xlsx ist default." Wer FORMAT-Token aufzaehlt, darf den Standard nicht stillschweigend
// abwaehlen -- csv NEBEN xlsx ist seit dem Entscheid 09.08. ausdruecklich erlaubt (FormatLage::beide).
TEST(W0bDurchstichKette, G1_JedesProfilMitFormatTokenTraegtXlsx) {
    auto const dir = profil_verzeichnis();
    ASSERT_FALSE(dir.empty()) << "COMDARE_DURCHSTICH_PROFIL_DIR nicht gesetzt -- die Wache kann nicht "
                                 "pruefen, also ist sie ROT (fail-closed).";
    ASSERT_TRUE(std::filesystem::is_directory(dir)) << "Profil-Verzeichnis fehlt: " << dir.string();

    auto              alle   = dateien_mit_endung(dir, ".xml");
    std::size_t const n_dir1 = alle.size();
    // Zweites Inventar, falls das ce als Unterprojekt des super gebaut wird (dort liegt das
    // F1-Durchstich-Profil). Fehlt es, bleibt der Nenner ehrlich bei einem Verzeichnis.
    auto const  dir2   = profil_verzeichnis_2();
    std::size_t n_dirs = 1;
    if (!dir2.empty() && std::filesystem::is_directory(dir2)) {
        ++n_dirs;
        for (auto const& p : dateien_mit_endung(dir2, ".xml")) alle.push_back(p);
    }
    std::printf("[G1] Inventare: verzeichnisse=%zu xml_in_1=%zu xml_gesamt=%zu (dir1=%s dir2=%s)\n", n_dirs, n_dir1,
                alle.size(), dir.string().c_str(), dir2.empty() ? "<nicht gesetzt>" : dir2.string().c_str());
    std::fflush(stdout);
    ASSERT_GT(alle.size(), 0U) << "0 Profile gefunden -- ein leerer Nenner ist kein gruener Nenner.";

    cx::XmlConfigParser parser;

    std::size_t              n_gesamt     = alle.size();
    std::size_t              n_geparst    = 0;
    std::size_t              n_deklariert = 0; // Profile MIT mindestens einem FORMAT-Token
    std::size_t              n_mit_xlsx   = 0;
    std::vector<std::string> ohne_xlsx;

    for (auto const& p : alle) {
        auto const tp = parser.parse_thesis_profile(p);
        if (!tp.has_value()) continue; // kein Thesis-Profil-Kanal (z.B. comdare_experiment) -- kein Fehler
        ++n_geparst;
        auto const w = naht::waehle_ergebnis_format(tp->writeback_methods);
        if (w.format_token == 0) continue; // nichts deklariert -> Default xlsx, nicht Teil der Frage
        ++n_deklariert;
        if (w.xlsx) {
            ++n_mit_xlsx;
        } else {
            ohne_xlsx.push_back(p.filename().string() + " [" + w.diagnose() + "]");
        }
    }

    std::string liste;
    for (auto const& s : ohne_xlsx) liste += "\n    " + s;
    // V-1: die Zahl steht nie nackt da -- jede Stufe druckt ihren Nenner.
    std::printf("[G1] Profil-Inventar: xml_gesamt=%zu thesis_geparst=%zu mit_format_token=%zu "
                "davon_xlsx=%zu ohne_xlsx=%zu\n",
                n_gesamt, n_geparst, n_deklariert, n_mit_xlsx, ohne_xlsx.size());
    std::fflush(stdout);

    EXPECT_EQ(n_mit_xlsx, n_deklariert)
        << "Owner-KERN verletzt: " << ohne_xlsx.size() << " von " << n_deklariert
        << " Profilen mit FORMAT-Token deklarieren xlsx NICHT -- fuer sie entsteht keine Mappe auf "
           "Platte, und der Durchstich (CSV -> persist -> xlsx -> anhang -> PDF) hat kein zweites "
           "Glied. Betroffen:"
        << liste;
}

// GEGENEINGANG zu G1 (T-4): die Wache muss NEIN sagen koennen. Waere sie blind, waere G1 auch dann
// gruen, wenn kein einziges Profil xlsx traegt.
TEST(W0bDurchstichKette, G1_Gegeneingang_CsvOnlyLiefertKeinXlsx) {
    auto const nur_csv = naht::waehle_ergebnis_format({"csv"});
    EXPECT_FALSE(nur_csv.xlsx) << "csv-only muesste xlsx abwaehlen: " << nur_csv.diagnose();
    EXPECT_TRUE(nur_csv.csv);
    EXPECT_EQ(nur_csv.lage, naht::FormatLage::gewaehlt);
    EXPECT_EQ(nur_csv.ausgaben(), 1U);

    auto const beide = naht::waehle_ergebnis_format({"csv", "xlsx"});
    EXPECT_TRUE(beide.xlsx);
    EXPECT_TRUE(beide.csv);
    EXPECT_EQ(beide.lage, naht::FormatLage::beide);
    EXPECT_EQ(beide.ausgaben(), 2U);

    // Kanal-Token sind KEINE Format-Token: eine reine Kanal-Liste faellt auf den xlsx-Default zurueck.
    auto const nur_kanal = naht::waehle_ergebnis_format({"latex_table", "comparison_metrics"});
    EXPECT_TRUE(nur_kanal.xlsx);
    EXPECT_EQ(nur_kanal.lage, naht::FormatLage::default_leer);
    EXPECT_EQ(nur_kanal.format_token, 0U);
    EXPECT_EQ(nur_kanal.kanal_token, 2U);
}

// =================================================================================================
// G2 -- SCHEMA-NAHT: N Spalten rein, N Spalten raus, 0 verloren.
// =================================================================================================
TEST(W0bDurchstichKette, G2_KopfUndZeileHabenDieselbeZellzahl) {
    std::string const kopf  = ex::lazy_csv_header();
    auto const        k_fld = semikolon_felder(kopf);
    auto const        zeile = ex::format_csv_row(mess_zeile_mit_token(wuerfle_token(), 128));
    auto const        z_fld = semikolon_felder(zeile);

    std::printf("[G2] Schema-Naht: kopf_spalten=%zu zeilen_felder=%zu differenz=%td\n", k_fld.size(), z_fld.size(),
                static_cast<std::ptrdiff_t>(z_fld.size()) - static_cast<std::ptrdiff_t>(k_fld.size()));
    std::fflush(stdout);

    ASSERT_GT(k_fld.size(), 20U) << "Ein Kopf mit weniger als 20 Spalten ist kein WIDE-Schema.";
    EXPECT_EQ(z_fld.size(), k_fld.size())
        << "Halb nachgezogener END-Append: eine Spalte im Kopf ohne Zelle verschoebe ALLE folgenden "
           "Werte der Auswertung -- der teuerste stille Fehler dieser Kette.";
}

// GEGENEINGANG zu G2: eine Zeile mit gestohlenem Feld MUSS als Abweichung gezaehlt werden. Ohne
// diesen Fall belegte G2 nur, dass zwei Zahlen zufaellig gleich sind.
TEST(W0bDurchstichKette, G2_Gegeneingang_GestohlenesFeldWirdGezaehlt) {
    TempDir           tmp;
    std::string const kopf  = ex::lazy_csv_header();
    std::string       zeile = ex::format_csv_row(mess_zeile_mit_token(wuerfle_token(), 64));
    // Genau EIN Trennzeichen entfernen -> eine Zelle weniger, alles danach rutscht.
    auto const pos = zeile.find(';');
    ASSERT_NE(pos, std::string::npos);
    zeile.erase(pos, 1);

    naht::MappenNaht m;
    m.oeffnen(tmp.pfad / "roh.csv", {"csv"});
    ASSERT_TRUE(m.scharf()) << m.diagnose();
    m.kopf_aus_csv(kopf);
    m.zeile_aus_csv(zeile);
    std::printf("[G2-gegen] feld_abweichungen=%zu von zeilen=%zu (kopf_spalten=%zu)\n", m.feld_abweichungen(),
                m.zeilen(), m.kopf_spalten());
    std::fflush(stdout);
    EXPECT_EQ(m.feld_abweichungen(), 1U) << "Die Naht haette die verschobene Zeile melden muessen.";
    (void)m.schliessen(lab::MaschinenSysinfo{}, {}, {});
}

// =================================================================================================
// G3 -- FORMAT -> PLATTE: der Token muss in BEIDEN Ausgaben derselben Mappe stehen.
// =================================================================================================
TEST(W0bDurchstichKette, G3_TokenStehtInXlsxUndInCsvDerselbenMappe) {
    TempDir           tmp;
    std::string const token = wuerfle_token();
    std::string const gegen = gegen_token(token);
    ASSERT_NE(token, gegen);

    naht::MappenNaht m;
    m.oeffnen(tmp.pfad / "roh.csv", {"csv", "xlsx"}); // der GEHEILTE Eingang
    ASSERT_TRUE(m.scharf()) << m.diagnose();
    ASSERT_EQ(m.wahl().ausgaben(), 2U) << m.diagnose();

    m.kopf_aus_csv(ex::lazy_csv_header());
    m.zeile_aus_csv(ex::format_csv_row(mess_zeile_mit_token(token, 4096)));
    // ziele() VOR schliessen() lesen: schliessen() leert die Ausgaben-Liste. Der erste Lauf dieses
    // Tests druckte "ziele=" leer -- eine hohle Ausgabe ist genau der Stellvertreter, den V-1 verbietet.
    std::string const ziele_vor_schliessen = m.ziele();
    ASSERT_FALSE(ziele_vor_schliessen.empty()) << "Eine ungenannte Datei ist eine unbeobachtete Datei.";
    ASSERT_TRUE(m.schliessen(lab::MaschinenSysinfo{}, {}, {})) << m.diagnose();

    auto const xlsx_dateien = dateien_mit_endung(tmp.pfad, ".xlsx");
    // Die csv-Strategie legt einen ORDNER mit flachen Sheet-Dateien an (OV-17: die Struktur der
    // Lagerhaltung ist Pflicht) -- deshalb wird rekursiv gesucht.
    std::vector<std::filesystem::path> csv_dateien;
    std::error_code                    ec;
    for (auto const& e : std::filesystem::recursive_directory_iterator(tmp.pfad, ec)) {
        if (e.is_regular_file() && e.path().extension() == ".csv") csv_dateien.push_back(e.path());
    }
    std::printf("[G3] Ausgaben laut Wahl=%zu | xlsx_auf_platte=%zu csv_auf_platte=%zu | ziele=%s\n",
                m.wahl().ausgaben(), xlsx_dateien.size(), csv_dateien.size(), ziele_vor_schliessen.c_str());
    std::fflush(stdout);

    ASSERT_EQ(xlsx_dateien.size(), 1U) << "Die xlsx-Strategie hat nichts hinterlassen: " << m.diagnose();
    ASSERT_GE(csv_dateien.size(), 1U) << "Die csv-Strategie hat nichts hinterlassen: " << m.diagnose();

    // (a) EINE Mappe, zwei Ausgaben: gemeinsamer Stamm. Zwei Laeufe haetten zwei Zeitstempel.
    std::string const stamm          = xlsx_dateien[0].stem().string();
    bool              stamm_gefunden = false;
    for (auto const& c : csv_dateien) {
        if (c.filename().string().rfind(stamm, 0) == 0) stamm_gefunden = true;
    }
    EXPECT_TRUE(stamm_gefunden) << "xlsx-Stamm '" << stamm
                                << "' taucht in keiner Sheet-CSV auf -- dann "
                                   "stammen die beiden Ausgaben nicht aus DERSELBEN Mappe.";

    // (b) GEGENSTAND statt Ankuendigung: die .xlsx wird ENTPACKT und der Token im Klartext gesucht.
    std::string const roh = lies_datei(xlsx_dateien[0]);
    ASSERT_GE(roh.size(), 4U);
    EXPECT_EQ(roh.compare(0, 2, "PK"), 0) << "Keine ZIP-Signatur -- das ist keine xlsx.";
    std::size_t       eintraege = 0;
    std::string const klar      = zip_klartext(roh, eintraege);
    std::printf("[G3] xlsx: bytes=%zu zip_eintraege=%zu klartext_bytes=%zu\n", roh.size(), eintraege, klar.size());
    std::fflush(stdout);
    ASSERT_GT(eintraege, 0U) << "Zentralverzeichnis leer -- die Mappe traegt keinen Inhalt.";
    EXPECT_EQ(zaehle_vorkommen(klar, token), 1U) << "Der Lauf-Token steht NICHT in der entpackten xlsx.";
    EXPECT_EQ(zaehle_vorkommen(klar, gegen), 0U) << "Der Gegenkoeder wurde in der xlsx gefunden -- die "
                                                    "Suche misst sich selbst, nicht den Transport.";

    // (c) dieselbe Zeile in der Sheet-CSV.
    std::size_t treffer_csv = 0;
    std::size_t gegen_csv   = 0;
    for (auto const& c : csv_dateien) {
        std::string const inhalt = lies_datei(c);
        treffer_csv += zaehle_vorkommen(inhalt, token);
        gegen_csv += zaehle_vorkommen(inhalt, gegen);
    }
    EXPECT_EQ(treffer_csv, 1U) << "Der Lauf-Token steht NICHT in der Sheet-CSV derselben Mappe.";
    EXPECT_EQ(gegen_csv, 0U) << "Der Gegenkoeder wurde in der Sheet-CSV gefunden.";
}

// GEGENEINGANG zu G3 -- der STEHENDE REGRESSIONSWAECHTER der Bruchstelle: der UNGEHEILTE, csv-only
// deklarierte Eingang darf keine .xlsx erzeugen. Genau diesen Zustand hatten am 10.08.2026 alle
// getrackten Thesis-Profile (siehe G1).
TEST(W0bDurchstichKette, G3_Gegeneingang_CsvOnlyErzeugtKeineXlsx) {
    TempDir           tmp;
    std::string const token = wuerfle_token();

    naht::MappenNaht m;
    m.oeffnen(tmp.pfad / "roh.csv", {"csv"});
    ASSERT_TRUE(m.scharf()) << m.diagnose();
    EXPECT_EQ(m.wahl().ausgaben(), 1U);
    m.kopf_aus_csv(ex::lazy_csv_header());
    m.zeile_aus_csv(ex::format_csv_row(mess_zeile_mit_token(token, 32)));
    (void)m.schliessen(lab::MaschinenSysinfo{}, {}, {});

    auto const xlsx_dateien = dateien_mit_endung(tmp.pfad, ".xlsx");
    std::printf("[G3-gegen] csv-only: ausgaben=%zu xlsx_auf_platte=%zu (erwartet 0)\n", m.wahl().ausgaben(),
                xlsx_dateien.size());
    std::fflush(stdout);
    EXPECT_EQ(xlsx_dateien.size(), 0U) << "csv-only hat trotzdem eine xlsx erzeugt -- dann traegt G1 keine "
                                          "Aussage mehr ueber den Lauf.";
}

// =================================================================================================
// G4 -- CSV -> LaTeX -> PDF. Das letzte Glied, an dem der Token wieder auftauchen muss.
// =================================================================================================
//
// WELCHE CSV: tools/latex_anhang ist das EINZIGE tex-erzeugende Werkzeug im ce. Es liest das
// NARROW-/pipeline16-Schema (Komma-getrennt, Spalte permutation_id) -- NICHT das WIDE-Schema aus
// lazy_csv_header (Semikolon, Spalte binary_id). Die Naht zwischen beiden Schemata liegt im super
// (Code/04_csv_to_latex, Code/08_appendix_generator) und ist hier NICHT gedeckt; die Messung dieser
// Luecke steht in G5 und ist dort mit Nenner ausgewiesen.
namespace {

/// Schreibt eine NARROW-CSV mit n Zeilen; jede traegt den Token in permutation_id.
void schreibe_narrow_csv(std::filesystem::path const& p, std::string const& token, std::size_t n) {
    std::ofstream os{p};
    os << "permutation_id,fingerprint,succeeded,workload_used,op_count,total_cycles,cache_misses_l1,"
          "cache_misses_l2,cache_misses_l3,dtlb_misses,coherence_invalidations,energy_micro_joules,"
          "bytes_allocated,bytes_in_use_peak,external_frag,internal_frag\n";
    for (std::size_t i = 0; i < n; ++i) {
        os << token << ",4711,1,ycsb_a," << (1000 + i) << ",2222,3,4,5,6,7,8,9,10,0.5,0.25\n";
    }
}

/// Baut das PDF im Verzeichnis der .tex. Rueckgabe: Prozess-rc von pdflatex (nie ein geratener Wert).
[[nodiscard]] int baue_pdf(std::filesystem::path const& dir, std::string const& tex_name) {
    std::string const cmd = "cd " + dir.string() + " && " + pdflatex_pfad() +
                            " -interaction=nonstopmode -halt-on-error " + tex_name + " > pdflatex.log 2>&1";
    int const         rc  = std::system(cmd.c_str());
    return rc;
}

void schreibe_wrapper_tex(std::filesystem::path const& p, std::string const& tabelle_rel, bool kaputt) {
    std::ofstream os{p};
    os << "\\documentclass[10pt]{article}\n";
    os << "\\usepackage{booktabs}\n";
    // Ohne diese beiden Zeilen komprimiert pdflatex die Inhalts-Streams -- dann ist der Token auch
    // nach der Normalisierung nicht auffindbar, und das Glied waere ein Falsch-Rot.
    os << "\\pdfcompresslevel=0\n\\pdfobjcompresslevel=0\n";
    os << "\\begin{document}\n";
    if (kaputt) os << "\\begin{tabular}{l}\n"; // GEGENEINGANG: nie geschlossen -> Uebersetzer bricht
    os << "\\input{" << tabelle_rel << "}\n";
    os << "\\end{document}\n";
}

} // namespace

TEST(W0bDurchstichKette, G4_TokenUeberlebtCsvTabellePdf) {
    std::string const pdflatex = pdflatex_pfad();
    if (pdflatex.empty()) {
        char const* pflicht = std::getenv("COMDARE_DURCHSTICH_PDF_PFLICHT");
        bool const  hart    = (pflicht != nullptr && std::string_view{pflicht} == "1");
        std::printf("[G4] PDF-Gate: pdflatex_gefunden=0 von 1 erwartet, pflicht=%d\n", hart ? 1 : 0);
        std::fflush(stdout);
        if (hart) {
            FAIL() << "COMDARE_DURCHSTICH_PDF_PFLICHT=1, aber CMake hat kein pdflatex gefunden -- "
                      "fail-closed: eine Wache, die nicht pruefen kann, ist ROT.";
        }
        GTEST_SKIP() << "pdflatex nicht im Bau-Image (0 von 1). Das PDF-Glied ist hier UNGEDECKT und "
                        "wird es mit COMDARE_DURCHSTICH_PDF_PFLICHT=1 hart.";
    }

    TempDir           tmp;
    std::string const token  = wuerfle_token();
    std::string const gegen  = gegen_token(token);
    std::size_t const n_rein = 3;

    auto const csv = tmp.pfad / "messwerte.csv";
    schreibe_narrow_csv(csv, token, n_rein);

    std::vector<latex_anhang::CsvRow> zeilen;
    ASSERT_EQ(latex_anhang::parse_csv(csv.string(), zeilen), latex_anhang::status_ok);
    // V-1: N Zeilen rein, N Zeilen raus, 0 verloren -- nie "die Datei ist da".
    std::printf("[G4] CSV: zeilen_rein=%zu zeilen_geparst=%zu verloren=%zu\n", n_rein, zeilen.size(),
                n_rein - zeilen.size());
    std::fflush(stdout);
    ASSERT_EQ(zeilen.size(), n_rein);

    auto const tabelle = tmp.pfad / "tabelle.tex";
    ASSERT_EQ(latex_anhang::write_latex(tabelle.string(), zeilen, "Durchstich F1", "tab:durchstich"),
              latex_anhang::status_ok);
    std::string const tex_inhalt = lies_datei(tabelle);
    EXPECT_EQ(zaehle_vorkommen(tex_inhalt, token), n_rein) << "Der Token ging zwischen CSV und .tex verloren.";
    EXPECT_EQ(zaehle_vorkommen(tex_inhalt, gegen), 0U);

    auto const doc = tmp.pfad / "durchstich.tex";
    schreibe_wrapper_tex(doc, "tabelle", false);
    int const  rc  = baue_pdf(tmp.pfad, "durchstich.tex");
    auto const pdf = tmp.pfad / "durchstich.pdf";
    std::printf("[G4] pdflatex rc=%d pdf_vorhanden=%d\n", rc, std::filesystem::exists(pdf) ? 1 : 0);
    std::fflush(stdout);
    ASSERT_EQ(rc, 0) << "pdflatex-Log:\n" << lies_datei(tmp.pfad / "pdflatex.log");
    ASSERT_TRUE(std::filesystem::exists(pdf));

    std::string const pdf_roh = lies_datei(pdf);
    ASSERT_EQ(pdf_roh.compare(0, 5, "%PDF-"), 0) << "Keine PDF-Signatur.";
    std::string const norm    = pdf_nur_buchstaben(pdf_roh);
    std::size_t const treffer = zaehle_vorkommen(norm, token);
    std::size_t const g_treff = zaehle_vorkommen(norm, gegen);
    std::printf("[G4] PDF: bytes=%zu buchstaben=%zu token_treffer=%zu gegenkoeder_treffer=%zu (erwartet %zu/0)\n",
                pdf_roh.size(), norm.size(), treffer, g_treff, n_rein);
    std::fflush(stdout);
    EXPECT_EQ(treffer, n_rein) << "Der gewuerfelte Lauf-Token kam nicht im PDF an -- ein Durchstich ohne "
                                  "wiedergefundenen Token ist eine Behauptung.";
    EXPECT_EQ(g_treff, 0U) << "Der Gegenkoeder wurde im PDF gefunden -- die Normalisierung findet alles.";
}

// GEGENEINGANG zu G4: eine syntaktisch kaputte .tex MUSS das Gate rot machen. Ohne diesen Fall
// belegte G4 nur, dass pdflatex ueberhaupt startet.
TEST(W0bDurchstichKette, G4_Gegeneingang_KaputteTexBrichtDasGate) {
    if (pdflatex_pfad().empty()) {
        GTEST_SKIP() << "pdflatex nicht im Bau-Image (0 von 1) -- Gegeneingang nicht fahrbar.";
    }
    TempDir           tmp;
    std::string const token = wuerfle_token();
    auto const        csv   = tmp.pfad / "messwerte.csv";
    schreibe_narrow_csv(csv, token, 1);
    std::vector<latex_anhang::CsvRow> zeilen;
    ASSERT_EQ(latex_anhang::parse_csv(csv.string(), zeilen), latex_anhang::status_ok);
    ASSERT_EQ(latex_anhang::write_latex((tmp.pfad / "tabelle.tex").string(), zeilen, "x", "tab:x"),
              latex_anhang::status_ok);

    schreibe_wrapper_tex(tmp.pfad / "kaputt.tex", "tabelle", true);
    int const rc = baue_pdf(tmp.pfad, "kaputt.tex");
    std::printf("[G4-gegen] kaputte tex: pdflatex rc=%d (erwartet != 0)\n", rc);
    std::fflush(stdout);
    EXPECT_NE(rc, 0) << "Eine nie geschlossene tabular-Umgebung ging durch -- dann ist das PDF-Gate blind.";
}

// =================================================================================================
// G5 -- DIE BENANNTE LUECKE: WIDE (Mess-Kette) und NARROW (tex-Werkzeug) sind heute DISJUNKT.
// =================================================================================================
//
// Das ist kein Defekt DIESES Pakets, sondern die ehrliche Vermessung der Stelle, an der die ce-Kette
// aufhoert: die Umsetzung WIDE -> Anhang-Tabelle liegt im super (Code/04_csv_to_latex,
// Code/08_appendix_generator). Der Test friert den IST-Zustand MIT NENNER ein. Wird die Naht eines
// Tages im ce gezogen, faellt dieser Test auf -- und zwingt zum Nachlesen statt zum Vergessen.
TEST(W0bDurchstichKette, G5_WideUndNarrowSchemaSindHeuteDisjunkt) {
    std::string const kopf = ex::lazy_csv_header();
    // Die 16 Pflichtspalten des NARROW-Lesers -- abgeschrieben aus tools/latex_anhang/main.cpp:55-70
    // (FREMDE Quelle: nicht aus dem WIDE-Erzeuger abgeleitet).
    char const*       narrow[] = {"permutation_id",
                                  "fingerprint",
                                  "succeeded",
                                  "workload_used",
                                  "op_count",
                                  "total_cycles",
                                  "cache_misses_l1",
                                  "cache_misses_l2",
                                  "cache_misses_l3",
                                  "dtlb_misses",
                                  "coherence_invalidations",
                                  "energy_micro_joules",
                                  "bytes_allocated",
                                  "bytes_in_use_peak",
                                  "external_frag",
                                  "internal_frag"};
    std::size_t const n_narrow = sizeof(narrow) / sizeof(narrow[0]);

    auto const  wide_felder = semikolon_felder(kopf);
    std::size_t getroffen   = 0;
    for (char const* name : narrow) {
        for (auto const& f : wide_felder) {
            if (f == name) {
                ++getroffen;
                break;
            }
        }
    }
    std::size_t const kommas = zaehle_vorkommen(kopf, ",");
    std::printf("[G5] Schema-Luecke: wide_spalten=%zu narrow_pflichtspalten=%zu davon_im_wide=%zu "
                "kommas_im_wide_kopf=%zu\n",
                wide_felder.size(), n_narrow, getroffen, kommas);
    std::fflush(stdout);

    EXPECT_EQ(getroffen, 0U) << "WIDE und NARROW ueberschneiden sich jetzt -- die im Kopf dieser Datei "
                                "beschriebene Luecke hat sich veraendert und muss neu bewertet werden.";
    EXPECT_EQ(kommas, 0U) << "Der WIDE-Kopf traegt jetzt Kommas -- der NARROW-Leser (split_csv_line auf "
                             "',') wuerde ihn anders zerlegen als gedacht.";
}
