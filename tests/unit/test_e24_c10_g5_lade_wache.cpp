// test_e24_c10_g5_lade_wache -- E-24 C10 (b) / Gate G5, Bauplan-Dossier docs/sessions/
// 20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.2-C10: "G5: Major-7-Altmodul (C6-Fixture) wird
// ABGELEHNT (Loader-Fehler literal), Major-8-Roundtrip gruen, Magic-Wechsel belegt, --version-Beleg".
//
// DIE AUSSAGE VON G5: nach dem Major-Bump C8 darf KEIN vor-C8-Binary mehr geladen werden. Das ist die
// Kante, an der die HY-D2-Begruendung des ganzen Fensters haengt (E24-DOSSIER:27): laege der Bump NACH
// dem Trigger, lehnte der Loader alle eingelagerten Binaries ab.
//
// BEFUND DES BAUS (deklariert, nicht kaschiert -- er widerspricht der naiven Erwartung "das Alt-Modul
// scheitert an status_abi_major_mismatch"): der Loader traegt ZWEI Schloesser und prueft sie in fester
// Reihenfolge (anatomy_module_loader.cpp: Magic-Check "Pre-Version", danach host_compatible_with). Weil
// die Magic den Major KODIERT (anatomy_module_abi_v1_decl.hpp: ".A7." -> ".A8."), bewegt ein echtes
// Alt-Modul BEIDE Werte und wird schon vom ERSTEN Schloss abgewiesen: der literale Status ist
// status_magic_mismatch, nicht status_abi_major_mismatch. Eine Probe, die nur das echte Alt-Modul
// faehrt, beweist deshalb ausschliesslich den Magic-Check -- der Major-Vergleich koennte still kaputt
// sein. Diese TU faehrt beide Schloesser EINZELN:
//   (1) perm_alt_major7          -- Magic ".A7." + Version 7.0 (herkunfts-treue Quelle der gesicherten
//                                   Fixture major7-referenz-44bcda99.so) -> magic_mismatch.
//   (2) perm_major7_neue_magic   -- HEUTIGE Magic + Version (Host-Major - 1) -> abi_major_mismatch.
//   (3) perm_set_d9              -- der heutige Stand -> ok (der Positiv-Zweig; ohne ihn belegte die TU
//                                   nur, dass der Loader ueberhaupt ablehnt).
// Beide Alt-Module bauen in ihrer Factory einen ECHTEN SetAbiAdapter: faellt eine der beiden Wachen weg,
// WUERDEN sie laden. Die Probe scheitert also an der Wache und nicht an einem leeren Stub.
//
// OPTIONAL --gesicherte-fixture=<pfad>: der Lauf gegen die AUSSERHALB des Repos gesicherte Original-.so
// (/home/comdare/e24-g5-fixture/major7-referenz-44bcda99.so, sha256 32b45bc0...0fd1c94e). Sie ist ein
// binaeres Beweismittel und gehoert nicht ins Repo; ctest faehrt deshalb die committete Nachbildung, und
// der Lauf gegen das Original steht als literaler Beleg im Fenster-Report. Ist das Argument gesetzt und
// die Datei vorhanden, prueft diese TU sie mit -- fehlt sie, wird das GEMELDET und nicht still uebergangen.
//
// Registriert in tests/unit/CMakeLists.txt (add_test + COMDARE_TEST_TARGETS): keine Waisen-TU (Auflage 13).

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>
#include <builder/pruef_dock/dock_error_classification.hpp>

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>
#include <profile_facade/g1_binary_version_stamp.hpp>
#include <profile_facade/system_version_suffix.hpp>

#include <anatomy/anatomy_base.hpp>
#include <anatomy/set_tier.hpp>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace al  = comdare::cache_engine::builder::anatomy_loader;
namespace ana = comdare::cache_engine::anatomy;
namespace pd  = comdare::cache_engine::builder::pruef_dock;
namespace pf  = comdare::cache_engine::profile_facade;
namespace bpf = comdare::cache_engine::builder::profile_facade;

int g_fail = 0;

