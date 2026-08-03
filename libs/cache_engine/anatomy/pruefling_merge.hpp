#pragma once
// V41.F.6.1.R5.C — Pruefling-Slot-Pattern + 3 Kompositionale Joins (Stufe 1/2/3)
//
// User-Direktive 2026-05-26 spaet (Doku 14 Teil 3 §18-§19):
// 3 Arten Kompositionale Joins:
//   Stufe 1 comdare_perms_ce            — KEINE Pruefling-Beteiligung (CE-only)
//   Stufe 2 comdare_perms_<pruefling>  — ERSETZT-mit-Fallback pro Achse via has_pruefling_v<>
//   Stufe 3 comdare_perms_full_join    — mp_unique<mp_append<DefaultList, PrueflingLists...>>
//
// Pruefling-Repos (z.B. prt-art) extenden den Slot per Concept-Detection.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §18-§19
// @task #699 V41.F.6.1.R5.C
// @related [[3-kompositionale-joins-anatomie]] [[pruefling-replace-not-extend]]

#include "anatomy_base.hpp"

#include <boost/mp11.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::anatomy::pruefling {

namespace mp = boost::mp11;

// ─────────────────────────────────────────────────────────────────────────────
// (1) PrueflingSlot — Default-leerer Slot pro Achse
// ─────────────────────────────────────────────────────────────────────────────

/// EmptyPrueflingSlot — Fallback wenn kein Pruefling fuer die Achse existiert.
struct EmptyPrueflingSlot {
    using PrueflingVariants             = mp::mp_list<>;
    static constexpr bool has_pruefling = false;
    // R5.C.B: Default-Gattung ist SearchAlgorithm (Mammal in Tier-Metapher).
    // Andere Gattungen muessen Slot::genus explizit ueberschreiben.
    static constexpr AnatomyGenus genus = AnatomyGenus::SearchAlgorithm;
};

/// PrueflingSlotConcept — ein Slot ist gueltig wenn er PrueflingVariants
/// (mp_list von Wrappers) + has_pruefling (bool) liefert.
///
/// **R5.C.B Erweiterung:** Slot DARF optional `static constexpr AnatomyGenus genus`
/// deklarieren. Wenn nicht: Default SearchAlgorithm (Mammal-Gattung) via
/// `slot_genus_v<Slot>` (siehe unten). Empfehlung: jeder neue Slot deklariert
/// genus explizit fuer Klarheit.
template <class Slot>
concept PrueflingSlotConcept = requires {
    typename Slot::PrueflingVariants;
    { Slot::has_pruefling } -> std::convertible_to<bool>;
};

/// HasPruefling<Slot> — Compile-Time-Predicate (true wenn Pruefling-Slot belegt).
template <class Slot>
constexpr bool HasPruefling_v = PrueflingSlotConcept<Slot> && Slot::has_pruefling;

// ─────────────────────────────────────────────────────────────────────────────
// (1.5) R5.C.B — Slot-Genus-Detection (Pflicht-Validierung fuer Gattungs-Constraint)
// ─────────────────────────────────────────────────────────────────────────────

/// HasExplicitGenus<Slot> — true wenn Slot::genus deklariert ist.
template <class Slot>
concept HasExplicitGenus = requires {
    { Slot::genus } -> std::convertible_to<AnatomyGenus>;
};

/// slot_genus_v<Slot> — liefert die Gattung des Slots.
/// Default: SearchAlgorithm (Mammal-Gattung) wenn Slot::genus nicht deklariert.
template <class Slot>
constexpr AnatomyGenus slot_genus_v = [] {
    if constexpr (HasExplicitGenus<Slot>) {
        return Slot::genus;
    } else {
        return AnatomyGenus::SearchAlgorithm;
    }
}();

/// IsSlotOfGenus<Slot, G> — Compile-Time-Predicate fuer Gattungs-Match.
/// Verwendet in SearchAlgorithmPermutationEngine static_assert (Doku 14 §32.3).
template <class Slot, AnatomyGenus G>
constexpr bool IsSlotOfGenus_v = (slot_genus_v<Slot> == G);

/// IsSearchAlgorithmSlot<Slot> — Concept fuer Mammal-Slot-Validierung.
template <class Slot>
concept IsSearchAlgorithmSlot = PrueflingSlotConcept<Slot> && IsSlotOfGenus_v<Slot, AnatomyGenus::SearchAlgorithm>;

// ─────────────────────────────────────────────────────────────────────────────
// (2) Stufe 1 — comdare_perms_ce (CE-only, KEINE Pruefling-Beteiligung)
// ─────────────────────────────────────────────────────────────────────────────

/// StufeOneAxis — identisch zur DefaultList (kein Pruefling).
template <class DefaultList>
using StufeOneAxis = DefaultList;

