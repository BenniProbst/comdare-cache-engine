// test_e24_c6v_growth_statistics -- E-24 C6-V (ABI-neutraler Vor-Baustein des b-Teils, Entscheid E5).
//
// GEGENSTAND: GrowthStatistics + ObservableGrowth<Policy> (anatomy/growth_policy_observable.hpp) --
// die Observable-Huelle der SEQUENCE-eigenen Achse growth_policy (Katalog-Zeile C-A,
// docs/sessions/20260804-DOSSIER-achsen-qualitaets-parameter-katalog.md Sektion 2 Punkt 1/2).
//
//   (A) CT-PINS            -- Feldzahl (7 x uint64, ueber sizeof gepinnt), Feld-TYPEN einzeln, POD-Eigenschaften,
//                             E1-Konvention (keine double-Groesse im POD; die _milli-Form haengt an der Huelle).
//   (B) LAUFZEIT-TREIBEN   -- die Huelle steckt im ECHTEN Sequence-Slot; push_back() treibt next_capacity();
//                             jeder Zaehler wird gegen die von Hand nachgerechnete Ereignis-Folge geprueft.
//   (C) ALGORITHMUS-VERGLEICH -- DoublingGrowth vs. ExactGrowth ueber DIESELBE Op-Folge: Exact ist die
//                             'ohne Ueberallokation'-Auspraegung (Katalog 1N.2 Zeile C-A) und zeigt die
//                             O(n^2)-Amortisation literal (elements_copied == n*(n-1)/2, peak_slack == 0).
//   (D) 'OHNE' vs. 'LEER'  -- Katalog 1N.1, strikt getrennt: eine nicht getriebene Huelle liefert EHRLICHE
//                             NULLEN MIT Identitaet (observable_count zaehlt sie); ein CarriedAxis-Slot
//                             liefert EmptyAxisSnapshot und wird NICHT gezaehlt.
//   (E) C3-EINSAMMLUNG     -- observe_axes() nimmt die Huelle automatisch auf (snapshot_of_t), ohne dass die
//                             Anatomie angefasst werden musste; observable_axis_count steigt 0 -> 1.
//
// Bau: plain int main() (kein gtest), COMDARE_CE_ENABLE_STATISTICS=1 (die Huelle ist gegated).

#include "anatomy/growth_policy_observable.hpp"

#include "anatomy/cross_genus_organ.hpp" // CarriedAxis (die produktive 'getragen'-Belegung, 1N.1)
#include "anatomy/sequence_anatomy.hpp"  // SequenceAnatomy / SequenceComposition (der ECHTE Slot)
#include "topics/sequence/axis_growth/axis_growth_policies.hpp" // ExactGrowth / GoldenRatioGrowth / FixedChunkGrowth

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen gleichnamiger Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;
namespace cag = comdare::cache_engine::sequence::axis_growth;

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

using DoublingHull = cea::ObservableGrowth<cea::DoublingGrowth>;
using ExactHull    = cea::ObservableGrowth<cag::ExactGrowth>;

/// Sequence-Gattung mit der Huelle im growth_policy-Slot; die acht geteilten Slots sind GETRAGEN (CarriedAxis).
template <class Growth>
using GrowthComp =
    cea::SequenceComposition<cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis,
                             cea::CarriedAxis, cea::CarriedAxis, cea::CarriedAxis, Growth>;
template <class Growth>
using GrowthOrgan = cea::SequenceAnatomy<GrowthComp<Growth>>;

// =============================================================================================
// (A) CT-PINS -- der Mindest-Feldsatz ist ein FREEZE-Gegenstand: 7 Felder, alle uint64.
// =============================================================================================
static_assert(sizeof(cea::GrowthStatistics) == 7 * sizeof(std::uint64_t),
              "C6-V: GrowthStatistics traegt GENAU die 7 Katalog-Mindest-Felder (Sektion 2 Punkt 2) als uint64 -- "
              "jede Feld-Aenderung ist ein bewusster Freeze-Eingriff.");
