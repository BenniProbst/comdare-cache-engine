// test_hy_a1_reroute_gate_negativ -- HY-A1: die NEGATIV-COMPILE-FIXTURE des Concept-Gates.
//
// DIESE UEBERSETZUNGSEINHEIT MUSS BRECHEN. Das ist kein Versehen, sondern ihr einziger Zweck.
//
// Owner-Auflage (09.08.2026, Zusatz-Anforderung 2): "diese Gattung ist nur unter bestimmten C++20
// Concepts gueltig und fuer bestimmte andere Gattungen+Genus kompilierbar, und die Einschraenkung
// muss HART PER TEST bewiesen werden."
//
// WARUM ES DIESE FIXTURE ZUSAETZLICH ZU DEN static_asserts BRAUCHT -- die Begruendung steht schon
// im Bestand, an test_e24_c1_organ_concept_negativ.cpp:1-7, und gilt hier unveraendert:
// ein erfuellbarer Negativ-Assert (`static_assert(!Concept<X>)`) beweist die Concept-AUSWERTUNG,
// nicht die WIRKUNG an der Verwendungsstelle. Und selbst ein `!requires{...}` beweist nur, dass
// der Compiler die Bildung ALS AUSDRUCK verweigert -- nicht, dass eine echte Uebersetzung, die
// den Typ WIRKLICH benutzt, daran scheitert. Genau das wird hier gezeigt.
//
// SIE WIRD DESHALB NIE VOM SAMMEL-BAU GEBAUT: das CMake-Ziel ist EXCLUDE_FROM_ALL und steht
// bewusst NICHT in COMDARE_TEST_TARGETS. Gebaut wird sie ausschliesslich vom ctest-Test gleichen
// Namens, der den Bau aufruft und die erwartete Diagnose per PASS_REGULAR_EXPRESSION auf den
// Marker "HY-A1-NEGATIV-PROBE" prueft. Baut sie versehentlich DURCH, fehlt der Marker -> Test ROT.
//
// ERWARTETE DIAGNOSEN (drei, drei VERSCHIEDENE Sperren -- nicht dreimal dieselbe):
//   HY-A1-NEGATIV-PROBE-1  Ziel-Bindung auf den Reroute-Genus (S1: "allein nicht ansprechbar").
//   HY-A1-NEGATIV-PROBE-2  Gattung und Genus passen nicht zueinander (S2: Paar-Konsistenz).
//   (3) die CRTP-Wache RerouteGuard beisst bei der KONSTRUKTION eines Traegers, der den
//       Strategie-Vertrag nicht erfuellt -- mit ihrer EIGENEN Meldung aus
//       heuristik_adapter_strategy.hpp. Das ist der Beleg, dass die Wache kein toter Buchstabe
//       ist, sondern an der Konstruktion wirklich haengt.

#include "hybrid/heuristik_adapter_gate.hpp"
#include "hybrid/heuristik_adapter_strategy.hpp"

#include <cstddef>
#include <string_view>

