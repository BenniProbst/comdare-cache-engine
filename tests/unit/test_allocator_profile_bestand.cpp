// test_allocator_profile_bestand -- Abdeckungs-Gate fuer algorithm_profiles/allocators/*.profile.xml.
//
// WARUM DIESES GATE EXISTIERT
// ---------------------------
// Die 23 Allokator-Akten sind DOKU-AKTEN: jede traegt den Kopf-Kommentar
//   "DOKU-AKTE (#48 Scheibe 2, 2026-07-22): kein Code-Leser; Allocator-Wahl = CT-Adapter
//    (Statischer-Dispatch-Doktrin)."
// Sie haben also BEWUSST keinen Produktions-Leser -- die Allokator-Wahl faellt zur Compile-Zeit ueber
// CT-Adapter, nicht ueber einen XML-Laufzeit-Leser. Das ist der Unterschied zur Nachbar-Familie
// load_profiles/ (dynamische Workload-Achse 2, darum via discover_load_profiles verdrahtet).
//
// Genau daraus folgt das Risiko, das dieses Gate abdeckt: ein Aktenbestand, den KEIN Code liest, kann
// beliebig verrotten, ohne dass irgendein Bau oder Test rot wird -- obwohl er eine Aussage der Abgabe
// traegt. Die Thesis (thesis-Repo, kapitel/de/03_state_of_the_art.tex + kapitel/de/03_messsystem_prtart.tex,
// Tabelle \label{tab:allocator-profiles}) listet ZEHN Allokator-Profile namentlich mit Familie, Lizenz und
// expected_workload und behauptet im Fliesstext "alle 10 Allokator-Profile sind getaggt". Verschwindet eine
// dieser zehn Akten oder verliert sie ihr expected_workload, wird die Thesis-Tabelle unbelegt -- heute
// wuerde das NICHTS bemerken. Dieses Gate bemerkt es.
//
// Das Gate liest mit dem BESTANDS-Reader (comdare::common::xml, KF-1) -- kein zweiter Parser, keine
// zweite Mechanik. Es verdrahtet die Akten NICHT in den Produktionspfad; das waere ein Runtime-Switch
// gegen die Statischer-Dispatch-Doktrin und damit eine Regression, kein Fortschritt.
//
// SELBSTCHECK: prueft NUR Existenz und Wohlgeformtheit des Aktenbestands (Metadaten). Es prueft NICHT
// die inhaltliche Richtigkeit von Lizenz-, Venue- oder Jahr-Angaben und NICHT deren Uebereinstimmung mit
// der Thesis-Tabelle (anderes Repo, hier nicht lesbar). Es beruehrt keine Messung: permutation_axes.xml,
// binary_id und golden bleiben unangetastet.

#include <serialization/xml_config_parser/xml_reader.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

namespace cx = ::comdare::common::xml;

int g_fail = 0;

void tr(std::string const& what, bool cond) {
    std::cout << (cond ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!cond) ++g_fail;
}

// Der ERWARTETE Aktenbestand. Woher die Zahl kommt: es sind die real committeten Akten in
// libs/cache_engine/algorithm_profiles/allocators/ (Stand 2026-08-07, `ls *.profile.xml` = 23).
// Die Zahl ist bewusst hart, damit ein STILLER Verlust auffaellt -- ein Bestands-Gate, das sich seine
// Erwartung aus demselben Verzeichnis holt, das es prueft, kann per Konstruktion nichts entdecken.
//
// WENN EINE AKTE DAZUKOMMT ODER WEGFAELLT: diese Zahl hier mitziehen. Das ist Absicht, keine Huerde --
// der rote Test ist die Aufforderung, den Zuwachs bewusst zu quittieren statt ihn durchrutschen zu
// lassen. Kommt eine Akte hinzu, gehoert sie ausserdem in die README des Verzeichnisses (die zum
// Stand 2026-08-07 noch "Mitglieder (10)" behauptet und damit selbst hinter dem Bestand zurueckliegt).
constexpr std::size_t kErwarteteAktenAnzahl = 23;

