// V41.F.6.1 R5.G — comdare-adhoc-emitter CLI: emittiert pro Permutation eines Pilot-Raums ein
// kompilierbares Permutations-Modul-.cpp (Umbrella-Include + COMDARE_DEFINE_ANATOMY_MODULE_ADHOC).
//
// Wird zur CMake-Configure-Time aufgerufen (2-Pass-Build, analog comdare-anatomy-codegen-tool); die
// emittierten .cpp werden anschließend per add_library als Permutations-DLLs gebaut. Damit baut der
// volle gemergte Permutations-Raum automatisch zu DLLs — die Skalierung des R5.G-Auto-Emitters.
//
// Pilot-Raum: 12 search_algo × 1^16 = 12 Permutationen (nur die GEMESSENE Achse search_algo variiert
// → 12 head-to-head messbare Kompositionen ueber die VOLLE Paradigmen-Palette: dense/sorted/k-ary/
// interpolation/eytzinger/skip-list/hash/unsortiert-linear/BST/B-Baum — geordnete Struktur jetzt in
// allen drei Balance-Auspraegungen, sodass F15 den Balancierungs- + Block-Orientierungs-Effekt misst).
//
// ⚠️ SUPERSEDED (V41.P2/P3, 2026-05-31) — NICHT die autoritative F15-Quelle:
// Dieser Pilot variiert search_algo über MONOLITHISCHE Tiere (Array256..BTree) + AdHoc-Default-Achsen.
// Das verletzt die Direktive „Achse=Organ, NIE ganze Tiere" (Doku 14 §3.1) und ist NICHT direktiven-konform.
// Planrunde 2026-05-31: Organe sind im AdHoc-Default-Pfad ill-formed (brauchen organ-kompatible Begleit-
// Achsen). Die AUTORITATIVE, direktiven-konforme F15-Mess-Quelle ist die named-Composition-Organ-Messung
// (`comdare_codegen_anatomy_module_list` über die 6 Observable-Organ-Compositions; `f15_compare --pipeline-csv`
// → reale i7-Mess-Zahlen → Pipeline → PDF). Dieser Emitter bleibt als DIAGNOSTISCHER Monolith-Pilot/Smoke-Test
// (R5.G-Skalierungs-Beleg), NICHT für die Diplomarbeits-F15-Headline-Resultate.
//
// @task V41.F.6.1 R5.G · V41.P2 SUPERSEDED

#include <builder/codegen/adhoc_emitter.hpp>
#include <builder/codegen/all_axes_umbrella.hpp> // alle Achsen-Typen + AdHocComposition + ADHOC-Makro
#include <anatomy/search_algorithm_permutation_engine.hpp>

#include <boost/mp11.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace ce  = ::comdare::cache_engine;
namespace ana = ::comdare::cache_engine::anatomy;
namespace cg  = ::comdare::cache_engine::builder::codegen;
namespace mp  = ::boost::mp11;

