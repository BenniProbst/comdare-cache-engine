// tests/unit/test_rf3_betriebssystem_feature_fehlt.cpp -- RF-3 (§70.3), 26.07.2026. UNREGISTRIERT.
//
// Absichtlich NICHT in tests/unit/CMakeLists.txt eingetragen (Sammel-Registrierung am Join, wie die
// Lane-C-Guards und der A9b-Guard): der Voll-ctest-Zaehler bleibt dadurch unveraendert. Bewusst OHNE
// gtest geschrieben, damit der Hand-Bau eine einzige Zeile ohne Link-Satz ist. Literal gelaufen (26.07.),
// hier ohne Zeilenfortsetzung notiert, weil ein Backslash am Zeilenende einen //-Kommentar fortsetzt
// (-Wcomment):
//
//   g++ -std=c++23 -O0 -Wall -I libs/cache_engine/include tests/unit/test_rf3_betriebssystem_feature_fehlt.cpp -o <build>/test_rf3_guard
//
// WAS HIER BEWIESEN WIRD. RF-3 zieht die fuenfte D1-Fehlerklasse ein: BetriebssystemFeatureFehlt, das
// OS-Analogon zu HardwareErweiterungFehlt. Nicht die Hardware fehlt, sondern eine OS-Faehigkeit ist auf
// dem Host nicht da oder nicht konfiguriert -- hugetlbfs nicht gemountet, Kernel-Feature nicht gebaut,
// OS-Schnittstelle abwesend. Das belegte Material der OS-Flotten-Erhebung: musl-libc-Abweichung unter
// Alpine, BSD-/msys-grep-Portabilitaetsluecken unter macOS und Windows, PowerShell-Executor-Semantik der
// Windows-Runner. Solche Zustaende gehoeren deklariert, nicht abgestuerzt.
//
// Die Klasse ist heute DEKLARIERT UND GETESTET, aber ohne Werfer: kein Aufrufer klassifiziert etwas als
// BetriebssystemFeatureFehlt. Der erste echte Nutzer kommt mit V-2 / den hugetlbfs-Pfaden. Genau deshalb
// pruefen die Teile 1-4 die TAXONOMIE (Nummer, Etikett, Wachen, Disjunktheit) und nicht ein Verhalten.
//
// Vier Teile:
//   Teil 1  NUMMER + ANHAENGEN  -- die neue Klasse ist 4, die vier Bestands-Nummern 0..3 sind
//                                  UNVERAENDERT. Das ist die Vertraeglichkeits-Aussage: Etiketten reisen
//                                  in Experiment-Logs, eine Umnummerierung waere ein stiller Bruch.
//   Teil 2  ETIKETT             -- "betriebssystem_feature_fehlt", und ALLE fuenf Etiketten sind
//                                  PAARWEISE verschieden. Diese Distinktheit prueft der registrierte
//                                  test_axis_error_taxonomy NICHT (er fuellt sein `seen`-Array, ohne es
//                                  zu vergleichen); hier steht der Beweis.
//   Teil 3  DRIFT-WACHEN        -- Count == 5 == letzter Enum-Wert + 1, und kein Wert faellt in den
//                                  "unbekannt"-Default des Etiketten-Switch.
//   Teil 4  DISJUNKTHEIT        -- die D2-Taxonomie (SampleStatus) bleibt bei 4 und ist unberuehrt; die
//                                  neue Klasse ist D1-only. D1 und D2 sind laut Kopf-Doku disjunkt.
//
// NEGATIV-BEWEIS zur Drift-Wache (26.07., Schatten-Kopie im Scratchpad, Arbeitsbaum unberuehrt): haengt
// man dem Enum eine SECHSTE Klasse an, ohne kCompilerCompilerErrorClassCount hochzuzaehlen, bricht die
// Uebersetzung an der static_assert in axis_error.hpp. Die Wache ist also scharf und nicht dekorativ.

