// D12 / L-76d — Viren-Pfad: GraphBfs (IVirusExecutionEngine) — produktive Graph-Engine ohne Achsen-System.
// Verifiziert: engine_kind()==Virus (Schwester-Zweig), algorithm_family, ECHTE BFS-Korrektheit (visited/edges/
// checksum) über CSR-Referenzgraph + isolierter Knoten + Lifecycle. Build: cl /I libs/cache_engine (kein Boost).

#include "virus/graph_bfs.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace cv  = comdare::cache_engine::virus;
namespace eng = comdare::cache_engine::execution_engine;

static int g_fail = 0;
template <class A, class B>
static void eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) {
        std::cout << " (erwartet " << e << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
static void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

int main() {
    std::cout << "==== D12 GraphBfs als Virus (IVirusExecutionEngine) ====\n";
    cv::GraphBfs                bfs;
    eng::IVirusExecutionEngine* v = &bfs; // über die Virus-Wurzel
    eng::IExecutionEngine*      e = &bfs; // über die gemeinsame Wurzel
    tr("engine_kind() == Virus (Nicht-Lebewesen)", e->engine_kind() == eng::ExecutionEngineKind::Virus);
    eq("engine_name == GraphBFS", std::string{e->engine_name()}, std::string{"GraphBFS"});
    eq("algorithm_family == GraphBFS", std::string{v->algorithm_family()}, std::string{"GraphBFS"});
    e->warm_up();
    e->run();
    tr("lifecycle Running", e->lifecycle_state() == eng::EngineLifecycleState::Running);

    std::cout << "\n==== D12 ECHTE BFS-Korrektheit (CSR-Referenzgraph) ====\n";
    // Ungerichteter Graph (5 Knoten): Kette 0-1-2-3-4 + Querkante 0-2.
    // 0:{1,2} 1:{0,2} 2:{0,1,3} 3:{2,4} 4:{3}
    bfs.set_graph(/*row_offsets=*/{0, 2, 4, 7, 9, 10},
                  /*col_indices=*/{1, 2, 0, 2, 0, 1, 3, 2, 4, 3});
    eq("node_count() == 5", bfs.node_count(), std::uint64_t{5});
    std::uint64_t const visited = bfs.run_bfs(0);
    eq("BFS von 0 erreicht alle 5 Knoten", visited, std::uint64_t{5});
    eng::VirusMeasurementSnapshotV1 snap{};
    v->virus_observe(&snap);
    eq("Snapshot visited_nodes == 5", snap.visited_nodes, std::uint64_t{5});
    eq("Snapshot edges_traversed == 10 (alle gerichteten CSR-Kanten)", snap.edges_traversed, std::uint64_t{10});
    eq("Snapshot result_checksum == 0+1+2+3+4 == 10", snap.result_checksum, std::uint64_t{10});
    eq("Snapshot run_count == 1", snap.run_count, std::uint64_t{1});

    std::cout << "\n==== D12 BFS mit unerreichbarem Knoten ====\n";
    // 6 Knoten: 0-1-2 Komponente + isolierte 3,4,5 (keine Kanten von 0 erreichbar).
    bfs.set_graph(/*row_offsets=*/{0, 1, 2, 2, 2, 2, 2}, /*col_indices=*/{1, 2});
    eq("node_count() == 6", bfs.node_count(), std::uint64_t{6});
    std::uint64_t const v2 = bfs.run_bfs(0);
    eq("BFS von 0 erreicht nur Komponente {0,1,2}", v2, std::uint64_t{3});

    std::cout << "\n==== D12 Virus: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