namespace {

// 17 Achsen-Default-Typen (via Umbrella verfügbar).
using SA0  = ce::traversal::axis_03a_search_algo::Array256SearchAlgo;         // dense direct-addressed (u8)
using SA1  = ce::traversal::axis_03a_search_algo::VectorU8U8SearchAlgo;       // sorted lower_bound (u8)
using SA2  = ce::traversal::axis_03a_search_algo::VectorU16U16SearchAlgo;     // sorted lower_bound (u16)
using SA3  = ce::traversal::axis_03a_search_algo::Array65535SearchAlgo;       // dense direct-addressed (u16)
using SA4  = ce::traversal::axis_03a_search_algo::KArySearchAlgo;             // k-ary search (DaMoN 2009)
using SA5  = ce::traversal::axis_03a_search_algo::InterpolationSearchAlgo;    // interpolation (CACM 1978)
using SA6  = ce::traversal::axis_03a_search_algo::EytzingerSearchAlgo;        // cache-conscious BFS (JEA 2017)
using SA7  = ce::traversal::axis_03a_search_algo::SkipListSearchAlgo;         // skip-list (Pugh CACM 1990)
using SA8  = ce::traversal::axis_03a_search_algo::HashSearchAlgo;             // open-addressing Hash (Knuth)
using SA9  = ce::traversal::axis_03a_search_algo::LinearScanSearchAlgo;       // unsorted linear (ART Node4)
using SA10 = ce::traversal::axis_03a_search_algo::BinarySearchTreeSearchAlgo; // unbalancierter BST (Knuth §6.2.2)
using SA11 = ce::traversal::axis_03a_search_algo::BTreeSearchAlgo; // balancierter B-Baum (Bayer/McCreight 1972)
using CT   = ce::traversal::axis_03b_cache_traversal::LinearFanout;
using MP   = ce::traversal::axis_03m_mapping::DirectPlacement;
using PC   = ce::nodes::axis_02_path_compression::PathCompressionNone;
using NT   = ce::nodes::axis_04_node_type::Node256NodeType;
// V41.F.6.1 R5.B (2026-05-29): 3. variierte Achse = memory_layout. Zwei behavioral-distinkte
// Zugriffsmuster (scan_field_sum): AoS-strided vs SoA-contiguous → echter Cache-Effekt.
using ML0 = ce::memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout; // AoS strided
using ML1 = ce::memory_layout::axis_05_memory_layout::SoAMemoryLayout;              // SoA contiguous
// V41.F.6.1 R5.B (2026-05-29): 2. variierte Achse = allocator. Zwei BEHAVIORAL-distinkte Varianten
// (ohne externes Linking): System-malloc vs. eigener std::pmr-Pool.
using AL0 = ce::allocator::axis_06_allocator::StdMalloc;             // System-malloc (Baseline)
using AL1 = ce::allocator::axis_06_allocator::PoolResourceAllocator; // eigener unsynchronized_pool_resource
using PF  = ce::prefetch::axis_07_prefetch::NonePrefetch;
using CC  = ce::concurrency::axis_08_concurrency::OlcOptimisticConcurrency;
using SE  = ce::serialization::axis_10_serialization::RawBinarySerialization;
using TM  = ce::telemetry::axis_11_telemetry::LeafOnlyCounter;
using VH  = ce::value_handle::axis_14_value_handle::InlineValueHandle;
using IS  = ce::hardware::axis_09_isa::Amd64Isa;
using IO  = ce::search_engine::axis_01_index_organization::IotIndexOrganization;
using IOD = ce::io::axis_io::InMemoryOnly;
using MG  = ce::migration::axis_migration::NoMigration;
using FL  = ce::filter::axis_filter::BloomFilter;
using Q1  = ce::queuing::axis_q1_queuing::NoBuffer;  // T17 queuing_q1 (Doc 30 §8.0: mandatorische SA-Achse, 19-Slot)
using Q2  = ce::queuing::axis_q2_queuing::LazyFlush; // T18 queuing_q2

// Pilot-Raum (R5.B 3-Achsen): search_algo (12) × allocator (2) × memory_layout (2) = 48 Permutationen.
struct C0 {
    using StaticAxisVariants = mp::mp_list<SA0, SA1, SA2, SA3, SA4, SA5, SA6, SA7, SA8, SA9, SA10, SA11>;
};
struct C1 {
    using StaticAxisVariants = mp::mp_list<CT>;
};
struct C2 {
    using StaticAxisVariants = mp::mp_list<MP>;
};
struct C3 {
    using StaticAxisVariants = mp::mp_list<PC>;
};
struct C4 {
    using StaticAxisVariants = mp::mp_list<NT>;
};
struct C5 {
    using StaticAxisVariants = mp::mp_list<ML0, ML1>;
}; // R5.B: 2 memory_layout-Varianten
struct C6 {
    using StaticAxisVariants = mp::mp_list<AL0, AL1>;
}; // R5.B: 2 Allocator-Varianten
struct C7 {
    using StaticAxisVariants = mp::mp_list<PF>;
};
struct C8 {
    using StaticAxisVariants = mp::mp_list<CC>;
};
struct C9 {
    using StaticAxisVariants = mp::mp_list<SE>;
};
struct C10 {
    using StaticAxisVariants = mp::mp_list<TM>;
};
struct C11 {
    using StaticAxisVariants = mp::mp_list<VH>;
};
struct C12 {
    using StaticAxisVariants = mp::mp_list<IS>;
};
struct C13 {
    using StaticAxisVariants = mp::mp_list<IO>;
};
struct C14 {
    using StaticAxisVariants = mp::mp_list<IOD>;
};
struct C15 {
    using StaticAxisVariants = mp::mp_list<MG>;
};
struct C16 {
    using StaticAxisVariants = mp::mp_list<FL>;
};
struct C17 {
    using StaticAxisVariants = mp::mp_list<Q1>;
}; // T17 queuing_q1 (19-Slot-Composition, Doc 30 §8.0)
struct C18 {
    using StaticAxisVariants = mp::mp_list<Q2>;
}; // T18 queuing_q2

using PilotEngine = ana::SearchAlgorithmPermutationEngine<C0, C1, C2, C3, C4, C5, C6, C7, C8, C9, C10, C11, C12, C13,
                                                          C14, C15, C16, C17, C18>;

// ─────────────────────────────────────────────────────────────────────────────
// V41.F.6.1 R5.D — VOLL-COVERAGE-Modus (--full-coverage): 1-wise-Ueberdeckungs-Stichprobe ueber die
// ECHTEN Achsen-Registry-Listen. Deckt ALLE Varianten der drei multi-varianten Achsen
// (search 17 × allocator 25 × layout 5) in max=25 Permutationen statt 2125 ab (each-value-Coverage,
// vgl. anatomy/combinatorial_coverage.hpp). Default bleibt der kuratierte Pilot → keine Regression.
// ─────────────────────────────────────────────────────────────────────────────
namespace fullcov {
using SearchList                  = ce::traversal::axis_03a_search_algo::AllStrategies;
using AllocList                   = ce::allocator::axis_06_allocator::AllVendors;
using LayoutList                  = ce::memory_layout::axis_05_memory_layout::AllLayouts;
inline constexpr std::size_t kNs  = mp::mp_size<SearchList>::value;
inline constexpr std::size_t kNa  = mp::mp_size<AllocList>::value;
inline constexpr std::size_t kNl  = mp::mp_size<LayoutList>::value;
inline constexpr std::size_t kMax = (kNs > kNa ? (kNs > kNl ? kNs : kNl) : (kNa > kNl ? kNa : kNl));

// Zeile r → Achse i = (r mod n_i): jede Variante jeder Achse erscheint mind. 1× (1-wise).
template <class R>
using SampledComposition = ana::AdHocComposition<mp::mp_at_c<SearchList, R::value % kNs>, // T0  search_algo
                                                 CT, MP, PC, NT,                          // T1..T4
                                                 mp::mp_at_c<LayoutList, R::value % kNl>, // T5  memory_layout
                                                 mp::mp_at_c<AllocList, R::value % kNa>,  // T6  allocator
                                                 PF, CC, SE, TM, VH, IS, IO, IOD, MG, FL, // T7..T16
                                                 Q1, Q2>; // T17 queuing_q1, T18 queuing_q2 (19-Slot, Doc 30 §8.0)

using SampleList = mp::mp_transform<SampledComposition, mp::mp_iota_c<kMax>>;

struct SampleEngine {
    [[nodiscard]] static constexpr std::size_t count() noexcept { return mp::mp_size<SampleList>::value; }
    template <class Visitor>
    static constexpr void for_each_composition_type(Visitor&& v) {
        mp::mp_for_each<mp::mp_transform<mp::mp_identity, SampleList>>([&](auto id) {
            using C = typename decltype(id)::type;
            v.template operator()<C>();
        });
    }
};
static_assert(SampleEngine::count() == kMax, "1-wise-Stichprobe muss max(Achsen-Varianten) Permutationen haben");
} // namespace fullcov

} // namespace