#include <cache_engine/measurement/axis_error.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace cem = ::comdare::cache_engine::measurement;
using Klasse  = cem::CompilerCompilerErrorClass;

static int g_fail = 0;

// Interne Bindung (static) fuer beide Helfer: seit der FK-0-Registrierung uebersetzt der Test unter dem
// Projekt-Warnsatz (COMDARE_set_default_warnings, u.a. -Wmissing-declarations). Ein Helfer mit externer
// Bindung und ohne vorherige Deklaration warnt dort -- der Hand-Bau (-Wall) sah das nicht. static ist
// hier die sachlich richtige Bindung: beide Helfer leben nur in dieser einen Uebersetzungseinheit.
template <typename A, typename B>
static void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

static void check_true(char const* what, bool cond) {
    std::cout << (cond ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!cond) ++g_fail;
}

// ---------------------------------------------------------------------------------------------------
// compile-time-Teil: was der Header selbst garantieren MUSS, steht hier als zweite, unabhaengige Wache.
// ---------------------------------------------------------------------------------------------------
static_assert(cem::kCompilerCompilerErrorClassCount == 5, "RF-3 (§70.3): die D1-Taxonomie hat 5 Klassen.");
static_assert(static_cast<std::uint8_t>(Klasse::BetriebssystemFeatureFehlt) == 4,
              "Die neue Klasse wird ANGEHAENGT (=4). Ein Einschieben wuerde Bestands-Nummern verschieben.");
// Die vier Bestands-Nummern sind eingefroren -- Etiketten und Nummern reisen in Experiment-Logs.
static_assert(static_cast<std::uint8_t>(Klasse::KonfigXmlParse) == 0);
static_assert(static_cast<std::uint8_t>(Klasse::ToolchainFehlt) == 1);
static_assert(static_cast<std::uint8_t>(Klasse::HardwareErweiterungFehlt) == 2);
static_assert(static_cast<std::uint8_t>(Klasse::CompileKombination) == 3);
// Drift-Wache, gespiegelt: Count == letzter Wert + 1.
static_assert(cem::kCompilerCompilerErrorClassCount ==
              static_cast<std::size_t>(Klasse::BetriebssystemFeatureFehlt) + 1);
// Das Etikett ist compile-time zementiert (es darf in Logs zitiert werden).
static_assert(cem::error_class_label(Klasse::BetriebssystemFeatureFehlt) ==
              std::string_view{"betriebssystem_feature_fehlt"});
// D2 ist disjunkt und von RF-3 unberuehrt.
static_assert(cem::kSampleStatusCount == 4, "RF-3 fasst die D2-Taxonomie NICHT an.");

// ---------------------------------------------------------------------------------------------------
// ANHAENGE-WACHE (RF-3-Befund, 26.07., empirisch belegt). Die Drift-Wache im Header pinnt den Count an
// einen BENANNTEN Enumerator (`Count == BetriebssystemFeatureFehlt + 1`). Damit faengt sie ein
// Umnummerieren -- aber NICHT das ANHAENGEN einer weiteren Klasse: haengt jemand eine sechste Klasse
// sauber an und ergaenzt ihr Etikett, aber vergisst den Count-Bump, bleibt jene Wache still (gemessen:
// die praeparierte Schatten-Kopie uebersetzte fehlerfrei). Das galt schon vor RF-3, als sie auf
// CompileKombination zeigte; RF-3 hat die Schwaeche nur sichtbar gemacht, nicht verursacht.
//
// Diese Wache schliesst die Luecke von der anderen Seite: der erste Wert JENSEITS des Counts darf kein
// Etikett tragen. Traegt er eines, gibt es eine Klasse, die der Count nicht kennt -- genau die Drift.
static_assert(cem::error_class_label(static_cast<Klasse>(cem::kCompilerCompilerErrorClassCount)) ==
                  std::string_view{"unbekannt"},
              "Drift: hinter dem Count liegt eine etikettierte Klasse. Wer eine Fehlerklasse anhaengt, "
              "MUSS kCompilerCompilerErrorClassCount mit hochzaehlen.");

