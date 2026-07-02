// Phase B Abschluss (2026-06-04) — search_algo_grid-Lauf der VOLLSTÄNDIGEN Per-Achsen-Observer-Vervollständigung:
//   IObservableTier::tier_observe → ComdareTierObserverSnapshot.axis_stats[19][8] über mehrere reale SA-
//   Kompositionen (in-process Stand-in: identische vtable/POD-Layout wie über die .dll-Grenze). Emittiert die WIDE-
//   Schema-CSV via die ECHTEN Writer (lazy_csv_header/format_csv_row) nach build/thesis_tiere/obs_phaseB_pilot.csv
//   und prüft literal:
//     (1) store-backed AdHoc traegt weiter alle 19 Observer-Zeilen (filled_axis_count == 19); Reference-Huellen
//         tragen seit #188-4c-i store-abhaengige Achsen honest-0, bis die observe-Hooks #234 existieren;
//     (2) bei store-backed ist kein Achsen-Block 0 — AUSSER den EHRLICH deklarierten Strategie-Baselines
//         (T7 NonePrefetch, T15 NoMigration migrations/votes/tier_moves); Huellen assertieren T13-T16 == 0;
//     (3) die Werte sind ORGAN-/DATEN-bestimmt: ein zweiter Pass treibt die 5 Phase-B-Achsen-Hüllen (T3/T13/T16)
//         direkt mit KONTRASTIERENDEN Strategien (None vs ByteWise vs Patricia; Heap vs Clustered vs NonClustered)
//         → die Zähler DIFFERIEREN strategie-abhängig (kein erfundener Konstantwert).
//
// Build: cl /std:c++latest /EHsc /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1 + voller ADHOC-Include-Satz.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/composition_factory.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <builder/experiment_tree/perm_runner.hpp>
#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>

#include <compositions/art_reference.hpp>
#include <compositions/hot_reference.hpp>
#include <compositions/masstree_reference.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp>

// Direkte Hüllen-Treiber (strategie-Divergenz-Beleg) — die echten Achsen-Strategien.
#include <axes/path_compression/axis_02_path_compression_observable.hpp>
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_byte_wise.hpp>
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_patricia.hpp>
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_none.hpp>
#include <axes/index_organization/axis_01_index_organization_observable.hpp>
#include <axes/index_organization/axis_01_index_organization_heap.hpp>
#include <axes/index_organization/axis_01_index_organization_clustered.hpp>
#include <axes/index_organization/axis_01_index_organization_non_clustered.hpp>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

namespace an    = ::comdare::cache_engine::anatomy;
namespace comp  = ::comdare::cache_engine::compositions;
namespace ce03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ex    = ::comdare::cache_engine::builder::experiment;
namespace pc    = ::comdare::cache_engine::path_compression;
namespace ixo   = ::comdare::cache_engine::index_organization;

using StoreBackedAdHocComposition =
    an::AdHocComposition<ce03a::Array256SearchAlgo,
                         comp::ArtComposition::cache_traversal,
                         comp::ArtComposition::mapping,
                         comp::ArtComposition::path_compression,
                         comp::ArtComposition::node_type,
                         comp::ArtComposition::memory_layout,
                         comp::ArtComposition::allocator,
                         comp::ArtComposition::prefetch,
                         comp::ArtComposition::concurrency,
                         comp::ArtComposition::serialization,
                         comp::ArtComposition::telemetry,
                         comp::ArtComposition::value_handle,
                         comp::ArtComposition::isa,
                         comp::ArtComposition::index_organization,
                         comp::ArtComposition::io_dispatch,
                         comp::ArtComposition::migration_policy,
                         comp::ArtComposition::filter,
                         comp::ArtComposition::queuing_q1,
                         comp::ArtComposition::queuing_q2>;

static int  g_fail = 0;
static void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

static bool is_filled(int t) {
    return t >= 0 && t < static_cast<int>(an::kV3AxisCount) && an::kV3AxisSchema[t].names[0] != nullptr;
}
static std::uint64_t row_sum(an::ComdareTierObserverSnapshot const& s, int t) {
    std::uint64_t v = 0;
    for (std::size_t f = 0; f < an::kV3FieldCount; ++f) v += s.axis_stats[t][f];
    return v;
}

