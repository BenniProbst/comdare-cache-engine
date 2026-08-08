// test_a9s3_mess_ebene_blattsorte -- A9-S3 Fassung 3 (Owner-Entscheid 08.08. + Nachtrag): die
// compare/Macro/Micro-Blattsorte. Belegt real: (a) interne Hyperlinks mit dem vendorierten Stand,
// (b)+(c) sind KEINE Code-Fragen mehr -- sie wurden vor dem Bau am bestehenden Mess-Code recherchiert
// (Befund im Bericht) und sind hier NICHT nachgestellt, weil der Writer die Daten nur ENTGEGENNIMMT.
//
// Die Prozess-/Thread-/Zeitpunkt-Werte in diesen Tests sind SYNTHETISCH und ALS SOLCHE benannt (Praefix
// "SYNTH_") -- sie belegen die SCHREIB-Mechanik (Sheet-Zaehlung, IN/OUT-Paarung, Hyperlink-Aufloesung),
// NICHT eine reale Messung. Die Befund-Kette dazu steht im Bau-Bericht: Prozess+Thread-Identitaet
// erreicht diesen Writer heute an KEINER Stelle der bestehenden Pipeline.

#include "lager_ablage/ergebnis_mappe.hpp"

#include <zlib.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace lab = comdare::cache_engine::builder::lager_ablage;

namespace {

struct TempDir {
    std::filesystem::path path;
    TempDir() {
        static std::atomic<unsigned> counter{0};
        path = std::filesystem::temp_directory_path() /
               ("a9s3_ebene_" + std::to_string(counter.fetch_add(1)) + "_" + std::to_string(::time(nullptr)));
        std::filesystem::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

std::string read_file(std::filesystem::path const& p) {
    std::ifstream is(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
}

// -- derselbe Minimal-ZIP-Leser wie test_a9s3_xlsx_ergebnis_writer.cpp (Zentral-Directory) --
std::uint16_t le16(std::string const& b, std::size_t off) {
    return static_cast<std::uint16_t>(static_cast<unsigned char>(b[off]) |
                                      (static_cast<unsigned char>(b[off + 1]) << 8));
}
std::uint32_t le32(std::string const& b, std::size_t off) {
    return static_cast<std::uint32_t>(static_cast<unsigned char>(b[off])) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(b[off + 1])) << 8) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(b[off + 2])) << 16) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(b[off + 3])) << 24);
}
std::string inflate_raw(char const* data, std::size_t csize, std::size_t usize) {
    std::string out(usize, '\0');
    if (usize == 0) return out;
    z_stream s{};
    if (inflateInit2(&s, -MAX_WBITS) != Z_OK) return {};
    s.next_in    = reinterpret_cast<Bytef*>(const_cast<char*>(data));
    s.avail_in   = static_cast<uInt>(csize);
    s.next_out   = reinterpret_cast<Bytef*>(out.data());
    s.avail_out  = static_cast<uInt>(usize);
    int const rc = inflate(&s, Z_FINISH);
    inflateEnd(&s);
    if (rc != Z_STREAM_END) return {};
    out.resize(s.total_out);
    return out;
}
std::string zip_entry(std::string const& zip, std::string const& name) {
    constexpr std::size_t kEocdMin = 22;
    if (zip.size() < kEocdMin) return {};
    std::size_t eocd = std::string::npos;
    for (std::size_t i = zip.size() - kEocdMin + 1; i-- > 0;) {
        if (le32(zip, i) == 0x06054b50u) {
            eocd = i;
            break;
        }
    }
    if (eocd == std::string::npos) return {};
    std::uint16_t const anzahl   = le16(zip, eocd + 10);
    std::uint32_t const cd_start = le32(zip, eocd + 16);
    if (cd_start >= zip.size()) return {};
    std::size_t pos = cd_start;
    for (std::uint16_t i = 0; i < anzahl; ++i) {
        if (pos + 46 > zip.size() || le32(zip, pos) != 0x02014b50u) return {};
        std::uint16_t const methode   = le16(zip, pos + 10);
        std::uint32_t const csize     = le32(zip, pos + 20);
        std::uint32_t const usize     = le32(zip, pos + 24);
        std::uint16_t const namelen   = le16(zip, pos + 28);
        std::uint16_t const extralen  = le16(zip, pos + 30);
        std::uint16_t const kommentar = le16(zip, pos + 32);
        std::uint32_t const lokal_off = le32(zip, pos + 42);
        std::string const   eintrag   = zip.substr(pos + 46, namelen);
        if (eintrag == name) {
            if (csize == 0xffffffffu || usize == 0xffffffffu) return {};
            if (lokal_off + 30 > zip.size() || le32(zip, lokal_off) != 0x04034b50u) return {};
            std::size_t const daten = lokal_off + 30 + le16(zip, lokal_off + 26) + le16(zip, lokal_off + 28);
            if (daten + csize > zip.size()) return {};
            if (methode == 0) return zip.substr(daten, usize);
            if (methode != 8) return {};
            return inflate_raw(zip.data() + daten, csize, usize);
        }
        pos += 46 + namelen + extralen + kommentar;
    }
    return {};
}

} // namespace

