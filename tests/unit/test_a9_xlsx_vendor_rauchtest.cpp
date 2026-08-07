// test_a9_xlsx_vendor_rauchtest -- A9-S1 (xlsx-Writer, Vendor-Scheibe).
//
// Belegt, dass die beiden Vendor-Snapshots (ext/io/libxlsxwriter Tag v1.2.4, ext/io/zlib Tag v1.3.2)
// nicht nur uebersetzen, sondern LAUFEN: es wird eine echte .xlsx geschrieben, danach als ZIP wieder
// geoeffnet und der Zellinhalt zurueckgelesen. "Es kompiliert" waere kein Beleg.
//
// Der ZIP-Leser ist bewusst EIGENER Code (kein Python, kein externes Werkzeug, kein zusaetzlicher
// Vendor): er laeuft ueber die Zentral-Directory des Archivs und entpackt die Eintraege mit dem
// vendorierten zlib. Damit haengt der Beleg an BEIDEN Snapshots -- ein stiller Ausfall von zlib
// wuerde diesen Test rot faerben. Das Muster ist der Vorgriff auf die in A9-S3 geplante
// ZIP-Struktur-Selbstpruefung (Design-Dossier Abschnitt 7, Zeile A9-S3).
//
// KEIN Anschluss an den Mess-Pfad, kein CSV-Schema, keine Blattform -- das ist S3/S4.

#include <xlsxwriter.h>
#include <zlib.h>

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

struct TempDir {
    std::filesystem::path path;
    TempDir() {
        static std::atomic<unsigned> counter{0};
        path = std::filesystem::temp_directory_path() /
               ("a9s1_xlsx_" + std::to_string(counter.fetch_add(1)) + "_" + std::to_string(::time(nullptr)));
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

// -----------------------------------------------------------------------------
// Minimaler ZIP-Leser (nur so viel, wie der Rauchtest braucht).
//
// Gelesen wird ueber die Zentral-Directory, NICHT ueber die lokalen Kopfsaetze: nur dort stehen die
// Groessen zuverlaessig. Setzt ein Schreiber das Streaming-Flag (Bit 3), sind csize/usize im lokalen
// Kopf null und die echten Werte stehen erst HINTER den Daten -- ein Leser, der dem lokalen Kopf
// glaubt, liest dann still nichts. Die Zentral-Directory hat diese Falle nicht.
// -----------------------------------------------------------------------------

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

// Roh-Deflate (ohne zlib-Rahmen) auspacken -- genau die Form, in der ZIP seine Eintraege ablegt.
std::string inflate_raw(char const* data, std::size_t csize, std::size_t usize) {
    std::string out(usize, '\0');
    if (usize == 0) return out;

    z_stream s{};
    if (inflateInit2(&s, -MAX_WBITS) != Z_OK) return {};
    s.next_in   = reinterpret_cast<Bytef*>(const_cast<char*>(data));
    s.avail_in  = static_cast<uInt>(csize);
    s.next_out  = reinterpret_cast<Bytef*>(out.data());
    s.avail_out = static_cast<uInt>(usize);

    int const rc = inflate(&s, Z_FINISH);
    inflateEnd(&s);
    if (rc != Z_STREAM_END) return {};
    out.resize(s.total_out);
    return out;
}

// Liefert den entpackten Inhalt des Eintrags `name`, oder einen leeren String, wenn er fehlt.
std::string zip_entry(std::string const& zip, std::string const& name) {
    // 1. End-of-Central-Directory rueckwaerts suchen (Signatur PK\x05\x06).
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

    // 2. Zentral-Directory durchlaufen (Signatur PK\x01\x02 je Eintrag).
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

        std::string const eintrag = zip.substr(pos + 46, namelen);
        if (eintrag == name) {
            // ZIP64-Ausweichwerte werden hier NICHT stillschweigend fehlgedeutet: der Rauchtest
            // schreibt eine winzige Mappe, die sie nie ausloest. Taeten sie es doch, ist der
            // ehrliche Ausgang ein leerer String -> roter Test, keine stille Falschangabe.
            if (csize == 0xffffffffu || usize == 0xffffffffu) return {};
            if (lokal_off + 30 > zip.size() || le32(zip, lokal_off) != 0x04034b50u) return {};
            std::size_t const daten = lokal_off + 30 + le16(zip, lokal_off + 26) + le16(zip, lokal_off + 28);
            if (daten + csize > zip.size()) return {};
            if (methode == 0) return zip.substr(daten, usize); // unkomprimiert abgelegt
            if (methode != 8) return {};                       // nur "stored" und "deflate"
            return inflate_raw(zip.data() + daten, csize, usize);
        }
        pos += 46 + namelen + extralen + kommentar;
    }
    return {};
}

} // namespace

// -----------------------------------------------------------------------------
// Versions-Pin: faengt einen stillen Austausch des Snapshots.
// -----------------------------------------------------------------------------
TEST(A9S1VendorRauchtest, VersionenSindDieGepinnten) {
    EXPECT_STREQ(LXW_VERSION, "1.2.4");
    EXPECT_STREQ(ZLIB_VERSION, "1.3.2");
    // zlibVersion() kommt aus dem uebersetzten Objekt, ZLIB_VERSION aus dem Header. Weichen sie ab,
    // ist ein fremdes zlib dazwischengeraten.
    EXPECT_STREQ(zlibVersion(), ZLIB_VERSION);
}

