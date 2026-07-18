// Phase B Abschluss (2026-06-04) — search_algo_grid-Lauf der VOLLSTÄNDIGEN Per-Achsen-Observer-Vervollständigung:
//   IObservableTier::tier_observe → ComdareTierObserverSnapshot.axis_stats[19][8] über mehrere reale SA-
//   Kompositionen (in-process Stand-in: identische vtable/POD-Layout wie über die .dll-Grenze). Emittiert die WIDE-
//   Schema-CSV via die ECHTEN Writer (lazy_csv_header/format_csv_row) nach build/thesis_tiere/obs_phaseB_pilot.csv
//   und prüft literal:
//     (1) store-backed AdHoc traegt weiter alle 19 Observer-Zeilen (filled_axis_count == 19); Reference-Huellen
//         tragen seit #188-4c-i store-abhaengige Achsen honest-0, bis die observe-Hooks #234 existieren;
//     (2) bei store-backed ist kein Achsen-Block 0 — AUSSER den EHRLICH deklarierten Strategie-Baselines
//         (T7 NonePrefetch, T14 NoMigration migrations/votes/tier_moves); Huellen assertieren T12-T15 == 0;
//     (3) die Werte sind ORGAN-/DATEN-bestimmt: ein zweiter Pass treibt die Phase-B-Achsen-Hüllen (T3/T12/T15)
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
#include <axes/index_organization/axis_01_index_organization_index_organized_table.hpp>

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

using StoreBackedAdHocComposition = an::AdHocComposition<
    ce03a::Array256SearchAlgo, comp::ArtComposition::cache_traversal, comp::ArtComposition::mapping,
    comp::ArtComposition::path_compression, comp::ArtComposition::node_type, comp::ArtComposition::memory_layout,
    comp::ArtComposition::allocator, comp::ArtComposition::prefetch, comp::ArtComposition::concurrency,
    comp::ArtComposition::serialization, comp::ArtComposition::value_handle, comp::ArtComposition::index_organization,
    comp::ArtComposition::io_dispatch, comp::ArtComposition::migration_policy, comp::ArtComposition::filter,
    comp::ArtComposition::queuing_q1, comp::ArtComposition::queuing_q2>;

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
    row.unified       = pr.unified;
    row.unified_real  = pr.unified_real;
    csv_out += ex::format_csv_row(row);

    std::cout << "  " << name << ": filled_axis_count=" << pr.unified.filled_axis_count << "  T0..T18 row_sum=";
    for (int t = 0; t < 17; ++t) std::cout << row_sum(pr.unified, t) << (t < 16 ? "," : "");
    std::cout << "\n";
    return pr.unified;
}

template <class C>
static void check_one(char const* name, an::ComdareTierObserverSnapshot const& s, bool store_axes_backed,
                      std::uint32_t expected_filled = 0) {
    if (store_axes_backed) {
        // Store-backed AdHoc bleibt die volle Phase-B-Coverage-Kontrolle: alle Schema-Achsen sind befüllt.
        tr((std::string{name} + ": filled_axis_count == 17 (store-backed; INC-2d)").c_str(),
           s.filled_axis_count == 17u);
        tr((std::string{name} + ": filled_axis_count == kV3FilledAxisCount (store-backed)").c_str(),
           s.filled_axis_count == an::kV3FilledAxisCount);
        // Bau-INC-2d: index_org/io/migration/filter sind von T12-T15 auf T11-T14 gerueckt (isa raus, Indizes >=11 -1);
        // path_compression (T3) bleibt unveraendert.
        for (int t : {3, 11, 12, 13, 14}) {
            tr((std::string{name} + ": T" + std::to_string(t) + " row_sum > 0 (store-backed)").c_str(),
               row_sum(s, t) > 0);
        }
        tr((std::string{name} + ": T13 total_decisions > 0 (Store-Scan getrieben)").c_str(), s.axis_stats[13][0] > 0);
        tr((std::string{name} + ": T13 tier_moves == 0 (honest, kein 2. Tier)").c_str(), s.axis_stats[13][4] == 0);
        return;
    }

    // #188-4c-i: Referenz-Hülle hat kein store_type → honest-0 (Re-Kopplung #234). Die store-slot-gescannten Achsen
    // (memory_layout/serialization/value_handle/isa/Node-Telemetrie) fallen für die Hülle weg; nur die per-op-/Nicht-
    // Store-Achsen bleiben befüllt. Anzahl komposition-spezifisch (build-verifiziert): Art/Hot=9, Masstree=8 (seit INC-2c ohne auto-gekoppelte Telemetrie-Zeile). Der EINE
    // Unterschied ist der allocator-Observer T6: Art/Hot registrieren Allokations-Aktivität (row_sum>0), Masstree nicht
    // (T6==0) → eine gefüllte Achse weniger. (NICHT Prefetch — alle drei sind NonePrefetch.)
    tr((std::string{name} + ": filled_axis_count == " + std::to_string(expected_filled) +
        " (Huelle honest-0 Store-Achsen)")
           .c_str(),
       s.filled_axis_count == expected_filled);
    tr((std::string{name} + ": T3 row_sum > 0 (Auto-Kopplung bleibt real)").c_str(), row_sum(s, 3) > 0);
    // Bau-INC-2d: die Store-Scan-Achsen (index_org/io/migration/filter) sind von T12-T15 auf T11-T14 gerueckt.
    for (int t : {11, 12, 13, 14}) {
        tr((std::string{name} + ": T" + std::to_string(t) + " row_sum == 0 (Huellen honest-0)").c_str(),
           row_sum(s, t) == 0u);
    }
    tr((std::string{name} + ": T13 total_decisions == 0 (kein Store-Scan bei Huellen)").c_str(),
       s.axis_stats[13][0] == 0u);
    tr((std::string{name} + ": T13 tier_moves == 0 (honest, kein 2. Tier)").c_str(), s.axis_stats[13][4] == 0u);
}