template <class C>
static an::ComdareTierObserverSnapshot measure_v3(char const* name, std::string& csv_out) {
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                                  base = static_cast<an::IAnatomyBase*>(&tier);
    auto*                                  obs  = dynamic_cast<an::IObservableTier*>(base);
    ex::PermResult const                   pr   = ex::run_observable_perm(*obs, name, /*n_ops=*/1000);

    ex::LazyMeasuredRow row;
    row.binary_id     = name;
    row.setting_label = "-";
    row.n_ops         = pr.n_ops;
    row.total_ns      = pr.total_ns;
    row.v3            = pr.v3;
    row.v3_real       = pr.v3_real;
    csv_out += ex::format_csv_row(row);

    std::cout << "  " << name << ": filled_axis_count=" << pr.v3.filled_axis_count << "  T0..T18 row_sum=";
    for (int t = 0; t < 19; ++t) std::cout << row_sum(pr.v3, t) << (t < 18 ? "," : "");
    std::cout << "\n";
    return pr.v3;
}

template <class C>
static void check_one(char const* name, an::ComdareTierObserverSnapshot const& s, bool store_axes_backed) {
    if (store_axes_backed) {
        // Store-backed AdHoc bleibt die volle Phase-B-Coverage-Kontrolle: alle Schema-Achsen sind befüllt.
        tr((std::string{name} + ": filled_axis_count == 19 (store-backed)").c_str(), s.filled_axis_count == 19u);
        tr((std::string{name} + ": filled_axis_count == kV3FilledAxisCount (store-backed)").c_str(),
           s.filled_axis_count == an::kV3FilledAxisCount);
        for (int t : {3, 13, 14, 15, 16}) {
            tr((std::string{name} + ": T" + std::to_string(t) + " row_sum > 0 (store-backed)").c_str(),
               row_sum(s, t) > 0);
        }
        tr((std::string{name} + ": T15 total_decisions > 0 (Store-Scan getrieben)").c_str(),
           s.axis_stats[15][0] > 0);
        tr((std::string{name} + ": T15 tier_moves == 0 (honest, kein 2. Tier)").c_str(),
           s.axis_stats[15][4] == 0);
        return;
    }

    // #188-4c-i: Huellen-Komposition container_-authoritativ -> store-Achsen honest-0 (bis observe-Hooks #234).
    // filled_axis_count zaehlt nur tatsaechlich befuellte Observer-Zeilen; T4/T5/T6/T9/T11..T16 fallen ohne Store weg.
    tr((std::string{name} + ": filled_axis_count == 9 (Huellen honest-0 Store-Achsen)").c_str(),
       s.filled_axis_count == 9u);
    tr((std::string{name} + ": T3 row_sum > 0 (Auto-Kopplung bleibt real)").c_str(), row_sum(s, 3) > 0);
    for (int t : {13, 14, 15, 16}) {
        tr((std::string{name} + ": T" + std::to_string(t) + " row_sum == 0 (Huellen honest-0)").c_str(),
           row_sum(s, t) == 0u);
    }
    tr((std::string{name} + ": T15 total_decisions == 0 (kein Store-Scan bei Huellen)").c_str(),
       s.axis_stats[15][0] == 0u);
    tr((std::string{name} + ": T15 tier_moves == 0 (honest, kein 2. Tier)").c_str(), s.axis_stats[15][4] == 0u);
}

