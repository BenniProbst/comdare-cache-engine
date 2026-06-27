// test_config_durability.cpp — #195 contract config-durability-Gate (Pipeline P0, EPIC #186)
//
// ZWECK (hartes CI-Gate, Stage `contract`, Job `contract:durability` in .gitlab-ci.yml):
//   Beweise, dass MÜLL- / kaputte / bösartige Config-XMLs die Parser NICHT zum Absturz bringen
//   (kein Segfault, kein Stack-Overflow, kein Undefined Behavior), sondern sauber zurückkehren.
//   Ein Crash = der Prozess stirbt = ctest meldet die Binary als FAILED = CI rot → das Gate hat
//   seinen Zweck erfüllt (es fängt eine Robustheits-Regression VOR dem Mess-/Deploy-Pfad ab).
//
// GETESTETE PARSER (beide self-contained, header-only):
//   - comdare::common::xml::parse_document(std::string_view) -> std::optional<XmlNode>
//       (libs/common/serialization/xml_config_parser/xml_reader.hpp) — STRING-basiert, ideal für
//       Fuzz ohne Temp-Dateien.
//   - comdare::cache_engine::builder::workload_driver::parse_load_profile(std::filesystem::path)
//       -> std::optional<LoadProfile>
//       (libs/cache_engine/builder/workload_driver/load_profile_parser.hpp) — datei-basiert,
//       nutzt intern parse_document.
//
// Mess-/Include-Konvention gespiegelt von test_a1_load_profile_opmix (gleiche drei Include-Roots:
// libs/cache_engine, libs/cache_engine/include, libs/common) + GTest-Registrierung gespiegelt von
// test_abi_interface (comdare_add_test → gtest/gtest_main + ein add_test je Binary).

#include <serialization/xml_config_parser/xml_reader.hpp>      // cx::parse_document / XmlNode
#include <builder/workload_driver/load_profile_parser.hpp>     // wd::parse_load_profile / LoadProfile

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace cx = ::comdare::common::xml;
namespace wd = ::comdare::cache_engine::builder::workload_driver;

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Der gemeinsame Müll-Korpus (~16 Einträge). Bewusst gemischt: leere/abgeschnittene
// Fragmente, ungültige Strukturen, Binär-Bytes inkl. eingebettetem '\0', pathologische
// Größen (sehr langes Attribut, sehr tiefe Verschachtelung) und wohlgeformtes-XML-mit-
// falschem-Schema. KEINER davon ist ein gültiges comdare_load_profile.
// ─────────────────────────────────────────────────────────────────────────────
[[nodiscard]] std::vector<std::string> make_garbage_inputs() {
    std::vector<std::string> in;

    in.emplace_back("");                                          // 1  komplett leer
    in.emplace_back(" ");                                         // 2  nur Whitespace
    in.emplace_back("<");                                         // 3  nacktes '<'
    in.emplace_back("<a");                                        // 4  Tag ohne Abschluss
    in.emplace_back("<a>");                                       // 5  offenes Element, kein close
    in.emplace_back("<a></b>");                                   // 6  falsch verschachtelt / mismatched close
    in.emplace_back("<<<>>>");                                    // 7  Klammer-Salat
    // 8  Binär-Müll inkl. eingebettetem '\0' (kein gültiges UTF-8, nicht null-terminiert).
    {
        static constexpr unsigned char raw[] = {
            0x00, 0x01, 0x02, 0xFF, 0x3C /*'<'*/, 0x00, 0x61 /*'a'*/, 0x3E /*'>'*/, 0xFE, 0x00, 0x7F};
        in.emplace_back(reinterpret_cast<char const*>(raw), sizeof(raw));
    }
    in.emplace_back("<?xml");                                     // 9  abgeschnittene XML-Deklaration
    in.emplace_back("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"); // 10 nur Deklaration, kein Wurzelelement
    // 11 sehr langes Attribut (~100k Zeichen) — Puffer-/Allokations-Robustheit.
    in.emplace_back("<a b=\"" + std::string(100000, 'x') + "\"/>");
    // 12 sehr tief verschachtelt (~200 Ebenen) — Rekursions-/Stack-Robustheit.
    {
        constexpr int kDepth = 200;
        std::string deep;
        deep.reserve(static_cast<std::size_t>(kDepth) * 8);
        for (int i = 0; i < kDepth; ++i) deep += "<n>";
        for (int i = 0; i < kDepth; ++i) deep += "</n>";
        in.emplace_back(std::move(deep));
    }
    in.emplace_back("<comdare_load_profile");                     // 13 abgeschnittenes Root-Tag (richtiger Name, kein '>')
    in.emplace_back("<root><child>x</child></root>");            // 14 wohlgeformtes XML, FALSCHES Schema
    in.emplace_back("<a b=\"unterminated>");                      // 15 nicht geschlossenes Attribut-Quote
    in.emplace_back("<a>&;&unknown;&#;</a>");                     // 16 kaputte/unbekannte Entities

    return in;
}

// Legt ein eindeutiges Temp-Arbeitsverzeichnis an (verhindert Kollisionen bei parallelen CI-Läufen
// auf demselben Runner). Wird am Ende der datei-basierten Tests via remove_all aufgeräumt.
[[nodiscard]] std::filesystem::path make_unique_tmp_dir() {
    std::random_device rd;
    std::string const token = std::to_string(rd()) + "_" + std::to_string(rd());
    std::filesystem::path dir = std::filesystem::temp_directory_path() / ("comdare_durability_" + token);
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir;
}

