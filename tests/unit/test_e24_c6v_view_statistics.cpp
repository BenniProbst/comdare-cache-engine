// test_e24_c6v_view_statistics -- E-24 C6-V (ABI-neutraler Vor-Baustein des b-Teils, Entscheid E5).
//
// GEGENSTAND: ExtentStatistics/LayoutStatistics/AccessorStatistics + ObservableExtent/ObservableLayout/
// ObservableAccessor (anatomy/view_policies_observable.hpp) -- die Observable-Huellen der DREI VIEW-eigenen
// Achsen (Katalog-Zeilen C-B/C-C/C-D, docs/sessions/20260804-DOSSIER-achsen-qualitaets-parameter-katalog.md
// Sektion 2 Punkt 1/2).
//
//   (A) CT-PINS            -- je POD 3 uint64-Felder (ueber sizeof gepinnt), Feld-Typen einzeln,
//                             POD-Eigenschaften, Concept-Erhalt (die Huelle bleibt Extent-/Layout-/
//                             AccessorPolicy -- sonst passt sie nicht in den Kompositions-Slot).
//   (B) LAUFZEIT-TREIBEN   -- die drei Huellen stecken in der ECHTEN ViewAnatomy; bind()/read() treiben sie.
//   (C) LAYOUT-ECHTHEIT    -- LayoutRight vs. LayoutStrided<2> ueber DIESELBE Index-Folge: die
//                             Lokalitaets-Zaehler kommen aus den REAL gelieferten Offsets, nicht aus dem Typ.
//   (D) ELISIONS-BEWEIS    -- StaticExtent<256> vs. DynamicExtent ueber DIESELBE Lese-Folge:
//                             bounds_checks_performed == 0 bei statischer Ausdehnung (Katalog C-B).
//   (E) 'OHNE' vs. 'LEER'  -- Katalog 1N.1: nicht getriebene Huellen liefern ehrliche Nullen MIT Identitaet,
//                             CarriedAxis-Slots liefern EmptyAxisSnapshot ohne Identitaet.
//
// Bau: plain int main() (kein gtest), COMDARE_CE_ENABLE_STATISTICS=1 (die Huellen sind gegated).

#include "anatomy/view_policies_observable.hpp"

#include "anatomy/cross_genus_organ.hpp" // CarriedAxis (die produktive 'getragen'-Belegung, 1N.1)
#include "anatomy/view_anatomy.hpp"      // ViewAnatomy / ViewComposition (die ECHTEN Slots)
#include "topics/view/view_policies.hpp" // StaticExtent / LayoutStrided / AlignedAccessor

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen gleichnamiger Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;
namespace vw  = comdare::cache_engine::view;

static int g_fail = 0;

template <class A, class B>
static void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

static void check_true(char const* what, bool cond) {
    std::cout << (cond ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!cond) ++g_fail;
}

using DynExtentHull  = cea::ObservableExtent<cea::DynamicExtent>;
using StatExtentHull = cea::ObservableExtent<vw::StaticExtent<256>>;
using RightHull      = cea::ObservableLayout<cea::LayoutRight>;
using StridedHull    = cea::ObservableLayout<vw::LayoutStrided<2>>;
using DefaultAccHull = cea::ObservableAccessor<cea::DefaultAccessor>;
using AlignedAccHull = cea::ObservableAccessor<vw::AlignedAccessor<64>>;

/// View-Gattung mit beobachtenden Achsen-Slots; memory_layout/value_handle sind GETRAGEN (CarriedAxis).
template <class Extent, class Layout, class Accessor>
using ViewComp = cea::ViewComposition<cea::CarriedAxis, cea::CarriedAxis, Extent, Layout, Accessor>;
template <class Extent, class Layout, class Accessor>
using ViewOrgan = cea::ViewAnatomy<ViewComp<Extent, Layout, Accessor>>;