template <class A, class B>
void eq(char const* w, A const& g, B const& e) {
    bool const ok = (g == e);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) {
        std::cout << "  (erwartet " << e << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

/// Laedt und meldet den Status LITERAL (Name + Zahl) -- ein Ablehnungs-Beweis ohne den Fehlertext im Log
/// ist kein Beweis.
[[nodiscard]] int lade_und_melde(std::string const& pfad, char const* rolle, al::AnatomyModuleHandle& out) {
    int const st = al::AnatomyModuleLoader::load(pfad, out);
    std::cout << "  " << rolle << ": " << pfad << "\n";
    std::cout << "    Loader-Status = " << al::status_name(st) << " (" << st << ")\n";
    return st;
}

[[nodiscard]] std::string arg_wert(int argc, char** argv, std::string_view praefix) {
    for (int i = 1; i < argc; ++i) {
        std::string_view const s{argv[i]};
        if (s.rfind(praefix, 0) == 0) return std::string{s.substr(praefix.size())};
    }
    return {};
}

} // namespace

int main(int argc, char** argv) {
    std::cout << "==== E-24 C10 (b) / G5: der Major-Wechsel als Lade-Wache ====\n";

    std::string const alt        = arg_wert(argc, argv, "--alt=");
    std::string const alt_magic  = arg_wert(argc, argv, "--alt-neue-magic=");
    std::string const neu        = arg_wert(argc, argv, "--neu=");
    std::string const gesicherte = arg_wert(argc, argv, "--gesicherte-fixture=");

    if (alt.empty() || alt_magic.empty() || neu.empty()) {
        std::cerr << "usage: test_e24_c10_g5_lade_wache --alt=<so> --alt-neue-magic=<so> --neu=<so> "
                     "[--gesicherte-fixture=<so>]\n";
        return 2;
    }

    // ----------------------------------------------------------------------------------------------
    // (0) Der Host-Stand, gegen den geprueft wird -- aus der Konstante, nicht aus einer Erinnerung.
    // ----------------------------------------------------------------------------------------------
    std::cout << "\n-- Host-ABI-Stand --\n";
    std::cout << "  COMDARE_ANATOMY_ABI_MAJOR = " << COMDARE_ANATOMY_ABI_MAJOR << "\n";
    std::cout << "  COMDARE_ANATOMY_ABI_MINOR = " << COMDARE_ANATOMY_ABI_MINOR << "\n";
    std::cout << "  COMDARE_ANATOMY_ABI_MAGIC = 0x" << std::hex << std::uppercase
              << static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAGIC) << std::dec << std::nouppercase << "\n";
    // NAHT-1 (09.08.2026): 8 -> 9, die Mess-Naht am Genus-Interface. Diese drei Zeilen sind die
    // LAUFZEIT-Haelfte der Major-Wache (die compile-time-Haelfte steht in test_v41_anatomy_module_abi).
    eq("Host-Major ist 9 (NAHT-1: DER MAJOR)", static_cast<int>(COMDARE_ANATOMY_ABI_MAJOR), 9);
    eq("Host-Minor ist 0 (Major-Bump setzt den Minor zurueck)", static_cast<int>(COMDARE_ANATOMY_ABI_MINOR), 0);
    eq("Magic kodiert den Major (\".A9.\")", static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAGIC),
       static_cast<std::uint64_t>(0x434F4D444141392EULL));
    // Der Magic-WECHSEL, belegt statt behauptet: der Alt-Wert unterscheidet sich in GENAU EINEM Byte
    // (dem ASCII-Ziffern-Byte des Majors) -- das ist die Historien-Regel "Magic kodiert den Major".
    {
        constexpr std::uint64_t kAltMagic = 0x434F4D444141382EULL; // ".A8." (Vorgaenger, NAHT-1)
        constexpr std::uint64_t kXor      = kAltMagic ^ COMDARE_ANATOMY_ABI_MAGIC;
        std::cout << "  Magic-Wechsel .A8. -> .A9., XOR = 0x" << std::hex << std::uppercase << kXor << std::dec
                  << std::nouppercase << "\n";
        eq("der Magic-Wechsel bewegt GENAU das Major-Ziffern-Byte ('8' ^ '9')", kXor,
           static_cast<std::uint64_t>(static_cast<std::uint64_t>('8' ^ '9') << 8));
    }

    // ----------------------------------------------------------------------------------------------
    // (1) SCHLOSS 1 -- das echte Alt-Modul (Magic .A7. + Version 7.0).
    // ----------------------------------------------------------------------------------------------
    std::cout << "\n-- Schloss 1: das Alt-Modul (Magic .A7. + Version 7.0) --\n";
    {
        al::AnatomyModuleHandle h;
        int const               st = lade_und_melde(alt, "Alt-Modul", h);
        tr("    das Alt-Modul wird ABGELEHNT (st != status_ok)", st != al::status_ok);
        eq("    Ablehnungs-Grund", std::string{al::status_name(st)}, std::string{"magic_mismatch"});
        tr("    kein Handle entstanden (nichts wurde instanziiert)", !h.valid());
        // FK-7 (C5): eine Lade-Ablehnung ist ein klassifizierter Fehlerpfad, nie eine stille Null.
        auto const klasse = pd::classify_loader_status(st);
        tr("    FK-7: die Ablehnung traegt eine D2-Fehlerklasse", klasse.has_value());
        if (klasse.has_value()) {
            std::cout << "    FK-7-Log: " << pd::dock_failure_log_line(*klasse, "perm_alt_major7", al::status_name(st))
                      << "\n";
            eq("    FK-7-CSV-Zelle", std::string{pd::dock_failure_cell(*klasse)}, std::string{"failed"});
        }
    }

    // ----------------------------------------------------------------------------------------------
    // (2) SCHLOSS 2 -- heutige Magic, aber Major-1: der Major-Vergleich, EINZELN beobachtet.
    // ----------------------------------------------------------------------------------------------
    std::cout << "\n-- Schloss 2: heutige Magic, Version (Host-Major - 1) --\n";
    {
        al::AnatomyModuleHandle h;
        int const               st = lade_und_melde(alt_magic, "Isolierungs-Probe", h);
        tr("    auch dieses Modul wird ABGELEHNT", st != al::status_ok);
        eq("    Ablehnungs-Grund", std::string{al::status_name(st)}, std::string{"abi_major_mismatch"});
        eq("    status_abi_major_mismatch ist 5 (errno-Stil)", al::status_abi_major_mismatch, 5);
        tr("    kein Handle entstanden", !h.valid());
    }

    // ----------------------------------------------------------------------------------------------
    // (3) POSITIV -- der heutige Stand laedt und ist treibbar (sonst belegte die TU nur Ablehnung).
    // ----------------------------------------------------------------------------------------------
    std::cout << "\n-- Positiv-Zweig: das LEBENDE Modul (Host-Major) --\n";
    {
        al::AnatomyModuleHandle h;
        int const               st = lade_und_melde(neu, "lebendes Modul", h);
        eq("    Loader-Status", std::string{al::status_name(st)}, std::string{"ok"});
        if (st == al::status_ok) {
            tr("    handle.valid()", h.valid());
            eq("    Modul-Major", static_cast<int>(h.module_version().major),
               static_cast<int>(COMDARE_ANATOMY_ABI_MAJOR));
            eq("    Modul-Minor", static_cast<int>(h.module_version().minor),
               static_cast<int>(COMDARE_ANATOMY_ABI_MINOR));
            auto* base = h.anatomy();
            tr("    anatomy() != nullptr (Factory lieferte eine Instanz)", base != nullptr);
            if (base != nullptr) {
                auto* tier = dynamic_cast<ana::ISetTier*>(base);
                tr("    dynamic_cast<ISetTier*> ueber die .so-Grenze traegt", tier != nullptr);
                if (tier != nullptr) {
                    tr("    insert(1) ueber die ABI-Grenze", tier->tier_set_insert(1));
                    tr("    insert(1) erneut -> false (Mengen-Semantik ueber die Grenze)", !tier->tier_set_insert(1));
                    eq("    tier_set_size()", tier->tier_set_size(), std::uint64_t{1});
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------------------
    // (4) Der ceb-contract-Beleg des --version-Blocks: die Aussenflaeche des Majors.
    // ----------------------------------------------------------------------------------------------
    std::cout << "\n-- ceb-contract (der --version-Block, K7b-4/G1) --\n";
    {
        // LITERAL gepinnt: "8.1" ist die ABSICHT des B14-NB4-Commits (Major 8 + codegen-Minor 1,
        // Vertrags-Erweiterung kV3AxisSchema[5][5]=line_bytes INNERHALB des Majors). Ein Pin, der nur
        // den abgeleiteten Wert mit sich selbst vergleicht, koennte jede Drehung stillschweigend
        // mitmachen -- deshalb steht hier die Zahl.
        // B14-NB4, AM OBJEKT GEFUNDEN: dieser Pin und der in (4) unten waren beim Bump 8.0 -> 8.1
        // UEBERSEHEN worden; der ctest-Doppellauf hat sie gefangen. Die Zusage im Decl-Header
        // ("der EINE literale Pin" in test_v41_anatomy_module_abi) war damit zum ZWEITEN Mal falsch --
        // sie ist dort jetzt durch die vollstaendige Fundstellen-Liste ersetzt.
        std::string const text = pf::ceb_contract_version_text();
        // NAHT-1 (09.08.2026): 8.1 -> 9.1. Der Major wandert (Mess-Naht am Genus-Interface), der
        // codegen-Minor blieb 1 -- jener Bump beruehrte kV3AxisSchema NICHT.
        // A-14/B-5b (18.08.2026): 9.1 -> 9.2. Jetzt wandert der MINOR: der #15-Bruch erweitert den
        // Vertrag INNERHALB des Majors (POD-Append name_line/len + Preimage-Grammatik). Genau dafuer
        // ist der codegen-Minor da -- und genau deshalb steht die Zahl hier literal und nicht abgeleitet.
        eq("ceb_contract_version_text()", text, std::string{"9.2"});
        // ... und ZUSAETZLICH die Ableitungs-Wache: der Text kommt aus derselben Quelle wie die
        // Lade-Wache oben. Beide zusammen schliessen aus, dass Pin und Wirkung auseinanderlaufen.
        eq("... und er ist aus dem Host-Major abgeleitet",
           std::to_string(COMDARE_ANATOMY_ABI_MAJOR) + "." +
               std::to_string(::comdare::cache_engine::abi::kCebContractCodegenMinor),
           text);

        // Der --version-Block selbst. system_build_version wird als Fixture hereingereicht (der Header
        // ist bewusst .cpp-frei); die ceb-contract-Zeile haengt nicht daran.
        std::string const block = bpf::g1_binary_version_block("comdare@0.0.0-fixture");
        std::cout << "  --version-Block:\n";
        for (std::size_t pos = 0, nl = 0; pos < block.size(); pos = nl + 1) {
            nl = block.find('\n', pos);
            if (nl == std::string::npos) break;
            std::cout << "    " << block.substr(pos, nl - pos) << "\n";
        }
        tr("der --version-Block traegt die Zeile \"ceb-contract=9.2\"",
           block.find("\nceb-contract=9.2\n") != std::string::npos);
    }

    // ----------------------------------------------------------------------------------------------
    // (5) OPTIONAL: die ausserhalb des Repos gesicherte Original-Fixture.
    // ----------------------------------------------------------------------------------------------
    std::cout << "\n-- gesicherte Original-Fixture (optional) --\n";
    if (gesicherte.empty()) {
        std::cout << "  uebersprungen: kein --gesicherte-fixture=<pfad> uebergeben (ctest faehrt die "
                     "committete Nachbildung perm_alt_major7; der Lauf gegen das Original steht im "
                     "Fenster-Report).\n";
    } else if (!std::filesystem::exists(gesicherte)) {
        // MELDEN statt still uebergehen: ein angegebener, aber fehlender Pfad ist ein Befund.
        std::cout << "  [ERR] --gesicherte-fixture wurde angegeben, existiert aber nicht: " << gesicherte << "\n";
        ++g_fail;
    } else {
        al::AnatomyModuleHandle h;
        int const               st = lade_und_melde(gesicherte, "gesicherte Major-7-Fixture", h);
        tr("    die gesicherte Original-Fixture wird ABGELEHNT", st != al::status_ok);
        eq("    Ablehnungs-Grund", std::string{al::status_name(st)}, std::string{"magic_mismatch"});
        tr("    kein Handle entstanden", !h.valid());
    }

    std::cout << "\n==== E-24 C10 (b) / G5: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
