#pragma once
// HY-A1 -- DAS CONCEPT-GATE der Gattung HEURISTIK-ADAPTER.
//
// ============================================================================================
// DER AUFTRAG, WOERTLICH (Owner, 09.08.2026, Zusatz-Anforderung 2)
// ============================================================================================
//   "diese Gattung ist nur unter bestimmten C++20 Concepts gueltig und fuer bestimmte andere
//    Gattungen+Genus kompilierbar, und die Einschraenkung muss HART PER TEST bewiesen werden."
//
// Und die Schranke selbst, aus der Bauart-Beschreibung vom 08.08. (LEDGER:470-477):
//   "eigene Klassifikation im Metaprogrammier-Stil, nur als Erweiterung einsetzbar,
//    ALLEIN NICHT ANSPRECHBAR."
// Das ist der VERBOTENE Fall in einem Satz: ein Hybrid ohne Ziel gibt es nicht. Er ist immer
// Erweiterung von etwas; er ist nie das Etwas.
//
// Und die Gegenrichtung, aus derselben Quelle:
//   "fuer JEDE Gattung+Genus per google test bewiesen durchlaessig."
// Ein Gate, das nur ablehnt, waere also genauso falsch wie eines, das alles durchlaesst. Beide
// Richtungen gehoeren ins Paket -- deshalb stehen unten BEIDE Sorten static_assert.
//
// ============================================================================================
// DIE DREI SCHRANKEN, UND WARUM ES DREI SIND
// ============================================================================================
// (S1) ZIELFAEHIGKEIT -- das Ziel muss ein ABI-sichtbares Genus sein.
//      Ein Reroute auf ein Klassifikations-Genus ist sinnlos: dahinter steht keine plain
//      Tier-Binary, sondern wieder nur eine Klassifikation. Das ist das "allein nicht
//      ansprechbar" in Code-Form.
// (S2) PAAR-KONSISTENZ -- gattung_of(G) muss die angegebene Gattung sein.
//      Ein Paar wie (Container, SearchAlgorithm) ist eine Luege ueber den Bestand: SearchAlgorithm
//      gehoert zur Gattung Map. Ohne S2 koennte ein Hybrid-Gehaeuse mit falscher Gattungs-Etikette
//      gebaut werden und am falschen Dock landen.
// (S3) EIN-GATTUNG-HYBRID -- alle Ziele EINES Gehaeuses teilen dieselbe Gattung.
//      Owner-GO Q6 vom 02.08.2026: "EIN Hybrid-Binary je Gattung ... Gemischte Gattungen in einer
//      Huelle sind damit ausgeschlossen; je Gattung eine eigene Huelle." S3 ist die
//      Mehr-Ziel-Form von S2 und wird ueber dieselbe Ableitung erzwungen.
//
// ALLE DREI SIND ABGELEITET, nicht aufgezaehlt: sie fussen auf gattung_of() und auf der Partition
// aus heuristik_adapter_klassifikation.hpp. Eine Aufzaehlung waere eine zweite Wahrheit -- und
// genau daran sind die bestehenden "Vollstaendigkeits-Wachen" gescheitert (dort nachgemessen).
//
// ============================================================================================
// WIE DAS COMPILE-SCHEITERN BELEGT IST -- ZWEI WEGE, BEGRUENDET BEIDE
// ============================================================================================
// (a) static_assert(!requires { typename HybridZielBindung<Ga, G>; }) -- HIER in dieser Datei.
//     Es prueft IM GRUENEN BAU, dass die verbotene Instanziierung nicht wohlgeformt ist. Vorteil:
//     laeuft in der normalen Suite dauerhaft mit, kostet nichts, kann nicht vergessen werden.
//     Wichtig ist die FORM: geprueft wird nicht "das Concept ist false" (das beweist nur die
//     Auswertung), sondern dass die VERWENDUNGSSTELLE bricht -- die Bildung des Typs selbst.
//     Genau diese Unterscheidung macht der Bestand an test_e24_c1_organ_concept_negativ.cpp:1-7
//     bereits auf; hier ist sie eingehalten.
// (b) Ein echter Negativ-Compile-Test, tests/unit/test_hy_a1_reroute_gate_negativ.cpp, nach dem
//     Haus-Muster (EXCLUDE_FROM_ALL + ctest ruft den Bau + PASS_REGULAR_EXPRESSION auf einen
//     Marker). Er beweist, dass eine ECHTE Uebersetzung scheitert -- was (a) nicht kann, weil (a)
//     naturgemaess nur im gelingenden Bau lebt.
// Die beiden Wege beweisen Verschiedenes und ersetzen einander nicht.

