#pragma once
// HY-A1 -- DIE 2D-MATRIX-LISTE der Synthese-Funktionen (Struktur + Dimensionen).
//
// ============================================================================================
// DER AUFTRAG, WOERTLICH (Owner-Entscheid E-1 final, 09.08.2026)
// ============================================================================================
//   "bedenke, dass ein Heuristik-Tier-Binary eine 2D-Matrix-Liste mit Synthese-Funktionen fuer
//    jeden inneren Layer und jede Layer-Node erhaelt."
// Also NICHT eine Funktion je Hybrid, sondern eine je ZELLE (Layer x Node).
//
// Und die Rekursions-Mechanik, aus der Bauart-Beschreibung vom 08.08. (LEDGER:479-481):
//   "Die Rekursion: baumfoermig compile-time bis zu begrenzter Tiefe, Mechanik = Herunterzaehlen
//    eines Rest-Tiefen-Integers bis 0 - Tiefe bei 1 belassen, muss aber funktionieren - Tiefe und
//    Heuristik-Funktions-Stufen-IDs im Stempel ablesbar - formal EINE Binary am Stueck
//    (bessere Optimierbarkeit)."
//
// ============================================================================================
// DIE BEIDEN DIMENSIONEN
// ============================================================================================
// LAYER (Zeile) = Rekursionsstufe. Sie kommt aus dem REST-TIEFEN-INTEGER und wird bis 0
//   heruntergezaehlt. Eine Matrix der Tiefe D traegt die Layer D, D-1, ..., 1; bei 0 bricht die
//   Rekursion ab. Der Layer-Index IST damit die Rest-Tiefe an dieser Stufe -- kein zweiter
//   Zaehler, keine zweite Wahrheit. Heute wird die Tiefe bei 1 belassen; die Struktur muss aber
//   fuer D > 1 funktionieren, und genau das ist unten per static_assert belegt (nicht behauptet).
// NODE (Spalte) = Dock dieser Stufe. Obergrenze aus der XML, Default siehe unten.
//
// Zellenzahl = D * Nodes. Jede Zelle ist ein eigener Spezialisierungspunkt fuer genau eine
// Synthese-Funktion.
//
// ============================================================================================
// ACHTUNG -- EINE ZAHL, ZWEI OWNER-QUELLEN. NICHT STILL VEREINHEITLICHT.
// ============================================================================================
// Fuer die Node-Obergrenze liegen zwei Angaben vor, beide Owner-gestuetzt, aus verschiedenen Tagen:
//   32 -- Paket-Auftrag vom 09.08.2026, woertlich "Obergrenze aus der XML, Default 32". Im Ledger
//         ist "32 Pruefdocks Default" zusaetzlich als bereits gepruefte Owner-Aussage indexiert
//         (LEDGER:35).
//    8 -- Owner-GO Q6 vom 02.08.2026, als CT-Kapazitaet der STATISCHEN Dock-Array-Policy
//         ("Default-MaxN = 8", soll_design Abschnitt 10.2 + Abschnitt 4 max_docks="8").
//
// MEINE LESART, ausdruecklich als Lesart markiert und NICHT als Entscheid: die beiden Zahlen
// koennten zwei verschiedene Dinge meinen -- 8 die Speicher-KAPAZITAET des Dock-Arrays einer
// Stufe, 32 die Obergrenze der Synthese-Matrix-Spalten. Sie koennten aber auch schlicht
// widerspruechlich sein, denn "Node = Dock dieser Stufe" macht beide zu Aussagen ueber dieselbe
// Groesse.
//
// WAS ICH DESHALB TUE: ich nehme fuer DIESE Matrix die juengere und fuer sie ausdrueckliche Zahl
// (32), lege sie als BENANNTE, ueberschreibbare Konstante an -- und melde den Widerspruch, statt
// ihn zu glaetten. Die 8 wird hier NICHT ueberschrieben; sie gehoert dem Dock-Array, das in
// diesem Paket nicht gebaut wird. Wer beides zusammenfuehrt, braucht einen Owner-Entscheid.
// >>> OFFEN FUER DEN OWNER: ist 32 die Spalten-Obergrenze der Synthese-Matrix UND 8 die
// >>> Dock-Array-Kapazitaet -- oder loest 32 die 8 ab?
//
// ============================================================================================
// WAS HIER BEWUSST NICHT STEHT
// ============================================================================================
// Keine Synthese-Funktions-INHALTE. Eine Synthese-Funktion verbindet die Ergebnisse mehrerer
// Reroute-Ziele zu einem; welche Verknuepfung das ist, haengt an der Snapshot-Aggregations-Semantik
// -- und die ist Auflage K5 des Design-Dokuments und ausdruecklich NOCH NICHT ENTSCHIEDEN
// ("Bis zum Entscheid gilt keine Variante als implizit gesetzt", soll_design Abschnitt 7 K5).
// Hier eine Verknuepfung zu erfinden hiesse, K5 stillschweigend zu entscheiden.

