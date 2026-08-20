#pragma once
// V41.F.6.1.R5.C.A — AnatomyBase Wurzel (Anatomie-Gattungen)
//
// User-Direktive 2026-05-26 sehr spaet (Doku 14 Teil 4 §25-§31):
// "Fuer die Anatomie eines Suchalgorithmus gibt es unter einer AnatomyBase
// verschiedene spezielle Suchalgorithmen oder Anatomie-Varianten. Suchalgorithmen
// und allgemeine Container gehoeren zu unterschiedlichen Gattungen wie Saeugetiere
// vs. Reptilien. Trotzdem sind alle Gattungen am Ende Lebewesen — fallen unter
// die abstrakte Klasse der AnatomyBase."
//
// Zwei-Schichten-Architektur:
//   1. AnatomyConcept    — Compile-Time C++23 Concept (Static Dispatch)
//   2. IAnatomyBase      — Virtual Interface (Runtime ABI fuer Module-Loader R5.D)
//
// Konkrete Anatomien (SearchAlgorithmAnatomy etc.) erfuellen den Concept; ABI-
// Wrapper-Adapter (in R5.D) bridge zur IAnatomyBase Virtual-Schicht.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §27 + §35.3 (R5.C.A2)
// @task #700 V41.F.6.1.R5.C.A + #701 V41.F.6.1.R5.C.A2
// @related [[anatomie-gattungen]] [[anatomie-nur-achsen-und-observer]] [[execution-engine-als-wurzel]]