// TU-lokale Fixtures: anonymer Namespace gegen ODR-Kollisionen der gleichnamigen Probe-Typen
// ueber die Negativ-TUs.
namespace {

namespace cea = ::comdare::cache_engine::anatomy;
namespace hy  = ::comdare::cache_engine::hybrid;

// --------------------------------------------------------------------------------------------
// EINE FALLE, DIE HIER AM OBJEKT AUFGELAUFEN IST -- UND WARUM DIE FIXTURE ZWEI FORMEN JE SPERRE HAT
// --------------------------------------------------------------------------------------------
// Erster Entwurf dieser Datei benutzte NUR die direkte Alias-Form:
//     using VerbotenesSelbstZiel = hy::HybridZielBindung<...>;   // bricht
//     static_assert(sizeof(VerbotenesSelbstZiel) > 0, "HY-A1-NEGATIV-PROBE-1: ...");
// Sie BRICHT korrekt -- aber der Marker erscheint NICHT in der Diagnose. Der Alias scheitert
// zuerst ("template constraint failure"), danach ist der Name unbekannt und der static_assert
// meldet nur noch `'VerbotenesSelbstZiel' was not declared in this scope`. Seine Meldung wird nie
// gedruckt. Ein PASS_REGULAR_EXPRESSION auf den Marker waere also ROT geblieben, obwohl die
// Fixture genau das Richtige tut -- ein gruener Haken ohne Deckung, nur mit umgekehrtem Vorzeichen.
// Literal nachgestellt, bevor diese Datei ihre Endform bekam.
//
// DESHALB JE SPERRE ZWEI FORMEN, die Verschiedenes leisten:
//   (a) DETEKTOR-FORM `static_assert(ZielBindungInstanziierbar<...>, "MARKER ...")` -- sie ist
//       FALSCH und bricht deshalb mit MEINER Meldung. Das ist die Form, die den Marker zuverlaessig
//       in die Diagnose bringt, und damit die, an der der ctest haengt.
//   (b) DIREKTE FORM -- der Typ wird wirklich gebildet. Nur sie beweist, dass eine ECHTE
//       Uebersetzung an der Verwendungsstelle scheitert und nicht bloss ein requires-Ausdruck
//       false wird. Ihr Alias-Name traegt den Marker mit, damit auch ihr Bruch zuordenbar bleibt.
// Keine der beiden Formen ersetzt die andere.

// (1) S1 -- der Hauptfall: ein Hybrid, der auf sich selbst zeigt. Das Paar ist in sich KONSISTENT
//     (gattung_of(FunctionInterfaceReroute) == HeuristikAdapter), und trotzdem ist es verboten.
//     Deshalb ist dieser Fall der Beweis, dass das Gate mehr prueft als blosse Konsistenz.
static_assert(
    hy::ZielBindungInstanziierbar<cea::AnatomyGattung::HeuristikAdapter, cea::AnatomyGenus::FunctionInterfaceReroute>,
    "HY-A1-NEGATIV-PROBE-1: ein Reroute auf ein Klassifikations-Genus ist verboten "
    "-- erwarteter Bruch");

using HY_A1_NEGATIV_PROBE_1_selbstziel =
    hy::HybridZielBindung<cea::AnatomyGattung::HeuristikAdapter, cea::AnatomyGenus::FunctionInterfaceReroute>;

// (2) S2 -- Paar-Bruch: SearchAlgorithm ist zielfaehig, gehoert aber zur Gattung Map, nicht
//     Container. Ohne diese zweite Sperre kaeme ein Gehaeuse mit falscher Gattungs-Etikette
//     durch und landete am falschen Pruef-Dock.
static_assert(hy::ZielBindungInstanziierbar<cea::AnatomyGattung::Container, cea::AnatomyGenus::SearchAlgorithm>,
              "HY-A1-NEGATIV-PROBE-2: Gattung und Genus gehoeren nicht zusammen -- erwarteter Bruch");

using HY_A1_NEGATIV_PROBE_2_fremdpaar =
    hy::HybridZielBindung<cea::AnatomyGattung::Container, cea::AnatomyGenus::SearchAlgorithm>;

// (3) Ein Traeger, der die CRTP-Wache erbt, ohne den Strategie-Vertrag zu erfuellen: strategie_name()
//     fehlt absichtlich. Das ist der reale Fehlerfall -- jemand schreibt eine neue Reroute-Strategie
//     und vergisst die Haelfte des Vertrags.
struct StrategieOhneNamen : hy::RerouteGuard<StrategieOhneNamen, cea::AnatomyGenus::Set> {
    StrategieOhneNamen() noexcept = default;
    [[nodiscard]] static constexpr cea::AnatomyGenus ziel_genus() noexcept { return cea::AnatomyGenus::Set; }
    // strategie_name() FEHLT ABSICHTLICH.
};

} // namespace

int main() {
    // Erst die KONSTRUKTION instanziiert den Ctor-Rumpf der CRTP-Wache und damit ihre static_asserts.
    StrategieOhneNamen traeger{};
    (void)traeger;
    return 0;
}
