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
//   Ebene 1  AnatomyGattung    = Außen-Interface/Prüf-Dock: SearchAlgorithm | Container | Graph  (NUR 3)
//   Ebene 2  AnatomyGenus      = TIER-UNTERKLASSE unter einem Gattungs-Interface (fester Achsen-Satz)
//   Ebene 3  Achsen            = Organe der Tier-Unterklasse (permutieren; KEINE optional)
// Set/Sequence/Adapter/View sind Tier-Unterklassen UNTER der Container-Gattung (Doc 24 Z.564 / Doc 27 §0),
// NICHT je eine eigene Gattung. SearchAlgorithm ist eine Gattung MIT einer Tier-Unterklasse (std::map-artig, 18 Achsen; INC-2c: telemetry ist System-Achse).
// ─────────────────────────────────────────────────────────────────────────────

/// AnatomyGattung — Ebene 1: das Außen-Interface zur Welt (Prüf-Dock je Gattung, Doc 24 §8.8). NUR 3.
enum class AnatomyGattung : std::uint8_t {
    SearchAlgorithm = 0, ///< K → V Schlüssel-Wert-Interface (std::map-artig); 1 Tier-Unterklasse gebaut
    Container       = 1, ///< Container-Interface; Tier-Unterklassen: Set/Sequence/Adapter/View
    Graph           = 2  ///< Graph-Interface (Tier-Unterklassen TBD)
};

/// gattung_name() — Compile-Time-String pro Gattung (Ebene 1).
[[nodiscard]] constexpr std::string_view gattung_name(AnatomyGattung g) noexcept {
    switch (g) {
        case AnatomyGattung::SearchAlgorithm: return "SearchAlgorithm";
        case AnatomyGattung::Container: return "Container";
        case AnatomyGattung::Graph: return "Graph";
    }
    return "Unknown";
}

/// AnatomyGenus — Ebene 2: die TIER-UNTERKLASSE (fester Achsen-Satz unter einem Gattungs-Interface).
/// HISTORISCHER NAME „Genus" (Refactor zu AnatomyTierSubclass via #90); konzeptionell = Tier-Unterklasse.
///
/// Tier-Metapher-Mapping (Doku 14 §27.2) + Gattungs-Zuordnung (Ebene 1):
/// | Tier-Metapher | AnatomyGenus (=Tier-Unterklasse) | Gattung (Ebene 1) | std::-Beispiele |
/// |---------------|----------------------------------|-------------------|-----------------|
/// | Saeugetier    | SearchAlgorithm                  | SearchAlgorithm   | map, multimap, unordered_map |
/// | Vogel         | Set                              | Container         | set, multiset, unordered_set |
/// | Reptil        | Sequence                         | Container         | vector, list, deque, array |
/// | Wirbelloses   | Adapter                          | Container         | stack, queue, priority_queue |
/// | Pflanze       | View                             | Container         | span, mdspan, string_view |
///
/// **Vokabular-Brücke (F1a, User-GO 2026-07-16 — golden-neutral, REIN DOKU):** Der User-Begriff
/// *„Gattung"* bezeichnet umgangssprachlich GENAU diese Tier-Unterklasse hier (Ebene 2, `AnatomyGenus`),
/// NICHT das gleichnamige Ebene-1-Außen-Interface `AnatomyGattung`. Diese Zwei-Namen-Realität
/// (Ebene-1 `AnatomyGattung` vs. Ebene-2 `AnatomyGenus`) wird bewusst benannt, damit „Gattung" im
/// User-Sprech und „Genus" im Code nicht verwechselt werden. *Set* ist dabei BEREITS ein eigenes
/// Genus (`Set = 1`) mit eigener Komposition/Anatomie/Observer und eigener ABI
/// (`ISetTier` / `SetObserverSnapshotV1`) samt eigenen `GenusBindingTraits<Set>` — also schon heute
/// eine vollwertige, native Tier-Unterklasse, kein bloßer Alias. Die ECHTE Ebene-1-Promotion (Set als
/// eigenständige `AnatomyGattung`) ist der SEPARATE, koordinierte ABI-Schritt F1b (NICHT hier;
/// s. container_framework.hpp #29-Vermerk + docs/architecture/37).
enum class AnatomyGenus : std::uint8_t {
    SearchAlgorithm = 0, ///< Tier-Unterklasse der SearchAlgorithm-Gattung (vollst. 19-Achsen-Anatomie)
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
/// SearchAlgorithm → eigene Gattung; Set/Sequence/Adapter/View → Container-Gattung (Doc 30 §8.1).
[[nodiscard]] constexpr AnatomyGattung gattung_of(AnatomyGenus tier_subclass) noexcept {
    switch (tier_subclass) {
        case AnatomyGenus::SearchAlgorithm: return AnatomyGattung::SearchAlgorithm;
        case AnatomyGenus::Set:
        case AnatomyGenus::Sequence:
        case AnatomyGenus::Adapter:
        case AnatomyGenus::View: return AnatomyGattung::Container;
    }
    return AnatomyGattung::Container;
}

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

    /// Anatomie-Gattung (Saeugetier/Vogel/Reptil/Wirbelloses/Pflanze)
    [[nodiscard]] virtual AnatomyGenus genus() const noexcept = 0;

    /// Anzahl Achsen (Pflicht 19 fuer Mammal = 17 Such-Achsen + queuing q1/q2; weniger fuer andere Gattungen)
    [[nodiscard]] virtual std::size_t organ_count() const noexcept = 0;
};

} // namespace comdare::cache_engine::anatomy
