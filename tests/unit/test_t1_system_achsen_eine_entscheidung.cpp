// -----------------------------------------------------------------------------
// test_t1_system_achsen_eine_entscheidung.cpp -- T-1: ZWEI PARSES DESSELBEN PROFILS, EINE ENTSCHEIDUNG.
//
// SELBSTCHECK dieses Blocks: jede Zahl unten wird von diesem Programm SELBST erhoben und mit Nenner
// gedruckt; keine ist aus einer Doku abgeschrieben. Der Koeder in kKoeder ist frisch gewuerfelt
// (head -c9 /dev/urandom | base32 | tr A-Z a-z) und steht NUR hier -- er beweist beim Lauf, dass die
// ausgefuehrte Binary aus DIESER Quelle stammt und nicht aus einem alten Objekt.
//
// DIE NAHT, UM DIE ES GEHT. Zwei Stellen entscheiden ueber DIESELBE Frage ("deklariert dieses Profil
// System-Achsen?") und muessen dieselbe Antwort geben:
//   FACADE  profile_run_facade.cpp -- entscheidet, ob a.compile_for_perm BELEGT wird.
//   ENTRY   profile_run_entry.hpp  -- entscheidet, ob die opt x simd-Perm-SCHLEIFE ueberhaupt LAEUFT.
// Driften sie, laeuft die Schleife OHNE compile_for_perm: perm_bau_je_zelle ist false, der per-Perm-
// Fingerprint-Provider greift NICHT, es wirkt der lauf-konstante -- und jede Zelle bekommt denselben
// Digest. Dann sind die Zell-Koordinaten der einzige Diskriminator im Lager-Schluessel.
//
// DER BEFUND, DEN DIESER TEST ZUERST FESTHAELT (Fall AEQUIVALENZ). Ueber EIN UND DENSELBEN Parse sind
// die beiden Ausdruecke De-Morgan-Duale und damit fuer JEDES Profil gleich:
//     FACADE  !opt.empty() || !simd.empty()
//     ENTRY   !( opt.empty() && simd.empty() )
// Es gibt also KEIN Profil, das sie ueber denselben Parse trennt. Dieser Test misst das (mit Nenner),
// statt es zu behaupten.
//
// WAS SIE TROTZDEM TRENNT -- UND ZWAR HEUTE UND OHNE JEDE CODE-AENDERUNG: es sind ZWEI UNABHAENGIGE
// PARSES DERSELBEN DATEI zu ZWEI ZEITPUNKTEN. Die Facade parst einmal, dann laeuft Arbeit (Lastprofil-
// Entdeckung, Methodik-Aufloesung, Aufbau der Achsen-Versionstabelle), dann parst run_profile die
// GLEICHE Datei ein zweites Mal. Aendert sich die Datei dazwischen, antworten die beiden Parses
// verschieden -- und nichts vergleicht sie. GENAU DAS bauen die Faelle TRENNUNG-VOR/-RUECK nach: real
// geschriebene Dateien, der echte Parser, eine echte Mutation dazwischen.
//
// DER ZWANG. Die Entscheidung wird ab jetzt EINMAL gefaellt (profil_deklariert_system_achsen) und als
// Wert MITGEFUEHRT; run_profile prueft den mitgefuehrten Wert gegen seinen eigenen Parse
// (system_achsen_entscheidung_haelt) und schaltet bei Drift FAIL-CLOSED ab. NichtGetragen bleibt
// zulaessig (Direkt-Aufrufer ohne Facade) und haelt immer -- sonst waere die Wache ein Bruch.
//
// DREI BETRIEBSARTEN (argv[1]):
//   (ohne)        Die Wache. Alle Faelle muessen halten. rc=0 bei "alle gehalten".
//   --mutant      Der MUTANT: der mitgefuehrte Wert wird durch einen ZWEITEN PARSE DES PFADES ersetzt --
//                 also exakt die heutige Struktur ohne Mitfuehrung. Bewertet wird normal -> die
//                 TRENNUNG-Faelle MUESSEN reissen, rc=1. Das ist der literale ROT-Beleg; er wird nicht
//                 behauptet, sondern gefahren.
//   --selbstbiss  Derselbe Mutant, aber die ERWARTUNG ist umgedreht: rc=0 nur, wenn GENAU die
//                 TRENNUNG-Faelle gerissen sind UND die SKIP-/AEQUIVALENZ-/DETERMINISMUS-Faelle
//                 gehalten haben. So faehrt der Biss bei jedem CI-Lauf mit, statt hier zu verjaehren.
//
// BEOBACHTBARKEIT DES MUTANTEN (Pflichtpruefung vor dem Koeder): der Mutant unterscheidet sich vom
// Original genau in den Faellen, in denen sich die Datei ZWISCHEN den Parsen aendert -- dort liefert
// der zweite Parse denselben Inhalt wie der eigene Parse, die Wache sieht Gleichheit und meldet
// faelschlich "haelt". In den SKIP-Faellen aendert sich nichts, dort sind Mutant und Original
// wertgleich. Es gibt also einen Eingang, bei dem sich MIT und OHNE unterscheidet -- und einen, bei
// dem sie sich nicht unterscheiden duerfen. Beide werden gefahren.
//
// Doktrin: header-only C++23, ASCII-Kommentare, eigener main() (kein gtest-Link), 0 = Erwartung gehalten.
// -----------------------------------------------------------------------------

