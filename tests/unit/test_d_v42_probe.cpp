// V42-Probe (2026-06-02) — empirischer Befund für den Composition-Driver (Doc 29 §3): Ist die telemetry-Achse
// (InsertCounter) direkt als Anatomie-Member haltbar (default-konstruierbar trotz protected CRTP-Base-ctor?) UND
// ObservableAxis (statistics()/snapshot_t)? Das entscheidet, ob telemetry per Member ODER per Hülle/Tuple verdrahtet
// werden muss. Build: cl /I libs/cache_engine /I src /I build/generated /I build/generated/axes/telemetry_axis -Boost.

#include <axes/telemetry_axis/axis_11_telemetry_insert_counter.hpp>
#include <axes/telemetry_axis/axis_11_telemetry_density_tracker.hpp>
#include <axes/telemetry_axis/axis_11_telemetry_latency_histogram.hpp>
#include "anatomy/observer_aggregate.hpp"

#include <iostream>
#include <string>
#include <type_traits>

namespace t = comdare::cache_engine::telemetry_axis;
namespace a = comdare::cache_engine::anatomy;

int main() {
    std::cout << "==== V42-Probe: telemetry-Achse Composition-Driver-Tauglichkeit ====\n";
    std::cout << "InsertCounter:    default_constructible="
              << std::is_default_constructible_v<t::InsertCounter> << "  ObservableAxis="
              << a::ObservableAxis<t::InsertCounter> << "\n";
    std::cout << "DensityTracker:   default_constructible="
              << std::is_default_constructible_v<t::DensityTracker> << "  ObservableAxis="
              << a::ObservableAxis<t::DensityTracker> << "\n";
    std::cout << "LatencyHistogram: default_constructible="
              << std::is_default_constructible_v<t::LatencyHistogram> << "  ObservableAxis="
              << a::ObservableAxis<t::LatencyHistogram> << "\n";
    // Direktes Halten (der im search_algorithm_anatomy.hpp:59 genannte Block — gilt er fuer telemetry-Derived?):
    t::InsertCounter ic; // kompiliert dies, ist der protected CRTP-Base-ctor KEIN Block fuer den Derived-Member
    (void)ic;
    std::cout << "InsertCounter als lokales Member/Objekt: KONSTRUIERT (kein CRTP-Block fuer Derived)\n";
    std::cout << "\n==== Befund dokumentiert (Doc 29 §3 Praezisierung) ====\n";
    return 0;
}
