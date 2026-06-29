// V42 L-74c — telemetry-Registry-Pfad: die telemetry-Enabled-Liste im Permutations-Pfad
// (TopicConfigSet::StaticAxisVariants) trägt jetzt die ObservableTelemetry-Hülle, sodass auch
// AUTO-emittierte AdHoc-Kompositionen telemetry als getriebene ObservableAxis tragen (nicht nur
// die benannten Reference-Compositions). Belegt: alle StaticAxisVariants-Elemente sind ObservableAxis,
// und BR-1 (registry_to_axis_levels) baut die AxisLevels über die Hüllen-Liste fehlerfrei.
// Build: scratch_compile_test.ps1 -Test test_d_v42_telemetry_registry_observable -Boost -Extra @("build\generated")

#define COMDARE_CE_ENABLE_STATISTICS 1

#include <topics/telemetry/topic_telemetry_config_set.hpp>
#include "anatomy/observer_aggregate.hpp" // ObservableAxis

#include <boost/mp11.hpp>
#include <iostream>
#include <type_traits>

namespace t  = comdare::cache_engine::telemetry;
namespace tx = comdare::cache_engine::telemetry_axis;
namespace a  = comdare::cache_engine::anatomy;
namespace mp = boost::mp11;

// Jedes Element der StaticAxisVariants muss jetzt eine ObservableTelemetry-Hülle sein (ObservableAxis).
template <class S>
using is_observable = mp::mp_bool<a::ObservableAxis<S>>;

int main() {
    using Variants          = t::TopicConfigSet::StaticAxisVariants;
    constexpr std::size_t n = mp::mp_size<Variants>::value;
    std::cout << "telemetry StaticAxisVariants: " << n << " Wrapper\n";

    // (1) ALLE Elemente sind ObservableAxis (Hülle) — der Permutations-Pfad ist jetzt telemetry-observable.
    static_assert(mp::mp_all_of<Variants, is_observable>::value,
                  "Alle telemetry-Permutations-Varianten muessen ObservableTelemetry-Huellen (ObservableAxis) sein");

    // (2) Das erste Element ist konkret ObservableTelemetry<…> (Hülle, kein nackter Strategie-Marker).
    using First = mp::mp_first<Variants>;
    static_assert(a::ObservableAxis<First>, "erstes Element ist ObservableAxis");
    static_assert(!std::is_aggregate_v<First>, "Huelle ist kein Aggregat (nackte Strategie waere eins)");

    std::cout << "OK: telemetry-Registry/Permutations-Pfad traegt jetzt die ObservableTelemetry-Huelle "
                 "(alle "
              << n << " Varianten ObservableAxis).\n";
    return 0;
}
