// (X) BUILD-VERIFIKATION: der per-Achsen-Timer auf ALLE 18 SearchAlgorithm-Achsen (Bau-INC-2c: telemetry ist System-Achse) ausgeweitet
// (IMeasurableWorkloadV3::run_workload_segmented_v2 → ComdareSegmentLatencyV2.seg_ns[18]) + Layout-Fix
// (aos_strict Stride 48 vs cache_line_aligned Stride 64). In-process Stand-in (identische vtable/POD-Layout
// wie über die .dll-Grenze) — als „search_algo_grid o.ä." über mehrere reale SA-Kompositionen.
//
// Emittiert die 18-Spalten-CSV via die ECHTEN Writer (lazy_csv_header/format_csv_row, ComdareSegmentLatencyV2)
// nach build/thesis_tiere/all19_pilot.csv und prüft literal:
//   (1) ALLE 18 seg_*_ns > 0 (kein n/a, kein 0) bei jeder Komposition;
//   (2) plausible Variation (nicht alle Segmente identisch; search_algo-Segment differiert zwischen Tieren);
//   (3) Layout-Fix: seg_memory_layout_ns aos_strict vs cache_line_aligned DEUTLICH verschieden (Stride 48 vs 64).
//
// Build: cl /std:c++latest /EHsc /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1 + voller ADHOC-Include-Satz.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/measurable_workload.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <anatomy/composition_factory.hpp>                           // AdHocComposition (Layout-Variante)
#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // lazy_csv_header / format_csv_row / LazyMeasuredRow

#include <compositions/art_reference.hpp>
#include <compositions/hot_reference.hpp>
#include <compositions/masstree_reference.hpp>

// Layout-Fix-Beleg: eine Art-Variante mit aos_strict statt cache_line_aligned (observable Hülle, wie im Original).
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_aos_strict.hpp>
#include <axes/layout/axis_05_memory_layout_observable.hpp>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace ml   = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;

static int  g_fail = 0;
static void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// Art-Variante: identisch zu ArtComposition, NUR memory_layout = aos_strict (Stride 48 statt CLA 64).
struct ArtAosStrictComposition {
    using search_algo        = comp::ArtComposition::search_algo;
    using cache_traversal    = comp::ArtComposition::cache_traversal;
    using mapping            = comp::ArtComposition::mapping;
    using path_compression   = comp::ArtComposition::path_compression;
    using node_type          = comp::ArtComposition::node_type;
    using memory_layout      = ml::ObservableMemoryLayout<ml::AoSStrictMemoryLayout>; // <-- abweichend
    using allocator          = comp::ArtComposition::allocator;
    using prefetch           = comp::ArtComposition::prefetch;
    using concurrency        = comp::ArtComposition::concurrency;
    using serialization      = comp::ArtComposition::serialization;
    using value_handle       = comp::ArtComposition::value_handle;
    using isa                = comp::ArtComposition::isa;
    using index_organization = comp::ArtComposition::index_organization;
    using io_dispatch        = comp::ArtComposition::io_dispatch;
    using migration_policy   = comp::ArtComposition::migration_policy;
    using filter             = comp::ArtComposition::filter;
    using queuing_q1         = comp::ArtComposition::queuing_q1;
    using queuing_q2         = comp::ArtComposition::queuing_q2;
    static constexpr std::string_view paper_id = "P01b ART-aos_strict (Layout-Fix-Beleg)";
    static constexpr std::string_view name     = "ArtAosStrictComposition";
};

template <class C>
static an::ComdareSegmentLatencyV2 measure19(char const* name, std::string& csv_out) {
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                       v3 = dynamic_cast<an::IMeasurableWorkloadV3*>(static_cast<an::IAnatomyBase*>(&tier));
    an::ComdareSegmentLatencyV2 seg{};
    std::uint64_t const         n =
        (v3 != nullptr) ? v3->run_workload_segmented_v2(/*ops*/ 4000, /*batches*/ 64, /*seed*/ 0xC2u, &seg) : 0;

    // Echte CSV-Zeile via format_csv_row (identisches Schema wie der E2E-Treiber).
    ex::LazyMeasuredRow row;
    row.binary_id     = name;
    row.setting_label = "-";
    row.n_ops         = 4000;
    row.total_ns      = seg.total_ns;
    for (int i = 0; i < 18; ++i)
        row.unified.seg_ns[i] = seg.seg_ns[i]; // I1-Konsolidierung: seg-Struct -> unified.seg_ns[18]
    row.unified.seg_framework_ns = seg.seg_framework_ns;
    row.unified.seg_run_total_ns = seg.seg_run_total_ns;
    row.unified.batches_measured = seg.batches_measured;
    row.unified_real             = (n > 0);
    csv_out += ex::format_csv_row(row);

    std::cout << "  " << name << ": batches=" << n << "  seg_ns[T0..T17]=";
    for (int i = 0; i < 18; ++i) std::cout << seg.seg_ns[i] << (i < 17 ? "," : "");
    std::cout << "  total=" << seg.total_ns << "\n";
    return seg;
}

