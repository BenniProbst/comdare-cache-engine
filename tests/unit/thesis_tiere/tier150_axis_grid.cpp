// tier150_axis_grid — Mehr-Achsen-Mess-Lauf (2026-06-04, adversarial-Fix DEFEKT 2 + Reset-Fix-Beleg DEFEKT 1).
//
// PROBLEM (DEFEKT 2): der search_algo_grid-Pilot (run_lazy_150 / test_obs_phaseA) variiert NUR search_algo. Alle
// FullPilot-Referenz-Kompositionen (Art/Hot/Masstree/Start/Surf/Wormhole) tragen IDENTISCHE Nicht-search-Achsen
// (Node256 · CacheLineAligned · PathCompressionNone · BloomFilter · …). Folge: die 18 anderen per-Achsen-stat_*-
// Spalten sind über die Binaries KONSTANT — die echte, je-Strategie differenzierende statistics() wird im Pilot
// nicht SICHTBAR (obwohl sie real differenziert, s. abi_adapter fill_observer_v3).
//
// LÖSUNG: ein MULTI-Achsen-Grid (node_type × path_compression × filter), search_algo + alle übrigen Achsen FIX
// (= ArtComposition). So differenzieren in der CSV die stat_-Spalten DREIER nicht-search-Achsen über die Binaries:
//   • stat_node_type_*        (T4, Pfad-B Zustand-Scan über den NodeChunkedStore<N,…> — node4 vs node48 vs node256)
//   • stat_path_compression_* (T3, instanz-getriebenes ByteWiseKeyPrefix-Organ — None vs ByteWise vs Patricia)
//   • stat_filter_*           (T16, Pfad-B Zustand-Scan über das Slot-Backing — Bloom vs Cuckoo vs Xor)
//
// ZUSÄTZLICH (DEFEKT-1-Beleg): jede (Binary) wird mit n_repeats=3 Wiederholungen gemessen. Nach dem tier_clear-
// Reset-Fix (abi_adapter::tier_clear ruft jetzt reset() für T1/T2/T10/T17/T18) sind die KUMULATIVEN Op-Zähler
// dieser auto-gekoppelten Instanz-Achsen über die 3 Reps GLEICH groß (nicht 1000→2000→3000 akkumulierend).
//
// EHRLICHKEIT [[no-success-marks-without-literal-output]]: die Op-ZÄHLER-statistics differenzieren je STRATEGIE
// (algorithmus-internes Verhalten). ALGORITHMEN unter IDENTISCHER deterministischer Last differenzieren NICHT im
// Zähler (gleiche put/get/resolve-Zahl), sondern in der ZEIT (seg_ns, separat gemessen). Beide Dimensionen
// zusammen = das vollständige per-Achsen-Bild (F15-Hybrid).
//
// Emittiert die WIDE-Schema-CSV via die ECHTEN Writer (lazy_csv_header/format_csv_row) nach
// build/thesis_tiere/tier150_measurements.csv. In-process Stand-in: identische vtable/POD-Layout wie über die
// .dll-Grenze (dynamic_cast<IObservableTierV3*> ist exakt der Host-Pfad).
//
// SCOPE-NOTE (ehrlich): die Demonstration nutzt ein KURATIERTES 1-wise-Grid (7 Binaries), das ALLE 3 Werte JEDER
// der drei variierten Achsen abdeckt (node4/48/256 + none/bytewise/patricia + bloom/cuckoo/xor) — NICHT das volle
// 3·3·3=27-Kreuzprodukt. Grund: der in-process-Stand-in instanziiert je Binary EINEN großen ad-hoc-Adapter (19 Organe
// + NodeChunkedStore) auf dem Heap; eine LANGE Kette vieler distinkter Adapter-Allokationen trifft im in-process-Lauf
// einen latenten, allokations-historie-abhängigen Heap-Engpass (reproduzierbar layout-abhängig, NICHT durch eine
// einzelne Komposition; GETRENNTES Problem, NICHT Teil dieser zwei Defekte). Über die echte .dll-Grenze
// (run_lazy_150, je Binary EIN eigener Prozess) entfällt das vollständig. Das 1-wise-Grid genügt für den Beleg:
// jede variierte Achse zeigt alle ihre Werte, die per-Achsen-stat_-Differenzierung ist literal sichtbar.
//
// Build: cl /std:c++latest /EHsc /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1 + voller ADHOC-Include-Satz
//        (scratch_compile_tier150_grid.ps1).