// -----------------------------------------------------------------------------
// Der eigentliche Rauchtest: schreiben -> wieder oeffnen -> Zelle zurueckLESEN.
// -----------------------------------------------------------------------------
TEST(A9S1VendorRauchtest, SchreibtXlsxUndLiestZelleZurueck) {
    TempDir const     tmp;
    auto const        datei = tmp.path / "a9s1_rauchtest.xlsx";
    std::string const text  = "A9-S1 Vendor-Rauchtest";

    // --- schreiben ---
    lxw_workbook* mappe = workbook_new(datei.string().c_str());
    ASSERT_NE(mappe, nullptr);
    lxw_worksheet* blatt = workbook_add_worksheet(mappe, "Rauchtest");
    ASSERT_NE(blatt, nullptr);
    ASSERT_EQ(worksheet_write_string(blatt, 0, 0, text.c_str(), nullptr), LXW_NO_ERROR);
    ASSERT_EQ(worksheet_write_number(blatt, 0, 1, 42.5, nullptr), LXW_NO_ERROR);
    ASSERT_EQ(workbook_close(mappe), LXW_NO_ERROR);

    // --- Datei ist da und ist ein ZIP ---
    ASSERT_TRUE(std::filesystem::exists(datei));
    std::string const roh = read_file(datei);
    ASSERT_GE(roh.size(), 4u);
    EXPECT_EQ(roh.compare(0, 4, "PK\x03\x04", 4), 0) << "Kein ZIP-Kopfsatz am Dateianfang";
    EXPECT_GT(std::filesystem::file_size(datei), 0u);

    // --- wieder oeffnen und zurueckLESEN ---
    // Der Text liegt in der gemeinsamen String-Tabelle, die Zahl direkt im Blatt.
    std::string const blatt_xml   = zip_entry(roh, "xl/worksheets/sheet1.xml");
    std::string const strings_xml = zip_entry(roh, "xl/sharedStrings.xml");

    ASSERT_FALSE(blatt_xml.empty()) << "xl/worksheets/sheet1.xml nicht entpackbar";
    ASSERT_FALSE(strings_xml.empty()) << "xl/sharedStrings.xml nicht entpackbar";

    // Zelle A1: Verweis in die String-Tabelle (t="s"), dort steht der Text woertlich.
    EXPECT_NE(blatt_xml.find("<c r=\"A1\" t=\"s\"><v>0</v></c>"), std::string::npos)
        << "Zelle A1 nicht als String-Verweis im Blatt gefunden. Blatt-XML: " << blatt_xml;
    EXPECT_NE(strings_xml.find(text), std::string::npos)
        << "Zellinhalt nicht in der String-Tabelle gefunden. Strings-XML: " << strings_xml;

    // Zelle B1: Zahl steht direkt im Blatt. Beleg fuer den dtoa-Zweig (USE_DTOA_LIBRARY):
    // locale-unabhaengige Ausgabe, also "42.5" und nie "42,5".
    EXPECT_NE(blatt_xml.find("<c r=\"B1\"><v>42.5</v></c>"), std::string::npos)
        << "Zelle B1 nicht mit dem erwarteten Zahlwert gefunden. Blatt-XML: " << blatt_xml;

    // Der Blattname landet in der Mappen-Struktur.
    std::string const workbook_xml = zip_entry(roh, "xl/workbook.xml");
    ASSERT_FALSE(workbook_xml.empty()) << "xl/workbook.xml nicht entpackbar";
    EXPECT_NE(workbook_xml.find("Rauchtest"), std::string::npos);
}

// -----------------------------------------------------------------------------
// Gegenprobe: der ZIP-Leser darf nicht bei jedem Namen etwas "finden".
// Ohne diese Probe koennte ein Leser, der immer denselben Eintrag liefert, oben gruen sein.
// -----------------------------------------------------------------------------
TEST(A9S1VendorRauchtest, ZipLeserFindetNichtsErfundenes) {
    TempDir const tmp;
    auto const    datei = tmp.path / "a9s1_gegenprobe.xlsx";

    lxw_workbook* mappe = workbook_new(datei.string().c_str());
    ASSERT_NE(mappe, nullptr);
    lxw_worksheet* blatt = workbook_add_worksheet(mappe, nullptr);
    ASSERT_NE(blatt, nullptr);
    ASSERT_EQ(worksheet_write_number(blatt, 0, 0, 1.0, nullptr), LXW_NO_ERROR);
    ASSERT_EQ(workbook_close(mappe), LXW_NO_ERROR);

    std::string const roh = read_file(datei);
    EXPECT_FALSE(zip_entry(roh, "xl/worksheets/sheet1.xml").empty());
    EXPECT_TRUE(zip_entry(roh, "xl/gibt/es/nicht.xml").empty());
}
