#pragma once
// hybrid/hybrid_binary_proxy.hpp -- HY-A2 (F8-MINIMAL-DoD): der PROXY einer Hybrid-Tier-Binary.
//
// WAS ER IST: die eine Klasse, die eine Hybrid-.so nach aussen zu einer ganz normalen Tier-Binary
// macht. Sie erfuellt IAnatomyBase, haelt intern das Dock-Array (HY-A1) und leitet den Antrieb an
// das Ziel durch, das an einem Dock steckt. Der Host merkt davon nichts -- und genau das ist die
// Zusage.
//
// ============================================================================================
// WEG C, UND WARUM genus() NICHT DEN REROUTE-GENUS LIEFERT
// ============================================================================================
// Owner-Entscheid E-1 final (09.08.2026), am Enum selbst ausformuliert (anatomy_base.hpp:135-169):
//   KLASSIFIKATIONS-Ebene (Weg A) -- AnatomyGenus::FunctionInterfaceReroute. Sie sortiert die
//     Hybrid-.so im Lagerbaum und benennt die ART des Reroutes. Eigenschaft des ARTEFAKTS.
//   INTERFACE-Ebene (Weg C)       -- genus() liefert das GEERBTE ZIEL-Genus (z.B. SearchAlgorithm).
//     Der Pass-through ist nach aussen transparent.
// Der naheliegende Fehler waere, aus genus() den Reroute-Wert zurueckzugeben: dann suchte der Host
// ein Pruef-Dock fuer einen Genus, der keines hat, und die Registry muesste von 5 auf 6 wachsen.
// SIE BLEIBT 5. Diese Datei ist die Stelle, an der das entschieden wird, deshalb steht die
// Begruendung hier und nicht nur im Ledger.
//
// ============================================================================================
// DIE CT-SPERRE: EIN NICHT DEKLARIERTES ZIEL BRICHT LAUT
// ============================================================================================
// Ein Reroute-Ziel ist NICHT jeder Genus, den das Enum kennt. Zwei Klassen fallen weg:
//   (a) Klassifikations-Genera (heute: FunctionInterfaceReroute selbst). Ein Hybrid, der auf einen
//       Hybrid-Genus umleitet, haette kein ABI-Interface am Ende der Kette -- Gate S1,
//       hybrid_status_kein_zielfaehiges_genus.
//   (b) Genera, die dieser Bau nicht deklariert hat. Der F8-Minimal-Schnitt deklariert GENAU EINEN
//       (SearchAlgorithm); ein zweiter kommt hinzu, indem jemand unten eine Spezialisierung
//       schreibt -- nicht, indem er einen Enum-Wert einsetzt und hofft.
// Beide Faelle brechen COMPILE-TIME mit benanntem Text. Das ist die Doktrin "erst laute
// Compile-Fehler, dann verschieben": ein nicht deklariertes Ziel darf nicht erst zur Laufzeit als
// leeres Dock auffallen, denn dort sieht es aus wie "noch nicht bestueckt" und nicht wie "falsch".

#include "heuristik_adapter_klassifikation.hpp"  // ist_abi_sichtbares_genus / ist_klassifikations_genus
#include "heuristik_adapter_synthese_matrix.hpp" // kHybridNodeObergrenzeDefault (Dock-Programm-Deckel)