// Die ZEHN Profile der Thesis-Tabelle \label{tab:allocator-profiles}. Diese Liste ist der eigentliche
// Abgabe-Anker: sie stammt NICHT aus dem Verzeichnis, sondern aus der Thesis (thesis-Repo, Stand
// eaf7fe8, kapitel/de/03_state_of_the_art.tex Tabelle bei Zeile 468ff. sowie die inhaltsgleiche Tabelle
// in kapitel/de/03_messsystem_prtart.tex; EN-Entsprechungen unter kapitel/en/). Faellt eine dieser
// Akten weg, verliert eine gedruckte Tabelle ihren Beleg -- darum werden sie NAMENTLICH gefordert und
// nicht bloss mitgezaehlt.
std::vector<std::string> const& thesis_tabellen_profile() {
    static std::vector<std::string> const ids{"hoard",    "michael_lockfree", "mimalloc", "jemalloc", "tcmalloc",
                                              "snmalloc", "scalloc",          "rpmalloc", "lrmalloc", "dlmalloc"};
    return ids;
}

[[nodiscard]] std::string read_file(std::filesystem::path const& p) {
    std::ifstream      in{p, std::ios::binary};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

struct AktenBefund {
    std::string dateiname;
    std::string id;
    std::string family_ref;
    std::string expected_workload;
    bool        lesbar = false; // wohlgeformtes XML MIT Wurzel comdare_allocator_profile
};

// Parst EINE Akte mit dem Bestands-Reader. `lesbar` == parse_document lieferte ein Dokument UND die
// Wurzel ist <comdare_allocator_profile>. Kein stiller Fallback auf eine fremde Wurzel -- der wuerde
// genau die Faelschung zulassen, die das Gate ausschliessen soll (gleiche Entscheidung wie in
// XmlConfigParser::parse_profile).
//
// GRENZE, am Objekt gemessen (Bissprobe 2026-08-07): der KF-1-Reader ist FEHLERTOLERANT, nicht
// validierend. Eine Akte mit nicht geschlossenem Tag liefert trotzdem ein DOM mit korrekter Wurzel und
// zaehlt hier als `lesbar`. Die Haerte gegen verstuemmelte Akten kommt darum NICHT aus diesem Flag,
// sondern aus der Pflichtfeld-Pruefung in main() (id / family_ref / expected_workload): im Biss fiel
// das abgeschnittene <expected_workload> weg und der Test wurde rot. Wer hier je eine echte
// Wohlgeformtheits-Zusage braucht, muss den Reader haerten -- dieses Gate behauptet sie nicht.
[[nodiscard]] AktenBefund parse_allocator_akte(std::filesystem::path const& p) {
    AktenBefund b;
    b.dateiname = p.filename().string();

    auto root = cx::parse_document(read_file(p));
    if (!root.has_value() || root->tag != "comdare_allocator_profile") return b;

    b.lesbar     = true;
    b.id         = root->attr("id");
    b.family_ref = root->attr("family_ref");
    if (auto const* ew = root->child("expected_workload")) b.expected_workload = ew->text;
    return b;
}

} // namespace

