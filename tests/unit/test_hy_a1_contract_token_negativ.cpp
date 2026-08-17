// test_hy_a1_contract_token_negativ -- HY-A1 (Dock-Schicht): die NEGATIV-COMPILE-FIXTURE der
// Vertrags-Registry.
//
// DIESE UEBERSETZUNGSEINHEIT MUSS BRECHEN. Das ist kein Versehen, sondern ihr einziger Zweck.
//
// ============================================================================================
// DER KOEDER, UND WARUM ER GEWUERFELT IST
// ============================================================================================
// Das Token `contract_f0a01c9e` stammt aus dem Bauplan-Wuerfelwurf (K13, /dev/urandom,
// super docs/plaene/20260809-HYBRID-bauplan-und-entscheidungsvorlage.md:137/145). Es steht in
// KEINER Doku, in keiner XML und in keiner Registry. Genau das macht es zum Koeder: ein Token, das
// irgendwo im Haus vorkommt, koennte versehentlich gueltig sein, und der Test bewiese dann nichts.
//
// Die Bauplan-Auflage woertlich: "ein Vertrag-Token contract_f0a01c9e, der NICHT in der
// constexpr-Registry steht, muss compile-time laut brechen (Concept-Fehler), nicht runtime.
// Beobachtbarkeit: der Bruch muss aus der Registry-Pruefung kommen -- Negativprobe mit gueltigem
// Token im selben Testfile, damit nicht eine fremde Wache (Include-Fehler) den Mutanten faengt."
//
// ============================================================================================
// WARUM COMPILE-TIME UND NICHT RUNTIME -- die Fehlerklasse dahinter
// ============================================================================================
// Ein unbekanntes Vertrags-Token, das erst zur Laufzeit auffiele, waere im schlechtesten Fall ein
// stiller Rueckfall auf den Standard-Vertrag. Das Haus hat diese Fehlerklasse bereits einmal
// bezahlt: run_methodology_for_ids liess bis zur Welle B/1 einen Tippfehler ("mesure") STILL auf
// measure zurueckfallen und loeste damit eine Messung aus, die niemand angefordert hatte
// (run_methodology_registry.hpp:170-173). Ein stiller Standard-Dock waere dieselbe Klasse: er
// misst mit einem Vertrag, den der Anwender nicht bestellt hat, und sieht dabei gruen aus.
//
// ============================================================================================
// SIE WIRD NIE VOM SAMMEL-BAU GEBAUT
// ============================================================================================
// Das CMake-Ziel ist EXCLUDE_FROM_ALL und steht bewusst NICHT in COMDARE_TEST_TARGETS. Gebaut wird
// sie ausschliesslich vom ctest-Test gleichen Namens, der den Bau aufruft und die erwartete
// Diagnose per PASS_REGULAR_EXPRESSION auf den Marker "HY-A1-VERTRAG-NEGATIV-PROBE" prueft. Baut
// sie versehentlich DURCH, fehlt der Marker -> Test ROT.
//
// ============================================================================================
// JE SPERRE ZWEI FORMEN -- die Falle steht ausfuehrlich in test_hy_a1_reroute_gate_negativ.cpp:42-62
// ============================================================================================
// (a) DETEKTOR-FORM: ein static_assert ueber einer bool-Funktion. Er ist FALSCH und bricht deshalb
//     mit MEINER Meldung -- das ist die Form, die den Marker zuverlaessig in die Diagnose bringt
//     und an der der ctest haengt.
// (b) DIREKTE FORM: der consteval-Aufruf wird wirklich ausgewertet. Nur sie beweist, dass eine
//     ECHTE Uebersetzung an einem falschen Token scheitert und nicht bloss ein Praedikat false wird.
// Keine der beiden ersetzt die andere.

#include "hybrid/hybrid_dock_contract.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