// ─────────────────────────────────────────────────────────────────────────────
// (3) Stufe 2 — comdare_perms_<pruefling> (ERSETZT-mit-Fallback)
// ─────────────────────────────────────────────────────────────────────────────

/// StufeTwoAxis — wenn Pruefling-Slot belegt: PrueflingVariants ERSETZEN DefaultList.
/// Sonst Fallback auf DefaultList.
///
/// Beispiel:
///   namespace prt_art::axis_03a {
///       struct Slot {
///           using PrueflingVariants = mp::mp_list<PrtArtRadix512>;
///           static constexpr bool has_pruefling = true;
///       };
///   }
///   using Stufe2 = StufeTwoAxis<ce::axis_03a::DefaultVariants, prt_art::axis_03a::Slot>;
///   // → Stufe2 = mp::mp_list<PrtArtRadix512>  (CE-Defaults ueberschrieben)
template <class DefaultList, class Slot>
using StufeTwoAxis = std::conditional_t<HasPruefling_v<Slot>, typename Slot::PrueflingVariants, DefaultList>;

// ─────────────────────────────────────────────────────────────────────────────
// (4) Stufe 3 — comdare_perms_full_join (Union non-redundant)
// ─────────────────────────────────────────────────────────────────────────────

/// StufeThreeAxis — Union aller Default + Pruefling-Varianten, dedupliziert.
/// MP11-Idiom: mp_unique<mp_append<...>>.
///
/// Identisch zu permutations::AxisFullJoin aber mit Slot-Detection statt
/// raw PrueflingLists.
template <class DefaultList, class... Slots>
using StufeThreeAxis = mp::mp_unique<mp::mp_append<DefaultList, typename Slots::PrueflingVariants...>>;

// ─────────────────────────────────────────────────────────────────────────────
// (5) MergeStrategy enum + dispatched Helper
// ─────────────────────────────────────────────────────────────────────────────

enum class MergeStrategy {
    Stufe1_CeOnly,           // KEINE Pruefling
    Stufe2_PrueflingReplace, // ERSETZT-mit-Fallback
    Stufe3_FullJoin          // Union non-redundant
};

// -----------------------------------------------------------------------------
// (5a) A13-M3/C5 -- die Q2-CT-WACHE an der MERGE-NAMENS-NAHT
// -----------------------------------------------------------------------------
//
// OWNER-Q2 (02.08.2026, Design-Invariante): die Merge-Strategie WIRD durchgefuehrt, aber sie hat seit
// A13-M3 KEINE eigene Stempel-ZEILE mehr (Owner-E2). Sie lebt im Stempel nur noch ueber das
// 'e'-Experimentalflag und ueber die erweiterten hierarchischen Namen ("prt-art.memory.abc@1.0.0c", seit
// A13-M1 parser-gedeckt). GENAU DARAUS folgt eine Pflicht, die vorher die merge-Zeile getragen hat:
//
//   Zwei BYTE-VERSCHIEDENE Merge-Binaries duerfen NIE namensgleiche Organ-Segmente rendern.
//
// Warum das jetzt scharf sein muss: das Organ-Segment ist "<achse>=<name()>@<X.Y.Z[flag]>". Rendern eine
// ce-only-Binary und ihre Stufe2-Ersetzung dasselbe Segment, sind ihre Organ-Zeilen identisch -- und damit
// (bei gleicher System-/Mess-Zeile) ihr SHA512-Fingerprint. Unter dem SHA512-only-Skip-Gate (F7) wuerde die
// eine fuer die andere wiederverwendet. Solange die merge-Zeile existierte, trennte sie die beiden; ohne sie
// ist der NAME die einzige Trennung, und die muss eine Wache sein, keine Absprache.
//
// VOLLSTAENDIG, NICHT LUECKENHAFT (Z-07-Lehre "eine Wache am Engpass kann niemand vergessen"): sie haengt
// an MergeImpl, dem EINEN Engpass, durch den jede MergeAxis<>-Instanziierung geht -- nicht opt-in an den
// Slot-Definitionen. Wer einen neuen Pruefling-Slot anlegt, kann sie nicht umgehen.
//
// EHRLICHE ABGRENZUNG (kein Schlupfloch, sondern die Definition): geprueft werden Paare STEMPELBARER
// Varianten (name() + algo_version). Ein Typ ohne beides kann gar kein Organ-Segment rendern -- er hat
// nichts, womit er kollidieren koennte. Damit bleiben reine Mechanik-Tests mit Dummy-Typen baubar, ohne dass
// die Wache an einer realen, stempelnden Achse je aussetzt.

/// Ein Typ, der ein Organ-Stempel-Segment rendern kann: er traegt name() UND algo_version.
template <class T>
concept StampableAxisVariant = requires {
    { T::name() } -> std::convertible_to<std::string_view>;
    { std::string_view{T::algo_version} } -> std::same_as<std::string_view>;
};