using ObservedView = ViewOrgan<DynExtentHull, RightHull, DefaultAccHull>;
using StridedView  = ViewOrgan<DynExtentHull, StridedHull, DefaultAccHull>;
using StaticView   = ViewOrgan<StatExtentHull, RightHull, DefaultAccHull>;
using StillView    = ViewOrgan<cea::DynamicExtent, cea::LayoutRight, cea::DefaultAccessor>;

// =============================================================================================
// (A) CT-PINS -- die drei Mindest-Feldsaetze sind FREEZE-Gegenstand: je 3 Felder, alle uint64.
// =============================================================================================
static_assert(sizeof(cea::ExtentStatistics) == 3 * sizeof(std::uint64_t),
              "C6-V: ExtentStatistics traegt GENAU die 3 Katalog-Mindest-Felder (C-B) als uint64.");
static_assert(sizeof(cea::LayoutStatistics) == 3 * sizeof(std::uint64_t),
              "C6-V: LayoutStatistics traegt GENAU die 3 Katalog-Mindest-Felder (C-C) als uint64.");
static_assert(sizeof(cea::AccessorStatistics) == 3 * sizeof(std::uint64_t),
              "C6-V: AccessorStatistics traegt GENAU die 3 Katalog-Mindest-Felder (C-D) als uint64.");
static_assert(std::is_standard_layout_v<cea::ExtentStatistics> && std::is_trivially_copyable_v<cea::ExtentStatistics>);
static_assert(std::is_standard_layout_v<cea::LayoutStatistics> && std::is_trivially_copyable_v<cea::LayoutStatistics>);
static_assert(std::is_standard_layout_v<cea::AccessorStatistics> &&
              std::is_trivially_copyable_v<cea::AccessorStatistics>);
static_assert(std::is_same_v<decltype(cea::ExtentStatistics::bounds_checks_performed), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::ExtentStatistics::oob_rejects), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::ExtentStatistics::rebind_count), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::LayoutStatistics::index_translations), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::LayoutStatistics::non_contiguous_steps), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::LayoutStatistics::max_offset_jump), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::AccessorStatistics::access_count), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::AccessorStatistics::unaligned_accesses), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::AccessorStatistics::conversion_ops), std::uint64_t>);

// Concept-Erhalt: eine Huelle, die die Concept-Member nicht forwarded, faellt aus dem Kompositions-Slot.
static_assert(cea::ExtentPolicy<DynExtentHull> && cea::ExtentPolicy<StatExtentHull>);
static_assert(cea::LayoutPolicy<RightHull> && cea::LayoutPolicy<StridedHull>);
static_assert(cea::AccessorPolicy<DefaultAccHull> && cea::AccessorPolicy<AlignedAccHull>);

// Die CT-Marke der Ausdehnungs-Klasse traegt den Elisions-Zweig (is_static() ist Laufzeit und taugt nicht).
static_assert(!DynExtentHull::is_static_extent && StatExtentHull::is_static_extent);
static_assert(AlignedAccHull::alignment_contract == 64 && DefaultAccHull::alignment_contract == alignof(std::uint64_t));

// Die C3-Einsammlung nimmt alle drei Huellen AUTOMATISCH auf (snapshot_of_t).
static_assert(std::is_same_v<decltype(ObservedView::axis_observation_t::extent_policy), cea::ExtentStatistics>);
static_assert(std::is_same_v<decltype(ObservedView::axis_observation_t::layout_policy), cea::LayoutStatistics>);
static_assert(std::is_same_v<decltype(ObservedView::axis_observation_t::accessor_policy), cea::AccessorStatistics>);
static_assert(std::is_same_v<decltype(StillView::axis_observation_t::layout_policy), cea::EmptyAxisSnapshot>,
              "Gegenprobe: die NACKTE Policy hat kein statistics() -- der Slot bleibt Empty (das war die Luecke).");
