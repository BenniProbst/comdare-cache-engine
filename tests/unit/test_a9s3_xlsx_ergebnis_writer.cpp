// test_a9s3_xlsx_ergebnis_writer -- A9-S3: der xlsx-Backend End-zu-Ende. Schreibt echte .xlsx-Dateien
// ueber die oeffentliche Factory (KEIN <xlsxwriter.h>-Include hier -- das ist der ganze Punkt der
// TU-Grenze) und liest sie ueber einen EIGENEN ZIP-Leser zurueck (Muster aus
// test_a9_xlsx_vendor_rauchtest.cpp, A9-S1: Zentral-Directory + vendoriertes zlib-Inflate, kein Python,
// kein externes Werkzeug).
//
// Bissproben hier: (1) ZipFehler bei nicht schreibbarem Ziel, (2) Token-Treue failed/gesperrt/
// nicht_gebaut/n-a UND numerische Zellen im selben Sheet, (3) INFO-Blatt zuletzt mit vollstaendiger
// Sheet-Legende. Das Zeilen-/Sheetnamen-Limit selbst ist bereits in test_a9s3_ergebnis_mappe_csv.cpp
// direkt gegen die Wache gebissen (dieselbe Wache traegt xlsx- UND csv-Backend) -- eine Million echte
// Zeilen hier zu schreiben waere kein staerkerer Beleg, nur ein langsamerer Test.

#include "lager_ablage/ergebnis_mappe.hpp"

#include <zlib.h>

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace lab = comdare::cache_engine::builder::lager_ablage;
namespace ms  = comdare::cache_engine::measurement;