// -----------------------------------------------------------------------------
// (a) Interner Hyperlink real geschrieben UND zurueckgelesen: compare -> Funktion (Macro).
// -----------------------------------------------------------------------------
TEST(A9S3MessEbeneBlattsorte, InternerHyperlinkAufloestSichZumZielsheet) {
    TempDir const tmp;
    auto          mappe = lab::ErgebnisMappenFactory::oeffne(tmp.path, "a9s3_hyperlink");
    ASSERT_NE(mappe, nullptr);
    mappe->info_blatt({}, {}, {});

    // Macro-Sheet ZUERST anlegen (das Hyperlink-Ziel muss existieren, damit Excel es nicht als
    // "kaputten Link" fuehrt) -- Reihenfolge ist Aufrufer-Pflicht wie bei blatt().
    auto& macro = mappe->mess_ebene_blatt(lab::MessEbenenSchluessel{lab::MessEbene::Macro, "insert"});
    macro.kopf(std::vector<std::string>{"prozess", "thread", "aufrufer", "checkpoint", "zeitpunkt"});
    macro.zeile(std::vector<std::string>{"SYNTH_ceb1", "SYNTH_t1", "compare", "in", "1"});

    auto& compare = mappe->mess_ebene_blatt(lab::MessEbenenSchluessel{lab::MessEbene::Compare, "rekomb_a"});
    compare.kopf(std::vector<std::string>{"verweis", "akkumulierter_wert"});
    // Der Hyperlink zeigt auf Zeile 2 (0-indiziert Zeile 1 = "A2"), wo die erste echte Macro-Zeile
    // steht (Zeile 0/A1 ist der Kopf) -- ein Ziel, das es im Zielsheet WIRKLICH gibt.
    compare.zeile_mit_verweis(std::vector<std::string>{}, "insert", "M_insert", "A2",
                              std::vector<std::string>{"123.5"});

    mappe->schliessen();

    auto const datei = tmp.path / "a9s3_hyperlink.xlsx";
    ASSERT_TRUE(std::filesystem::exists(datei));
    std::string const roh = read_file(datei);
    ASSERT_GE(roh.size(), 4u);
    EXPECT_EQ(roh.compare(0, 4, "PK\x03\x04", 4), 0);

    // compare ist das ERSTE Fassung-3-Sheet -> sheet1.xml (Blatt-Reihenfolge = Anlage-Reihenfolge:
    // macro wurde zwar ZUERST angelegt, dann compare -- also ist macro sheet1, compare sheet2).
    std::string const compare_xml = zip_entry(roh, "xl/worksheets/sheet2.xml");
    ASSERT_FALSE(compare_xml.empty()) << "sheet2.xml (compare) nicht gefunden/entpackbar.";
    EXPECT_NE(compare_xml.find("<hyperlinks>"), std::string::npos)
        << "Das compare-Sheet muss einen <hyperlinks>-Block tragen: " << compare_xml;
    EXPECT_NE(compare_xml.find("'M_insert'!A2"), std::string::npos)
        << "Das Hyperlink-Ziel muss woertlich auf 'M_insert'!A2 zeigen: " << compare_xml;
    // Der SICHTBARE Zellinhalt ("insert") muss weiterhin als normaler Zell-String vorhanden sein --
    // der Hyperlink ersetzt den Inhalt nicht durch die URL.
    std::string const strings_xml = zip_entry(roh, "xl/sharedStrings.xml");
    EXPECT_NE(strings_xml.find("insert"), std::string::npos) << strings_xml;

    // Sheet-Namen bestaetigt lesbar (M_insert / C_rekomb_a), nicht S00N-Fallback.
    std::string const workbook_xml = zip_entry(roh, "xl/workbook.xml");
    EXPECT_NE(workbook_xml.find("M_insert"), std::string::npos) << workbook_xml;
    EXPECT_NE(workbook_xml.find("C_rekomb_a"), std::string::npos) << workbook_xml;
}

