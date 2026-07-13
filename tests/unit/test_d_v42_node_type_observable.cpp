// V42 L-74c — ObservableNodeType<Strategy> Driver-Baustein-Test (2026-06-03). 4. + letztes Exemplar.
// Build: scratch_compile_test.ps1 -Test test_d_v42_node_type_observable -Boost -Extra @("build\generated")

#define COMDARE_CE_ENABLE_STATISTICS 1

#include <axes/node/axis_04_node_type_observable.hpp>
#include <axes/node/axis_04_node_type_node256.hpp>
#include "anatomy/observer_aggregate.hpp" // ObservableAxis

#include <cassert>
#include <cstdint>
#include <iostream>
#include <type_traits>

namespace nd = comdare::cache_engine::node;
namespace a  = comdare::cache_engine::anatomy;

// M-CE-22 / NDEBUG-No-Op-Fix (2026-07-13, Muster-F): harte Checks statt assert(). assert() ist unter NDEBUG
// (Release) ein No-Op -> dieser Test degenerierte im Release-Build zu `return 0` (kein echter Beweis). CE_CHECK
// prueft NDEBUG-unabhaengig und liefert bei Verletzung `return 1` (rot). static_assert() bleibt (compile-time).
#define CE_CHECK(cond)                                                                                                 \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::cerr << "[FAIL] " #cond " @ " << __FILE__ << ":" << __LINE__ << "\n";                                 \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

int main() {
    using Strat = nd::Node256NodeType;
    using Obs   = nd::ObservableNodeType<Strat>;

    static_assert(std::is_aggregate_v<Strat>, "Strategie ist Aggregat");
    static_assert(!a::ObservableAxis<Strat>, "nackte Strategie hat kein statistics()");
    static_assert(a::ObservableAxis<Obs>, "Huelle muss ObservableAxis sein");
    static_assert(!std::is_aggregate_v<Obs>, "Huelle darf kein Aggregat sein");
    static_assert(nd::concepts::NodeTypeStrategy<Obs>, "Huelle muss NodeTypeStrategy erfuellen (N in ComposedStore)");
    static_assert(Obs::max_capacity() == 256u, "max_capacity durchgereicht");
    static_assert(std::is_standard_layout_v<nd::NodeTypeSnapshot> && std::is_trivially_copyable_v<nd::NodeTypeSnapshot>,
                  "Snapshot ABI-tauglich");

    std::uint8_t const stored[4]  = {1u, 2u, 3u, 4u};
    std::uint8_t const queries[3] = {2u, 4u, 9u}; // 2(+2), 4(+4), 9(miss) -> Summe 6

    // static Pass-Through trackt NICHT:
    CE_CHECK(Obs::node_find_scan(stored, 4, queries, 3) == 6u);

    Obs node;
    CE_CHECK(node.statistics().find_count == 0);
    std::uint64_t const checksum = node.observe_node_find(stored, 4, queries, 3); // Instanz-Driver: trackt
    auto const          after    = node.statistics();
    std::cout << "find_count=" << after.find_count << " keys_stored=" << after.keys_stored
              << " queries_run=" << after.queries_run << " checksum=" << after.last_checksum << "\n";

    CE_CHECK(checksum == 6u);
    CE_CHECK(after.find_count == 1u);
    CE_CHECK(after.keys_stored == 4u);
    CE_CHECK(after.queries_run == 3u);
    CE_CHECK(after.last_checksum == 6u);
    CE_CHECK(after.find_count != 0u); // Delta > 0

    node.reset();
    CE_CHECK(node.statistics().find_count == 0u);

    std::cout << "OK: node_type-Achse ist echte getriebene ObservableAxis (node_find_scan durchgereicht + getrackt).\n";
    return 0;
}