#include "anatomy/anatomy_base.hpp"
#include "heuristik_adapter_gate.hpp"

#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::hybrid {

// ------------------------------------------------------------------------------------------
// (1) DIE DIMENSIONS-KONSTANTEN
// ------------------------------------------------------------------------------------------

/// Node-Obergrenze je Layer. Aus der XML uebersteuerbar (dort `max_docks`); dies ist der Default.
/// Siehe den Zahlen-Widerspruch im Kopf -- 32 ist die juengere, ausdrueckliche Angabe.
inline constexpr std::size_t kHybridNodeObergrenzeDefault = 32;

/// Heutige Rekursionstiefe. Owner: "Tiefe bei 1 belassen, muss aber funktionieren."
/// Die 1 ist eine EINSTELLUNG, keine Annahme der Struktur -- unten ist D=3 mitbelegt.
inline constexpr std::size_t kHybridRestTiefeDefault = 1;

// ------------------------------------------------------------------------------------------
// (2) DIE ZELLE -- Spezialisierungspunkt EINER Synthese-Funktion
// ------------------------------------------------------------------------------------------

/// SyntheseZelle<Layer, Node, Ziel> -- die Adresse GENAU EINER Synthese-Funktion in der Matrix.
///
/// Sie ist ueber das Ziel-Genus mit-parametrisiert, weil die Synthese nicht gattungs-blind sein
/// kann: die Snapshot-Felder eines Set-Tiers sind andere als die eines Sequence-Tiers. Das
/// Concept-Gate haengt deshalb auch hier -- eine Zelle fuer ein Klassifikations-Genus gibt es nicht.
///
/// HEUTE traegt sie nur ihre Adresse und ihre Stufen-ID. Die eigentliche Synthese-Funktion wird in
/// HY-A2 je Zelle spezialisiert -- das ist der Sinn eines Spezialisierungspunkts.
template <std::size_t Layer, std::size_t Node, anatomy::AnatomyGenus Ziel>
    requires HybridZielGenus<Ziel>
struct SyntheseZelle {
    static constexpr std::size_t           layer = Layer; ///< == Rest-Tiefe an dieser Stufe
    static constexpr std::size_t           node  = Node;  ///< Dock-Index innerhalb der Stufe
    static constexpr anatomy::AnatomyGenus ziel  = Ziel;

    /// Die im Stempel ablesbare Heuristik-Funktions-Stufen-ID (LEDGER:481).
    /// Linearisierung (layer, node) -> ID. Sie ist STABIL: der Node-Index laeuft schnell, der
    /// Layer langsam; damit bleiben die IDs einer Stufe zusammenhaengend, was sie im Stempel
    /// lesbar macht. Die Obergrenze geht in die Formel ein -- eine XML, die max_docks aendert,
    /// aendert damit bewusst auch die IDs, und das muss im Stempel sichtbar sein.
    template <std::size_t NodeObergrenze = kHybridNodeObergrenzeDefault>
    [[nodiscard]] static constexpr std::size_t stufen_id() noexcept {
        static_assert(Node < NodeObergrenze, "SyntheseZelle: Node-Index liegt ausserhalb der Obergrenze.");
        return Layer * NodeObergrenze + Node;
    }
};

// ------------------------------------------------------------------------------------------
// (3) DIE MATRIX -- compile-time Rekursion durch Herunterzaehlen der Rest-Tiefe
// ------------------------------------------------------------------------------------------
// "formal EINE Binary am Stueck": die Rekursion wird zur Uebersetzungszeit vollstaendig
// abgewickelt. Es entsteht KEIN Laufzeit-Baum, KEIN Zeiger-Geflecht, KEIN dynamischer Abstieg --
// nur ineinandergeschachtelte Typen, die der Optimierer als ein Stueck sieht.

/// SyntheseMatrix<RestTiefe, Ziel, Nodes> -- die 2D-Matrix-Liste einer Heuristik-Tier-Binary.
template <std::size_t RestTiefe, anatomy::AnatomyGenus Ziel, std::size_t Nodes = kHybridNodeObergrenzeDefault>
    requires HybridZielGenus<Ziel>
struct SyntheseMatrix {
    static constexpr std::size_t           rest_tiefe = RestTiefe;
    static constexpr std::size_t           nodes      = Nodes;
    static constexpr anatomy::AnatomyGenus ziel       = Ziel;

    /// Die naechste innere Stufe -- HIER wird heruntergezaehlt.
    using naechste_stufe = SyntheseMatrix<RestTiefe - 1, Ziel, Nodes>;

    /// Zelle (dieser Layer, node).
    template <std::size_t Node>
    using zelle = SyntheseZelle<RestTiefe, Node, Ziel>;

    /// Zahl der Layer dieser Matrix (D, D-1, ..., 1).
    [[nodiscard]] static constexpr std::size_t layer_anzahl() noexcept { return RestTiefe; }