int main() {
    std::cout << "==== Phase B Abschluss: Per-Achsen-Observer-V3 ALLE 17 Achsen (search_algo_grid; INC-2d) ====\n";

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
    // #188-4c-i: Referenz-Hüllen honest-0 auf den Store-Achsen → filled_axis_count komposition-spezifisch (Re-Kopplung #234).
    check_one<comp::ArtComposition>("Art", art, false, /*expected_filled=*/9u);
    check_one<comp::HotComposition>("Hot", hot, false, /*expected_filled=*/9u);
    check_one<comp::MasstreeComposition>("Masstree", mass, false, /*expected_filled=*/8u);

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

    // T13 index_organization (#24 Option A, honest-100%, 2026-07-13): predicate_evals/indirect_lookups werden vom
    // strategie-echten Scan (index_org_scan_counted) ZURUECKGEMELDET, nicht mehr aus Flags synthetisiert. Heap +
    // NonClustered fuehren je Record einen REALEN Predicate-Vergleich aus (== n); NonClustered zusaetzlich je Lookup
    // einen REALEN Pointer-Hop (indirect_lookups == n); Clustered/IOT (sequentieller Summen-Scan) melden 0/0.
    auto drive_ix = [&](auto organ) -> ixo::IndexOrgStatistics {
        (void)organ.index_org_observe(buf, sizeof(buf) / 16, 16);
        return organ.statistics();
    };
    constexpr std::uint64_t kIxRecords = sizeof(buf) / 16; // 4096/16 = 256 gescannte Records
    auto                    ix_heap    = drive_ix(ixo::ObservableIndexOrg<ixo::HeapIndexOrganization>{});
    auto                    ix_clus    = drive_ix(ixo::ObservableIndexOrg<ixo::ClusteredIndexOrganization>{});
    auto                    ix_nonc    = drive_ix(ixo::ObservableIndexOrg<ixo::NonClusteredIndexOrganization>{});
    auto                    ix_iot     = drive_ix(ixo::ObservableIndexOrg<ixo::IotIndexOrganization>{});
    std::cout << "  T13 predicate_evals: Heap=" << ix_heap.predicate_evals << " Clustered=" << ix_clus.predicate_evals
              << " NonClustered=" << ix_nonc.predicate_evals
              << " | indirect_lookups: NonClustered=" << ix_nonc.indirect_lookups << " IOT=" << ix_iot.indirect_lookups
              << "\n";
    // Heap = O(n) Full-Scan mit Predicate-Branch je Record → predicate_evals == n (REAL gemessen, kein Flag).
    tr("T13 (#24): Heap.predicate_evals == 256 (real gemessen, ein Predicate je Record)",
       ix_heap.predicate_evals == kIxRecords);
    // Clustered = sequentieller Summen-Scan ohne Predicate → 0 (honest, #24-neutral).
    tr("T13: Clustered.predicate_evals == 0 (sequential, honest)", ix_clus.predicate_evals == 0);
    // #24 Option A: die Divergenz ist jetzt REAL gemessen + design-fest → HARTE Assertion (nicht mehr informativ).
    tr("T13 (#24): Heap.predicate_evals != Clustered.predicate_evals (real gemessene Divergenz)",
       ix_heap.predicate_evals != ix_clus.predicate_evals);
    // NonClustered = Pointer-Hop je Lookup + Residual-Predicate je geholtem Record (Option A, Thesis-Anhang-D:612).
    tr("T13 (#24): NonClustered.indirect_lookups == 256 (real ausgefuehrter Pointer-Hop je Lookup)",
       ix_nonc.indirect_lookups == kIxRecords);
    tr("T13 (#24): NonClustered.predicate_evals == 256 (real ausgefuehrter Residual-Predicate je Record)",
       ix_nonc.predicate_evals == kIxRecords);
    // IOT = data_embedded_in_leaf → KEIN Pointer-Hop → indirect_lookups == 0 (Korrektur der frueheren Flag-Fabrikation).
    tr("T13 (#24): IOT.indirect_lookups == 0 (data_embedded_in_leaf, kein Pointer-Hop; Fabrikation behoben)",
       ix_iot.indirect_lookups == 0);

    std::cout << "==== Phase B Abschluss Per-Achsen-Observer-V3: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