static_assert(std::is_standard_layout_v<cea::GrowthStatistics> && std::is_trivially_copyable_v<cea::GrowthStatistics>,
              "C6-V: die Form muss ohne Umbau in den Gattungs-Wire (C6) heben koennen.");
static_assert(std::is_same_v<decltype(cea::GrowthStatistics::growth_events), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::GrowthStatistics::elements_copied), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::GrowthStatistics::bytes_copied), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::GrowthStatistics::requested_total), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::GrowthStatistics::granted_total), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::GrowthStatistics::final_capacity), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::GrowthStatistics::peak_slack), std::uint64_t>);

// E1 (LEDGER:3828): KEINE double-Groesse im POD; die einzige double-Groesse der Achse (growth_factor) haengt
// als MILLI-Fixpunkt an der Huelle -- Suffix _milli, Rueckgabe uint64.
static_assert(
    requires(DoublingHull const& h) {
        { h.growth_factor_milli() } -> std::same_as<std::uint64_t>;
    }, "C6-V/E1: die double->uint64-Konversion der Achse traegt den _milli-Suffix und liefert uint64.");
static_assert(
    requires(DoublingHull const& h) {
        { h.growth_factor() } -> std::same_as<double>;
    }, "Observable-Wrapper muss die Concept-Member der Achse forwarden (GrowthPolicy: next_capacity + growth_factor).");
static_assert(cea::GrowthPolicy<DoublingHull> && cea::GrowthPolicy<ExactHull>,
              "Die Huelle bleibt eine GrowthPolicy -- sonst passt sie nicht in den Kompositions-Slot.");
static_assert(DoublingHull::element_bytes == sizeof(std::uint64_t),
              "bytes_copied rechnet mit der ECHTEN Element-Breite der Sequence-Gattung (sequence_anatomy.hpp:85).");

// (E) Die C3-Einsammlung nimmt die Huelle AUTOMATISCH auf -- keine Anatomie-Aenderung noetig.
static_assert(std::is_same_v<decltype(cea::SequenceAxisObservation<GrowthComp<DoublingHull>>::growth_policy),
                             cea::GrowthStatistics>,
              "C6-V: snapshot_of_t zieht GrowthStatistics in den C3-Slot der Sequence-Gattung.");
static_assert(std::is_same_v<decltype(cea::SequenceAxisObservation<GrowthComp<cea::DoublingGrowth>>::growth_policy),
                             cea::EmptyAxisSnapshot>,
              "Gegenprobe: die NACKTE Policy hat kein statistics() -- der Slot bleibt Empty (das war die Luecke).");
static_assert(GrowthOrgan<cea::DoublingGrowth>::observable_axis_count() == 0, "vor C6-V: stumm");
static_assert(GrowthOrgan<DoublingHull>::observable_axis_count() == 1, "nach C6-V: observable_count steigt");

} // namespace