#include "profile_facade/system_axes_entscheidung.hpp" // DIE EINE ENTSCHEIDUNG + ihre Wache

#include "xml_config_parser/xml_config_parser.hpp" // XmlConfigParser / ThesisProfile (der echte Parser)

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cx  = ::comdare::builder::xml;
namespace pfe = ::comdare::cache_engine::profile_facade;
namespace fs  = std::filesystem;

namespace {

// K13: FRISCH GEWUERFELT (head -c9 /dev/urandom | base32 | tr A-Z a-z), nicht abgeschrieben.
constexpr char const* kKoeder = "4e2ec7b6bypjqca=";

// Die Sorten entscheiden, was --selbstbiss von jedem Fall namentlich erwartet.
enum class Sorte { Trennung, Skip, Aequivalenz, Determinismus };

// Die drei Felder tragen einen expliziten Default, obwohl JEDE Konstruktion (notiere(), unten)
// alle drei setzt: cppcheck 2.21 meldet uninitMemberVarNoCtor sonst als style-Verstoss, und
// lint:static faellt darauf mit exit 2 -- gemessen in ce-Pipeline 15482/15485, Job 370390.
// Semantisch neutral (die Werte werden ohnehin ueberschrieben), aber der Default macht aus
// "zufaellig immer gesetzt" ein "kann gar nicht ungesetzt sein".
struct Fall {
    std::string name;
    Sorte       sorte    = Sorte::Trennung;
    bool        gehalten = false;
};

std::vector<Fall> g_faelle;

void notiere(std::string name, Sorte sorte, bool gehalten) {
    std::cout << (gehalten ? "  [HALT] " : "  [RISS] ") << name << "\n";
    g_faelle.push_back(Fall{std::move(name), sorte, gehalten});
}

// -- Der Profil-Baukasten. MINIMAL, aber echt: der Parser laeuft ueber genau diese Bytes. ------------
// `system_achsen` ist der einzige bewegliche Teil -- alles andere ist in beiden Fassungen identisch,
// damit ein Riss NUR aus <system_axes> stammen kann und nicht aus irgendeiner Nachbar-Aenderung.
std::string profil_text(std::string_view system_achsen) {
    std::string s;
    s += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    s += "<comdare_thesis_profile id=\"t1_naht\" schema_version=\"1\">\n";
    s += "  <permute_axes>\n";
    s += "    <axis ref=\"search_algo\"><value>art</value></axis>\n";
    s += "  </permute_axes>\n";
    s += "  <compile_dims>\n";
    s += "    <workloads>YCSB_A</workloads>\n";
    s += "  </compile_dims>\n";
    s += "  <modes>\n";
    s += "    <mode name=\"ce_only\" merge=\"Stufe1_CeOnly\" active_axes=\"search_algo\"/>\n";
    s += "  </modes>\n";
    s += system_achsen;
    s += "</comdare_thesis_profile>\n";
    return s;
}

// OHNE <system_axes> -- beide Wege muessen "nein" sagen.
std::string const kOhne = profil_text("");
// MIT <opt_level> -- beide Wege muessen "ja" sagen.
std::string const kMitOpt = profil_text("  <system_axes>\n"
                                        "    <compiler><opt_level>\n"
                                        "      <option value=\"O2\"/>\n"
                                        "      <option value=\"O3\"/>\n"
                                        "    </opt_level></compiler>\n"
                                        "  </system_axes>\n");
// MIT <simd> allein -- der zweite Zweig des ODER; er darf die Antwort genauso tragen.
std::string const kMitSimd = profil_text("  <system_axes>\n"
                                         "    <external_utils><simd>\n"
                                         "      <option value=\"avx2\"/>\n"
                                         "      <option value=\"avx512\"/>\n"
                                         "    </simd></external_utils>\n"
                                         "  </system_axes>\n");
// MIT <system_axes>, aber LEEREN Containern -- die Grenze: deklariert, traegt aber keine Auspraegung.
std::string const kLeer = profil_text("  <system_axes>\n"
                                      "    <compiler><opt_level/></compiler>\n"
                                      "    <external_utils><simd/></external_utils>\n"
                                      "  </system_axes>\n");
// MIT <atomic128> allein -- eine ANDERE Unter-Achse unter <compiler>: sie ist KEIN opt_level und
// KEIN simd, also muessen BEIDE Wege "nein" sagen. Trennt die Wege eine Unter-Achse, faellt es hier auf.
std::string const kAtomic = profil_text("  <system_axes>\n"
                                        "    <compiler><atomic128>\n"
                                        "      <option value=\"cx16\"/>\n"
                                        "    </atomic128></compiler>\n"
                                        "  </system_axes>\n");

void schreibe(fs::path const& p, std::string const& text) {
    std::ofstream os(p, std::ios::binary | std::ios::trunc);
    os << text;
    os.flush();
}

std::optional<cx::ThesisProfile> parse(fs::path const& p) {
    cx::XmlConfigParser const parser;
    return parser.parse_thesis_profile(p);
}

// DER ENTRY-WEG, WOERTLICH wie er in profile_run_entry.hpp steht (negiert, weil dort der NICHT-Walk-
// Zweig geschrieben ist). Diese Kopie existiert NUR fuer den Aequivalenz-Befund unten -- die Produktion
// ruft nach dem Zwang die gemeinsame Funktion.
bool entry_walk_wortlaut(cx::ThesisProfile const& tp) {
    return !(tp.compiler.opt_levels.empty() && tp.external_utils.simd_options.empty());
}

// DER FACADE-WEG, WOERTLICH wie er in profile_run_facade.cpp steht (inkl. der nullopt-Behandlung).
bool facade_wortlaut(std::optional<cx::ThesisProfile> const& tp_opt) {
    return tp_opt && (!tp_opt->compiler.opt_levels.empty() || !tp_opt->external_utils.simd_options.empty());
}

// -- Die zwei Parses, wie die Produktion sie wirklich fahrt. -----------------------------------------
// `vorher` ist der Inhalt zum Zeitpunkt des FACADE-Parses, `nachher` der zum Zeitpunkt des ENTRY-
// Parses. Sind beide gleich, ist das der ungestoerte Lauf. Zurueck kommt, ob die WACHE die Lage
// akzeptiert -- true heisst "die eine Entscheidung haelt".
bool wache_akzeptiert(fs::path const& p, std::string const& vorher, std::string const& nachher, bool mutant) {
    schreibe(p, vorher);
    std::optional<cx::ThesisProfile> const facade_parse = parse(p);
    // Die Facade faellt die Entscheidung EINMAL und fuehrt sie als Wert mit.
    pfe::SystemAchsenEntscheidung getragen =
        facade_parse ? pfe::system_achsen_entscheidung_von(pfe::profil_deklariert_system_achsen(*facade_parse))
                     : pfe::SystemAchsenEntscheidung::Nein;

    schreibe(p, nachher); // <-- die Mutation ZWISCHEN den Parsen (bei vorher==nachher: keine)

    std::optional<cx::ThesisProfile> const entry_parse = parse(p);
    if (!entry_parse) {
        notiere("VORBEDINGUNG: der Entry-Parse ist lesbar", Sorte::Skip, false);
        return false;
    }
    if (mutant) {
        // DER MUTANT: statt des mitgefuehrten Wertes wird der PFAD ein zweites Mal geparst -- exakt die
        // heutige Struktur (zwei unabhaengige Parses, kein Vergleich). Der zweite Parse sieht denselben
        // Inhalt wie der Entry-Parse, also stimmt er per Konstruktion immer zu.
        std::optional<cx::ThesisProfile> const zweiter = parse(p);
        getragen = zweiter ? pfe::system_achsen_entscheidung_von(pfe::profil_deklariert_system_achsen(*zweiter))
                           : pfe::SystemAchsenEntscheidung::Nein;
    }
    return pfe::system_achsen_entscheidung_haelt(getragen, *entry_parse);
}

} // namespace