// -----------------------------------------------------------------------------
// (Gegenprobe zu a) ein Hyperlink auf ein NICHT existierendes Ziel-Sheet wird vom Vendor dennoch
// geschrieben (libxlsxwriter validiert Ziel-Sheets nicht) -- das ist KEIN Fehler dieses Writers,
// sondern dokumentiertes Vendor-Verhalten; die Probe belegt, dass wir nichts VORTAEUSCHEN, was der
// Vendor nicht leistet (keine eigene Validierung erfunden).
// -----------------------------------------------------------------------------
TEST(A9S3MessEbeneBlattsorte, HyperlinkAufNichtVorhandenesZielWirdGeschriebenOhneVendorseitigeValidierung) {
    TempDir const tmp;
    auto          mappe = lab::ErgebnisMappenFactory::oeffne(tmp.path, "a9s3_hyperlink_baumelnd");
    mappe->info_blatt({}, {}, {});
    auto& compare = mappe->mess_ebene_blatt(lab::MessEbenenSchluessel{lab::MessEbene::Compare, "x"});
    compare.kopf(std::vector<std::string>{"verweis", "wert"});
    EXPECT_NO_THROW(compare.zeile_mit_verweis(std::vector<std::string>{}, "gibt_es_nicht", "M_existiert_nicht", "A1",
                                              std::vector<std::string>{"1"}));
    EXPECT_NO_THROW(mappe->schliessen());
}