// TU-lokale Fixtures: anonymer Namespace gegen ODR-Kollisionen der Probe-Namen ueber die Negativ-TUs.
namespace {

namespace cea = ::comdare::cache_engine::anatomy;
namespace hy  = ::comdare::cache_engine::hybrid;

// --------------------------------------------------------------------------------------------
// (0) DIE GEGENPROBE ZUERST -- sie MUSS durchgehen
// --------------------------------------------------------------------------------------------
// Ohne sie waere jeder Bruch dieser Datei mehrdeutig: er koennte auch von einem kaputten Include,
// einem Namens-Tippfehler oder einer entkernten Registry kommen. Diese drei Zeilen belegen, dass
// die Registry hier ERREICHBAR und FUNKTIONSFAEHIG ist -- und dass der Bruch unten deshalb wirklich
// vom Koeder-Token stammt.
static_assert(hy::hybrid_dock_contract_token_bekannt("standard"));
static_assert(hy::hybrid_dock_contract_of_token("standard") == hy::HybridDockContract::Standard);
static_assert(hy::kHybridDockContractCount == 4);

// --------------------------------------------------------------------------------------------
// (1) DETEKTOR-FORM -- bringt den Marker in die Diagnose
// --------------------------------------------------------------------------------------------
static_assert(hy::hybrid_dock_contract_token_bekannt("contract_f0a01c9e"),
              "HY-A1-VERTRAG-NEGATIV-PROBE-1: das gewuerfelte Token steht in keiner Registry-Zeile "
              "-- erwarteter Bruch. Ginge diese Zeile durch, waere entweder der Koeder versehentlich "
              "eingetragen worden oder die Token-Pruefung auf immer-wahr degeneriert.");

// --------------------------------------------------------------------------------------------
// (2) DIREKTE FORM -- eine ECHTE Auswertung scheitert
// --------------------------------------------------------------------------------------------
// hybrid_dock_contract_of_token ist consteval; der Aufruf MUSS in einem konstanten Kontext
// ausgewertet werden. Fuer ein unbekanntes Token erreicht die Auswertung das throw -- und ein throw
// in einem consteval-Aufruf ist kein Laufzeit-Wurf, sondern die Feststellung, dass der Ausdruck
// kein konstanter Ausdruck ist: harter Uebersetzungsfehler.
//
// NACHGEMESSEN, und deshalb hier festgehalten: diese Probe druckt IHREN Marker NICHT. Die Diagnose
// lautet `expression '<throw-expression>' is not a constant expression` und zeigt auf die
// Wurf-Zeile in hybrid_dock_contract.hpp -- der Variablenname taucht in der Meldung nicht auf.
// Das ist genau die Falle, die schon test_hy_a1_reroute_gate_negativ.cpp:42-62 beschreibt, und
// genau der Grund, warum Probe 1 daneben steht: haenge man den ctest allein an diese Form, waere
// das PASS_REGULAR_EXPRESSION rot, obwohl die Fixture das Richtige tut. Ihr Beweiswert liegt
// woanders -- sie ist die einzige der drei, bei der eine ECHTE Auswertung stattfindet.
constexpr auto HY_A1_VERTRAG_NEGATIV_PROBE_2_unbekanntes_token = hy::hybrid_dock_contract_of_token("contract_f0a01c9e");

// --------------------------------------------------------------------------------------------
// (3) DIE DRITTE SPERRE: ein Deskriptor mit einem Klassifikations-Genus als Ziel
// --------------------------------------------------------------------------------------------
// Andere Sperre, anderer Grund -- damit ein Wegfall EINER Wache nicht von einer anderen verdeckt
// bleibt. Hier ist der VERTRAG gueltig und das ZIEL falsch: hinter einem Reroute-Genus steht keine
// plain Tier-Binary (Gate-Schranke S1, "allein nicht ansprechbar").
static_assert(hy::pruefe_dock_deskriptor(hy::DockContractDescriptor{
                  static_cast<std::uint8_t>(hy::HybridDockContract::Standard),
                  cea::AnatomyGenus::FunctionInterfaceReroute}) == hy::hybrid_status_ok,
              "HY-A1-VERTRAG-NEGATIV-PROBE-3: ein Hybrid-Dock auf ein Klassifikations-Genus ist "
              "verboten -- erwarteter Bruch. Der Vertrag ist hier gueltig; es scheitert allein am "
              "Ziel, und genau das trennt diese Sperre von Probe 1.");

} // namespace

int main() { return 0; }