int main(int argc, char** argv) {
    std::string_view const modus  = (argc > 1) ? std::string_view{argv[1]} : std::string_view{};
    bool const             mutant = (modus == "--mutant") || (modus == "--selbstbiss");
    bool const             biss   = (modus == "--selbstbiss");

    std::cout << "T-1 EINE ENTSCHEIDUNG -- Koeder=" << kKoeder
              << " Modus=" << (modus.empty() ? std::string_view{"(wache)"} : modus) << "\n";

    std::error_code ec;
    fs::path const  base = fs::temp_directory_path(ec) / "comdare_t1_eine_entscheidung";
    fs::remove_all(base, ec);
    fs::create_directories(base, ec);
    fs::path const p = base / "t1.profile.xml";

    // -- (1) AEQUIVALENZ-BEFUND: ueber EINEN Parse koennen die beiden Wortlaute nicht auseinanderlaufen.
    //        Gemessen, nicht behauptet -- mit Nenner.
    struct Form {
        char const*        name;
        std::string const* text;
    };
    std::vector<Form> const formen{{"ohne <system_axes>", &kOhne},
                                   {"<opt_level> gefuellt", &kMitOpt},
                                   {"<simd> gefuellt", &kMitSimd},
                                   {"<system_axes> mit LEEREN Containern", &kLeer},
                                   {"nur <atomic128>", &kAtomic}};
    std::size_t             gleich = 0;
    for (auto const& f : formen) {
        schreibe(p, *f.text);
        std::optional<cx::ThesisProfile> const tp = parse(p);
        if (!tp) {
            notiere(std::string{"AEQUIVALENZ-VORBEDINGUNG parsbar: "} + f.name, Sorte::Aequivalenz, false);
            continue;
        }
        bool const wf = facade_wortlaut(tp);
        bool const we = entry_walk_wortlaut(*tp);
        // Und dieselbe Frage ueber die GEMEINSAME Funktion -- sie muss beide Wortlaute reproduzieren.
        bool const wg = pfe::profil_deklariert_system_achsen(*tp);
        if (wf == we && we == wg) ++gleich;
    }
    std::cout << "  [BEFUND] ueber EINEN Parse antworten Facade-Wortlaut, Entry-Wortlaut und die gemeinsame\n"
              << "           Funktion identisch in " << gleich << " von " << formen.size() << " Profil-Formen.\n";
    notiere("AEQUIVALENZ: kein Profil trennt die Wege ueber denselben Parse", Sorte::Aequivalenz,
            gleich == formen.size());

    // -- (2) TRENNUNG-VOR: die Datei gewinnt <system_axes> ZWISCHEN den Parsen.
    //        Facade sagt "nein" (kein compile_for_perm), Entry laeuft die Schleife. GENAU DER DEFEKT.
    notiere("TRENNUNG-VOR: ohne -> mit <opt_level> zwischen den Parsen wird GEFANGEN", Sorte::Trennung,
            !wache_akzeptiert(p, kOhne, kMitOpt, mutant));

    // -- (3) TRENNUNG-RUECK: die Gegenrichtung. Facade sagt "ja" (Basis-build_version OHNE Suffix,
    //        compile_for_perm belegt), Entry laeuft die Schleife NICHT -- die Provenienz faellt weg.
    notiere("TRENNUNG-RUECK: mit -> ohne <system_axes> zwischen den Parsen wird GEFANGEN", Sorte::Trennung,
            !wache_akzeptiert(p, kMitOpt, kOhne, mutant));

    // -- (4) TRENNUNG-ACHSE: von <opt_level> auf <simd> ist KEINE Trennung (beide sagen "ja") -- aber von
    //        <simd> auf die LEEREN Container ist eine. Zeigt, dass die Wache am WERT haengt, nicht am Text.
    notiere("TRENNUNG-ACHSE: <simd> -> leere Container zwischen den Parsen wird GEFANGEN", Sorte::Trennung,
            !wache_akzeptiert(p, kMitSimd, kLeer, mutant));

    // -- (5..8) SKIP: ohne Mutation MUSS die Wache halten. Ein Waechter, der immer abbricht, waere
    //           genauso falsch wie einer, der nie abbricht -- er machte jeden regulaeren Lauf unmoeglich.
    notiere("SKIP: unveraendert ohne <system_axes> haelt", Sorte::Skip, wache_akzeptiert(p, kOhne, kOhne, mutant));
    notiere("SKIP: unveraendert mit <opt_level> haelt", Sorte::Skip, wache_akzeptiert(p, kMitOpt, kMitOpt, mutant));
    notiere("SKIP: unveraendert mit <simd> haelt", Sorte::Skip, wache_akzeptiert(p, kMitSimd, kMitSimd, mutant));
    notiere("SKIP: <opt_level> -> <simd> ist KEINE Drift (beide 'ja') und haelt", Sorte::Skip,
            wache_akzeptiert(p, kMitOpt, kMitSimd, mutant));

    // -- (9) NICHT-GETRAGEN: ein Direkt-Aufrufer ohne Facade fuehrt keinen Wert mit. Die Wache darf ihn
    //        NICHT abschiessen -- sonst waere sie ein Bruch statt einer Wache.
    {
        schreibe(p, kMitOpt);
        std::optional<cx::ThesisProfile> const tp = parse(p);
        notiere("NICHT-GETRAGEN: ein Aufrufer ohne mitgefuehrte Entscheidung haelt immer", Sorte::Skip,
                tp && pfe::system_achsen_entscheidung_haelt(pfe::SystemAchsenEntscheidung::NichtGetragen, *tp));
    }

    // -- (10) DETERMINISMUS: zweimal dieselbe unveraenderte Datei -> zweimal dieselbe Entscheidung.
    //         Ein lauf-variabler Anteil machte jede Wache unbrauchbar.
    {
        schreibe(p, kMitSimd);
        std::optional<cx::ThesisProfile> const a = parse(p);
        std::optional<cx::ThesisProfile> const b = parse(p);
        bool const det = a && b && pfe::profil_deklariert_system_achsen(*a) == pfe::profil_deklariert_system_achsen(*b);
        notiere("DETERMINISMUS: derselbe Inhalt ergibt dieselbe Entscheidung", Sorte::Determinismus, det);
    }

    fs::remove_all(base, ec);

    // -- Auswertung. Jede Zahl mit Nenner. ------------------------------------------------------------
    std::size_t gehalten = 0;
    std::size_t tren_ges = 0;
    std::size_t tren_ris = 0;
    std::size_t rest_ges = 0;
    std::size_t rest_hal = 0;
    for (auto const& f : g_faelle) {
        if (f.gehalten) ++gehalten;
        if (f.sorte == Sorte::Trennung) {
            ++tren_ges;
            if (!f.gehalten) ++tren_ris;
        } else {
            ++rest_ges;
            if (f.gehalten) ++rest_hal;
        }
    }
    std::cout << "  ERGEBNIS: " << gehalten << " von " << g_faelle.size() << " Faellen gehalten"
              << "  (Trennung: " << tren_ris << " von " << tren_ges << " gerissen; uebrige: " << rest_hal << " von "
              << rest_ges << " gehalten)\n";

    if (biss) {
        // SELBSTBISS: der Mutant muss GENAU die Trennungs-Faelle reissen und die uebrigen halten.
        bool const ok = (tren_ris == tren_ges) && (rest_hal == rest_ges) && tren_ges > 0;
        std::cout << (ok ? "  SELBSTBISS GEHALTEN: der Mutant reisst genau die Trennungs-Faelle\n"
                         : "  SELBSTBISS VERFEHLT: der Mutant reisst nicht genau die Trennungs-Faelle\n");
        return ok ? 0 : 1;
    }
    bool const alle = (gehalten == g_faelle.size()) && !g_faelle.empty();
    std::cout << (alle ? "  ALLE FAELLE GEHALTEN\n" : "  MINDESTENS EIN FALL GERISSEN\n");
    return alle ? 0 : 1;
}
