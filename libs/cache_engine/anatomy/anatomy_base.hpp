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
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §27
// @task #700 V41.F.6.1.R5.C.A
// @related [[anatomie-gattungen]] [[anatomie-nur-achsen-und-observer]]

#include <concepts>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::anatomy {

// ─────────────────────────────────────────────────────────────────────────────
// AnatomyGenus — 5 Gattungen (Compile-Time Enum)
// ─────────────────────────────────────────────────────────────────────────────

/// AnatomyGenus — Klassifizierung der Anatomie-Auspraegung.
///
/// Tier-Metapher-Mapping (Doku 14 §27.2):
/// | Tierwelt-Gattung | AnatomyGenus     | std::-Container Beispiele |
/// |------------------|------------------|---------------------------|
/// | Saeugetier       | SearchAlgorithm  | map, multimap, unordered_map, flat_map |
/// | Vogel            | Set              | set, multiset, unordered_set, flat_set |
/// | Reptil           | Sequence         | vector, list, deque, array |
/// | Wirbelloses      | Adapter          | stack, queue, priority_queue |
/// | Pflanze          | View             | span, mdspan, string_view |
enum class AnatomyGenus : std::uint8_t {
    SearchAlgorithm = 0,  ///< K → V, Mammal-Gattung (vollstaendige 17-Achsen-Anatomie)
    Set             = 1,  ///< K only, Bird-Gattung
    Sequence        = 2,  ///< V indexed, Reptile-Gattung
    Adapter         = 3,  ///< Wrapper ueber Inner-Container, Invertebrate-Gattung
    View            = 4   ///< non-owning, Plant-Gattung
};

/// genus_name<G>() — Compile-Time-String pro Gattung.
[[nodiscard]] constexpr std::string_view genus_name(AnatomyGenus g) noexcept {
    switch (g) {
        case AnatomyGenus::SearchAlgorithm: return "SearchAlgorithm";
        case AnatomyGenus::Set:             return "Set";
        case AnatomyGenus::Sequence:        return "Sequence";
        case AnatomyGenus::Adapter:         return "Adapter";
        case AnatomyGenus::View:            return "View";
    }
    return "Unknown";
}

/// kingdom_name() — wie in der Taxonomie: alle Gattungen sind "Animalia" Lebewesen.
[[nodiscard]] constexpr std::string_view kingdom_name() noexcept {
    return "Animalia";  // alle 5 Anatomie-Gattungen sind Lebewesen
}

// ─────────────────────────────────────────────────────────────────────────────
// AnatomyConcept — Compile-Time-Wurzel-Concept aller Anatomien
// ─────────────────────────────────────────────────────────────────────────────

/// AnatomyConcept — jedes konkrete Anatomie-Template muss diese statischen
/// Pflicht-Members liefern. Wird als Concept-Constraint in Templates verwendet.
template <class A>
concept AnatomyConcept = requires {
    typename A::composition_t;
    { A::composition_name() } -> std::convertible_to<std::string_view>;
    { A::paper_id() }         -> std::convertible_to<std::string_view>;
    { A::organ_count() }      -> std::convertible_to<std::size_t>;
    { A::genus() }            -> std::convertible_to<AnatomyGenus>;
};

// ─────────────────────────────────────────────────────────────────────────────
// IAnatomyBase — Virtual Interface (Runtime ABI fuer Module-Loader R5.D)
// ─────────────────────────────────────────────────────────────────────────────

/// IAnatomyBase — abstract base fuer alle Anatomie-Gattungen (Runtime-Polymorph).
///
/// **Verwendung:** AusschliesslichR5.D Module-Loader-Adapter. Konkrete Anatomien
/// nutzen die Compile-Time-Concept-Schicht (AnatomyConcept), NICHT diese Virtual-
/// Schicht — fuer Hot-Path-Performance.
///
/// **Zweck Virtual:** Ein .so/.dll exportiert genau EINE IAnatomyBase-Instanz
/// (extern "C" Factory), die der CacheEngineBuilder ueber dlopen lossticht.
class IAnatomyBase {
public:
    virtual ~IAnatomyBase() = default;

    /// Composition-Identifier (z.B. "ArtComposition")
    [[nodiscard]] virtual std::string_view composition_name() const noexcept = 0;

    /// Paper-Referenz (z.B. "P01 Leis ICDE 2013")
    [[nodiscard]] virtual std::string_view paper_id() const noexcept = 0;

    /// Anatomie-Gattung (Saeugetier/Vogel/Reptil/Wirbelloses/Pflanze)
    [[nodiscard]] virtual AnatomyGenus     genus() const noexcept = 0;

    /// Anzahl Achsen (Pflicht 17 fuer Mammal, weniger fuer andere Gattungen)
    [[nodiscard]] virtual std::size_t      organ_count() const noexcept = 0;
};

}  // namespace comdare::cache_engine::anatomy
