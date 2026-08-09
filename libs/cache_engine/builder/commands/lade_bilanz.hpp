#pragma once
// lade_bilanz.hpp -- D4e (2026-08-09): DIE BILANZ DES MESS-LAUFS, MIT NENNER UND SUMMENPROBE.
//
// @subsystem CEB (Mess-Auswertung, neben result_aggregator.hpp / multi_compare.hpp)
// @phase_owner CEB
//
// ------------------------------------------------------------------------------------------------
// DER BEFUND, DEN DIESE DATEI HEILT
// ------------------------------------------------------------------------------------------------
// Die f15-CLI hatte DREI benannte Ausschluss-Gruende in ihrer Lade-Schleife -- "nicht mess-faehig",
// "Konformitaet fehlgeschlagen", "< 2 Samples" -- und jeder schrieb eine Zeile nach stderr. Gezaehlt
// wurde keiner. Die Ausgabe nannte am Ende nur "N DLLs gemessen"; wie viele vorgelegt worden waren
// und wo die Differenz geblieben ist, stand nirgends. Eine Zahl ohne Grundgesamtheit ist von einem
// Freispruch nicht zu unterscheiden: "12 gemessen" liest sich gleich, ob 12 oder 40 geladen wurden.
//
// Dazu fehlte eine VIERTE Gruppe ganz: ein Ergebnis, dessen Proben ausschliesslich aus Nullen
// bestehen (ExecutionResult::degeneriert, D4d). So eine Probe hat einen p50 von 0 und GEWINNT damit
// jedes Ranking, das nach p50 sortiert.
//
// ------------------------------------------------------------------------------------------------
// WARUM DAS HIER STEHT UND NICHT IN DER main()
// ------------------------------------------------------------------------------------------------
// Owner-KERN "Google Tests statt Shell-Proben": der einzige Test der f15-CLI ist heute ein
// CTest-PASS_REGULAR_EXPRESSION auf ihre Standardausgabe, der gegen echte R5.G-DLLs faehrt -- also
// gegen Daten, die per Konstruktion NICHT degeneriert sind. Ein Regex auf einer Ausgabe, in der der
// Fall gar nicht vorkommen kann, ist kein Testat. In einer eigenen Datei kann die Bilanz mit
// synthetischen Zahlen befragt werden, ohne eine einzige DLL zu laden. Die CLI-Smoke bleibt
// zusaetzlich als Integrationsprobe bestehen -- sie prueft etwas anderes (dass die Binary laeuft).
//
// DOKTRIN: header-only C++23, ASCII-only, nur stdlib. Kein Loader-, kein ABI-, kein Dock-Wissen.

#include <cstddef>
#include <string>

namespace comdare::cache_engine::builder::commands {

/// Die Bilanz EINES f15-Mess-Laufs ueber ein DLL-Verzeichnis.
///
/// `geladen` ist der NENNER -- die Zahl der Binaries, die der Loader geoeffnet hat. Alles andere
/// sind Teilmengen davon, und sie muessen sich zu ihm aufaddieren. Genau das prueft summe_stimmt():
/// eine Bilanz, die nicht aufgeht, bedeutet, dass ein Pfad durch die Lade-Schleife existiert, der
/// keine Kategorie erhoeht -- also eine still verschwundene Binary.
struct LadeBilanz {
    std::size_t geladen{0};            ///< NENNER: vom Loader geoeffnete Module
    std::size_t nicht_messfaehig{0};   ///< A: kein IMeasurableWorkload
    std::size_t konformitaet_fehlt{0}; ///< B: std::map-Huellen-Gate nicht bestanden
    std::size_t zu_wenige_proben{0};   ///< C: run_workload lieferte < 2 Samples
    std::size_t tote_proben{0};        ///< D: Proben da, aber keine einzige > 0 (D4d)
    std::size_t gemessen{0};           ///< in die Auswertung uebernommen

    /// Summe der fuenf Kategorien.
    [[nodiscard]] constexpr std::size_t verbucht() const noexcept {
        return nicht_messfaehig + konformitaet_fehlt + zu_wenige_proben + tote_proben + gemessen;
    }

    /// A + B + C + D + gemessen == geladen. FAIL-CLOSED gedacht: der Aufrufer soll bei false
    /// nicht weiterrechnen, sondern melden -- eine unerklaerte Differenz ist ein Befund ueber die
    /// Lade-Schleife, kein Rundungsfehler.
    [[nodiscard]] constexpr bool summe_stimmt() const noexcept { return verbucht() == geladen; }

    /// Es wurde ueberhaupt etwas verwertbares gemessen.
    [[nodiscard]] constexpr bool hat_messung() const noexcept { return gemessen > 0; }

    /// Die EINE Bilanz-Zeile. Sie nennt IMMER den Nenner und IMMER alle vier Ausschluss-Gruende,
    /// auch wenn sie 0 sind -- eine Kategorie, die nur bei Treffern erscheint, laesst den Leser
    /// raten, ob sie geprueft wurde. Bei nicht aufgehender Summe steht die Differenz explizit da.
    [[nodiscard]] std::string zeile() const {
        std::string s = "BILANZ: " + std::to_string(geladen) + " geladen = " + std::to_string(gemessen) +
                        " gemessen + " + std::to_string(nicht_messfaehig) + " nicht mess-faehig + " +
                        std::to_string(konformitaet_fehlt) + " Konformitaet + " + std::to_string(zu_wenige_proben) +
                        " zu wenige Proben + " + std::to_string(tote_proben) + " tote Proben";
        if (!summe_stimmt()) {
            s += "  [BEFUND: Summe geht NICHT auf, verbucht=" + std::to_string(verbucht()) +
                 " gegen geladen=" + std::to_string(geladen) +
                 " -- es gibt einen unverbuchten Pfad durch die Lade-Schleife]";
        }
        return s;
    }
};

/// Exit-Code-Taxonomie der f15-CLI, soweit D4e sie beruehrt.
///
/// WARUM 6 UND NICHT 5: 5 ist bereits vergeben -- der Lastprofil-Pfad gibt ihn zurueck, wenn keine
/// einzige DLL gemessen werden konnte. Zwei Bedeutungen unter einem Code waeren fuer ein CI, das
/// auf den Code schaut, nicht auseinanderzuhalten.
inline constexpr int kExitToteProbenGefunden = 6; ///< mindestens eine DLL lieferte nur Nullen
inline constexpr int kExitBilanzGehtNichtAuf = 7; ///< A+B+C+D+gemessen != geladen

/// Der Exit-Code zu einer Bilanz. 0 heisst: alles verbucht und nichts Totes dabei.
///
/// REIHENFOLGE ist tragend: eine nicht aufgehende Bilanz wiegt schwerer als tote Proben, weil sie
/// bedeutet, dass die Zaehlung selbst nicht stimmt -- dann ist auch die Aussage "0 tote Proben"
/// nichts wert.
[[nodiscard]] constexpr int lade_bilanz_exit_code(LadeBilanz const& b) noexcept {
    if (!b.summe_stimmt()) return kExitBilanzGehtNichtAuf;
    if (b.tote_proben > 0) return kExitToteProbenGefunden;
    return 0;
}

} // namespace comdare::cache_engine::builder::commands
