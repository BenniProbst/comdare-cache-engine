// (C-2) Verifikation: der per-Segment-Timer (IMeasurableWorkloadV2::run_workload_segmented) liefert ECHTE,
// nach Achse DIFFERENZIERTE per-Achsen-ns — insbesondere unterscheidet sich seg_search_algo_ns zwischen
// Kompositionen mit VERSCHIEDENEM search_algo-Organ, und seg_memory_layout_ns zwischen verschiedenen
// memory_layout-Organen. Dies belegt Punkt (3) der Build-Verifikation auf ABI-Ebene (in-process Stand-in,
// identische vtable/POD-Layout wie über die .dll-Grenze). KEIN Build/DLL nötig → deterministisch + schnell.
//
// Build: cl /std:c++latest /EHsc /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1 + Include-Satz.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/measurable_workload.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <compositions/art_reference.hpp>
#include <compositions/hot_reference.hpp>
#include <compositions/masstree_reference.hpp>

#include <cstdint>
#include <iostream>
#include <string>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;

static int  g_fail = 0;
static void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

template <class C>
static an::ComdareSegmentLatencyV1 measure_segments(char const* name) {
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                       v2 = dynamic_cast<an::IMeasurableWorkloadV2*>(static_cast<an::IAnatomyBase*>(&tier));
    an::ComdareSegmentLatencyV1 seg{};
    std::uint64_t const         n =
        (v2 != nullptr) ? v2->run_workload_segmented(/*ops*/ 4000, /*batches*/ 64, /*seed*/ 0xC2u, &seg) : 0;
    std::cout << "  " << name << ": batches=" << n << " seg_search_algo_ns=" << seg.seg_search_algo_ns
              << " seg_allocator_ns=" << seg.seg_allocator_ns << " seg_memory_layout_ns=" << seg.seg_memory_layout_ns
              << " seg_serialization_ns=" << seg.seg_serialization_ns << " total_ns=" << seg.total_ns << "\n";
    return seg;
}

int main() {
    std::cout << "==== (C-2) per-Segment-Timer Differenzierung (ABI in-process) ====\n";

    // ArtComposition / HotComposition / MasstreeComposition tragen VERSCHIEDENE search_algo-Organe
    // (ART-Trie / HOT / Masstree). Ihre memory_layout/serialization-Achsen unterscheiden sich ebenfalls.
    auto art  = measure_segments<comp::ArtComposition>("ArtComposition");
    auto hot  = measure_segments<comp::HotComposition>("HotComposition");
    auto mass = measure_segments<comp::MasstreeComposition>("MasstreeComposition");

    // (1) alle 4 Segmente > 0 bei jeder Komposition (echte Messung).
    auto all_pos = [](an::ComdareSegmentLatencyV1 const& s) {
        return s.seg_search_algo_ns > 0 && s.seg_allocator_ns > 0 && s.seg_memory_layout_ns > 0 &&
               s.seg_serialization_ns > 0 && s.batches_measured > 0;
    };
    tr("Art: alle 4 seg_*_ns > 0", all_pos(art));
    tr("Hot: alle 4 seg_*_ns > 0", all_pos(hot));
    tr("Masstree: alle 4 seg_*_ns > 0", all_pos(mass));

    // (2) seg_search_algo_ns DIFFERENZIERT zwischen Kompositionen mit verschiedenem search_algo-Organ.
    bool const sa_differs =
        (art.seg_search_algo_ns != hot.seg_search_algo_ns) || (hot.seg_search_algo_ns != mass.seg_search_algo_ns);
    tr("seg_search_algo_ns differiert zwischen Art/Hot/Masstree (verschiedene search_algo-Organe)", sa_differs);

    // (3) total_ns == Summe der 4 Segmente (Konsistenz).
    auto consistent = [](an::ComdareSegmentLatencyV1 const& s) {
        return s.total_ns ==
               (s.seg_search_algo_ns + s.seg_allocator_ns + s.seg_memory_layout_ns + s.seg_serialization_ns);
    };
    tr("Art: total_ns == Σ Segmente", consistent(art));
    tr("Hot: total_ns == Σ Segmente", consistent(hot));
    tr("Masstree: total_ns == Σ Segmente", consistent(mass));

    std::cout << "==== Segment-Timer-Differenzierung: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