#include "anatomy/anatomy_base.hpp"
#include "heuristik_adapter_klassifikation.hpp"

namespace comdare::cache_engine::hybrid {

// ------------------------------------------------------------------------------------------
// (S1) HybridZielGenus -- darf G Ziel eines Reroutes sein?
// ------------------------------------------------------------------------------------------

/// HybridZielGenus<G> -- true gdw. G ein ABI-sichtbares Genus ist, hinter dem eine echte plain
/// Tier-Binary stehen kann. FALSE fuer jeden Reroute-/Klassifikations-Genus.
template <cea::AnatomyGenus G>
concept HybridZielGenus = ist_abi_sichtbares_genus(G);

// ------------------------------------------------------------------------------------------
// (S2/S3) HybridZielPaar -- passt das Paar Gattung+Genus, und ist es zielfaehig?
// ------------------------------------------------------------------------------------------

/// HybridZielPaar<Ga, G> -- true gdw. G zielfaehig ist UND tatsaechlich zur Gattung Ga gehoert.
///
/// Die zweite Haelfte ist nicht kosmetisch. Ohne sie waere (Container, SearchAlgorithm) erlaubt --
/// ein Gehaeuse mit Container-Etikette, das ein Map-Genus bedient. Es wuerde am Container-Dock
/// angemeldet und dort ein Modul antreffen, dessen Antriebs-Sub-Interface es nicht gibt.
template <cea::AnatomyGattung Ga, cea::AnatomyGenus G>
concept HybridZielPaar = HybridZielGenus<G> && (cea::gattung_of(G) == Ga);

// ------------------------------------------------------------------------------------------
// DIE VERWENDUNGSSTELLE: der Typ, den ein Hybrid-Gehaeuse je bedientem Ziel bildet
// ------------------------------------------------------------------------------------------
// WARUM UEBERHAUPT EIN TYP UND NICHT NUR EIN CONCEPT: ein Concept allein ist eine Aussage; erst
// eine CONSTRAINED Verwendungsstelle macht die Aussage wirksam. HybridZielBindung ist diese
// Stelle -- und zugleich der Anker, an dem das Folgepaket HY-A2 den echten Reroute aufhaengt.
// Heute traegt sie nur Identitaet; das ist Absicht (Struktur, nicht Implementierung).

/// HybridZielBindung<Ga, G> -- die compile-time Identitaet EINES bedienten Ziels eines
/// Hybrid-Gehaeuses der Gattung Ga. Existiert NUR fuer erlaubte Paare; jede andere Instanziierung
/// ist nicht wohlgeformt (das ist der harte Teil des Gates).
template <cea::AnatomyGattung Ga, cea::AnatomyGenus G>
    requires HybridZielPaar<Ga, G>
struct HybridZielBindung {
    /// Die Gattung des GEHAEUSES -- identisch mit der des Ziels (Q6: ein Hybrid je Gattung).
    static constexpr cea::AnatomyGattung gattung = Ga;
    /// Das bediente ZIEL-Genus. ACHTUNG: das ist der Wert, den genus() der Hybrid-Binary nach
    /// aussen meldet (Owner E-1 final, Weg C) -- NICHT FunctionInterfaceReroute.
    static constexpr cea::AnatomyGenus ziel_genus = G;
    /// Die KLASSIFIKATION der Binary, die diese Bindung traegt. Sie ist konstant der Reroute-Genus
    /// und liegt damit bewusst neben ziel_genus: die beiden Ebenen stehen hier nebeneinander,
    /// damit niemand sie zusammenzieht.
    static constexpr cea::AnatomyGenus klassifikations_genus = cea::AnatomyGenus::FunctionInterfaceReroute;