// Schreibt Inhalt (auch binär, inkl. '\0') exakt byteweise in eine Datei.
void write_bytes(std::filesystem::path const& p, std::string const& content) {
    std::ofstream o{p, std::ios::binary};
    o.write(content.data(), static_cast<std::streamsize>(content.size()));
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// (1) XML-DOM-Reader: darf an KEINER Müll-Eingabe abstürzen.
//     Ergebnis darf nullopt ODER ein Node sein — beides ist „graceful". Entscheidend ist nur,
//     dass der Aufruf für jede Eingabe zurückkehrt (kein Segfault/UB/Stack-Overflow).
// ─────────────────────────────────────────────────────────────────────────────
TEST(ConfigDurability, XmlReaderNeverCrashesOnGarbage) {
    std::vector<std::string> const inputs = make_garbage_inputs();

    std::size_t parsed_as_node = 0;
    for (std::string const& g : inputs) {
        // parse_document nimmt einen string_view; std::string konvertiert implizit. Bei der
        // Binär-Eingabe bleibt die Länge (inkl. '\0') über den string_view korrekt erhalten.
        std::optional<cx::XmlNode> const node = cx::parse_document(g);
        if (node.has_value()) ++parsed_as_node;  // Ergebnis konsumieren — Wert ODER nullopt ist OK.
    }

    // Bis hierher gekommen = jeder der ~16 Müll-Inputs ist sauber zurückgekehrt → kein Crash.
    SUCCEED() << parsed_as_node << " von " << inputs.size()
              << " Müll-Eingaben ergaben einen Node (Rest nullopt); keine stürzte ab.";
}

// ─────────────────────────────────────────────────────────────────────────────
// (2) parse_load_profile: dieselben Müll-Inhalte als Temp-Dateien → IMMER nullopt.
//     Plus zwei Sonderfälle ohne lesbaren Inhalt: nicht existierender Pfad und Verzeichnis-als-Pfad.
// ─────────────────────────────────────────────────────────────────────────────
TEST(ConfigDurability, ParseLoadProfileRejectsGarbage) {
    std::filesystem::path const dir = make_unique_tmp_dir();
    // Umgebungs-Vorbedingung (kein Parser-Befund): ohne nutzbares Temp-Verzeichnis kann das Gate
    // nicht laufen — als ASSERT mit klarer Meldung, damit es nicht als Parser-Regression fehldeutet.
    ASSERT_TRUE(std::filesystem::is_directory(dir))
        << "Temp-Arbeitsverzeichnis konnte nicht angelegt werden: " << dir.string();
    std::vector<std::string> const inputs = make_garbage_inputs();

    for (std::size_t i = 0; i < inputs.size(); ++i) {
        std::filesystem::path const f = dir / ("garbage_" + std::to_string(i) + ".xml");
        write_bytes(f, inputs[i]);
        EXPECT_FALSE(wd::parse_load_profile(f).has_value())
            << "Müll-Datei #" << i << " hätte nullopt liefern müssen.";
    }

    // Nicht existierender Pfad → nullopt (ifstream öffnet nicht, Inhalt leer).
    std::filesystem::path const missing = dir / "does_not_exist_zzz.xml";
    EXPECT_FALSE(wd::parse_load_profile(missing).has_value())
        << "Nicht existierender Pfad hätte nullopt liefern müssen.";

    // Verzeichnis-als-Pfad → nullopt (kein regulärer, lesbarer Datei-Inhalt).
    EXPECT_FALSE(wd::parse_load_profile(dir).has_value())
        << "Verzeichnis-als-Pfad hätte nullopt liefern müssen.";

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);  // Aufräumen (best effort; EXPECT ist non-fatal → läuft immer).
}

// ─────────────────────────────────────────────────────────────────────────────
// (3) Positiv-Kontrolle: ein MINIMAL gültiges Lastprofil MUSS akzeptiert werden.
//     Verhindert, dass das Gate trivial „durch Immer-nullopt" besteht.
//
//     HINWEIS: Das in der Aufgabe vorgeschlagene <comdare_load_profile id="t"></...> genügt NICHT —
//     load_profile_parser.hpp verlangt zusätzlich ein <workload>-Kind MIT nicht-leerem <op_mix>
//     (Audit A1 / MAJOR-MESS-05: fehlendes/leeres op_mix → nullopt; vgl. test_a1_load_profile_opmix
//     und das echte Beispiel ycsb_c.xml). Das hier ist das tatsächliche Minimum.
// ─────────────────────────────────────────────────────────────────────────────
TEST(ConfigDurability, ParseLoadProfileAcceptsMinimalValid) {
    std::filesystem::path const dir = make_unique_tmp_dir();
    // Umgebungs-Vorbedingung (kein Parser-Befund): ohne nutzbares Temp-Verzeichnis wuerde die
    // Positiv-Kontrolle sonst faelschlich „durchfallen". Klare ASSERT statt irrefuehrender EXPECT-Fehler.
    ASSERT_TRUE(std::filesystem::is_directory(dir))
        << "Temp-Arbeitsverzeichnis konnte nicht angelegt werden: " << dir.string();

    std::string const minimal_valid =
        "<comdare_load_profile id=\"t\">"
        "  <workload>"
        "    <op_mix lookup=\"1.0\"/>"
        "  </workload>"
        "</comdare_load_profile>";

    std::filesystem::path const f = dir / "minimal_valid.xml";
    write_bytes(f, minimal_valid);

    std::optional<wd::LoadProfile> const lp = wd::parse_load_profile(f);
    EXPECT_TRUE(lp.has_value()) << "Minimal gültiges Lastprofil hätte akzeptiert werden müssen.";
    if (lp.has_value()) {
        EXPECT_EQ(lp->id, "t");
        EXPECT_DOUBLE_EQ(lp->config.pct_lookup, 1.0);
    }

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}