namespace {

struct TempDir {
    std::filesystem::path path;
    TempDir() {
        static std::atomic<unsigned> counter{0};
        path = std::filesystem::temp_directory_path() /
               ("a9s3_xlsx_" + std::to_string(counter.fetch_add(1)) + "_" + std::to_string(::time(nullptr)));
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
// Minimaler ZIP-Leser (identisches Muster zu test_a9_xlsx_vendor_rauchtest.cpp, A9-S1) -- Zentral-
// Directory statt lokaler Kopfsaetze (Streaming-Flag-Falle, s. dort).
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
std::vector<std::string> zip_entry_names(std::string const& zip) {
    std::vector<std::string> names;
    constexpr std::size_t    kEocdMin = 22;
    if (zip.size() < kEocdMin) return names;
    std::size_t eocd = std::string::npos;
    for (std::size_t i = zip.size() - kEocdMin + 1; i-- > 0;) {
        if (le32(zip, i) == 0x06054b50u) {
            eocd = i;
            break;
        }
    }
    if (eocd == std::string::npos) return names;
    std::uint16_t const anzahl   = le16(zip, eocd + 10);
    std::uint32_t const cd_start = le32(zip, eocd + 16);
    if (cd_start >= zip.size()) return names;
    std::size_t pos = cd_start;
    for (std::uint16_t i = 0; i < anzahl; ++i) {
        if (pos + 46 > zip.size() || le32(zip, pos) != 0x02014b50u) break;
        std::uint16_t const namelen   = le16(zip, pos + 28);
        std::uint16_t const extralen  = le16(zip, pos + 30);
        std::uint16_t const kommentar = le16(zip, pos + 32);
        names.push_back(zip.substr(pos + 46, namelen));
        pos += 46 + namelen + extralen + kommentar;
    }
    return names;
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
// Zwei Sheets + INFO, Token-Treue, numerische Zellen als echte Zahlen.
// -----------------------------------------------------------------------------
TEST(A9S3XlsxErgebnisWriter, ZweiSheetsPlusInfoTokenTreueUndNumerik) {
    TempDir const tmp;
    auto          mappe = lab::ErgebnisMappenFactory::oeffne(tmp.path, "a9s3_xlsx_e2e"); // Default = xlsx
    ASSERT_NE(mappe, nullptr);

    lab::MaschinenSysinfo sys{};
    sys.hostname         = "prod1";
    sys.machine_id       = "prod1_zen5";
    sys.cpu_fabrication  = "amd_zen5_avx512";
    sys.identity_verdict = "match";
    lab::HauptAchsenBelegung haupt{{"measurement_tooling", "wallclock"}};
    lab::KonstantenMeta      konst{{"target_isa", "amd64_v3"}};
    mappe->info_blatt(sys, haupt, konst);

    auto& s1 = mappe->blatt(lab::SheetSchluessel{"ycsb_a", "", ""});
    s1.kopf(std::vector<std::string>{"binary_id", "n_ops", "status"});
    s1.zeile(std::vector<std::string>{"bin-a", "10000", "failed"});
    s1.zeile(std::vector<std::string>{"bin-b", "42.5", "gesperrt"});

    auto& s2 = mappe->blatt(lab::SheetSchluessel{"ycsb_c", "", ""});
    s2.kopf(std::vector<std::string>{"binary_id", "n_ops", "status"});
    s2.zeile(std::vector<std::string>{"bin-c", "n/a", "nicht_gebaut"});

    mappe->schliessen();

    auto const datei = tmp.path / "a9s3_xlsx_e2e.xlsx";
    ASSERT_TRUE(std::filesystem::exists(datei));
    EXPECT_GT(std::filesystem::file_size(datei), 0u);
    EXPECT_FALSE(std::filesystem::exists(tmp.path / "a9s3_xlsx_e2e.xlsx.tmp")) << "keine .tmp-Reste (atomar).";

    std::string const roh = read_file(datei);
    ASSERT_GE(roh.size(), 4u);
    EXPECT_EQ(roh.compare(0, 4, "PK\x03\x04", 4), 0);

    auto const  names           = zip_entry_names(roh);
    std::size_t sheet_xml_count = 0;
    for (auto const& n : names)
        if (n.starts_with("xl/worksheets/sheet") && n.ends_with(".xml")) ++sheet_xml_count;
    EXPECT_EQ(sheet_xml_count, 3u) << "S001 + S002 + INFO -- 3 Sheet-XML-Eintraege erwartet.";

    std::string const workbook_xml = zip_entry(roh, "xl/workbook.xml");
    ASSERT_FALSE(workbook_xml.empty());
    EXPECT_NE(workbook_xml.find("S001"), std::string::npos) << workbook_xml;
    EXPECT_NE(workbook_xml.find("S002"), std::string::npos) << workbook_xml;
    EXPECT_NE(workbook_xml.find("INFO"), std::string::npos) << workbook_xml;

    // sheet1.xml = S001 (erste angelegte): failed/gesperrt woertlich in sharedStrings, 10000 als Zahl.
    std::string const strings_xml = zip_entry(roh, "xl/sharedStrings.xml");
    ASSERT_FALSE(strings_xml.empty());
    EXPECT_NE(strings_xml.find("failed"), std::string::npos) << "Token-Treue 'failed': " << strings_xml;
    EXPECT_NE(strings_xml.find("gesperrt"), std::string::npos) << "Token-Treue 'gesperrt': " << strings_xml;
    EXPECT_NE(strings_xml.find("nicht_gebaut"), std::string::npos) << "Token-Treue 'nicht_gebaut': " << strings_xml;
    EXPECT_NE(strings_xml.find(">n/a<"), std::string::npos) << "Token-Treue 'n/a': " << strings_xml;

    std::string const sheet1_xml = zip_entry(roh, "xl/worksheets/sheet1.xml");
    ASSERT_FALSE(sheet1_xml.empty());
    // n_ops=10000 (Zeile 2, Spalte B, 0-indiziert row=1 col=1 -> B2) MUSS eine echte Zahlzelle sein
    // (kein t="s"-String-Verweis): <c r="B2"><v>10000</v></c>.
    EXPECT_NE(sheet1_xml.find("<c r=\"B2\"><v>10000</v></c>"), std::string::npos)
        << "n_ops=10000 MUSS als Zahl gerendert werden: " << sheet1_xml;
    EXPECT_NE(sheet1_xml.find("<c r=\"B3\"><v>42.5</v></c>"), std::string::npos)
        << "42.5 MUSS als Zahl gerendert werden (dtoa-Determinismus, Punkt nicht Komma): " << sheet1_xml;
    // status="failed" (Spalte C) MUSS ein String-Verweis sein (t="s"), NIE eine Zahl.
    EXPECT_NE(sheet1_xml.find("<c r=\"C2\" t=\"s\">"), std::string::npos)
        << "'failed' MUSS als String-Zelle gerendert werden: " << sheet1_xml;
}

// -----------------------------------------------------------------------------
// (rot) Ein nicht existierendes/nicht schreibbares Ausgabeverzeichnis -> ErgebnisSchreibFehler{ZipFehler}
// statt einer stillen leeren/kaputten Datei.
// -----------------------------------------------------------------------------
TEST(A9S3XlsxErgebnisWriter, NichtSchreibbaresZielWirftZipFehler) {
    bool geworfen = false;
    try {
        auto  mappe = lab::ErgebnisMappenFactory::oeffne("/nicht_vorhanden_a9s3_xlsx_xyz/tief", "x");
        auto& s     = mappe->blatt(lab::SheetSchluessel{"x", "", ""});
        s.kopf(std::vector<std::string>{"a"});
        s.zeile(std::vector<std::string>{"1"});
        mappe->schliessen();
    } catch (lab::ErgebnisSchreibFehler const& e) {
        geworfen = true;
        EXPECT_EQ(e.klasse(), ms::LagerAblageFehlerKlasse::ZipFehler) << e.what();
    }
    EXPECT_TRUE(geworfen) << "ein nicht schreibbares Ziel MUSS werfen, nicht still eine kaputte Datei hinterlassen.";
}

// -----------------------------------------------------------------------------
// (gruen) Gegenprobe: ein gueltiges Ziel wirft nicht -- die Wache oben haengt an der echten Ursache.
// -----------------------------------------------------------------------------
TEST(A9S3XlsxErgebnisWriter, GueltigesZielWirftNicht) {
    TempDir const tmp;
    EXPECT_NO_THROW({
        auto  mappe = lab::ErgebnisMappenFactory::oeffne(tmp.path, "gruen_xlsx");
        auto& s     = mappe->blatt(lab::SheetSchluessel{"x", "", ""});
        s.kopf(std::vector<std::string>{"a"});
        s.zeile(std::vector<std::string>{"1"});
        mappe->schliessen();
    });
}
