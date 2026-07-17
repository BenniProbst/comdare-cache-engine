// CMD-2 / #252 — Regressions-Gate der host-seitigen Container-in-SA-Attribution (E2-Sidecar, Variante a).
// Reine Host-Arithmetik auf einem synthetischen ComdareTierObserverSnapshot -> gate-frei (kein #156, keine DLL,
// keine Cluster-Daten). Fixiert: c1-Formel (lookup+insert+erase), den Doppelzaehl-Ausschluss von hit/miss/peak,
// die "0 bleibt 0"-Semantik und das c3-als-Label (thesis-kanonische Store-Achsen T4/T5/T6, NICHT T11/T13).

#include <builder/experiment_tree/container_attribution.hpp>

#include <cstdint>
#include <cstdio>
#include <string_view>

namespace ex = comdare::cache_engine::builder::experiment;
namespace an = comdare::cache_engine::anatomy;

static int  g_fail = 0;
static void check(bool ok, char const* msg) {
    std::printf("  [%s] %s\n", ok ? "OK" : "FAIL", msg);
    if (!ok) ++g_fail;
}
static void check_eq_u64(std::uint64_t a, std::uint64_t b, char const* msg) {
    check(a == b, msg);
    if (a != b)
        std::printf("        erwartet %llu, war %llu\n", static_cast<unsigned long long>(b),
                    static_cast<unsigned long long>(a));
}

int main() {
    // T1 Basis: c1 = lookup([0][0]) + insert([0][3]) + erase([0][4]).
    {
        an::ComdareTierObserverSnapshot s{};
        s.axis_stats[0][0] = 100; // lookup
        s.axis_stats[0][3] = 30;  // insert
        s.axis_stats[0][4] = 10;  // erase
        check_eq_u64(ex::container_attribution(s).store_ops, 140u, "T1 store_ops == lookup+insert+erase (140)");
    }
    // T2 Doppelzaehl-Schutz: hit/miss (Teil von lookup) und peak (Gauge) duerfen store_ops NICHT veraendern.
    // Regressions-Sentinel: wuerde jemand die Indexmenge auf {0,1,2} aendern, kaeme 340 -> der Test fixiert 140.
    {
        an::ComdareTierObserverSnapshot s{};
        s.axis_stats[0][0] = 100;
        s.axis_stats[0][3] = 30;
        s.axis_stats[0][4] = 10;
        s.axis_stats[0][1] = 60;   // hit  (in lookup enthalten)
        s.axis_stats[0][2] = 40;   // miss (in lookup enthalten)
        s.axis_stats[0][5] = 9999; // peak (Gauge, kein Op-Zaehler)
        check_eq_u64(ex::container_attribution(s).store_ops, 140u,
                     "T2 hit/miss/peak aendern store_ops NICHT (kein Doppelzaehlen)");
    }
    // T3 all-zero: leerer POD -> echtes 0 (kein Crash, kein UB).
    {
        an::ComdareTierObserverSnapshot s{};
        check_eq_u64(ex::container_attribution(s).store_ops, 0u, "T3 leerer POD -> store_ops == 0");
    }
    // T4 Store-Achsen anderer Achsen (T4/T5/T6/T10/T12) beeinflussen c1 NICHT (c1 zieht ausschliesslich T0).
    {
        an::ComdareTierObserverSnapshot s{};
        s.axis_stats[0][0]  = 7; // lookup
        s.axis_stats[4][0]  = 999;
        s.axis_stats[5][0]  = 999;
        s.axis_stats[6][1]  = 999;
        s.axis_stats[10][0] = 999;
        s.axis_stats[12][0] = 999; // Store-/Value-/Index-Achsen: irrelevant fuer c1
        check_eq_u64(ex::container_attribution(s).store_ops, 7u, "T4 nur T0 speist c1 (Store-Achsen irrelevant)");
    }
    // T5 c3-als-Label: die thesis-kanonischen Store-Achsen sind genau T4/T5/T6 (KEINE Summe, kein T10/T12).
    {
        check_eq_u64(ex::kStoreAxes.size(), 3u, "T5 kStoreAxes == 3 (thesis-kanonisch T4/T5/T6)");
        check(ex::kStoreAxes[0] == std::string_view{"node_type"}, "T5 kStoreAxes[0] == node_type");
        check(ex::kStoreAxes[1] == std::string_view{"memory_layout"}, "T5 kStoreAxes[1] == memory_layout");
        check(ex::kStoreAxes[2] == std::string_view{"allocator"}, "T5 kStoreAxes[2] == allocator");
    }

    std::printf(g_fail == 0 ? "CMD-2 CONTAINER-ATTRIBUTION: ALLE OK\n" : "CMD-2 CONTAINER-ATTRIBUTION: %d FAIL\n",
                g_fail);
    return g_fail == 0 ? 0 : 1;
}