/// Kollidieren die gerenderten Organ-SEGMENTE zweier VERSCHIEDENER Varianten? (gleicher Name UND gleiche
/// Version -> byte-gleiches Segment trotz verschiedener Binary).
template <class A, class B>
[[nodiscard]] consteval bool merge_segments_collide() noexcept {
    if constexpr (StampableAxisVariant<A> && StampableAxisVariant<B>) {
        return !std::is_same_v<A, B> && std::string_view{A::name()} == std::string_view{B::name()} &&
               std::string_view{A::algo_version} == std::string_view{B::algo_version};
    } else {
        return false; // nicht stempelbar -> rendert kein Segment -> kann keines duplizieren
    }
}

/// Rendern alle Paare (a aus ListA, b aus ListB) UNTERSCHEIDBARE Organ-Segmente?
template <class ListA, class ListB>
[[nodiscard]] consteval bool merge_lists_render_distinct_segments() noexcept {
    bool ok = true;
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, ListA>>([&](auto a) {
        mp::mp_for_each<mp::mp_transform<mp::mp_identity, ListB>>([&](auto b) {
            if (merge_segments_collide<typename decltype(a)::type, typename decltype(b)::type>()) ok = false;
        });
    });
    return ok;
}

namespace detail {
template <MergeStrategy S, class Default, class... Slots>
struct MergeImpl;

template <class Default, class... Slots>
struct MergeImpl<MergeStrategy::Stufe1_CeOnly, Default, Slots...> {
    using type = StufeOneAxis<Default>;
    // Stufe1 ist die ce-only-Referenz: schon INNERHALB der Default-Liste darf kein Paar dasselbe Segment
    // rendern, sonst waeren zwei ce-Binaries untereinander ununterscheidbar.
    static_assert(merge_lists_render_distinct_segments<type, type>(),
                  "A13-M3/C5 (Owner-Q2): zwei VERSCHIEDENE Achsen-Varianten der ce-Default-Liste rendern "
                  "dasselbe Organ-Segment <achse>=<name>@<version>. Byte-verschiedene Binaries waeren im "
                  "Stempel -- und damit im SHA512-Skip-Gate -- nicht mehr unterscheidbar. Name ODER Version "
                  "unterscheidbar machen.");
};

template <class Default, class Slot>
struct MergeImpl<MergeStrategy::Stufe2_PrueflingReplace, Default, Slot> {
    using type = StufeTwoAxis<Default, Slot>;
    // Der ERSETZUNGS-Fall ist der eigentliche Q2-Fall: die ce-only-Binary (Default) und die ersetzte
    // Binary (type) existieren NEBENEINANDER, obwohl die ce-Variante aus DIESER Binary verschwunden ist.
    // Ihre Segmente muessen sich unterscheiden -- KREUZWEISE, nicht nur je Liste fuer sich.
    static_assert(merge_lists_render_distinct_segments<Default, type>(),
                  "A13-M3/C5 (Owner-Q2): eine Pruefling-Variante rendert dasselbe Organ-Segment wie die "
                  "ce-Variante, die sie ERSETZT. Seit dem Wegfall der merge-Zeile (Owner-E2) ist der Name die "
                  "einzige Trennung der beiden byte-verschiedenen Binaries -- der Pruefling braucht einen "
                  "erweiterten hierarchischen Namen oder das 'e'-Experimentalflag (Owner-Q2).");
    static_assert(merge_lists_render_distinct_segments<type, type>(),
                  "A13-M3/C5 (Owner-Q2): zwei Varianten der ERSETZTEN Liste rendern dasselbe Organ-Segment.");
};

template <class Default, class... Slots>
struct MergeImpl<MergeStrategy::Stufe3_FullJoin, Default, Slots...> {
    using type = StufeThreeAxis<Default, Slots...>;
    // Die Union dedupliziert per mp_unique nur gleiche TYPEN. Zwei verschiedene Typen mit gleichem Namen
    // und gleicher Version ueberleben sie -- und ergaeben zwei Permutationen mit identischem Segment.
    static_assert(merge_lists_render_distinct_segments<type, type>(),
                  "A13-M3/C5 (Owner-Q2): die FullJoin-Union enthaelt zwei verschiedene Varianten, die "
                  "dasselbe Organ-Segment <achse>=<name>@<version> rendern. mp_unique dedupliziert nur "
                  "gleiche TYPEN -- die Stempel-Identitaet braucht unterschiedliche NAMEN oder Versionen.");
};
} // namespace detail

/// MergeAxis<Strategy, Default, Slots...> — disambiguated dispatch zu Stufe 1/2/3.
template <MergeStrategy S, class Default, class... Slots>
using MergeAxis = typename detail::MergeImpl<S, Default, Slots...>::type;

} // namespace comdare::cache_engine::anatomy::pruefling