int main() {
    std::cout << "==== (X) per-Achsen-Timer auf ALLE 18 Achsen + Layout-Fix (in-process) ====\n";

    std::string csv  = ex::lazy_csv_header();
    auto        art  = measure19<comp::ArtComposition>("ArtComposition", csv);
    auto        hot  = measure19<comp::HotComposition>("HotComposition", csv);
    auto        mass = measure19<comp::MasstreeComposition>("MasstreeComposition", csv);
    auto        aos  = measure19<ArtAosStrictComposition>("ArtAosStrictComposition", csv);

    // CSV nach build/thesis_tiere/all19_pilot.csv schreiben.
    char const* out_path = "build/thesis_tiere/all19_pilot.csv";
    {
        std::ofstream f{out_path, std::ios::trunc};
        if (f) f << csv;
    }
    std::cout << "CSV: " << out_path << "\n";

    // (1) ALLE 18 seg_*_ns > 0 bei jeder Komposition (kein n/a, kein 0).
    auto all19_pos = [](an::ComdareSegmentLatencyV2 const& s) {
        if (s.batches_measured == 0) return false;
        for (int i = 0; i < 18; ++i)
            if (s.seg_ns[i] <= 0) return false;
        return true;
    };
    tr("Art: alle 18 seg_*_ns > 0", all19_pos(art));
    tr("Hot: alle 18 seg_*_ns > 0", all19_pos(hot));
    tr("Masstree: alle 18 seg_*_ns > 0", all19_pos(mass));
    tr("ArtAosStrict: alle 18 seg_*_ns > 0", all19_pos(aos));

    // (2) plausible Variation: nicht alle 18 Segmente identisch (echte, achsen-spezifische Last).
    auto varies = [](an::ComdareSegmentLatencyV2 const& s) {
        for (int i = 1; i < 18; ++i)
            if (s.seg_ns[i] != s.seg_ns[0]) return true;
        return false;
    };
    tr("Art: Segmente variieren (nicht alle gleich)", varies(art));
    // search_algo-Segment (T0) differiert zwischen Tieren mit verschiedenem Such-Organ.
    bool const sa_differs = (art.seg_ns[0] != hot.seg_ns[0]) || (hot.seg_ns[0] != mass.seg_ns[0]);
    tr("seg_search_algo_ns (T0) differiert zwischen Art/Hot/Masstree", sa_differs);

    // (3) Layout-Fix: seg_memory_layout_ns (Index 5) aos_strict vs cache_line_aligned DEUTLICH verschieden.
    // Die 5%-Schwelle ist die memory_layout-SENSITIVITÄT (Stride 48 vs 64 → verschiedene Cache-Line-Kosten) — eine
    // EIGENE Sache, NICHT die Compiler-Achsen-5% (verschiedene Compiler-Binaries → bis 5% Delta gegen dieselbe
    // Achsenkonfiguration; das wird separat über die Compiler-System-Achse ausgemessen, User-Klärung 2026-07-17).
    // MEDIAN-OF-N stabilisiert die per-Messungs-Varianz (KEINE Schwellen-Aufweichung). Der volle Layout-Effekt (>20%)
    // ist nur UNTER OPTIMIERUNG sichtbar; im unoptimierten build-conf misst der Median konsistent ~3% (< 5%) → dieser
    // Check ist bis Bau-INC-2c.opt (Option A: Optimierungsstufe -O3 als Hardware-System-Achse-Flag, planbasiert)
    // ERWARTET-ROT auf build-conf und wird mit -O3 grün (Effekt dann >5%). Die 5%-Schwelle bleibt exakt.
    constexpr int kMedN       = 7;
    auto          median_seg5 = [&csv](auto tag_composition, char const* nm) -> std::int64_t {
        using CompT = decltype(tag_composition);
        std::int64_t vals[kMedN];
        for (int r = 0; r < kMedN; ++r) vals[r] = measure19<CompT>(nm, csv).seg_ns[5];
        std::sort(vals, vals + kMedN);
        return vals[kMedN / 2];
    };
    std::int64_t const cla  = median_seg5(comp::ArtComposition{}, "ArtComposition");  // cache_line_aligned (Stride 64)
    std::int64_t const aosL = median_seg5(ArtAosStrictComposition{}, "ArtAosStrict"); // aos_strict (Stride 48)
    double const       rel  = (cla > 0) ? (static_cast<double>(aosL - cla) / static_cast<double>(cla)) : 0.0;
    std::cout << "  Layout-Fix (Median-of-" << kMedN << "): seg_memory_layout_ns CLA(64)=" << cla
              << "  aos_strict(48)=" << aosL << "  rel_diff=" << rel << "\n";
    // Stride 64 (CLA) liest mehr Cache-Lines/Record als Stride 48 (aos) → CLA i.d.R. langsamer; jedenfalls
    // dürfen sie NICHT identisch sein (das war der Duplikat-Bug). Toleranz: >5% relativer Unterschied.
    tr("Layout-Fix: aos_strict(48) != cache_line_aligned(64) seg_memory_layout (>5%, Median-of-N)",
       cla > 0 && aosL > 0 && (rel > 0.05 || rel < -0.05));

    // (4) total_ns == Σ der 18 Segmente (Konsistenz).
    auto consistent = [](an::ComdareSegmentLatencyV2 const& s) {
        std::int64_t sum = 0;
        for (int i = 0; i < 18; ++i) sum += s.seg_ns[i];
        return s.total_ns == sum;
    };
    tr("Art: total_ns == Σ 18 Segmente", consistent(art));

    std::cout << "==== 18-Segment-Timer + Layout-Fix: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
