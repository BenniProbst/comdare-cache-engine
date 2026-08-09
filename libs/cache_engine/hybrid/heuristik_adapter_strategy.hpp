#pragma once
// HY-A1 -- STRATEGY-STRUKTUR der Gattung HEURISTIK-ADAPTER: je bedientem Genus eine Strategie.
//
// ============================================================================================
// DER AUFTRAG, WOERTLICH (Owner, 09.08.2026, Zusatz-Anforderung 3)
// ============================================================================================
//   "Die Hybrid-Binaries muessen ein Strategy Pattern fuer JEDES Genus unterstuetzen, das sie
//    bedienen duerfen."
// Und die Begruendung, warum es nicht EINE gemeinsame Reroute-Implementierung sein darf (E-1
// final): "weil sich die Art und Weise der Reroute-Genus fuer den Anwendungszweck unterscheiden
// kann -- etwa wenn ein Graph ein anderes Reroute benoetigt als SearchAlgorithm."
//
// ============================================================================================
// WAS HIER STEHT -- UND WAS BEWUSST NICHT
// ============================================================================================
// HIER: das Primaertemplate, der Spezialisierungspunkt je Genus, der Vertrag als Concept und die
//   CRTP-Wache. Also die STRUKTUR.
// NICHT HIER: die Reroute-Implementierungen. Sie kommen mit den echten Reroutes in HY-A2. Eine
//   heute erfundene Implementierung waere Fabrikation -- es gibt noch kein Dock und kein
//   Tier-Modul, gegen das sie sich beweisen koennte.
//
// ============================================================================================
// DAS HAUSMUSTER, DEM DIESE DATEI FOLGT -- KEIN NEUES ERFUNDEN
// ============================================================================================
// (1) PRIMAERTEMPLATE UNDEFINIERT + SPEZIALISIERUNG JE GENUS.
//     Vorbild im Bestand: builder/experiment_tree/genus_binding_traits.hpp:29-32
//     ("Primaer UNDEFINIERT -- jede UNTERSTUETZTE Gattung liefert eine vollstaendige
//     Spezialisierung"), samt dem dortigen Erkennungs-Concept GenusBound<G> (:178-184).
//     Diese Datei ist dasselbe Muster, eine Ebene weiter: statt "ist das Genus baubar?" fragt sie
//     "hat das Genus eine Reroute-Strategie?".
// (2) CRTP + CONCEPT-WACHE.
//     Vorbild im Bestand: anatomy/organ_concept.hpp:222-238 (OrganGuard<Derived> -- leere Basis,
//     protected Ctor, static_asserts IM Ctor-Rumpf, dadurch Pruefung bei der ERSTEN Konstruktion,
//     zero-cost, kein vtable). RerouteGuard unten ist exakt diese Bauart, nur um den
//     Genus-Parameter erweitert.
// (3) KEIN virtueller Dispatch, KEIN Laufzeit-switch, KEIN std::variant.
//     Die Auswahl der Strategie ist eine Template-Spezialisierung und damit vollstaendig
//     compile-time. (Die eine erlaubte variant-Stelle des Designs liegt woanders -- im DockSlot
//     des Dock-Arrays, soll_design Abschnitt 3.2 -- und wird bei genau EINEM Dock-Typ ohnehin
//     nicht gebraucht.)

#include "anatomy/anatomy_base.hpp"
#include "heuristik_adapter_gate.hpp"
#include "heuristik_adapter_klassifikation.hpp"

#include <concepts>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::hybrid {

// ------------------------------------------------------------------------------------------
// (1) DER VERTRAG, den jede Reroute-Strategie erfuellen muss
// ------------------------------------------------------------------------------------------
// Bewusst KLEIN gehalten. Der Vertrag beschreibt heute nur die IDENTITAET einer Strategie, weil
// die Reroute-Operationen selbst noch nicht existieren. Ihn jetzt breiter zu schneiden, hiesse
// Signaturen zu erfinden, die kein Aufrufer kennt -- und sie waeren beim ersten echten Reroute
// wieder falsch. Er ist additiv erweiterbar; das ist der Punkt.

/// RerouteStrategieVertrag<S> -- die Pflicht-Flaeche einer Reroute-Strategie.
template <class S>
concept RerouteStrategieVertrag = requires {
    { S::ziel_genus() } -> std::convertible_to<anatomy::AnatomyGenus>;
    { S::strategie_name() } -> std::convertible_to<std::string_view>;
};

// ------------------------------------------------------------------------------------------
// (2) DIE CRTP-WACHE -- Hausmuster OrganGuard
// ------------------------------------------------------------------------------------------

