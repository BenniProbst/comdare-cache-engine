#pragma once
// mess_warmup_paar.hpp -- C-05/#38b (KON47-04, gebaut 2026-08-20): DAS WIEDERHOLUNGS-PAAR.
//
// DIE REGEL (Owner-KERN KON47-04, PFLICHT und MUSS GETESTET SEIN): je Wiederholung wird ein PAAR
// gefahren -- Lauf 1 misst und wird VERWORFEN (Warmup unter realer Messlast), Lauf 2 misst und wird
// GESPEICHERT. Mit den drei KF-10-Wiederholungen sind das 6 Laeufe fuer 3 persistierte Werte.
// EINZIGE Ausnahme: --debug misst GENAU EINMAL, kalt (das Flag reist als RunMethodology-Zustand,
// WorkModeInfo "misst UND parallel"; Zugang heute nur ueber die Zustands-Injektion, s.
// resolve_mess_kaltlauf_of_mode in measure_parallelism.hpp -- S-8/W2 verdrahtet die --debug-CLI).
//
// VORBILD UND ABGRENZUNG: detail::two_phase_measure (tier_observe_trace_abi.hpp) faehrt dasselbe
// Muster auf OP-Ebene (save -> Warmup verworfen -> rollback -> Messung; KON45-03). DIESES Paar liegt
// eine Ebene DARUEBER: es klammert den GANZEN Zell-Messlauf (PermResult) in der lebenden
// Mess-Schleife -- jede Zahl, die eine Zeile traegt, stammt aus einem WARMEN zweiten Lauf. Das
// Drift-Gate urteilt damit ueber warme Werte (die Klammer sitzt INNERHALB der [T-15-KLAMMER], je
// Drift-Probe ein Paar). Der Arena-Faktor 2 dieser Verdopplung steht als EIGENE Planer-Zeile
// (paar_faktor, planner_mengen_types.hpp) -- nicht stillschweigend im Produkt.
//
// WARUM VERWERFEN und nicht mitteln: der Mess-Kanon verlangt "separat, nie interpoliert" (KF-10).
// Der erste Lauf ist WARMUP -- Caches/Branch-Predictors/Allocator sind danach in Arbeitslage; sein
// Wert ist eine Kalt-Zahl und wuerde die Zeile verfaelschen. Er wird deshalb VOLLSTAENDIG verworfen,
// nicht verrechnet. Der Beweis, DASS verworfen wird, ist die Bilanz (laeufe/verworfen) -- die Tests
// halten ihn mit einem Lauf-1-gespeichert-Koeder fest.
//
// LEICHT (nur stdlib): separat testbar ohne die schwere Iterator-/DLL-Include-Kette -- dasselbe
// Muster wie mess_retry_klammer.hpp/measure_parallelism.hpp.

#include <cstddef>
#include <type_traits>
#include <utility>

namespace comdare::cache_engine::builder::experiment {

/// Die Bilanz EINES Paares: der GESPEICHERTE Lauf + die Lauf-Zaehlung (Beweis-Traeger der Tests --
/// die Zahlen reisen NICHT in die CSV: das Zeilen-Schema bleibt byte-identisch).
template <class Payload>
struct MessPaarBilanz {
    Payload     payload{};     // der Wert, der die Zeile traegt (Lauf 2; im --debug der eine Kaltlauf)
    std::size_t laeufe    = 0; // gefahrene Messlaeufe dieses Paares (2; --debug: 1)
    std::size_t verworfen = 0; // davon verworfen (1; --debug: 0)
};

/// Faehrt EIN Wiederholungs-Paar um `messlauf()` (den vollen Zell-Messlauf).
///
/// kaltlauf_debug == false (PRODUKTIONS-PFLICHT): Lauf 1 messen + VERWERFEN, Lauf 2 messen +
/// zurueckgeben -- fuer ALLE Messpfade (der Aufrufer klammert die eine Stelle, durch die observable-
/// UND workload-Pfad laufen). kaltlauf_debug == true (--debug): GENAU EIN Lauf, kalt.
///
/// Der Verwurf ist ein echter Verwurf: der Rueckgabewert von Lauf 1 wird keiner Variablen ausserhalb
/// dieses Rumpfes zugefuehrt (kein Mittel, keine Auswahl "besserer" Werte -- beides waere eine
/// Statistik, die niemand festgelegt hat).
template <class MessLauf>
[[nodiscard]] auto mess_warmup_paar(bool kaltlauf_debug, MessLauf&& messlauf)
    -> MessPaarBilanz<std::decay_t<std::invoke_result_t<MessLauf&>>> {
    using Payload = std::decay_t<std::invoke_result_t<MessLauf&>>;
    MessPaarBilanz<Payload> out;
    if (kaltlauf_debug) {
        out.payload   = messlauf(); // --debug: genau EIN Lauf, kalt (KON47-04-Ausnahme)
        out.laeufe    = 1;
        out.verworfen = 0;
        return out;
    }
    (void)messlauf();           // Lauf 1: Warmup unter realer Messlast -- VERWORFEN
    out.payload   = messlauf(); // Lauf 2: die Zahl, die die Zeile traegt
    out.laeufe    = 2;
    out.verworfen = 1;
    return out;
}

} // namespace comdare::cache_engine::builder::experiment