    [[nodiscard]] static constexpr std::string_view ziel_name() noexcept { return cea::genus_name(G); }
};

/// ZielBindungInstanziierbar<Ga, G> -- der DETEKTOR fuer die Verwendungsstelle.
/// Er fragt nicht "ist das Concept erfuellt?", sondern "laesst sich der Typ bilden?". Nur diese
/// zweite Frage beweist die Wirkung des Gates. Die Bildung eines template-id mit unerfuellten
/// Constraints ist eine Substitutions-Fehlschlag im unmittelbaren Kontext -- der requires-Ausdruck
/// wird dadurch false statt das Programm ungueltig.
template <cea::AnatomyGattung Ga, cea::AnatomyGenus G>
concept ZielBindungInstanziierbar = requires { typename HybridZielBindung<Ga, G>; };

// ------------------------------------------------------------------------------------------
// BEWEIS-BLOCK 1: DIE ERLAUBTE RICHTUNG IST GRUEN (Gegenprobe gegen ein wertloses Gate)
// ------------------------------------------------------------------------------------------
// Owner: "fuer JEDE Gattung+Genus per google test bewiesen durchlaessig". Hier die
// compile-time-Haelfte davon; die Laufzeit-/gtest-Haelfte steht in
// tests/unit/test_hy_a1_heuristik_adapter_gattung.cpp.
// EIN GATE, DAS ALLES ABLEHNT, WAERE GENAUSO FALSCH WIE EINES, DAS ALLES ZULAESST.

static_assert(HybridZielGenus<cea::AnatomyGenus::SearchAlgorithm>);
static_assert(HybridZielGenus<cea::AnatomyGenus::Set>);
static_assert(HybridZielGenus<cea::AnatomyGenus::Sequence>);
static_assert(HybridZielGenus<cea::AnatomyGenus::Adapter>);
static_assert(HybridZielGenus<cea::AnatomyGenus::View>);

// ... und dieselben fuenf an der VERWENDUNGSSTELLE, je mit ihrer echten Gattung.
static_assert(ZielBindungInstanziierbar<cea::AnatomyGattung::Map, cea::AnatomyGenus::SearchAlgorithm>);
static_assert(ZielBindungInstanziierbar<cea::AnatomyGattung::Container, cea::AnatomyGenus::Set>);
static_assert(ZielBindungInstanziierbar<cea::AnatomyGattung::Container, cea::AnatomyGenus::Sequence>);
static_assert(ZielBindungInstanziierbar<cea::AnatomyGattung::Container, cea::AnatomyGenus::Adapter>);
static_assert(ZielBindungInstanziierbar<cea::AnatomyGattung::Container, cea::AnatomyGenus::View>);

// Und die Identitaet stimmt wirklich durch (nicht nur "es uebersetzt").
static_assert(HybridZielBindung<cea::AnatomyGattung::Map, cea::AnatomyGenus::SearchAlgorithm>::ziel_genus ==
              cea::AnatomyGenus::SearchAlgorithm);
static_assert(HybridZielBindung<cea::AnatomyGattung::Map, cea::AnatomyGenus::SearchAlgorithm>::klassifikations_genus ==
              cea::AnatomyGenus::FunctionInterfaceReroute);
static_assert(HybridZielBindung<cea::AnatomyGattung::Container, cea::AnatomyGenus::View>::ziel_name() ==
              std::string_view{"View"});

// ------------------------------------------------------------------------------------------
// BEWEIS-BLOCK 2: DER VERBOTENE FALL SCHEITERT COMPILE-TIME
// ------------------------------------------------------------------------------------------
// Das ist die Kern-Aussage des Pakets. Drei unterschiedliche Verbots-Gruende, jeder einzeln
// belegt -- und JEDER an der Verwendungsstelle, nicht nur an der Concept-Auswertung.

// (V1) DER HAUPTFALL: Reroute auf den Reroute-Genus selbst. "allein nicht ansprechbar".
static_assert(!HybridZielGenus<cea::AnatomyGenus::FunctionInterfaceReroute>,
              "HY-A1-GATE V1: ein Hybrid darf NICHT auf ein Klassifikations-Genus zeigen.");
static_assert(
    !ZielBindungInstanziierbar<cea::AnatomyGattung::HeuristikAdapter, cea::AnatomyGenus::FunctionInterfaceReroute>,
    "HY-A1-GATE V1 (Verwendungsstelle): HybridZielBindung darf sich fuer das "
    "Reroute-Genus NICHT bilden lassen -- auch nicht mit der passenden Gattung. "
    "Das Paar ist in sich konsistent und trotzdem verboten; genau daran zeigt sich, "
    "dass das Gate mehr prueft als Konsistenz.");

// (V2) PAAR-BRUCH: richtige Genera, falsche Gattung. Hier ist das Ziel zielfaehig -- es scheitert
//      allein an S2. Der Fall trennt S1 von S2 sauber, sodass ein Wegfall EINER Schranke sichtbar
//      wird und nicht von der anderen verdeckt bleibt.
static_assert(!ZielBindungInstanziierbar<cea::AnatomyGattung::Container, cea::AnatomyGenus::SearchAlgorithm>,
              "HY-A1-GATE V2: SearchAlgorithm gehoert zur Gattung Map, nicht Container.");
static_assert(!ZielBindungInstanziierbar<cea::AnatomyGattung::Map, cea::AnatomyGenus::Set>,
              "HY-A1-GATE V2: Set gehoert zur Gattung Container, nicht Map.");

// (V3) GRAPH: die Gattung existiert als Ebene-1-Stub, hat aber KEIN Genus (Q5, nach Abgabe).
//      Es gibt daher kein Paar, das gegen sie zielfaehig waere. Belegt, damit ein spaeteres
//      Graph-Genus bewusst durch dieses Gate muss und nicht stillschweigend hindurchrutscht.
static_assert(!ZielBindungInstanziierbar<cea::AnatomyGattung::Graph, cea::AnatomyGenus::SearchAlgorithm>);
static_assert(!ZielBindungInstanziierbar<cea::AnatomyGattung::Graph, cea::AnatomyGenus::View>);

// ------------------------------------------------------------------------------------------
// BEWEIS-BLOCK 3: DAS GATE IST NICHT TRIVIAL
// ------------------------------------------------------------------------------------------
// Zwei Aussagen, die ein degeneriertes Gate sofort entlarven: es gibt mindestens ein erlaubtes
// UND mindestens ein verbotenes Paar. Ohne sie koennte ein spaeterer Umbau das Gate auf
// "immer true" oder "immer false" stellen, ohne dass ein einziger der Asserts oben bricht --
// denn die oben stehenden Asserts pruefen jeweils nur EINE Richtung.
static_assert(ZielBindungInstanziierbar<cea::AnatomyGattung::Map, cea::AnatomyGenus::SearchAlgorithm> !=
                  ZielBindungInstanziierbar<cea::AnatomyGattung::Map, cea::AnatomyGenus::Set>,
              "HY-A1-GATE: das Gate unterscheidet nicht mehr -- es ist entweder auf immer-true "
              "oder auf immer-false degeneriert.");

} // namespace comdare::cache_engine::hybrid