#include "../execution_engine/execution_engine_base.hpp" // R5.C.A2 ExecutionEngine Wurzel

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::anatomy {

// ─────────────────────────────────────────────────────────────────────────────
// 3-EBENEN-MODELL (Doc 30 §8.0/§8.1, korr. 2026-06-03 — vorher fälschlich „5 Gattungen"):
//   Ebene 1  AnatomyGattung    = Aussen-Interface/Pruef-Dock: Map | Container | Graph | HeuristikAdapter
//            NACHZUG HY-A1 (09.08.2026): hier stand "NUR 3". Das war bis zum 08.08. korrekt; der
//            Owner hat mit GO-3 eine VIERTE Gattung benannt (HEURISTIK-ADAPTER, LEDGER:2488). Die
//            alte Zahl bleibt als Historie lesbar, die lebende Zahl ist VIER. Die neue Gattung ist
//            allerdings KEINE Pruef-Dock-Gattung: sie hat kein eigenes Dock, weil ihre Binaries per
//            genus() das ZIEL-Genus melden und am Dock des Ziels andocken (Owner E-1 final, Weg C).
//   Ebene 2  AnatomyGenus      = TIER-UNTERKLASSE unter einem Gattungs-Interface (fester Achsen-Satz)
//   Ebene 3  Achsen            = Organe der Tier-Unterklasse (permutieren; KEINE optional)
// Set/Sequence/Adapter/View sind Tier-Unterklassen UNTER der Container-Gattung (Doc 24 Z.564 / Doc 27 §0),
// NICHT je eine eigene Gattung. SEARCHALGORITHM IST DAS GENUS, NICHT DIE GATTUNG -- die Ebene-1-Kategorie
// heisst MAP (Owner-KERN NACHTRAG 4, LEDGER:3836: "SearchAlgorithm ist ein Genus unter der Gattung 'Map'").
// Die Map-Gattung hat am Ist GENAU EIN Genus (SearchAlgorithm, std::map-artig, 18 Organ-Haupt-Achsen ==
// abi::kOrganAxisCount; INC-2c/2d: telemetry+isa sind System-Achsen, STRUKT-R ORG-18 brachte
// persistence_target als 18.); eine Schnittmenge ueber ein Element IST das Element -- deshalb braucht
// Map kein eigenes Kopf-Framework (C7-4, Manager-Entscheid LEDGER:3844).
// NACHZUG E-24 C11 (OP-9): hier stand "17 Achsen" -- eine Zahl aus der Zeit VOR STRUKT-R ORG-18
// (Major 6->7, organ_count() 17->18). Sie ist NICHT mit der 17 aus dem golden-Kontext zu verwechseln
// ("17 Achsen je 2 x persistence_target je 1 == 131072", source_catalog.hpp): dort werden 17 Achsen
// VARIIERT und die 18. ist gepinnt -- das ist eine Aussage ueber den Sweep, nicht ueber die Anatomie.
// ─────────────────────────────────────────────────────────────────────────────

/// AnatomyGattung — Ebene 1: das Außen-Interface zur Welt (Prüf-Dock je Gattung, Doc 24 §8.8). NUR 3.
enum class AnatomyGattung : std::uint8_t {
    // E-24 C7-1 (2026-08-04): UMBENANNT SearchAlgorithm -> Map. Der Enumerator etikettierte die Ebene-1-
    // Kategorie nach ihrem EINZIGEN Genus (Owner-Befund mittags-3 "Terminologie Fehler der Zuordnung",
    // LEDGER:3834; finales Modell NACHTRAG 4, LEDGER:3836). Der ZAHLENWERT 0 bleibt unangetastet -- die
    // Enum-Reihenfolge ist TABU (E24-DOSSIER:168), umbenannt wird ausschliesslich der NAME.
    // Ebene 2 bleibt korrekt: AnatomyGenus::SearchAlgorithm heisst weiterhin so (das GENUS heisst so).
    Map       = 0, ///< K -> V Schluessel-Wert-Interface (std::map-artig); Genus: SearchAlgorithm
    Container = 1, ///< Container-Interface; Genera: Set/Sequence/Adapter/View
    Graph     = 2, ///< Graph-Interface (Stub -- noch kein Genus implementiert, Q5 nach Abgabe)
    // ------------------------------------------------------------------------------------------
    // HY-A1 (09.08.2026): DIE VIERTE EBENE-1-KATEGORIE -- ANGEHAENGT, NIE EINGESCHOBEN.
    // Map/Container/Graph behalten 0/1/2 unangetastet; die Enum-REIHENFOLGE ist TABU
    // (E24-DOSSIER:168). Der Kopf-Kommentar oben sagte "NUR 3" -- das war der Stand bis heute und
    // bleibt als Historie lesbar; die lebende Zahl ist VIER.
    //
    // AUTORITAET, woertlich (Owner GO-3 vom 08.08.2026, LEDGER:2488):
    //   "erzeugt eine neue HEURISTIK-ADAPTER Gattung und ein Genus >>Function-Interface-Reroute<<
    //    denn die Hybrid-Tier-Binary macht nichts anderes, als per compile time die Interfaces
    //    einer Gattung+Genus zu erben und diese nach heuristischen Anforderungen (wie ein Heuristik
    //    gesteuertes Mutex) an die eigentlichen Tier-Binary Interfaces durchzustellen und die
    //    Ergebnisse zu empfangen."
    // Bestaetigt und praezisiert durch Owner-Entscheid E-1 final vom 09.08.2026: die Klassifikation
    // (Gattung+Genus, Weg A) UND der transparente Pass-through (geerbtes Ziel-Genus, Weg C) gelten
    // GEMEINSAM auf verschiedenen Ebenen -- siehe die C-Seiten-Notiz an IAnatomyBase::genus() unten.
    //
    // NAMENS-WARNUNG (die Kollision wurde geprueft, nicht angenommen): der Wortbestandteil "Adapter"
    // ist in diesem Repo BEREITS vergeben -- AnatomyGenus::Adapter ist die Container-Tier-Unterklasse
    // nach Vorbild std::stack/std::queue, mit eigenem Dock, eigener ABI und eigenem Snapshot-POD.
    // Eine C++-Mehrdeutigkeit besteht NICHT (zwei verschiedene scoped enums, jeder Gebrauch ist
    // qualifiziert), wohl aber eine LESE-Verwechslungsgefahr. Deshalb die Merkregel, die ueberall
    // gilt: "Adapter" OHNE Praefix meint IMMER den Container-Genus. Die Hybrid-Gattung heisst NIE
    // verkuerzt "Adapter", sondern immer "HeuristikAdapter" -- Owner-Name "HEURISTIK-ADAPTER".
    // Der GENUS-Name (FunctionInterfaceReroute, s.u.) teilt bewusst KEIN Token mit "Adapter": auf der
    // Genus-Ebene, wo die Kollision real waere, gibt es damit keine Beruehrung.
    // ------------------------------------------------------------------------------------------
    HeuristikAdapter = 3 ///< Heuristik-Adapter-Interface; Genera: Reroute-Genera (Ebene 2, s.u.)
};

/// gattung_name() — Compile-Time-String pro Gattung (Ebene 1).
[[nodiscard]] constexpr std::string_view gattung_name(AnatomyGattung g) noexcept {
    switch (g) {
        case AnatomyGattung::Map: return "Map";
        case AnatomyGattung::Container: return "Container";
        case AnatomyGattung::Graph: return "Graph";
        case AnatomyGattung::HeuristikAdapter: return "HeuristikAdapter";
    }
    return "Unknown";
}

/// AnatomyGenus — Ebene 2: die TIER-UNTERKLASSE (fester Achsen-Satz unter einem Gattungs-Interface).
/// HISTORISCHER NAME „Genus" (Refactor zu AnatomyTierSubclass via #90); konzeptionell = Tier-Unterklasse.
///
/// Tier-Metapher-Mapping (Doku 14 §27.2) + Gattungs-Zuordnung (Ebene 1):
/// NACHZUG E-24 C11 (OP-9): die Gattungs-Spalte der Saeugetier-Zeile nannte bis hier "SearchAlgorithm"
/// -- also den NAMEN DES GENUS in der GATTUNGS-Spalte, exakt der Owner-benannte Terminologie-Fehler.
/// C7-1 hat den Enumerator umbenannt, diese Tabelle direkt darunter aber nicht mitgezogen; sie ist die
/// meistgelesene Zuordnungs-Quelle des Repos und stand damit im Widerspruch zum Enum drei Zeilen ueber ihr.
/// | Tier-Metapher | AnatomyGenus (=Tier-Unterklasse) | Gattung (Ebene 1) | std::-Beispiele |
/// |---------------|----------------------------------|-------------------|-----------------|
/// | Saeugetier    | SearchAlgorithm                  | Map               | map, multimap, unordered_map |
/// | Vogel         | Set                              | Container         | set, multiset, unordered_set |
/// | Reptil        | Sequence                         | Container         | vector, list, deque, array |
/// | Wirbelloses   | Adapter                          | Container         | stack, queue, priority_queue |
/// | Pflanze       | View                             | Container         | span, mdspan, string_view |
///
/// **Vokabular-Bruecke -- FORTGESCHRIEBEN durch E-24 C7-1 (Owner-KERN NACHTRAG 4, LEDGER:3836):**
/// Die alte Bruecke (F1a, 2026-07-16) hielt fest, der User-Begriff "Gattung" meine die Ebene-2-
/// Tier-Unterklasse. Das FINALE Owner-Modell vergibt "Gattung" nun eindeutig an EBENE 1 und benennt
/// deren drei Kategorien: **Map** (Huelle <Key,Value>, map-Gleichnis) / **Container** (Huelle <T>,
/// vector-Gleichnis) / **Graph** (Stub). Ebene 2 sind die GENERA: SearchAlgorithm IN Map;
/// Set/Sequence/Adapter/View IN Container. "Das Genus erbt von der gemeinsamen Gattung" (verbatim) --
/// der Gattungs-Kern ist die mathematische Schnittmenge der Genus-Interfaces, das Genus erweitert sie.
/// Die alte Bruecken-Aussage ist damit SUPERSEDED, nicht geloescht: wer sie in aelteren Dokumenten
/// findet, liest sie in dieser Richtung. *Set* bleibt ein vollwertiges, natives Genus (`Set = 1`) mit
/// eigener Komposition/Anatomie/Observer und eigener ABI (`ISetTier` / `SetObserverSnapshotV1`) samt
/// eigenen `GenusBindingTraits<Set>` -- eine Ebene-1-Promotion von Set ist damit gegenstandslos
/// geworden (Set gehoert zur Gattung Container, nicht daneben; Diskrepanz-Dossier Abschnitt 2.2).
enum class AnatomyGenus : std::uint8_t {
    /// NACHZUG E-24 C11 (OP-9): diese Zeile trug ZWEI stale Angaben -- als Ebene-1-Etikett den Namen des
    /// Genus (die Gattung heisst seit C7-1 Map) und die Achsen-Zahl 17 (die Anatomie fuehrt seit
    /// STRUKT-R ORG-18 achtzehn Organ-Haupt-Achsen, abi::kOrganAxisCount).
    SearchAlgorithm = 0, ///< Tier-Unterklasse der Gattung Map (volle 18-Organ-Achsen-Anatomie, INC-2d + ORG-18)
    Set             = 1, ///< Tier-Unterklasse der Container-Gattung (K only, Bird)
    Sequence        = 2, ///< Tier-Unterklasse der Container-Gattung (V indexed, Reptile)
    Adapter         = 3, ///< Tier-Unterklasse der Container-Gattung (Wrapper über Inner-Substrat, Invertebrate)
    View            = 4, ///< Tier-Unterklasse der Container-Gattung (non-owning, Plant)
    // ------------------------------------------------------------------------------------------
    // HY-A1 (09.08.2026): DER ERSTE REROUTE-GENUS der Gattung HeuristikAdapter. ANGEHAENGT
    // (Wert 5); 0..4 bleiben unangetastet, die Enum-REIHENFOLGE ist TABU (E24-DOSSIER:168).
    //
    // ER IST BEWUSST NICHT DIE ANTWORT VON genus(). Owner-Entscheid E-1 final (09.08.2026) verbindet
    // zwei Ebenen, die man nicht verwechseln darf:
    //   KLASSIFIKATIONS-Ebene (Weg A) -- DIESER Enum-Wert. Er sortiert die Hybrid-.so im Lagerbaum
    //     (Gattung -> Genus -> REST) und benennt die ART des Reroutes. Er ist eine Eigenschaft des
    //     ARTEFAKTS.
    //   INTERFACE-Ebene (Weg C) -- genus() einer Hybrid-Binary liefert das GEERBTE ZIEL-Genus, also
    //     z.B. SearchAlgorithm, NIE diesen Wert. Der Pass-through ist nach aussen transparent.
    // Wer diesen Wert je aus genus() zurueckgibt, bricht Weg C. Die ausfuehrliche Begruendung samt
    // Folgen fuer Dock-Auswahl und Registry steht an IAnatomyBase::genus() unten -- an genau der
    // Stelle, an der jemand es sonst falsch machen wuerde.
    //
    // WARUM DIE STRUKTUR MEHRERE REROUTE-GENERA TRAGEN MUSS (Owner E-1 final, woertlich): "weil sich
    // die Art und Weise der Reroute-Genus fuer den Anwendungszweck unterscheiden kann -- etwa wenn ein
    // Graph ein anderes Reroute benoetigt als SearchAlgorithm. Daher ist das durchaus eine eigene
    // Gattung mit multiplen Genus (spaeter)." HEUTE gibt es GENAU EINEN. Ein zweiter wird hier
    // angehaengt (Wert 6) und bekommt eine HeuristikRerouteStrategy-Spezialisierung -- mehr nicht:
    // die Klassifikations-Partition (hybrid/heuristik_adapter_klassifikation.hpp) und das
    // Concept-Gate sind ueber gattung_of() formuliert, nicht ueber Aufzaehlungen einzelner Werte.
    //
    // NAMENSWAHL, begruendet: der Owner-Name ist "Function-Interface-Reroute". Als Enumerator
    // FunctionInterfaceReroute. Der naheliegende Kurzname "Adapter" ist VERGEBEN (View/Set/Sequence/
    // Adapter sind Container-Tier-Unterklassen; Adapter = std::stack-Vorbild, ~170 Fundstellen).
    // "Reroute" statt "Adapter" macht die Verwechslung schon lexikalisch unmoeglich und trifft
    // zugleich die Sache genauer: es wird DURCHGESTELLT, nicht gekapselt.
    // ------------------------------------------------------------------------------------------
    FunctionInterfaceReroute = 5 ///< Reroute-Genus der Gattung HeuristikAdapter -- NIE von genus()
};

/// genus_name<G>() — Compile-Time-String pro Tier-Unterklasse (Ebene 2).
[[nodiscard]] constexpr std::string_view genus_name(AnatomyGenus g) noexcept {
    switch (g) {
        case AnatomyGenus::SearchAlgorithm: return "SearchAlgorithm";
        case AnatomyGenus::Set: return "Set";
        case AnatomyGenus::Sequence: return "Sequence";
        case AnatomyGenus::Adapter: return "Adapter";
        case AnatomyGenus::View: return "View";
        case AnatomyGenus::FunctionInterfaceReroute: return "FunctionInterfaceReroute";
    }
    return "Unknown";
}

/// gattung_of() — Ebene 2 → Ebene 1: die Gattung (Außen-Interface), zu der eine Tier-Unterklasse gehört.
/// NACHTRAG Q2/V-06 + KON101-02 (18.08.2026): diese Funktion ist ab jetzt AUCH die ABI-Wahrheit. Die
/// Modul-Makros materialisieren comdare_anatomy_gattung NICHT aus einem zweiten Literal, sondern
/// GENAU aus diesem Aufruf -- die Gattung reist damit nirgends als eigene Angabe, die auseinanderlaufen
/// koennte. Der Loader prueft dieselbe Gleichung noch einmal am geladenen Modul und lehnt eine
/// Abweichung fail-closed ab (status_identity_mismatch, anatomy_module_loader.hpp).
/// E-24 C7-1: SearchAlgorithm -> Gattung MAP; Set/Sequence/Adapter/View -> Container-Gattung
/// (Owner-KERN NACHTRAG 4, LEDGER:3836). Das Genus ERBT das Gattungs-Interface -- diese Funktion ist die
/// constexpr-Form dieser Vererbungs-Zuordnung und zugleich der host-seitige Weg von genus() zur Gattung.
[[nodiscard]] constexpr AnatomyGattung gattung_of(AnatomyGenus tier_subclass) noexcept {
    switch (tier_subclass) {
        case AnatomyGenus::SearchAlgorithm: return AnatomyGattung::Map;
        case AnatomyGenus::Set:
        case AnatomyGenus::Sequence:
        case AnatomyGenus::Adapter:
        case AnatomyGenus::View: return AnatomyGattung::Container;
        // HY-A1: der Reroute-Genus gehoert zur Gattung HeuristikAdapter. Diese Zuordnung ist der
        // EINZIGE Ort, an dem die Ebene-1-Zugehoerigkeit der Hybrid-Klassifikation steht -- das
        // Concept-Gate (hybrid/heuristik_adapter_gate.hpp) leitet daraus ab, statt Werte aufzuzaehlen.
        case AnatomyGenus::FunctionInterfaceReroute: return AnatomyGattung::HeuristikAdapter;
    }
    // Review #15 Fix 2 (18.08.2026): dieser Fallthrough ist eine reine -Wreturn-type-BERUHIGUNG fuer
    // Bytes ausserhalb der Enum-Werte. Er darf NIE ABI-Antwortquelle sein: ein rohes uint8 von der
    // C-ABI-Grenze wird ZUERST mit genus_bekannt() (unten) geprueft -- sonst wuerde ein unbekanntes
    // Byte (z.B. 250) hier STILL als Container klassifiziert und liefe als in sich konsistente Luege
    // durch jeden nachgelagerten Gleichheits-Vergleich (der Loader-Riegel prueft Konsistenz, keinen
    // Wertebereich).
    return AnatomyGattung::Container;
}

/// genus_bekannt() -- die WERTKLASSEN-Wache fuer rohe uint8-Genus-Bytes von der C-ABI-Grenze
/// (Review #15 Fix 2, 18.08.2026). Sie beantwortet die Frage, die KEINE Nachbar-Funktion
/// beantwortet: "ist dieses Byte ueberhaupt ein AnatomyGenus?" -- gattung_of() ist ueber den
/// Fallthrough oben TOTAL (defaultet unbekannte Bytes still auf Container), und
/// hybrid::ist_abi_sichtbares_genus() leitet aus gattung_of() ab, laesst also jedes Nicht-Enum-Byte
/// passieren (ist_abi_sichtbares_genus(250) == true). Der Loader prueft deshalb BEIDE Wertklassen
/// nacheinander, denn jede allein liesse eine durch: genus_bekannt(5) == true (der Reroute-Wert IST
/// ein Enum-Wert), ist_abi_sichtbares_genus(250) == true (Container-Default).
[[nodiscard]] constexpr bool genus_bekannt(std::uint8_t roh_genus) noexcept {
    switch (static_cast<AnatomyGenus>(roh_genus)) {
        case AnatomyGenus::SearchAlgorithm:
        case AnatomyGenus::Set:
        case AnatomyGenus::Sequence:
        case AnatomyGenus::Adapter:
        case AnatomyGenus::View:
        case AnatomyGenus::FunctionInterfaceReroute: return true;
    }
    return false;
}

// Die Wertklassen-Pins am Ort der Funktion: alle sechs Enum-Werte JA; die erste Nicht-Enum-Zelle (6)
// und das am Objekt verifizierte Container-Default-Byte (250) NEIN.
static_assert(genus_bekannt(0) && genus_bekannt(1) && genus_bekannt(2) && genus_bekannt(3) && genus_bekannt(4) &&
              genus_bekannt(5));
static_assert(!genus_bekannt(6) && !genus_bekannt(250),
              "genus_bekannt() muss jedes Nicht-Enum-Byte abweisen -- sonst traegt eine konsistente "
              "Luege (Symbol, gattung_of und Instanz einig auf einem Phantasie-Wert) falsche "
              "Identitaet in Stempel, Lager und Messreihe (Review #15 Fix 2).");

// ---------------------------------------------------------------------------------------------
// E-24 C7-6 -- DIE GATTUNG WIRD ABI-SICHTBAR, OHNE EINEN NEUEN WIRE-STRING EINZUFRIEREN
// ---------------------------------------------------------------------------------------------
//
// AUFTRAG (C7-Auflage C7-6, LEDGER:3843): "Wire OHNE neuen String (host-seitig constexpr gattung_of
// aus genus(); falls Wire-Member je noetig: uint8 NACH C7-1)".
//
// DER ENTSCHEID, AUSDRUECKLICH: es kommt WEDER ein gattung()-vtable-Member NOCH ein gattung-Feld
// NOCH ein Gattungs-String auf den Draht. Begruendung am Objekt:
//   (1) genus() TRAEGT DIE INFORMATION BEREITS. gattung_of ist total und constexpr -- die Ebene-1-
//       Zuordnung ist aus der Ebene-2-Identitaet VERLUSTFREI ableitbar. Ein zweites Feld waere eine
//       zweite Wahrheit, die auseinanderlaufen kann (und irgendwann wird).
//   (2) EIN gattung()-Member an IAnatomyBase waere ein VTABLE-ANHANG an der Wurzel-Flaeche -- genau
//       das, was die append-only-Doktrin (Auflage 5) im ganzen Fenster verbietet. Alle C6-Formen sind
//       eigenen Sub-Interfaces ausgewichen; die Wurzel darf nicht die Ausnahme sein.
//   (3) EIN STRING waere zusaetzlich EINGEFROREN: Etiketten reisen in Experiment-Logs (RF-3-Auflage) --
//       ein Gattungs-String waere ab dem ersten Lauf unveraenderlich. Genau diese Falle hat das
//       Fenster bei FK-8 bereits benannt (Manager-Entscheid: Etiketten BELASSEN, LEDGER:3844).
// DIE ORDNUNG IST DAMIT EINGEHALTEN: C7-1 hat das Etikett korrigiert, BEVOR hier ueberhaupt etwas
// sichtbar wird -- und weil nichts eingefroren wird, bleibt die Korrektur auch nachtraeglich frei.

/// module_gattung() -- Ebene-1-Zuordnung einer GELADENEN Anatomie, host-seitig abgeleitet.
/// Das ist die C7-6-Sichtbarmachung: der Host beantwortet "welche Gattung ist dieses Modul?" ohne
/// ein einziges neues Byte auf dem Draht. noexcept, allokationsfrei, ein virtueller Aufruf (genus())
/// plus ein constexpr-Sprung -- der Dock ruft sie 1x kalt beim Andocken, nie im Hot-Loop.
[[nodiscard]] inline AnatomyGattung module_gattung(class IAnatomyBase const& m) noexcept;

/// kingdom_name() — wie in der Taxonomie: alle Gattungen/Tier-Unterklassen sind "Animalia" Lebewesen.
[[nodiscard]] constexpr std::string_view kingdom_name() noexcept { return "Animalia"; }

// ─────────────────────────────────────────────────────────────────────────────
// AnatomyConcept — Compile-Time-Wurzel-Concept aller Anatomien
// ─────────────────────────────────────────────────────────────────────────────

/// AnatomyConcept — jedes konkrete Anatomie-Template muss diese statischen
/// Pflicht-Members liefern. Wird als Concept-Constraint in Templates verwendet.
template <class A>
concept AnatomyConcept = requires {
    typename A::composition_t;
    { A::composition_name() } -> std::convertible_to<std::string_view>;
    { A::paper_id() } -> std::convertible_to<std::string_view>;
    { A::organ_count() } -> std::convertible_to<std::size_t>;
    { A::genus() } -> std::convertible_to<AnatomyGenus>;
};

// ─────────────────────────────────────────────────────────────────────────────
// IAnatomyBase — Virtual Interface (Runtime ABI fuer Module-Loader R5.D)
// ─────────────────────────────────────────────────────────────────────────────

/// IAnatomyBase — abstract base fuer alle Anatomie-Gattungen (Runtime-Polymorph).
///
/// **Wurzel-Inheritance (R5.C.A2):** Erbt von `IExecutionEngine` — Anatomien
/// sind eine spezielle Form von ExecutionEngine (Lebewesen vs Viren, Doku 14 §33-§40).
///
/// **Verwendung:** Ausschliesslich R5.E Module-Loader-Adapter. Konkrete Anatomien
/// nutzen die Compile-Time-Concept-Schicht (AnatomyConcept), NICHT diese Virtual-
/// Schicht — fuer Hot-Path-Performance.
///
/// **Zweck Virtual:** Ein .so/.dll exportiert genau EINE IAnatomyBase-Instanz
/// (extern "C" Factory), die der CacheEngineBuilder ueber dlopen lossticht.
class IAnatomyBase : public ::comdare::cache_engine::execution_engine::IExecutionEngine {
public:
    // ─────────────────────────────────────────────────────────────────────
    // IExecutionEngine-Pflicht-Override: engine_kind() = Anatomy (final)
    // ─────────────────────────────────────────────────────────────────────
    [[nodiscard]] ::comdare::cache_engine::execution_engine::ExecutionEngineKind engine_kind() const noexcept final {
        return ::comdare::cache_engine::execution_engine::ExecutionEngineKind::Anatomy;
    }

    // engine_name() wird in konkreten Anatomien implementiert (z.B. composition_name)
    // warm_up/reset/shutdown bleiben Pflicht-API der Subklasse

    // ─────────────────────────────────────────────────────────────────────
    // Anatomie-spezifische Pflicht-API (zusaetzlich zu IExecutionEngine)
    // ─────────────────────────────────────────────────────────────────────

    /// Composition-Identifier (z.B. "ArtComposition")
    [[nodiscard]] virtual std::string_view composition_name() const noexcept = 0;

    /// Paper-Referenz (z.B. "P01 Leis ICDE 2013")
    [[nodiscard]] virtual std::string_view paper_id() const noexcept = 0;

    /// Das GENUS (Ebene 2; Tier-Metapher Saeugetier/Vogel/Reptil/Wirbelloses/Pflanze). E-24 C7-1: der
    /// frueher hier stehende Begriff "Anatomie-Gattung" war Ebene-2-Sprache und ist nach dem finalen
    /// Owner-Modell (LEDGER:3836) falsch -- die GATTUNG ist Ebene 1 (Map/Container/Graph) und wird
    /// host-seitig constexpr aus diesem Wert abgeleitet (gattung_of), NICHT ueber den Draht getragen.
    ///
    /// ----------------------------------------------------------------------------------------
    /// HY-A1 (09.08.2026) -- DIE HYBRID-REGEL. HIER, WEIL HIER DER FEHLER PASSIEREN WUERDE.
    /// ----------------------------------------------------------------------------------------
    /// Eine HYBRID-Tier-Binary (Gattung HeuristikAdapter) liefert aus genus() das **geerbte
    /// ZIEL-Genus** -- also SearchAlgorithm, Set, Sequence, Adapter oder View -- und NIEMALS
    /// AnatomyGenus::FunctionInterfaceReroute. Owner-Entscheid E-1 final vom 09.08.2026, Weg C:
    /// der Pass-through ist nach aussen TRANSPARENT; wer die Binary befragt, sieht das Ziel.
    ///
    /// DER NAHELIEGENDE FEHLER: "die Binary ist ein HeuristikAdapter, also gibt genus() den
    /// HeuristikAdapter-Wert zurueck." Das ist falsch, und es ist teuer falsch -- drei Folgen:
    ///   (1) PruefDockRegistry::select_for waehlt das Dock ueber die IM MODUL deklarierte Gattung
    ///       (pruef_dock_registry.hpp, "EIN Dock je Gattung"). Gaebe eine Hybrid-Binary ihren
    ///       eigenen Wert zurueck, faende select_for KEIN Dock -- die Binary waere unmessbar.
    ///   (2) Genau deshalb braucht die Hybrid-Stufe KEIN sechstes Pruef-Dock: sie dockt an dem
    ///       Dock ihres ZIEL-Genus an und wird dort gemessen wie jedes andere Tier (soll_design
    ///       Abschnitt 2 + Abschnitt 5: "wird am CEB-Pruef-Dock gemessen wie jedes andere Tier").
    ///   (3) Die Hybrid-NATUR steht damit NICHT in genus(), sondern im STEMPEL bzw. im
    ///       Sidecar-Manifest -- und in der Klassifikation (dem Enum-Wert), die den Lagerbaum
    ///       sortiert. Das ist Weg A, und er widerspricht Weg C nicht: verschiedene Ebenen.
    ///
    /// KURZFORM: FunctionInterfaceReroute ist ein KLASSIFIKATIONS-Wert (Artefakt-Eigenschaft),
    /// kein INTERFACE-Wert (Laufzeit-Antwort). Die Partition beider Mengen ist compile-hart in
    /// hybrid/heuristik_adapter_klassifikation.hpp festgelegt und dort per static_assert bewacht.
    [[nodiscard]] virtual AnatomyGenus genus() const noexcept = 0;

    /// Anzahl Achsen (Pflicht 17 fuer Mammal = 15 Such-Achsen + queuing q1/q2, INC-2d; weniger fuer andere Gattungen)
    [[nodiscard]] virtual std::size_t organ_count() const noexcept = 0;

    // E-24 C7-6: HIER steht BEWUSST KEIN gattung()-Member. Die Ebene-1-Zuordnung wird host-seitig aus
    // genus() abgeleitet (module_gattung, s. o.) -- ein Member hier waere ein vtable-Anhang an der
    // Wurzel-Flaeche und braeche jede geladene Alt-DLL.
};

/// E-24 C7-6: die Ableitung, jetzt wo IAnatomyBase vollstaendig ist (Deklaration s. o.).
[[nodiscard]] inline AnatomyGattung module_gattung(IAnatomyBase const& m) noexcept { return gattung_of(m.genus()); }

} // namespace comdare::cache_engine::anatomy