static_assert(StillView::observable_axis_count() == 0, "vor C6-V: alle drei View-eigenen Achsen stumm");
static_assert(ObservedView::observable_axis_count() == 3, "nach C6-V: observable_count steigt 0 -> 3");

} // namespace

int main() {
    std::cout << "E-24 C6-V: Extent/Layout/AccessorStatistics (die drei View-eigenen Achsen, Katalog C-B/C-C/C-D):\n";

    alignas(64) std::uint64_t const buffer[4] = {70, 71, 72, 73};

    // -- (B) LAUFZEIT-TREIBEN ueber die ECHTE Gattungs-API: bind + read(0),read(1),read(2),read(99) --
    ObservedView view{};
    view.bind(buffer, 4);
    (void)view.read(0);
    (void)view.read(1);
    (void)view.read(2);
    (void)view.read(99); // out of bounds -> die Achse weist real ab
    ObservedView::axis_observation_t const a = view.observe_axes();

    std::cout << "\nextent_policy (DynamicExtent-Huelle) nach 4 read + 1 bind:\n";
    check_eq("bounds_checks_performed (4 reale Pruefungen)", a.extent_policy.bounds_checks_performed, std::uint64_t{4});
    check_eq("oob_rejects (read(99))", a.extent_policy.oob_rejects, std::uint64_t{1});
    check_eq("rebind_count (erste Bindung ist kein RE-bind)", a.extent_policy.rebind_count, std::uint64_t{0});
    view.bind(buffer, 4); // zweite Bindung == echtes Re-Binding
    check_eq("rebind_count nach zweitem bind", view.observe_axes().extent_policy.rebind_count, std::uint64_t{1});

    std::cout << "\nlayout_policy (LayoutRight-Huelle), Offsets 0,1,2,99:\n";
    check_eq("index_translations", a.layout_policy.index_translations, std::uint64_t{4});
    check_eq("non_contiguous_steps (nur der Sprung 2 -> 99)", a.layout_policy.non_contiguous_steps, std::uint64_t{1});
    check_eq("max_offset_jump (99 - 2)", a.layout_policy.max_offset_jump, std::uint64_t{97});

    std::cout << "\naccessor_policy (DefaultAccessor-Huelle):\n";
    check_eq("access_count (nur die 3 gueltigen Lesungen erreichen den Accessor)", a.accessor_policy.access_count,
             std::uint64_t{3});
    check_eq("unaligned_accesses (64-B-ausgerichteter Puffer, Vertrag alignof(uint64))",
             a.accessor_policy.unaligned_accesses, std::uint64_t{0});
    check_eq("conversion_ops (DefaultAccessor konvertiert nicht -- ehrliche 0 als MESSERGEBNIS)",
             a.accessor_policy.conversion_ops, std::uint64_t{0});
    check_eq("Kreuz-Probe gegen die flache Gattungs-Sicht: read_count", view.observe_all().read_count,
             std::uint64_t{4});

    // -- (C) LAYOUT-ECHTHEIT: dieselbe Index-Folge, anderes Layout -> andere REALE Offsets --
    std::cout << "\nLayoutStrided<2> ueber DIESELBE Index-Folge 0,1,2 (Zaehler aus REALEN Offsets):\n";
    StridedView strided{};
    strided.bind(buffer, 4);
    (void)strided.read(0);
    (void)strided.read(1);
    (void)strided.read(2);
    cea::LayoutStatistics const s = strided.observe_axes().layout_policy;
    check_eq("index_translations", s.index_translations, std::uint64_t{3});
    check_eq("non_contiguous_steps (Offsets 0,2,4 -- jeder Schritt springt)", s.non_contiguous_steps, std::uint64_t{2});
    check_eq("max_offset_jump (Stride 2)", s.max_offset_jump, std::uint64_t{2});

    ObservedView right{};
    right.bind(buffer, 4);
    (void)right.read(0);
    (void)right.read(1);
    (void)right.read(2);
    check_eq("Gegenprobe LayoutRight, gleiche Indizes: non_contiguous_steps",
             right.observe_axes().layout_policy.non_contiguous_steps, std::uint64_t{0});

    // -- (D) ELISIONS-BEWEIS: statische Ausdehnung fuehrt KEINEN dynamischen Grenz-Check --
    std::cout << "\nStaticExtent<256> vs. DynamicExtent ueber DIESELBE Lese-Folge (Katalog C-B):\n";
    StaticView stat{};
    stat.bind(buffer, 4);
    (void)stat.read(0);
    (void)stat.read(1);
    (void)stat.read(2);
    (void)stat.read(99);
    cea::ExtentStatistics const st = stat.observe_axes().extent_policy;
    check_eq("bounds_checks_performed == 0 (ELISIONS-BEWEIS, ehrliche Null)", st.bounds_checks_performed,
             std::uint64_t{0});
    check_eq("oob_rejects == 0 (die Achse hat nichts geprueft, also nichts abgewiesen)", st.oob_rejects,
             std::uint64_t{0});
    check_true("dynamische Ausdehnung prueft dagegen real (4 > 0)",
               a.extent_policy.bounds_checks_performed > st.bounds_checks_performed);
    check_true("der Speicher-Guard der Anatomie bleibt unberuehrt: read(99) liefert weiterhin kein Element",
               !stat.read(99).has_value());

    // -- Ausrichtungs-Vertrag: REALE Adresse gegen den REALEN Vertrag (AlignedAccessor<64>) --
    std::cout << "\nAlignedAccessor<64>: die Pruefung laeuft auf der REALEN Adresse, nicht auf dem Typ-Namen:\n";
    AlignedAccHull const aligned{};
    (void)aligned.access(buffer, 0); // Puffer ist alignas(64) -> vertragskonform
    (void)aligned.access(buffer, 1); // +8 B -> verletzt den 64-B-Vertrag
    (void)aligned.access(buffer, 3); // +24 B -> verletzt den 64-B-Vertrag
    cea::AccessorStatistics const al = aligned.statistics();
    check_eq("access_count", al.access_count, std::uint64_t{3});
    check_eq("unaligned_accesses (Index 1 und 3)", al.unaligned_accesses, std::uint64_t{2});

    // -- (E) 'OHNE' vs. 'LEER' (Katalog 1N.1) --
    std::cout << "\n'ohne'-vs-leer-Probe (1N.1: 'ohne' liefert Werte, 'leer' liefert keine):\n";
    ObservedView const                     untouched{};
    ObservedView::axis_observation_t const u = untouched.observe_axes();
    check_true("nicht getriebene Extent-Huelle: EHRLICHE NULLEN", u.extent_policy == cea::ExtentStatistics{});
    check_true("nicht getriebene Layout-Huelle: EHRLICHE NULLEN", u.layout_policy == cea::LayoutStatistics{});
    check_true("nicht getriebene Accessor-Huelle: EHRLICHE NULLEN", u.accessor_policy == cea::AccessorStatistics{});
    check_eq("...aber MIT Mess-Identitaet: observable_axis_count", ObservedView::observable_axis_count(),
             std::size_t{3});
    check_true("getragene Slots (CarriedAxis) liefern EmptyAxisSnapshot -- KEINE Mess-Identitaet",
               std::is_same_v<decltype(u.memory_layout), cea::EmptyAxisSnapshot> &&
                   std::is_same_v<decltype(u.value_handle), cea::EmptyAxisSnapshot>);
    check_eq("stumme Komposition (nackte Policies): observable_axis_count", StillView::observable_axis_count(),
             std::size_t{0});
    check_eq("total_slots bleibt aus der Komposition gerechnet", ObservedView::axis_observation_t::total_slots(),
             std::size_t{5});

    std::cout << "\n==== E-24 C6-V View-Statistics: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
