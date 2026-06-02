// D8 / L-74c — R5.B-Operabilitäts-Klassifikation: ehrliche, literal belegte Einteilung der 17 Komposition-Achsen
// in real-getrieben / operativ-fähig / Deskriptor. Kein Fake (Memory feedback_no_success_marks_without_literal_output):
// die 2 Operative (search_algo+allocator) sind durch BR-4 (br4_load: observe_all über DLL search_insert==256) real
// belegt; die 11 Deskriptoren werden EHRLICH als passiv markiert (kein Pseudo-Observer). Build: cl /I libs/cache_engine.

#include "builder/experiment_tree/axis_operability_classification.hpp"

#include <cstddef>
#include <iostream>
#include <string>

namespace ex = comdare::cache_engine::builder::experiment;

static_assert(ex::kAxisOperability.size() == 17, "17 SearchAlgorithm-Komposition-Achsen");
static_assert(ex::count_operability(ex::AxisOperability::Operative) == 2, "2 real operativ (search_algo+allocator)");
static_assert(ex::count_operability(ex::AxisOperability::OperativeCapable) == 4, "4 operativ-faehig");
static_assert(ex::count_operability(ex::AxisOperability::Descriptor) == 11, "11 Deskriptor (ehrlich passiv)");
static_assert(ex::operative_axis_count() == 2);

static int g_fail = 0;
template <class A, class B> static void eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e); std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) { std::cout << " (erwartet " << e << ")"; ++g_fail; } std::cout << "\n"; }
static void tr(char const* w, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n"; if (!c) ++g_fail; }

int main() {
    std::cout << "==== D8 R5.B-Operabilitaets-Klassifikation (ehrlich, 17 Achsen) ====\n";
    eq("kAxisOperability.size() == 17", ex::kAxisOperability.size(), std::size_t{17});
    eq("Operative (real getrieben) == 2", ex::count_operability(ex::AxisOperability::Operative), std::size_t{2});
    eq("OperativeCapable (verdrahtbar) == 4", ex::count_operability(ex::AxisOperability::OperativeCapable), std::size_t{4});
    eq("Descriptor (ehrlich passiv) == 11", ex::count_operability(ex::AxisOperability::Descriptor), std::size_t{11});
    eq("Summe == 17 (kein Achsen-Wegschrumpfen)",
       ex::count_operability(ex::AxisOperability::Operative) + ex::count_operability(ex::AxisOperability::OperativeCapable)
       + ex::count_operability(ex::AxisOperability::Descriptor), std::size_t{17});

    // Die 2 Operative SIND search_algo (T0) + allocator (T6) — die R5.B-Grenze (node_value_measurement.hpp), via BR-4 real belegt.
    tr("T0 == search_algo, Operative", std::string{ex::kAxisOperability[0].axis} == "search_algo"
       && ex::kAxisOperability[0].operability == ex::AxisOperability::Operative);
    tr("T6 == allocator, Operative", std::string{ex::kAxisOperability[6].axis} == "allocator"
       && ex::kAxisOperability[6].operability == ex::AxisOperability::Operative);
    // memory_layout ehrlich PMC-pending (nicht als voll-operativ ueberbehauptet).
    tr("T5 == memory_layout, OperativeCapable (PMC-pending, nicht Operative)",
       std::string{ex::kAxisOperability[5].axis} == "memory_layout"
       && ex::kAxisOperability[5].operability == ex::AxisOperability::OperativeCapable);
    // filter ehrlich Deskriptor (kein Pseudo-Observer).
    tr("T16 == filter, Descriptor", std::string{ex::kAxisOperability[16].axis} == "filter"
       && ex::kAxisOperability[16].operability == ex::AxisOperability::Descriptor);

    std::cout << "\n==== D8 Operabilitaet: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
