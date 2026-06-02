// D14 / L-CLUSTER (gate-frei) — result_ingest: Cluster-/perm_runner-Mess-Zeilen → Baum-NodeValue (sparse).
// Der host-seitige Ergebnis-Rückführungs-Pfad (Doc 28 §5), LOKAL verifizierbar (kein Cluster-Gate). Build: cl /I libs/cache_engine.

#include "builder/experiment_tree/result_ingest.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

namespace ex = comdare::cache_engine::builder::experiment;

static int g_fail = 0;
template <class A, class B> static void eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e); std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) { std::cout << " (erwartet " << e << ")"; ++g_fail; } std::cout << "\n"; }
static void tr(char const* w, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n"; if (!c) ++g_fail; }

int main() {
    std::cout << "==== D14 result_ingest (Cluster-Ergebnis → Baum-NodeValue) ====\n";
    auto factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};

    // 2 Ergebnis-Zeilen (binary_id + 13 Observer-Felder) + 1 Kommentar + 1 Leerzeile.
    std::string const text =
        "# perm-results batch (Cluster/perm_runner)\n"
        "search_algo=Array256/allocator=mimalloc;100;80;20;256;0;512;4096;2048;256;10;0;2;256\n"
        "\n"
        "search_algo=Patricia/allocator=jemalloc;50;40;10;128;0;256;2048;1024;128;5;0;2;128\n";

    std::size_t const n = ex::ingest_results(tree, text);
    eq("ingest_results == 2 eingespielte Knoten (Kommentar/Leerzeile ignoriert)", n, std::size_t{2});
    eq("measured_node_count() == 2 (sparse)", tree.measured_node_count(), std::size_t{2});

    auto const nv1 = tree.node_value("search_algo=Array256/allocator=mimalloc");
    tr("Knoten 1: observer_real && has_result", nv1.observer_real && nv1.has_result);
    eq("Knoten 1: search_insert_count == 256", nv1.observer.search_insert_count, std::uint64_t{256});
    eq("Knoten 1: search_lookup_count == 100", nv1.observer.search_lookup_count, std::uint64_t{100});
    eq("Knoten 1: alloc_bytes_allocated == 4096", nv1.observer.alloc_bytes_allocated, std::uint64_t{4096});
    eq("Knoten 1: observable_axis_count == 2", nv1.observer.observable_axis_count, std::uint64_t{2});
    eq("Knoten 1: tier_fill_level == 256", nv1.observer.tier_fill_level, std::uint64_t{256});

    auto const nv2 = tree.node_value("search_algo=Patricia/allocator=jemalloc");
    eq("Knoten 2: search_insert_count == 128", nv2.observer.search_insert_count, std::uint64_t{128});
    tr("Knoten 2: observer_real", nv2.observer_real);

    auto const nv3 = tree.node_value("search_algo=nonexistent");
    tr("ungemessener Knoten: observer_real == false (sparse, kein Eintrag)", !nv3.observer_real);

    // Einzelzeile: Kommentar wird nicht eingespielt.
    tr("Kommentar-Zeile → ingest_result_line == false", !ex::ingest_result_line(tree, "# nur kommentar"));
    tr("zu kurze Zeile → false", !ex::ingest_result_line(tree, "id;1;2;3"));

    std::cout << "\n==== D14 result_ingest: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
