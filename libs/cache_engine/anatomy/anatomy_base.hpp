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
#include <string_view>

namespace comdare::cache_engine::anatomy {

// ─────────────────────────────────────────────────────────────────────────────
// 3-EBENEN-MODELL (Doc 30 §8.0/§8.1, korr. 2026-06-03 — vorher fälschlich „5 Gattungen"):
//   Ebene 1  AnatomyGattung    = Aussen-Interface/Pruef-Dock: Map | Container | Graph  (NUR 3)
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
    Graph     = 2  ///< Graph-Interface (Stub -- noch kein Genus implementiert, Q5 nach Abgabe)
};

/// gattung_name() — Compile-Time-String pro Gattung (Ebene 1).
[[nodiscard]] constexpr std::string_view gattung_name(AnatomyGattung g) noexcept {
    switch (g) {
        case AnatomyGattung::Map: return "Map";
        case AnatomyGattung::Container: return "Container";
        case AnatomyGattung::Graph: return "Graph";
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
    View            = 4  ///< Tier-Unterklasse der Container-Gattung (non-owning, Plant)
};

/// genus_name<G>() — Compile-Time-String pro Tier-Unterklasse (Ebene 2).
[[nodiscard]] constexpr std::string_view genus_name(AnatomyGenus g) noexcept {
    switch (g) {
        case AnatomyGenus::SearchAlgorithm: return "SearchAlgorithm";
        case AnatomyGenus::Set: return "Set";
        case AnatomyGenus::Sequence: return "Sequence";
        case AnatomyGenus::Adapter: return "Adapter";
        case AnatomyGenus::View: return "View";
    }
    return "Unknown";
}

/// gattung_of() — Ebene 2 → Ebene 1: die Gattung (Außen-Interface), zu der eine Tier-Unterklasse gehört.
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
    }
    return AnatomyGattung::Container;
}

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