int main() {
    std::cout << "E-24 C6-V: GrowthStatistics + ObservableGrowth (Achse growth_policy, Katalog C-A):\n";

    // -- (B) LAUFZEIT-TREIBEN ueber die ECHTE Gattungs-API; die Ereignis-Folge ist von Hand nachgerechnet:
    //    push1: next_capacity(0,1)=1  -> Ereignis (copied 0, slack 0)
    //    push2: next_capacity(1,2)=2  -> Ereignis (copied 1, slack 0)
    //    push3: next_capacity(2,3)=4  -> Ereignis (copied 2, slack 1)
    //    push4: 3 < 4                 -> KEIN Ereignis
    //    push5: next_capacity(4,5)=8  -> Ereignis (copied 4, slack 3)
    GrowthOrgan<DoublingHull> doubling{};
    for (std::uint64_t i = 0; i < 5; ++i) doubling.push_back(i);
    cea::GrowthStatistics const d = doubling.observe_axes().growth_policy;
    std::cout << "\nDoublingGrowth (x2) nach 5 push_back:\n";
    check_eq("growth_events", d.growth_events, std::uint64_t{4});
    check_eq("elements_copied (0+1+2+4)", d.elements_copied, std::uint64_t{7});
    check_eq("bytes_copied (7 * 8 B)", d.bytes_copied, std::uint64_t{56});
    check_eq("requested_total (1+2+3+5)", d.requested_total, std::uint64_t{11});
    check_eq("granted_total (1+2+4+8)", d.granted_total, std::uint64_t{15});
    check_eq("final_capacity", d.final_capacity, std::uint64_t{8});
    check_eq("peak_slack (8-5)", d.peak_slack, std::uint64_t{3});
    check_eq("Kreuz-Probe gegen die flache Gattungs-Sicht: growth_events", doubling.observe_all().growth_events,
             d.growth_events);
    check_eq("growth_factor_milli (E1: 2.0 -> 2000)", doubling.growth_policy_organ().growth_factor_milli(),
             std::uint64_t{2000});

    // -- (C) ALGORITHMUS-VERGLEICH ueber DIESELBE Op-Folge: 'ohne Ueberallokation' == ExactGrowth (1N.2 C-A).
    GrowthOrgan<ExactHull> exact{};
    for (std::uint64_t i = 0; i < 5; ++i) exact.push_back(i);
    cea::GrowthStatistics const e = exact.observe_axes().growth_policy;
    std::cout << "\nExactGrowth (1:1, die 'ohne Ueberallokation'-Auspraegung) nach 5 push_back:\n";
    check_eq("growth_events (jedes push waechst)", e.growth_events, std::uint64_t{5});
    check_eq("elements_copied == n*(n-1)/2 (O(n^2) literal)", e.elements_copied, std::uint64_t{10});
    check_eq("peak_slack == 0 (keine Ueberallokation)", e.peak_slack, std::uint64_t{0});
    check_eq("granted_total == requested_total", e.granted_total, e.requested_total);
    check_eq("growth_factor_milli (E1: 1.0 -> 1000)", exact.growth_policy_organ().growth_factor_milli(),
             std::uint64_t{1000});
    check_true("Amortisations-Beleg: Exact kopiert MEHR als Doubling (10 > 7)", e.elements_copied > d.elements_copied);
    check_true("Speicher-Beleg: Doubling ueberallokiert, Exact nicht", d.peak_slack > e.peak_slack);

    // Die uebrigen zwei Registry-Policies laufen ueber dieselbe Huelle (E1-Sentinel-Fall inklusive).
    cea::ObservableGrowth<cag::GoldenRatioGrowth> const    golden{};
    cea::ObservableGrowth<cag::FixedChunkGrowth<64>> const chunk{};
    check_eq("GoldenRatioGrowth: growth_factor_milli (1.5 -> 1500)", golden.growth_factor_milli(), std::uint64_t{1500});
    check_eq("FixedChunkGrowth: growth_factor_milli (additiv-Sentinel 0.0 -> 0)", chunk.growth_factor_milli(),
             std::uint64_t{0});

    // -- (D) 'OHNE' vs. 'LEER' (Katalog 1N.1) --
    std::cout << "\n'ohne'-vs-leer-Probe (1N.1: 'ohne' liefert Werte, 'leer' liefert keine):\n";
    GrowthOrgan<DoublingHull> const untouched{};
    cea::GrowthStatistics const     zero = untouched.observe_axes().growth_policy;
    check_true("nicht getriebene Huelle: EHRLICHE NULLEN (kein erfundener Wert)", zero == cea::GrowthStatistics{});
    check_eq("...aber MIT Mess-Identitaet: observable_axis_count", GrowthOrgan<DoublingHull>::observable_axis_count(),
             std::size_t{1});
    check_true("getragener Slot (CarriedAxis) liefert EmptyAxisSnapshot -- KEINE Mess-Identitaet",
               std::is_same_v<decltype(untouched.observe_axes().allocator), cea::EmptyAxisSnapshot>);
    check_eq("stumme Komposition (nackte Policy): observable_axis_count",
             GrowthOrgan<cea::DoublingGrowth>::observable_axis_count(), std::size_t{0});
    check_eq("total_slots bleibt aus der Komposition gerechnet",
             GrowthOrgan<DoublingHull>::axis_observation_t::total_slots(), std::size_t{9});

    std::cout << "\n==== E-24 C6-V GrowthStatistics: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