// -----------------------------------------------------------------------------
// Sheet-Zaehlung FEST: 1 compare + |Funktionen| + |Achsen|, waechst NICHT mit der Zahl der Aufrufe
// (mehrere IN/OUT-Zeilenpaare im SELBEN Sheet).
// -----------------------------------------------------------------------------
TEST(A9S3MessEbeneBlattsorte, BlattzahlBleibtFestUnabhaengigVonDerAufrufzahl) {
    TempDir const tmp;
    auto          mappe = lab::ErgebnisMappenFactory::oeffne(tmp.path, "a9s3_fest");
    mappe->info_blatt({}, {}, {});

    // 1 compare-Rekombination + 2 Funktionen + 3 Achsen = 6 Sheets erwartet + INFO = 7.
    auto& cmp = mappe->mess_ebene_blatt(lab::MessEbenenSchluessel{lab::MessEbene::Compare, "einzige_rekomb"});
    cmp.kopf(std::vector<std::string>{"verweis", "wert"});

    auto& insert_fn = mappe->mess_ebene_blatt(lab::MessEbenenSchluessel{lab::MessEbene::Macro, "insert"});
    insert_fn.kopf(std::vector<std::string>{"prozess", "thread", "aufrufer", "checkpoint", "zeitpunkt"});
    auto& lookup_fn = mappe->mess_ebene_blatt(lab::MessEbenenSchluessel{lab::MessEbene::Macro, "lookup"});
    lookup_fn.kopf(std::vector<std::string>{"prozess", "thread", "aufrufer", "checkpoint", "zeitpunkt"});

    auto& search_algo_achse = mappe->mess_ebene_blatt(lab::MessEbenenSchluessel{lab::MessEbene::Micro, "search_algo"});
    search_algo_achse.kopf(std::vector<std::string>{"prozess", "thread", "aufrufer", "checkpoint", "zeitpunkt"});
    auto& allocator_achse = mappe->mess_ebene_blatt(lab::MessEbenenSchluessel{lab::MessEbene::Micro, "allocator"});
    allocator_achse.kopf(std::vector<std::string>{"prozess", "thread", "aufrufer", "checkpoint", "zeitpunkt"});
    auto& prefetch_achse = mappe->mess_ebene_blatt(lab::MessEbenenSchluessel{lab::MessEbene::Micro, "prefetch"});
    prefetch_achse.kopf(std::vector<std::string>{"prozess", "thread", "aufrufer", "checkpoint", "zeitpunkt"});

    // VIELE Aufrufe (IN/OUT-Paare) in die BESTEHENDEN Sheets -- Blattzahl darf sich NICHT aendern.
    for (int i = 0; i < 25; ++i) {
        std::string const t = std::to_string(i);
        insert_fn.zeile(std::vector<std::string>{"SYNTH_ceb1", "SYNTH_t" + t, "compare:einzige_rekomb", "in", t});
        insert_fn.zeile(std::vector<std::string>{"SYNTH_ceb1", "SYNTH_t" + t, "compare:einzige_rekomb", "out", t});
    }

    // erneuter Aufruf mit DEMSELBEN Schluessel liefert dasselbe Sheet (Idempotenz wie bei blatt()).
    auto& insert_fn_again = mappe->mess_ebene_blatt(lab::MessEbenenSchluessel{lab::MessEbene::Macro, "insert"});
    EXPECT_EQ(&insert_fn, &insert_fn_again);

    mappe->schliessen();

    std::string const roh = read_file(tmp.path / "a9s3_fest.xlsx");
    ASSERT_FALSE(roh.empty());
    auto const workbook_xml = zip_entry(roh, "xl/workbook.xml");
    // 6 Fassung-3-Sheets + 1 INFO = 7 <sheet -Eintraege.
    std::size_t sheet_tag_count = 0;
    std::size_t pos             = 0;
    while ((pos = workbook_xml.find("<sheet ", pos)) != std::string::npos) {
        ++sheet_tag_count;
        pos += 7;
    }
    EXPECT_EQ(sheet_tag_count, 7u) << workbook_xml;
}

// -----------------------------------------------------------------------------
// IN/OUT als ZWEI Zeilen je Aufruf, Thread-Kennung UND Zeitpunkt in JEDER Zeile (nicht nur der ersten).
// -----------------------------------------------------------------------------
TEST(A9S3MessEbeneBlattsorte, InUndOutSindZweiZeilenMitThreadUndZeitpunktInJederZeile) {
    TempDir const tmp;
    auto          mappe = lab::ErgebnisMappenFactory::oeffne(tmp.path, "a9s3_inout", lab::ErgebnisFormat::csv);
    mappe->info_blatt({}, {}, {});
    auto& macro = mappe->mess_ebene_blatt(lab::MessEbenenSchluessel{lab::MessEbene::Macro, "erase"});
    macro.kopf(std::vector<std::string>{"prozess", "thread", "aufrufer", "checkpoint", "zeitpunkt", "messwert"});
    macro.zeile(std::vector<std::string>{"SYNTH_ceb1", "SYNTH_t7", "compare:x",
                                         std::string{lab::checkpoint_label(lab::Checkpoint::In)}, "42", "-"});
    macro.zeile(std::vector<std::string>{"SYNTH_ceb1", "SYNTH_t7", "compare:x",
                                         std::string{lab::checkpoint_label(lab::Checkpoint::Out)}, "58", "913"});
    mappe->schliessen();

    std::string const inhalt = read_file(tmp.path / "a9s3_inout__macro_erase.csv");
    // ZWEI Zeilen (+ Kopfzeile), beide tragen dieselbe Thread-Kennung, VERSCHIEDENE Zeitpunkte.
    std::size_t const zeilen = std::count(inhalt.begin(), inhalt.end(), '\n');
    EXPECT_EQ(zeilen, 3u) << inhalt; // Kopf + IN + OUT
    EXPECT_NE(inhalt.find("SYNTH_t7;compare:x;in;42;"), std::string::npos) << inhalt;
    EXPECT_NE(inhalt.find("SYNTH_t7;compare:x;out;58;"), std::string::npos) << inhalt;
}