namespace {
// Emittiert den Raum des gewaehlten Engine + schreibt manifest.txt (idx → search/allocator/memory_layout).
template <class Engine>
int emit_with(std::string const& out_dir, char const* mode) {
    auto const files = cg::emit_adhoc_modules<Engine>(out_dir);
    for (auto const& f : files) std::cout << f.string() << "\n";
    std::ofstream manifest(std::filesystem::path{out_dir} / "manifest.txt", std::ios::trunc);
    int           mi = 0;
    Engine::for_each_composition_type([&]<class C>() {
        std::string const line = std::to_string(mi) + "\t" + std::string{C::search_algo::name()} + "\t" +
                                 std::string{C::allocator::name()} + "\t" + std::string{C::memory_layout::name()};
        std::cerr << "  [idx " << line << "]\n";
        manifest << line << "\n";
        ++mi;
    });
    std::cerr << "comdare-adhoc-emitter: " << files.size()
              << " Permutations-Modul-.cpp geschrieben (count=" << Engine::count() << ", Modus=" << mode
              << ", manifest.txt geschrieben).\n";
    return files.empty() ? 2 : 0;
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: comdare-adhoc-emitter <output-dir> [--full-coverage]\n"
                     "  Default: kuratierter 3-Achsen-Pilot (search×allocator×memory_layout = 48).\n"
                     "  --full-coverage: 1-wise-Ueberdeckung ueber ALLE Achsen-Varianten "
                     "(17×25×5 → 25 Permutationen, jede Variante mind. 1×).\n";
        return 1;
    }
    std::string const out_dir       = argv[1];
    bool              full_coverage = false;
    for (int i = 2; i < argc; ++i)
        if (std::string{argv[i]} == "--full-coverage") full_coverage = true;

    return full_coverage ? emit_with<fullcov::SampleEngine>(out_dir, "full-coverage-1wise")
                         : emit_with<PilotEngine>(out_dir, "kuratiert-3achsen");
}