int main() {
    std::cout << "\n== Teil 1: die Nummer haengt hinten an, die Bestands-Nummern stehen still ==\n";
    check_eq("BetriebssystemFeatureFehlt", static_cast<int>(Klasse::BetriebssystemFeatureFehlt), 4);
    check_eq("Klassen-Zahl (Single-Source)", cem::kCompilerCompilerErrorClassCount, std::size_t{5});
    check_true("die vier Bestands-Nummern 0..3 sind unveraendert",
               static_cast<int>(Klasse::KonfigXmlParse) == 0 && static_cast<int>(Klasse::ToolchainFehlt) == 1 &&
                   static_cast<int>(Klasse::HardwareErweiterungFehlt) == 2 &&
                   static_cast<int>(Klasse::CompileKombination) == 3);

    std::cout << "\n== Teil 2: Etikett-Roundtrip, alle fuenf paarweise verschieden ==\n";
    check_eq("Etikett der neuen Klasse", std::string{cem::error_class_label(Klasse::BetriebssystemFeatureFehlt)},
             std::string{"betriebssystem_feature_fehlt"});
    check_eq("das OS-Analogon steht neben seinem HW-Vorbild",
             std::string{cem::error_class_label(Klasse::HardwareErweiterungFehlt)},
             std::string{"hardware_erweiterung_fehlt"});

    std::string_view etikett[cem::kCompilerCompilerErrorClassCount];
    for (std::size_t i = 0; i < cem::kCompilerCompilerErrorClassCount; ++i)
        etikett[i] = cem::error_class_label(static_cast<Klasse>(i));

    bool alle_belegt = true;
    for (auto const& e : etikett) alle_belegt = alle_belegt && !e.empty() && e != std::string_view{"unbekannt"};
    check_true("alle fuenf Etiketten sind belegt und keines faellt in den unbekannt-Default", alle_belegt);

    // Die PAARWEISE Distinktheit -- genau die Luecke des registrierten Taxonomie-Tests.
    bool        paarweise_verschieden = true;
    std::string kollision;
    for (std::size_t i = 0; i < cem::kCompilerCompilerErrorClassCount; ++i)
        for (std::size_t j = i + 1; j < cem::kCompilerCompilerErrorClassCount; ++j)
            if (etikett[i] == etikett[j]) {
                paarweise_verschieden = false;
                kollision = std::string{etikett[i]} + " (" + std::to_string(i) + "==" + std::to_string(j) + ")";
            }
    check_true("alle fuenf Etiketten sind PAARWEISE verschieden", paarweise_verschieden);
    if (!kollision.empty()) std::cout << "        Kollision: " << kollision << "\n";

    std::cout << "\n== Teil 3: die Drift-Wachen sind scharf ==\n";
    check_true("Count == letzter Enum-Wert + 1", cem::kCompilerCompilerErrorClassCount ==
                                                     static_cast<std::size_t>(Klasse::BetriebssystemFeatureFehlt) + 1);
    // Ein Wert AUSSERHALB der Taxonomie muss sichtbar im Default landen -- kein stiller Skip, kein UB.
    check_eq("out-of-range-Cast faellt sichtbar in den Default",
             std::string{cem::error_class_label(static_cast<Klasse>(99))}, std::string{"unbekannt"});

    std::cout << "\n== Teil 4: D2 bleibt disjunkt und unberuehrt ==\n";
    check_eq("SampleStatus-Zahl", cem::kSampleStatusCount, std::size_t{4});
    check_eq("das failed-Token ist unveraendert", std::string{cem::sample_status_token(cem::SampleStatus::Failed)},
             std::string{"failed"});

    std::cout << "\n==== RF-3 BetriebssystemFeatureFehlt: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