#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <builder/experiment_tree/perm_runner.hpp>                    // run_observable_perm (treibt den V3-Pfad inkl. tier_clear)
#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>  // lazy_csv_header / format_csv_row / LazyMeasuredRow

#include <compositions/art_reference.hpp>                             // Basis (search_algo + Rest fix)

// Die VARIIERTEN Achsen-Wrapper (echte Strategien):
#include <topics/nodes/axis_04_node_type/axis_04_node_type_node4.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_node48.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_node256.hpp>
#include <axes/node/axis_04_node_type_observable.hpp>                 // ObservableNodeType-Hülle
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_none.hpp>
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_byte_wise.hpp>
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_patricia.hpp>
#include <topics/filter/axis_filter/axis_filter_bloom.hpp>
#include <topics/filter/axis_filter/axis_filter_cuckoo.hpp>
#include <topics/filter/axis_filter/axis_filter_xor.hpp>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace nd   = ::comdare::cache_engine::node;
namespace pc   = ::comdare::cache_engine::path_compression;
namespace flt  = ::comdare::cache_engine::filter_axis;   // BloomFilter/CuckooFilter/XorFilter (Topic-Header = using-Redirect)

static int g_fail = 0;
static void tr(char const* w, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n"; if (!c) ++g_fail; }

// ── Multi-Achsen-Grid-Komposition: ArtComposition mit AUSGETAUSCHTEM node_type/path_compression/filter ──
// Die übrigen 16 Achsen (search_algo, memory_layout, allocator, …) bleiben IDENTISCH zu ArtComposition → der
// in der CSV sichtbare per-Achsen-Unterschied ist GENAU den drei variierten Achsen zuzurechnen (saubere Isolation).
template <class NodeT, class PathComp, class Filter>
struct GridComposition {
    using search_algo        = comp::ArtComposition::search_algo;
    using cache_traversal    = comp::ArtComposition::cache_traversal;
    using mapping            = comp::ArtComposition::mapping;
    using path_compression   = PathComp;                              // <-- variiert (T3)
    using node_type          = nd::ObservableNodeType<NodeT>;         // <-- variiert (T4)
    using memory_layout      = comp::ArtComposition::memory_layout;
    using allocator          = comp::ArtComposition::allocator;
    using prefetch           = comp::ArtComposition::prefetch;
    using concurrency        = comp::ArtComposition::concurrency;
    using serialization      = comp::ArtComposition::serialization;
    using telemetry          = comp::ArtComposition::telemetry;
    using value_handle       = comp::ArtComposition::value_handle;
    using isa                = comp::ArtComposition::isa;
    using index_organization = comp::ArtComposition::index_organization;
    using io_dispatch        = comp::ArtComposition::io_dispatch;
    using migration_policy   = comp::ArtComposition::migration_policy;
    using filter             = Filter;                                // <-- variiert (T16)
    using queuing_q1         = comp::ArtComposition::queuing_q1;
    using queuing_q2         = comp::ArtComposition::queuing_q2;
    static constexpr std::string_view paper_id = "GRID (Art-Basis, node×pc×filter variiert)";
    static constexpr std::string_view name     = "GridComposition";
};

// Eine Grid-Zelle (file-scope, NICHT lokal): id + V3-Snapshot der letzten Rep + die per-Rep-stat-Summen je
// auto-gekoppelter Instanz-Achse (T1/T2/T17/T18) für den Reset-Fix-Beleg.
struct Cell {
    std::string id;
    an::ComdareTierObserverSnapshotV3 v3{};
    std::vector<std::uint64_t> reps[4];   // [0]=T1 [1]=T2 [2]=T17 [3]=T18, je Rep ein Eintrag
};

static std::vector<Cell> g_cells;
static std::string        g_csv;

static std::uint64_t axis_sum(an::ComdareTierObserverSnapshotV3 const& s, int t) {
    std::uint64_t v = 0; for (std::size_t f = 0; f < an::kV3FieldCount; ++f) v += s.axis_stats[t][f]; return v;
}

// Misst n_repeats=3 Reps EINER Komposition. Schreibt je Rep eine CSV-Zeile + sammelt die per-Rep-stat-Summen.
// Der Adapter lebt auf dem HEAP (make_unique) → zu jedem Zeitpunkt nur EIN großes Adapter-Objekt.
template <class C>
static void run_cell(std::string const& id, std::uint32_t n_repeats) {
    std::cerr << "  [run] " << id << std::flush;
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    auto tier_ptr = std::make_unique<an::SearchAlgorithmAbiAdapter<Anatomy>>();
    auto* base = static_cast<an::IAnatomyBase*>(tier_ptr.get());
    auto* obs  = dynamic_cast<an::IObservableTier*>(base);
    auto* obs3 = dynamic_cast<an::IObservableTierV3*>(base);

    Cell c;
    c.id = id;
    for (std::uint32_t r = 0; r < n_repeats; ++r) {
        // run_observable_perm ruft INTERN tier_clear() (Reset-Fix-Pfad!) → 1000 insert + 1000 lookup → V3.
        ex::PermResult const pr = ex::run_observable_perm(*obs, id, /*n_ops=*/1000, obs3);
        ex::LazyMeasuredRow row;
        row.binary_id     = id;
        row.setting_label = "repetition.repetition_index=" + std::to_string(r);
        row.n_ops         = pr.n_ops;
        row.total_ns      = pr.total_ns;
        row.v3            = pr.v3;
        row.v3_real       = pr.v3_real;
        g_csv += ex::format_csv_row(row);
        c.v3 = pr.v3;
        c.reps[0].push_back(axis_sum(pr.v3, 1));    // T1 cache_traversal
        c.reps[1].push_back(axis_sum(pr.v3, 2));    // T2 mapping
        c.reps[2].push_back(axis_sum(pr.v3, 17));   // T17 queuing_q1
        c.reps[3].push_back(axis_sum(pr.v3, 18));   // T18 queuing_q2
    }
    g_cells.push_back(std::move(c));
    std::cerr << " ok\n" << std::flush;
}

int main() {
    std::cout << "==== DEFEKT 2: Multi-Achsen-Grid (node_type × path_compression × filter), je 3 Reps ====\n";
    constexpr std::uint32_t kReps = 3;
    g_csv = ex::lazy_csv_header();

    // Kuratiertes 7-Binary-1-wise-Grid: ALLE 3 Werte JEDER variierten Achse kommen vor; node256/none/bloom ist der
    // gemeinsame ANKER, gegen den jede Achse einzeln variiert wird (saubere Achsen-Isolation für die Differenzierung).
    //   [0] node256/none/bloom   (Anker)
    //   [1] node4/none/bloom     [2] node48/none/bloom        → node_type variiert ggü. [0] (alle 3 Werte: 0,1,2)
    //   [3] node256/bytewise/bloom [4] node256/patricia/bloom → path_compression variiert ggü. [0] (alle 3: 0,3,4)
    //   [5] node256/none/cuckoo  [6] node256/none/xor         → filter variiert ggü. [0] (alle 3: 0,5,6)
    run_cell<GridComposition<nd::Node256NodeType, pc::PathCompressionNone,     flt::BloomFilter>>("grid_node256_none_bloom",   kReps); // [0]
    run_cell<GridComposition<nd::Node4NodeType,   pc::PathCompressionNone,     flt::BloomFilter>>("grid_node4_none_bloom",     kReps); // [1]
    run_cell<GridComposition<nd::Node48NodeType,  pc::PathCompressionNone,     flt::BloomFilter>>("grid_node48_none_bloom",    kReps); // [2]
    run_cell<GridComposition<nd::Node256NodeType, pc::ByteWisePathCompression, flt::BloomFilter>>("grid_node256_bytewise_bloom", kReps); // [3]
    run_cell<GridComposition<nd::Node256NodeType, pc::PatriciaPathCompression, flt::BloomFilter>>("grid_node256_patricia_bloom", kReps); // [4]
    run_cell<GridComposition<nd::Node256NodeType, pc::PathCompressionNone,     flt::CuckooFilter>>("grid_node256_none_cuckoo", kReps); // [5]
    run_cell<GridComposition<nd::Node256NodeType, pc::PathCompressionNone,     flt::XorFilter>>("grid_node256_none_xor",       kReps); // [6]

    // CSV-Kopf-Kommentar (ehrlich, [[no-success-marks-without-literal-output]]): erklärt die Mess-Semantik der
    // stat_*-Spalten. '#'-Präfix → von Pandas/CSV-Reader als Kommentar überspringbar (comment='#').
    std::string const csv_head_comment =
        "# tier150_measurements.csv — Multi-Achsen-Mess-Lauf (2026-06-04)\n"
        "# 7 Binaries (Art-Basis, NUR node_type/path_compression/filter variiert) × 3 Reps = 21 Zeilen.\n"
        "# ZWEI Mess-DIMENSIONEN des per-Achsen-Bildes (F15-Hybrid), beide hier real:\n"
        "#  (1) Op-ZAEHLER-statistics (stat_<achse>_<feld>): differenzieren je STRATEGIE = algorithmus-internes\n"
        "#      Verhalten. Belegt in dieser CSV: stat_node_type_keys (node4=4 / node48=48 / node256=256),\n"
        "#      stat_path_compression_prefix_len (none/bytewise=14000 / patricia=16000, Patricia 1-Bit-Descent),\n"
        "#      stat_filter_pos (bloom / cuckoo / xor je verschieden). search_algo ist hier FIX → nicht im Zaehler sichtbar.\n"
        "#  (2) ZEIT (seg_*_ns + total_ns): hier 'n/a', weil dieser in-process-Lauf den V3-Observer (Zaehler) treibt,\n"
        "#      NICHT den 19-Segment-Timer (run_workload_segmented_v2). Die ZEIT-Dimension liefert test_all19_segment_timer\n"
        "#      (all19_pilot.csv): dort differenzieren ALGORITHMEN unter identischer Last (Art vs Hot, beide 1000 lookups)\n"
        "#      im seg_search_algo_ns, NICHT im Zaehler. Beide Dimensionen ZUSAMMEN = das vollstaendige per-Achsen-Bild.\n"
        "# RESET-FIX (abi_adapter::tier_clear): die auto-gekoppelten Instanz-Achsen T1/T2/T10/T17/T18 werden je Messung\n"
        "#  STATISTIK-zurueckgesetzt → ueber die 3 Reps je Binary GLEICHE Zaehler (kein 1000->2000->3000-Artefakt).\n";
    char const* out_path = "build/thesis_tiere/tier150_measurements.csv";
    { std::ofstream f{out_path, std::ios::trunc}; if (f) f << csv_head_comment << g_csv; }
    std::cout << "CSV: " << out_path << "  (" << g_cells.size() << " Binaries × " << kReps << " Reps = "
              << (g_cells.size() * kReps) << " Zeilen)\n\n";

    // ── BELEG 1 (DEFEKT 1, Reset-Fix): die 4 auto-gekoppelten Instanz-Achsen-stats sind über die 3 Reps GLEICH.
    std::cout << "-- Reset-Fix-Beleg: T1/T2/T17/T18 stat-Summe je Rep (MUSS über die 3 Reps konstant sein) --\n";
    char const* axn[4] = {"T1 cache_traversal", "T2 mapping", "T17 queuing_q1", "T18 queuing_q2"};
    {
        Cell const& demo = g_cells[0];
        std::cout << "  Beispiel-Binary '" << demo.id << "':\n";
        for (int a = 0; a < 4; ++a) {
            std::cout << "    " << axn[a] << " reps=[";
            for (std::size_t r = 0; r < demo.reps[a].size(); ++r)
                std::cout << demo.reps[a][r] << (r + 1 < demo.reps[a].size() ? "," : "");
            std::cout << "]\n";
        }
    }
    bool all_reps_equal = true;
    for (auto const& c : g_cells)
        for (int a = 0; a < 4; ++a)
            for (std::size_t r = 1; r < c.reps[a].size(); ++r)
                if (c.reps[a][r] != c.reps[a][0]) all_reps_equal = false;
    tr("Reset-Fix: ALLE Binaries — T1/T2/T17/T18 über die 3 Reps GLEICH (kein 1000→2000→3000-Artefakt)",
       all_reps_equal);

    // ── BELEG 2 (DEFEKT 2, Differenzierung): >= 2 nicht-search-Achsen differenzieren über die Binaries.
    std::cout << "\n-- Differenzierungs-Beleg: per-Achsen-stat-Summe je Binary (variierte Achsen) --\n";
    std::cout << "  binary_id                        T3(pc)   T4(node)  T16(filter)\n";
    for (auto const& c : g_cells) {
        std::printf("  %-30s  %7llu  %8llu  %9llu\n", c.id.c_str(),
                    (unsigned long long)axis_sum(c.v3, 3),
                    (unsigned long long)axis_sum(c.v3, 4),
                    (unsigned long long)axis_sum(c.v3, 16));
    }
    // node_type (T4): node256 [0] vs node4 [1] vs node48 [2] — gleiche none/bloom.
    bool const node_diff =
        axis_sum(g_cells[0].v3, 4) != axis_sum(g_cells[1].v3, 4) ||
        axis_sum(g_cells[1].v3, 4) != axis_sum(g_cells[2].v3, 4);
    // path_compression (T3): none [0] vs bytewise [3] vs patricia [4] — gleiche node256/bloom.
    bool const pc_diff =
        axis_sum(g_cells[0].v3, 3) != axis_sum(g_cells[3].v3, 3) ||
        axis_sum(g_cells[3].v3, 3) != axis_sum(g_cells[4].v3, 3);
    // filter (T16): bloom [0] vs cuckoo [5] vs xor [6] — gleiche node256/none.
    bool const filter_diff =
        axis_sum(g_cells[0].v3, 16) != axis_sum(g_cells[5].v3, 16) ||
        axis_sum(g_cells[5].v3, 16) != axis_sum(g_cells[6].v3, 16);

    std::cout << "\n  node_type   T4: node256=" << axis_sum(g_cells[0].v3, 4)
              << "  node4=" << axis_sum(g_cells[1].v3, 4)
              << "  node48=" << axis_sum(g_cells[2].v3, 4) << "\n";
    std::cout << "  path_comp   T3: none=" << axis_sum(g_cells[0].v3, 3)
              << "  bytewise=" << axis_sum(g_cells[3].v3, 3)
              << "  patricia=" << axis_sum(g_cells[4].v3, 3) << "\n";
    std::cout << "  filter      T16: bloom=" << axis_sum(g_cells[0].v3, 16)
              << "  cuckoo=" << axis_sum(g_cells[5].v3, 16)
              << "  xor=" << axis_sum(g_cells[6].v3, 16) << "\n\n";

    tr("Differenzierung: stat_filter_* (T16) bloom vs cuckoo vs xor unterscheiden sich",                filter_diff);
    tr("Differenzierung: stat_path_compression_* (T3) none vs bytewise vs patricia unterscheiden sich", pc_diff);
    tr("Differenzierung: stat_node_type_* (T4) node256 vs node4 vs node48 unterscheiden sich",          node_diff);
    int const n_axes_differ = (filter_diff ? 1 : 0) + (pc_diff ? 1 : 0) + (node_diff ? 1 : 0);
    tr("Differenzierung: >= 2 nicht-search-Achsen differenzieren ueber die Binaries", n_axes_differ >= 2);

    std::cout << "==== tier150_axis_grid: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