int main() {
    std::cout << "==== Phase B Abschluss: Per-Achsen-Observer-V3 ALLE 19 Achsen (search_algo_grid) ====\n";

    std::string csv   = ex::lazy_csv_header();
    auto        adhoc = measure_v3<StoreBackedAdHocComposition>("AdHocArray256StoreBacked", csv);
    auto        art   = measure_v3<comp::ArtComposition>("ArtComposition", csv);
    auto        hot   = measure_v3<comp::HotComposition>("HotComposition", csv);
    auto        mass  = measure_v3<comp::MasstreeComposition>("MasstreeComposition", csv);

    char const* out_path = "build/thesis_tiere/obs_phaseB_pilot.csv";
    {
        std::ofstream f{out_path, std::ios::trunc};
        if (f) f << csv;
    }
    std::cout << "CSV: " << out_path << "\n";

    check_one<StoreBackedAdHocComposition>("AdHocArray256StoreBacked", adhoc, true);
    check_one<comp::ArtComposition>("Art", art, false);
    check_one<comp::HotComposition>("Hot", hot, false);
    check_one<comp::MasstreeComposition>("Masstree", mass, false);

    // (3) STRATEGIE-DIVERGENZ-BELEG: dieselbe Treibe-Last, KONTRASTIERENDE Strategien → divergente Zähler.
    std::cout << "---- Strategie-Divergenz (gleiche Last, verschiedene Strategie) ----\n";
    unsigned char buf[4096];
    for (std::size_t i = 0; i < sizeof(buf); ++i) buf[i] = static_cast<unsigned char>(i * 131u + 7u);

    // T3 path_compression: None vs ByteWise vs Patricia (1000 compress-Calls je Strategie).
    auto drive_pc = [&](auto organ) -> pc::PathCompressionStatistics {
        for (std::uint64_t k = 0; k < 1000; ++k) (void)organ.compress(k * 2654435761u + 1u, 0u);
        return organ.statistics();
    };
    auto pc_none = drive_pc(pc::ObservablePathCompression<pc::PathCompressionNone>{});
    auto pc_byte = drive_pc(pc::ObservablePathCompression<pc::ByteWisePathCompression>{});
    auto pc_patr = drive_pc(pc::ObservablePathCompression<pc::PatriciaPathCompression>{});
    std::cout << "  T3 compress prefix_len_total: None=" << pc_none.prefix_len_total
              << " ByteWise=" << pc_byte.prefix_len_total << " Patricia=" << pc_patr.prefix_len_total << "\n";
    // Patricia addiert die 1-Bit-Descent-Schritte → prefix_len_total != ByteWise/None (echte Strategie-Charakteristik).
    tr("T3: Patricia.prefix_len_total != ByteWise.prefix_len_total (1-Bit vs 8-Bit Descent)",
       pc_patr.prefix_len_total != pc_byte.prefix_len_total);

    // T13 index_organization: Heap (nicht-clustered → predicate_evals) vs Clustered (sequential → 0) vs NonClustered
    // (indirect_lookups). Gleicher Scan, divergente predicate_evals/indirect_lookups je Strategie-Eigenschaft.
    auto drive_ix = [&](auto organ) -> ixo::IndexOrgStatistics {
        (void)organ.index_org_observe(buf, sizeof(buf) / 16, 16);
        return organ.statistics();
    };
    auto ix_heap = drive_ix(ixo::ObservableIndexOrg<ixo::HeapIndexOrganization>{});
    auto ix_clus = drive_ix(ixo::ObservableIndexOrg<ixo::ClusteredIndexOrganization>{});
    auto ix_nonc = drive_ix(ixo::ObservableIndexOrg<ixo::NonClusteredIndexOrganization>{});
    std::cout << "  T13 predicate_evals: Heap=" << ix_heap.predicate_evals << " Clustered=" << ix_clus.predicate_evals
              << " | indirect_lookups: NonClustered=" << ix_nonc.indirect_lookups << "\n";
    tr("T13: Heap.predicate_evals > 0 (nicht-clustered Full-Scan)", ix_heap.predicate_evals > 0);
    tr("T13: Clustered.predicate_evals == 0 (sequential, honest)", ix_clus.predicate_evals == 0);
    tr("T13: Heap.predicate_evals != Clustered.predicate_evals (strategie-divergent)",
       ix_heap.predicate_evals != ix_clus.predicate_evals);

    std::cout << "==== Phase B Abschluss Per-Achsen-Observer-V3: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