/// RerouteGuard<Derived, G> -- CRTP-Wache: wer sie erbt, wird bei der ERSTEN Konstruktion
/// compile-hart geprueft. Leere Basis, protected Ctor, keine virtuelle Funktion -- zero-cost,
/// kein Layout-Ereignis (EBO), exakt wie OrganGuard (organ_concept.hpp:222-238).
///
/// SIE PRUEFT ZWEIERLEI, und beides ist noetig:
///   - dass G ueberhaupt bedient werden DARF (das Gate, S1) -- sonst koennte jemand eine
///     Strategie fuer den Reroute-Genus selbst bauen und das Gate an der Strategie vorbei
///     umgehen;
///   - dass Derived den Vertrag erfuellt -- sonst waere die Strategie nicht ansprechbar.
template <class Derived, anatomy::AnatomyGenus G>
class RerouteGuard {
protected:
    RerouteGuard() noexcept {
        static_assert(HybridZielGenus<G>,
                      "RerouteGuard<Derived, G>: G ist kein zielfaehiges Genus. Eine Reroute-Strategie "
                      "fuer ein Klassifikations-Genus gibt es nicht -- ein Hybrid ist immer Erweiterung "
                      "eines echten Ziels, nie sein eigenes Ziel (Owner: allein NICHT ansprechbar).");
        static_assert(RerouteStrategieVertrag<Derived>,
                      "RerouteGuard<Derived, G>: Derived erfuellt RerouteStrategieVertrag nicht "
                      "(ziel_genus() / strategie_name() sind Pflicht).");
    }
};

// ------------------------------------------------------------------------------------------
// (3) DAS PRIMAERTEMPLATE -- constrained UND undefiniert
// ------------------------------------------------------------------------------------------
// ZWEI SPERREN UEBEREINANDER, mit verschiedener Reichweite -- das ist Absicht, keine Doppelung:
//   CONSTRAINT (requires HybridZielGenus<G>): fuer ein Klassifikations-Genus laesst sich der
//     template-id NICHT EINMAL BILDEN. Das faengt den verbotenen Fall.
//   UNDEFINIERT: fuer ein zielfaehiges Genus OHNE Spezialisierung entsteht ein unvollstaendiger
//     Typ. Das faengt den VERGESSENEN Fall -- ein kuenftiges sechstes ABI-sichtbares Genus, fuer
//     das niemand eine Strategie geschrieben hat.
// Nur die erste allein waere zu wenig (Vergessen bliebe still), nur die zweite allein auch
// (das verbotene Genus bekaeme eine ganz gewoehnliche "fehlt halt noch"-Diagnose statt einer
// Aussage ueber die Regel).

/// HeuristikRerouteStrategy<G> -- die Reroute-Strategie fuer das bediente Genus G.
/// PRIMAER UNDEFINIERT (Muster genus_binding_traits.hpp:29-32).
template <anatomy::AnatomyGenus G>
    requires HybridZielGenus<G>
struct HeuristikRerouteStrategy;

/// RerouteStrategieGebunden<G> -- true gdw. G eine Strategie-Spezialisierung HAT.
/// Muster: GenusBound<G> (genus_binding_traits.hpp:178-184). Der sizeof-Ausdruck ist der
/// Vollstaendigkeits-Test: er ist fuer einen unvollstaendigen Typ nicht wohlgeformt.
template <anatomy::AnatomyGenus G>
concept RerouteStrategieGebunden = requires { sizeof(HeuristikRerouteStrategy<G>); };

// ------------------------------------------------------------------------------------------
// (4) DIE FUENF SPEZIALISIERUNGSPUNKTE -- Struktur, keine Implementierung
// ------------------------------------------------------------------------------------------
// Je bedientem Genus GENAU EINE Spezialisierung. Sie tragen heute Identitaet und die CRTP-Wache
// und sonst nichts. Der eigentliche Reroute (Durchstellen der geerbten Interface-Aufrufe an die
// plain Tier-Binary, Empfangen der Ergebnisse, Overhead-Messung an der 4. Mess-Ebene) haengt sich
// in HY-A2 hier ein -- an fuenf getrennten Stellen, nicht an einer gemeinsamen.
//
// WARUM NICHT EIN GEMEINSAMES TEMPLATE MIT if constexpr: weil der Owner ausdruecklich sagt, dass
// sich die ART des Reroutes je Anwendungszweck unterscheidet. Ein gemeinsamer Rumpf mit Fallunter-
// scheidung waere genau die "gemeinsame Implementierung", die ausgeschlossen wurde -- und er
// wuerde beim ersten wirklich abweichenden Genus (Graph) aufbrechen.

#define COMDARE_HY_REROUTE_STRATEGY(GENUS_ENUMERATOR, STRATEGIE_NAME)                                                  \
    template <>                                                                                                        \
    struct HeuristikRerouteStrategy<anatomy::AnatomyGenus::GENUS_ENUMERATOR>                                           \
        : RerouteGuard<HeuristikRerouteStrategy<anatomy::AnatomyGenus::GENUS_ENUMERATOR>,                              \
                       anatomy::AnatomyGenus::GENUS_ENUMERATOR> {                                                      \
        [[nodiscard]] static constexpr anatomy::AnatomyGenus ziel_genus() noexcept {                                   \
            return anatomy::AnatomyGenus::GENUS_ENUMERATOR;                                                            \
        }                                                                                                              \
        [[nodiscard]] static constexpr std::string_view strategie_name() noexcept { return STRATEGIE_NAME; }           \
    }