    /// Zahl der Synthese-Funktionen INSGESAMT: Layer x Node. Das ist die Kern-Zahl des Auftrags.
    [[nodiscard]] static constexpr std::size_t zellen_gesamt() noexcept { return RestTiefe * Nodes; }
};

/// Abbruch der Rekursion bei Rest-Tiefe 0 -- "Herunterzaehlen bis 0".
/// Sie traegt KEINE Zellen: Tiefe 0 heisst, es gibt keine innere Stufe mehr.
template <anatomy::AnatomyGenus Ziel, std::size_t Nodes>
    requires HybridZielGenus<Ziel>
struct SyntheseMatrix<0, Ziel, Nodes> {
    static constexpr std::size_t           rest_tiefe = 0;
    static constexpr std::size_t           nodes      = Nodes;
    static constexpr anatomy::AnatomyGenus ziel       = Ziel;

    [[nodiscard]] static constexpr std::size_t layer_anzahl() noexcept { return 0; }
    [[nodiscard]] static constexpr std::size_t zellen_gesamt() noexcept { return 0; }
};

// ------------------------------------------------------------------------------------------
// (4) WACHEN
// ------------------------------------------------------------------------------------------

// DIE HEUTIGE EINSTELLUNG: Tiefe 1, ein Layer, 32 Knoten -> 32 Synthese-Funktionen.
using HeutigeMatrix = SyntheseMatrix<kHybridRestTiefeDefault, anatomy::AnatomyGenus::SearchAlgorithm>;
static_assert(HeutigeMatrix::layer_anzahl() == 1);
static_assert(HeutigeMatrix::nodes == 32);
static_assert(HeutigeMatrix::zellen_gesamt() == 32);

// "MUSS ABER FUNKTIONIEREN": dieselbe Struktur bei Tiefe 3. Das ist der Beleg, dass die 1 eine
// Einstellung ist und keine versteckte Annahme -- ohne ihn waere "funktioniert auch tiefer" eine
// Behauptung. Drei Layer x 32 Knoten = 96 Synthese-Funktionen.
using TiefeMatrix = SyntheseMatrix<3, anatomy::AnatomyGenus::Set>;
static_assert(TiefeMatrix::layer_anzahl() == 3);
static_assert(TiefeMatrix::zellen_gesamt() == 96);

// Und das Herunterzaehlen erreicht wirklich die 0 (statt irgendwo haengenzubleiben).
static_assert(TiefeMatrix::naechste_stufe::rest_tiefe == 2);
static_assert(TiefeMatrix::naechste_stufe::naechste_stufe::rest_tiefe == 1);
static_assert(TiefeMatrix::naechste_stufe::naechste_stufe::naechste_stufe::rest_tiefe == 0);
static_assert(TiefeMatrix::naechste_stufe::naechste_stufe::naechste_stufe::zellen_gesamt() == 0);

// Eine abweichende Obergrenze schlaegt wirklich durch (XML-Uebersteuerung, hier simuliert).
static_assert(SyntheseMatrix<2, anatomy::AnatomyGenus::View, 8>::zellen_gesamt() == 16);

// STUFEN-IDs: zusammenhaengend je Layer, und ueber die Layer verschieden.
static_assert(SyntheseZelle<1, 0, anatomy::AnatomyGenus::Set>::stufen_id<32>() == 32);
static_assert(SyntheseZelle<1, 31, anatomy::AnatomyGenus::Set>::stufen_id<32>() == 63);
static_assert(SyntheseZelle<2, 0, anatomy::AnatomyGenus::Set>::stufen_id<32>() == 64);
static_assert(SyntheseZelle<1, 0, anatomy::AnatomyGenus::Set>::stufen_id<32>() !=
                  SyntheseZelle<2, 0, anatomy::AnatomyGenus::Set>::stufen_id<32>(),
              "HY-A1-MATRIX: zwei Zellen verschiedener Layer duerfen nie dieselbe Stufen-ID tragen "
              "-- sie sind im Stempel sonst nicht unterscheidbar.");

// DAS GATE HAENGT AUCH HIER: keine Matrix und keine Zelle fuer ein Klassifikations-Genus.
template <anatomy::AnatomyGenus G>
concept MatrixBildbar = requires { typename SyntheseMatrix<1, G, 4>; };

static_assert(MatrixBildbar<anatomy::AnatomyGenus::SearchAlgorithm>);
static_assert(MatrixBildbar<anatomy::AnatomyGenus::View>);
static_assert(!MatrixBildbar<anatomy::AnatomyGenus::FunctionInterfaceReroute>,
              "HY-A1-MATRIX: eine Synthese-Matrix fuer das Reroute-Genus selbst darf es nicht geben "
              "-- synthetisiert wird ueber ECHTE Ziele, nicht ueber eine Klassifikation.");

} // namespace comdare::cache_engine::hybrid
