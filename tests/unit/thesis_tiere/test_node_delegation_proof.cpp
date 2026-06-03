// Audit-30 Fix Q2 / Beweis-Schnitt (2026-06-03) — vertikaler Beleg, dass ein DELEGIERENDES Such-Organ die
// node_type-Achse REAL konsumiert (im Gegensatz zu den Monolith-Wrappern wie Array256SearchAlgo, die node_type
// beschatten). Aufbau: ComposedSearch<LinearScanTraversal, NodeChunkedStore<NodeX, CacheLineAligned, Mimalloc>>
// für NodeX ∈ {Node4, Node16, Node48, Node256}, identischer Workload (n Inserts + n Lookups). Erwartung:
// slot_count + hits IDENTISCH (gleiche Semantik, std::map-Interface), aber chunk_count / Node-Allokationen
// UNTERSCHIEDLICH (= ceil(n / node_capacity)) → die node_type-Achse wirkt jetzt nachweisbar.
//
// Build (Voll-ADHOC-Include-Satz, cl /LD nicht nötig — reines exe):
//   siehe tests/unit/thesis_tiere/build_and_measure_thesis_tiere.ps1 (-Proof) bzw. README.
// Exit 0 = Beweis bestanden (alle 4 chunk_count paarweise verschieden + Semantik identisch).

#include <axes/node/axis_04_node_type_chunked_store.hpp>
#include <axes/node/axis_04_node_type_node4.hpp>
#include <axes/node/axis_04_node_type_node16.hpp>
#include <axes/node/axis_04_node_type_node48.hpp>
#include <axes/node/axis_04_node_type_node256.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp>
#include <topics/traversal/axis_03a_search_algo/composable/composable_search.hpp>

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace cn  = ::comdare::cache_engine::node;
namespace cml = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace cal = ::comdare::cache_engine::allocator::axis_06_allocator;
namespace ctr = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable;

struct Row { std::string node; std::size_t cap, slots, hits, chunks, chunk_allocs; };

template <class NodeT>
Row run_one(std::uint64_t n) {
    using Store  = cn::NodeChunkedStore<NodeT, cml::CacheLineAlignedMemoryLayout, cal::MimallocAllocator>;
    using Search = ctr::ComposedSearch<ctr::LinearScanTraversal, Store>;
    Search s;
    for (std::uint64_t i = 0; i < n; ++i) s.insert(i, i * 7u + 1u);
    std::size_t hits = 0;
    for (std::uint64_t i = 0; i < n; ++i) if (s.lookup(i).has_value()) ++hits;
    Store const& st = s.store();
    return Row{ std::string(st.node_name()), st.node_capacity(), s.occupied_count(),
                hits, st.chunk_count(), st.chunk_alloc_count() };
}

int main() {
    constexpr std::uint64_t n = 1000;
    std::vector<Row> rows{
        run_one<cn::Node4NodeType>(n),
        run_one<cn::Node16NodeType>(n),
        run_one<cn::Node48NodeType>(n),
        run_one<cn::Node256NodeType>(n) };

    std::printf("# node_delegation_proof  n_ops=%llu  (ComposedSearch<LinearScan, NodeChunkedStore<N,L,A>>)\n",
                static_cast<unsigned long long>(n));
    std::printf("node;node_cap;slot_count;hits;chunk_count;chunk_allocs\n");
    for (auto const& r : rows)
        std::printf("%s;%zu;%zu;%zu;%zu;%zu\n", r.node.c_str(), r.cap, r.slots, r.hits, r.chunks, r.chunk_allocs);

    // Beweis-Kriterien: (1) Semantik identisch (alle slot_count==n, hits==n) → std::map-Äquivalenz gewahrt.
    // (2) node_type WIRKT: chunk_count paarweise verschieden + == ceil(n/cap).
    bool semantics_ok = true, node_effect_ok = true;
    for (auto const& r : rows) {
        if (r.slots != n || r.hits != n) semantics_ok = false;
        std::size_t const expect = (n + r.cap - 1) / r.cap;
        if (r.chunks != expect) node_effect_ok = false;
    }
    for (std::size_t i = 0; i < rows.size(); ++i)
        for (std::size_t j = i + 1; j < rows.size(); ++j)
            if (rows[i].chunks == rows[j].chunks) node_effect_ok = false;   // paarweise verschieden

    std::printf("semantics_identical=%d  node_type_effective=%d\n", semantics_ok ? 1 : 0, node_effect_ok ? 1 : 0);
    if (semantics_ok && node_effect_ok) { std::printf("PROOF_OK: delegierendes Such-Organ -> node_type runtime-wirksam\n"); return 0; }
    std::printf("PROOF_FAIL\n");
    return 1;
}