#include "anatomy/anatomy_base.hpp" // IAnatomyBase / AnatomyGenus / AnatomyGattung

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::hybrid {

// ------------------------------------------------------------------------------------------
// (1) DIE ZIEL-DEKLARATION -- die CT-Sperre in ihrer positiven Form
// ------------------------------------------------------------------------------------------

/// RerouteZiel<G> -- primaer UNDEFINIERT. Wer ein Ziel-Genus benutzt, das hier keine
/// Spezialisierung hat, bekommt einen Fehler an der Instanziierungsstelle. Die Sperre ist damit
/// eine Eigenschaft des TYPSYSTEMS, nicht eines Laufzeit-if: sie kann nicht uebersprungen,
/// vergessen oder mit einem Flag abgeschaltet werden.
template <anatomy::AnatomyGenus G>
struct RerouteZiel;

/// Das EINE deklarierte Ziel des F8-Minimal-Schnitts. Ein zweites kommt als weitere
/// Spezialisierung dazu -- die Zeile ist dann der sichtbare Beleg, dass jemand die Zulaessigkeit
/// GEPRUEFT hat, statt sie zu unterstellen.
template <>
struct RerouteZiel<anatomy::AnatomyGenus::SearchAlgorithm> {
    static constexpr anatomy::AnatomyGenus genus     = anatomy::AnatomyGenus::SearchAlgorithm;
    static constexpr std::string_view      ziel_name = "SearchAlgorithm";
};

/// reroute_ziel_deklariert<G>() -- die LESBARE Fassung derselben Frage. Sie existiert, damit die
/// Fehlermeldung des Proxys sagen kann, WAS fehlt; ohne sie meldete nur "incomplete type".
template <anatomy::AnatomyGenus G>
[[nodiscard]] consteval bool reroute_ziel_deklariert() noexcept {
    return requires { RerouteZiel<G>::genus; };
}

static_assert(reroute_ziel_deklariert<anatomy::AnatomyGenus::SearchAlgorithm>(),
              "HY-A2: SearchAlgorithm ist das deklarierte Ziel des F8-Minimal-Schnitts.");
static_assert(!reroute_ziel_deklariert<anatomy::AnatomyGenus::FunctionInterfaceReroute>(),
              "HY-A2/Gate S1: der Reroute-Genus selbst ist NIE ein Ziel -- ein Hybrid auf einen "
              "Hybrid haette am Ende der Kette kein ABI-Interface.");
static_assert(!reroute_ziel_deklariert<anatomy::AnatomyGenus::View>(),
              "HY-A2: die CT-Sperre unterscheidet noch -- View ist ABI-sichtbar, aber von DIESEM Bau "
              "nicht als Reroute-Ziel deklariert. Faellt diese Zeile, ist die Sperre auf immer-wahr "
              "degeneriert und liesse jeden Enum-Wert durch.");

// ------------------------------------------------------------------------------------------
// (2) DER CT-SLOT-DECKEL DES REROUTE-GENUS
// ------------------------------------------------------------------------------------------

/// kRerouteGenusCtSlotCount -- die COMPILE-ZEIT-Slot-Zahl der Gattung HeuristikAdapter/Genus
/// FunctionInterfaceReroute, wie sie das Bau-Gate (builder/experiment_tree/genus_build_admission.hpp)
/// konsumiert.
///
/// WARUM SIE DER DOCK-DECKEL IST UND KEINE ACHSEN-ZAHL: die fuenf ABI-sichtbaren Genera zaehlen hier
/// ihre KOMPOSITIONS-Slots (SearchAlgorithm 18 Organ-Achsen, Set 13, ...). Ein Reroute-Genus hat
/// keine eigene Komposition -- er hat DOCKS. Seine Bau-Aritaet ist deshalb die Zahl der Docks, die
/// eine Hybrid-Binary tragen darf, und das ist der Programm-Deckel kHybridNodeObergrenzeDefault
/// (32, KON28-03, Owner 12.08. "maximal 32").
///
/// RT <= CT, und das ist die Richtung, auf die es ankommt: diese Zahl ist die OBERGRENZE. Eine
/// konkrete Hybrid-Binary bestueckt weniger (der F8-Minimal-Schnitt genau EINS); sie darf nie mehr.
/// Die Laufzeit-Seite dieser Invariante wacht DockArray::attach mit hybrid_status_array_voll -- sie
/// steht dort und nicht hier, weil nur dort beide Mengen zugleich sichtbar sind.
inline constexpr std::size_t kRerouteGenusCtSlotCount = kHybridNodeObergrenzeDefault;

static_assert(kRerouteGenusCtSlotCount == 32,
              "HY-A3: die CT-Slot-Zahl des Reroute-Genus IST der Dock-Programm-Deckel. Wer ihn "
              "bewegt, bewegt die Bau-Aritaet der Gattung mit -- und muss den Cross-Pin in "
              "genus_build_admission.hpp im SELBEN Commit nachziehen.");

// ------------------------------------------------------------------------------------------
// (3) DER PROXY -- NOCH NICHT GEBAUT, UND WARUM DIESE DATEI IHN TROTZDEM VORBEREITET
// ------------------------------------------------------------------------------------------
//
// GEMESSENER BEFUND (18.08.2026, Probe-TU gegen genau diese Header, nicht geschaetzt): eine Klasse,
// die IAnatomyBase direkt erbt, ist ABSTRAKT -- IAnatomyBase zieht ueber
// anatomy/../execution_engine/execution_engine_base.hpp die volle IExecutionEngine-Flaeche mit
// (engine_name, lifecycle_state, warm_up, run, reset, ...). Der Compiler nennt sie einzeln:
//   "cannot declare variable to be of abstract type HybridBinaryProxy<...>"
//   "because the following virtual functions are pure: engine_name, lifecycle_state, warm_up, run,
//    reset, ..."
//
// WAS DARAUS FOLGT -- und das ist eine DESIGN-Aussage, keine Fleissaufgabe: der Proxy darf diese
// Flaeche NICHT selbst nachbauen. Fuer plain Tiere loest sie der bestehende
// SearchAlgorithmAbiAdapter (anatomy/abi_adapter.hpp), und ein zweiter, handgeschriebener
// Lebenszyklus im Hybrid waere genau die ZWEITE WAHRHEIT, gegen die K2 gebaut ist: zwei Stellen,
// die "warm_up" beantworten, laufen auseinander, und die Mess-Semantik haengt daran.
// Der richtige Schnitt ist deshalb, den Proxy AUF den Adapter zu setzen (Delegation an das
// gebundene Ziel) statt NEBEN ihn -- das ist Bau-Arbeit am Adapter-Vertrag und gehoert in einen
// Schnitt, der ihn mit ansieht, nicht in einen Header, der ihn nur benutzen wollte.
//
// WAS HIER SCHON STEHT UND TRAEGT: die Ziel-DEKLARATION mit ihrer CT-Sperre (1) und der
// CT-Slot-Deckel (2). Beide sind unabhaengig vom Lebenszyklus, beide sind compile-time bewiesen,
// und (2) ist die Einzelquelle, aus der das Bau-Gate seine sechste Zeile zieht
// (builder/experiment_tree/genus_build_admission.hpp). Sie sind damit KEIN Vorgriff, sondern der
// Teil von HY-A2, der ohne den Adapter-Schnitt vollstaendig ist.
//
// OFFEN (benannter Posten, nicht stillschweigend weggelassen): HybridBinaryProxy selbst, das
// hybrid_tier_module.cpp mit den vier ABI-Pflicht-Symbolen und der F8-Reroute-Roundtrip-Test.

} // namespace comdare::cache_engine::hybrid
