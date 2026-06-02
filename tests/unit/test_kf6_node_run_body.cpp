// test_kf6_node_run_body — KF-6 (2026-06-02)
// Belegt: die node-Achse ist runtime-operativ — node_find_scan DIVERGIERT je ART-Node-Format
// (Node4/16 Linear-Scan mit Kapazitätsgrenze · Node48 256-Byte-Index-Indirektion · Node256 Direkt-Index).
// Identischer Input → 4 DISTINKTE Prüfsummen = Beleg der per-Format-Divergenz. + cacheline_prefetch-Hook (KF-5).
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> /I<include> /I<src> /I<build/generated> /I<boost_mp11> ...

#include <axes/node/axis_04_node_type_node4.hpp>
#include <axes/node/axis_04_node_type_node16.hpp>
#include <axes/node/axis_04_node_type_node48.hpp>
#include <axes/node/axis_04_node_type_node256.hpp>
#include <axes/cacheline/cacheline_config.hpp>

#include <cstdint>
#include <iostream>
#include <set>
#include <string>

namespace nd = comdare::cache_engine::node;
namespace cl = comdare::cache_engine::cacheline;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) { std::cout << "  (erwartet: " << want << ")"; ++g_fail; }
    std::cout << "\n";
}
void check_true(char const* what, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n"; if (!c) ++g_fail; }

int main() {
    // Identischer Input für ALLE 4 Formate → die Divergenz steckt allein im Format-Zugriffsmuster.
    std::uint8_t const stored[]  = {10, 20, 30, 40, 50, 60};   // sortierte Schlüssel (n=6)
    std::uint8_t const queries[] = {30, 55};                    // ein Treffer (30) + ein Fehlschlag (55)
    constexpr std::size_t n = 6, q = 2;

    std::uint64_t const r4   = nd::Node4NodeType::node_find_scan(stored, n, queries, q);
    std::uint64_t const r16  = nd::Node16NodeType::node_find_scan(stored, n, queries, q);
    std::uint64_t const r48  = nd::Node48NodeType::node_find_scan(stored, n, queries, q);
    std::uint64_t const r256 = nd::Node256NodeType::node_find_scan(stored, n, queries, q);

    std::cout << "KF-6: node_find_scan je ART-Node-Format (identischer Input):\n";

    // Exakte erwartete Prüfsummen (per Hand verifizierte ART-Zugriffsmuster):
    check_eq("Node4   (Linear-Scan, cap 4 → scannt {10,20,30,40})", r4,   std::uint64_t{160});
    check_eq("Node16  (Linear-Scan, cap 16 → scannt alle 6)",       r16,  std::uint64_t{270});
    check_eq("Node48  (Index-Touch + Child-Touch je Treffer)",      r48,  std::uint64_t{115});
    check_eq("Node256 (Direkt-Touch, nur Treffer)",                 r256, std::uint64_t{30});

    // KERN-BELEG: alle 4 Prüfsummen DISTINKT → echte per-Format-Divergenz (kein hohler Stub).
    std::set<std::uint64_t> distinct{r4, r16, r48, r256};
    check_eq("4 Formate → 4 DISTINKTE Prüfsummen (Divergenz-Beleg)", distinct.size(), std::size_t{4});

    // Kapazitäts-Divergenz Node4 vs Node16 (gleicher Scan-Code, nur cap unterschiedlich → ungleich bei n>4).
    check_true("Node4 != Node16 (Kapazitätsgrenze 4 vs 16 wirkt)", r4 != r16);
    // Zugriffsmuster-Divergenz Node48 (2 Touches) vs Node256 (1 Touch).
    check_true("Node48 != Node256 (Indirektion vs Direkt)", r48 != r256);

    // cacheline_prefetch-Hook (KF-5) ist je Format aufrufbar (Default None → no-op, kein Crash).
    nd::Node4NodeType::cacheline_prefetch(stored);
    check_true("cacheline_prefetch-Hook je Node-Format aufrufbar", cl::CacheLineConfigurable<nd::Node256NodeType>);

    std::cout << "\n==== KF-6 node-Achse Run-Body-Divergenz: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
