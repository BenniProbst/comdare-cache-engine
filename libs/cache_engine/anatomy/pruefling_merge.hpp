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

#include <boost/mp11.hpp>

#include <concepts>
#include <type_traits>

namespace comdare::cache_engine::anatomy::pruefling {

namespace mp = boost::mp11;

// ─────────────────────────────────────────────────────────────────────────────
// (1) PrueflingSlot — Default-leerer Slot pro Achse
// ─────────────────────────────────────────────────────────────────────────────

/// EmptyPrueflingSlot — Fallback wenn kein Pruefling fuer die Achse existiert.
struct EmptyPrueflingSlot {
    using PrueflingVariants = mp::mp_list<>;
    static constexpr bool has_pruefling = false;
};

/// PrueflingSlotConcept — ein Slot ist gueltig wenn er PrueflingVariants
/// (mp_list von Wrappers) + has_pruefling (bool) liefert.
template <class Slot>
concept PrueflingSlotConcept = requires {
    typename Slot::PrueflingVariants;
    { Slot::has_pruefling } -> std::convertible_to<bool>;
};

/// HasPruefling<Slot> — Compile-Time-Predicate (true wenn Pruefling-Slot belegt).
template <class Slot>
constexpr bool HasPruefling_v = PrueflingSlotConcept<Slot> && Slot::has_pruefling;

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
using StufeTwoAxis = std::conditional_t<
    HasPruefling_v<Slot>,
    typename Slot::PrueflingVariants,
    DefaultList
>;

// ─────────────────────────────────────────────────────────────────────────────
// (4) Stufe 3 — comdare_perms_full_join (Union non-redundant)
// ─────────────────────────────────────────────────────────────────────────────

/// StufeThreeAxis — Union aller Default + Pruefling-Varianten, dedupliziert.
/// MP11-Idiom: mp_unique<mp_append<...>>.
///
/// Identisch zu permutations::AxisFullJoin aber mit Slot-Detection statt
/// raw PrueflingLists.
template <class DefaultList, class... Slots>
using StufeThreeAxis = mp::mp_unique<
    mp::mp_append<DefaultList, typename Slots::PrueflingVariants...>
>;

// ─────────────────────────────────────────────────────────────────────────────
// (5) MergeStrategy enum + dispatched Helper
// ─────────────────────────────────────────────────────────────────────────────

enum class MergeStrategy {
    Stufe1_CeOnly,           // KEINE Pruefling
    Stufe2_PrueflingReplace, // ERSETZT-mit-Fallback
    Stufe3_FullJoin          // Union non-redundant
};

namespace detail {
    template <MergeStrategy S, class Default, class... Slots>
    struct MergeImpl;

    template <class Default, class... Slots>
    struct MergeImpl<MergeStrategy::Stufe1_CeOnly, Default, Slots...> {
        using type = StufeOneAxis<Default>;
    };

    template <class Default, class Slot>
    struct MergeImpl<MergeStrategy::Stufe2_PrueflingReplace, Default, Slot> {
        using type = StufeTwoAxis<Default, Slot>;
    };

    template <class Default, class... Slots>
    struct MergeImpl<MergeStrategy::Stufe3_FullJoin, Default, Slots...> {
        using type = StufeThreeAxis<Default, Slots...>;
    };
}  // namespace detail

/// MergeAxis<Strategy, Default, Slots...> — disambiguated dispatch zu Stufe 1/2/3.
template <MergeStrategy S, class Default, class... Slots>
using MergeAxis = typename detail::MergeImpl<S, Default, Slots...>::type;

}  // namespace comdare::cache_engine::anatomy::pruefling