int main() {
    std::cout << "==== Abdeckungs-Gate: allocators/*.profile.xml (Doku-Akten, #48 Scheibe 2) ====\n";

    std::filesystem::path const dir{COMDARE_CE_ALLOCATOR_PROFILES_DIR};
    std::cout << "Aktenverzeichnis: " << dir.string() << "\n";

    // (1) NENNER-PRUEFUNG, Teil 1: das Verzeichnis muss ueberhaupt existieren. Ohne diese Pruefung
    //     wuerde ein verschobenes/umbenanntes Verzeichnis in einen GRUENEN Lauf mit 0 Akten muenden --
    //     der Defekt, den dieses Gate gerade verhindern soll.
    std::error_code ec;
    bool const      dir_da = std::filesystem::is_directory(dir, ec);
    tr("Aktenverzeichnis existiert", dir_da);
    if (!dir_da) {
        std::cout << "\nNENNER = 0 (Verzeichnis fehlt) -- Gate ROT.\n";
        return 1;
    }

    std::vector<std::filesystem::path> akten;
    for (auto const& e : std::filesystem::directory_iterator(dir, ec)) {
        if (!e.is_regular_file()) continue;
        std::string const name = e.path().filename().string();
        if (name.size() > std::string_view{".profile.xml"}.size() && name.ends_with(".profile.xml"))
            akten.push_back(e.path());
    }
    std::sort(akten.begin(), akten.end());

    // (2) NENNER-PRUEFUNG, Teil 2: der Nenner steht SICHTBAR in der Ausgabe und 0 ist hart rot.
    //     "Gruen mit Nenner 0" waere die gefaehrlichste Form des Bestehens.
    std::cout << "\nNENNER: " << akten.size() << " Akten gefunden (erwartet: " << kErwarteteAktenAnzahl << ")\n";
    tr("Nenner ist nicht 0 (es wurde ueberhaupt etwas geprueft)", !akten.empty());
    if (akten.empty()) {
        std::cout << "\nGate ROT: 0 Akten -- jede weitere Pruefung waere vakuum-wahr.\n";
        return 1;
    }

    // (3) Bestands-Zahl: stiller Zuwachs/Verlust faellt auf.
    tr("Aktenanzahl == erwarteter Bestand (" + std::to_string(kErwarteteAktenAnzahl) + ")",
       akten.size() == kErwarteteAktenAnzahl);

    // (4) Jede einzelne Akte parsen -- Pflichtfelder sind id, family_ref und expected_workload.
    //     expected_workload ist Pflicht, weil die Thesis genau diese Spalte tabelliert und im
    //     Fliesstext behauptet, die Profile seien "getaggt".
    std::cout << "\n-- Einzelpruefung je Akte --\n";
    std::vector<AktenBefund> befunde;
    std::size_t              lesbar_count = 0;
    for (auto const& p : akten) {
        AktenBefund const b = parse_allocator_akte(p);
        befunde.push_back(b);
        if (b.lesbar) ++lesbar_count;

        bool const  vollstaendig = b.lesbar && !b.id.empty() && !b.family_ref.empty() && !b.expected_workload.empty();
        std::string detail       = b.dateiname;
        if (b.lesbar)
            detail +=
                "  (id=" + b.id + ", family_ref=" + b.family_ref + ", expected_workload=" + b.expected_workload + ")";
        else
            detail += "  (UNLESBAR: kein Dokument mit Wurzel <comdare_allocator_profile>)";
        tr(detail, vollstaendig);
    }
    std::cout << "\nPARSE-QUOTE: " << lesbar_count << "/" << akten.size() << " Akten sauber geparst\n";

    // (5) ids muessen paarweise eindeutig sein -- zwei Akten mit derselben id wuerden einander in
    //     jeder id-basierten Auswertung verdecken.
    std::set<std::string> ids;
    std::size_t           doppelte = 0;
    for (auto const& b : befunde)
        if (!b.id.empty() && !ids.insert(b.id).second) {
            ++doppelte;
            std::cout << "  doppelte id: " << b.id << " (" << b.dateiname << ")\n";
        }
    tr("alle ids paarweise eindeutig", doppelte == 0);

    // (6) DER ABGABE-ANKER: die zehn namentlich in der Thesis tabellierten Profile muessen da sein.
    std::cout << "\n-- Thesis-Anker (Tabelle tab:allocator-profiles, 10 Profile) --\n";
    std::size_t gefunden = 0;
    for (auto const& soll : thesis_tabellen_profile()) {
        auto const it = std::find_if(befunde.begin(), befunde.end(), [&soll](AktenBefund const& b) {
            return b.lesbar && b.id == soll && !b.expected_workload.empty();
        });
        bool const da = (it != befunde.end());
        if (da) ++gefunden;
        tr("Thesis-Tabellenzeile belegt: " + soll, da);
    }
    std::cout << "THESIS-ANKER: " << gefunden << "/" << thesis_tabellen_profile().size()
              << " tabellierte Profile durch eine lesbare, getaggte Akte belegt\n";

    std::cout << "\n==== Ergebnis: " << (g_fail == 0 ? "GRUEN" : "ROT") << " (" << g_fail << " Fehler, Nenner "
              << akten.size() << ") ====\n";
    return (g_fail == 0) ? 0 : 1;
}