// DAS MAKRO IST HIER DIE EHRLICHERE FORM, nicht die bequemere: fuenf handgeschriebene, heute
// zeichengleiche Spezialisierungen wuerden vortaeuschen, sie seien bereits verschieden. Sie sind
// es noch nicht. Sobald eine Strategie in HY-A2 echten Inhalt bekommt, wird sie aus dem Makro
// herausgeschrieben -- das Makro ist ein Geruest, kein Endzustand.
COMDARE_HY_REROUTE_STRATEGY(SearchAlgorithm, "Reroute<SearchAlgorithm>");
COMDARE_HY_REROUTE_STRATEGY(Set, "Reroute<Set>");
COMDARE_HY_REROUTE_STRATEGY(Sequence, "Reroute<Sequence>");
COMDARE_HY_REROUTE_STRATEGY(Adapter, "Reroute<Adapter>");
COMDARE_HY_REROUTE_STRATEGY(View, "Reroute<View>");

#undef COMDARE_HY_REROUTE_STRATEGY

// ------------------------------------------------------------------------------------------
// (5) WACHEN
// ------------------------------------------------------------------------------------------

// ERLAUBTE RICHTUNG: alle fuenf ABI-sichtbaren Genera HABEN eine Strategie.
static_assert(RerouteStrategieGebunden<anatomy::AnatomyGenus::SearchAlgorithm>);
static_assert(RerouteStrategieGebunden<anatomy::AnatomyGenus::Set>);
static_assert(RerouteStrategieGebunden<anatomy::AnatomyGenus::Sequence>);
static_assert(RerouteStrategieGebunden<anatomy::AnatomyGenus::Adapter>);
static_assert(RerouteStrategieGebunden<anatomy::AnatomyGenus::View>);

// VERBOTENE RICHTUNG an der Verwendungsstelle: fuer den Reroute-Genus laesst sich die Strategie
// nicht einmal benennen.
static_assert(!RerouteStrategieGebunden<anatomy::AnatomyGenus::FunctionInterfaceReroute>,
              "HY-A1-STRATEGY: fuer ein Klassifikations-Genus darf es keine Reroute-Strategie geben.");

// Die Vertraege halten wirklich (nicht nur "es uebersetzt").
static_assert(RerouteStrategieVertrag<HeuristikRerouteStrategy<anatomy::AnatomyGenus::SearchAlgorithm>>);
static_assert(HeuristikRerouteStrategy<anatomy::AnatomyGenus::Set>::ziel_genus() == anatomy::AnatomyGenus::Set);
static_assert(HeuristikRerouteStrategy<anatomy::AnatomyGenus::View>::strategie_name() ==
              std::string_view{"Reroute<View>"});

// DIE VOLLSTAENDIGKEITS-KOPPLUNG: so viele Strategien wie ABI-sichtbare Genera.
// Sie zaehlt NICHT von Hand, sondern ueber die Einzelquelle kAlleGenera. Kaeme ein sechstes
// ABI-sichtbares Genus dazu, ohne dass jemand eine Strategie schreibt, bricht es HIER.
[[nodiscard]] constexpr std::size_t gebundene_strategie_anzahl() noexcept {
    std::size_t n = 0;
    // Die Schleife laeuft ueber Werte; RerouteStrategieGebunden ist aber ein Template-Argument.
    // Deshalb die explizite Entfaltung ueber genau die Werte der Einzelquelle -- und ein
    // Gegen-Test unten, dass die Einzelquelle nicht laenger geworden ist.
    if constexpr (RerouteStrategieGebunden<anatomy::AnatomyGenus::SearchAlgorithm>) ++n;
    if constexpr (RerouteStrategieGebunden<anatomy::AnatomyGenus::Set>) ++n;
    if constexpr (RerouteStrategieGebunden<anatomy::AnatomyGenus::Sequence>) ++n;
    if constexpr (RerouteStrategieGebunden<anatomy::AnatomyGenus::Adapter>) ++n;
    if constexpr (RerouteStrategieGebunden<anatomy::AnatomyGenus::View>) ++n;
    return n;
}

static_assert(gebundene_strategie_anzahl() == kPruefDockPflichtigeGenusAnzahl,
              "HY-A1-STRATEGY: es gibt nicht mehr genauso viele Reroute-Strategien wie ABI-sichtbare "
              "Genera. Entweder ist ein Genus dazugekommen und seine Strategie fehlt, oder eine "
              "Strategie wurde entfernt. Owner-Auflage: eine Strategie fuer JEDES bediente Genus.");

static_assert(kAlleGenera.size() == 6,
              "HY-A1-STRATEGY: die Genus-Einzelquelle hat ihre Laenge geaendert. gebundene_strategie_"
              "anzahl() entfaltet die fuenf ABI-sichtbaren Werte NAMENTLICH -- wer die Liste "
              "verlaengert, prueft hier, ob die Entfaltung mitgezogen werden muss.");

} // namespace comdare::cache_engine::hybrid
