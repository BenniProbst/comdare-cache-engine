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
// @task V41.F.6.1 R5.G

#include <builder/codegen/adhoc_emitter.hpp>
#include <builder/codegen/all_axes_umbrella.hpp>   // alle Achsen-Typen + AdHocComposition + ADHOC-Makro
#include <anatomy/search_algorithm_permutation_engine.hpp>

#include <boost/mp11.hpp>

#include <iostream>
#include <string>

namespace ce  = ::comdare::cache_engine;
namespace ana = ::comdare::cache_engine::anatomy;
namespace cg  = ::comdare::cache_engine::builder::codegen;
namespace mp  = ::boost::mp11;

namespace {

// 17 Achsen-Default-Typen (via Umbrella verfügbar).
using SA0 = ce::traversal::axis_03a_search_algo::Array256SearchAlgo;       // dense direct-addressed (u8)
using SA1 = ce::traversal::axis_03a_search_algo::VectorU8U8SearchAlgo;     // sorted lower_bound (u8)
using SA2 = ce::traversal::axis_03a_search_algo::VectorU16U16SearchAlgo;   // sorted lower_bound (u16)
using SA3 = ce::traversal::axis_03a_search_algo::Array65535SearchAlgo;     // dense direct-addressed (u16)
using SA4 = ce::traversal::axis_03a_search_algo::KArySearchAlgo;           // k-ary search (DaMoN 2009)
using SA5 = ce::traversal::axis_03a_search_algo::InterpolationSearchAlgo;  // interpolation (CACM 1978)
using SA6 = ce::traversal::axis_03a_search_algo::EytzingerSearchAlgo;      // cache-conscious BFS (JEA 2017)
using SA7 = ce::traversal::axis_03a_search_algo::SkipListSearchAlgo;       // skip-list (Pugh CACM 1990)
using SA8 = ce::traversal::axis_03a_search_algo::HashSearchAlgo;           // open-addressing Hash (Knuth)
using SA9 = ce::traversal::axis_03a_search_algo::LinearScanSearchAlgo;     // unsorted linear (ART Node4)
using SA10 = ce::traversal::axis_03a_search_algo::BinarySearchTreeSearchAlgo; // unbalancierter BST (Knuth §6.2.2)
using SA11 = ce::traversal::axis_03a_search_algo::BTreeSearchAlgo;            // balancierter B-Baum (Bayer/McCreight 1972)
using CT  = ce::traversal::axis_03b_cache_traversal::LinearFanout;
using MP  = ce::traversal::axis_03m_mapping::DirectPlacement;
using PC  = ce::nodes::axis_02_path_compression::PathCompressionNone;
using NT  = ce::nodes::axis_04_node_type::Node256Layout;
using ML  = ce::memory_layout::axis_05_memory_layout::CacheLineAlignedMemoryLayout;
using AL  = ce::allocator::axis_06_allocator::MimallocAllocator;
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

// Pilot-Raum: nur search_algo variiert (12 Varianten) → 12 Permutationen.
struct C0  { using StaticAxisVariants = mp::mp_list<SA0, SA1, SA2, SA3, SA4, SA5, SA6, SA7, SA8, SA9, SA10, SA11>; };
struct C1  { using StaticAxisVariants = mp::mp_list<CT>;  };
struct C2  { using StaticAxisVariants = mp::mp_list<MP>;  };
struct C3  { using StaticAxisVariants = mp::mp_list<PC>;  };
struct C4  { using StaticAxisVariants = mp::mp_list<NT>;  };
struct C5  { using StaticAxisVariants = mp::mp_list<ML>;  };
struct C6  { using StaticAxisVariants = mp::mp_list<AL>;  };
struct C7  { using StaticAxisVariants = mp::mp_list<PF>;  };
struct C8  { using StaticAxisVariants = mp::mp_list<CC>;  };
struct C9  { using StaticAxisVariants = mp::mp_list<SE>;  };
struct C10 { using StaticAxisVariants = mp::mp_list<TM>;  };
struct C11 { using StaticAxisVariants = mp::mp_list<VH>;  };
struct C12 { using StaticAxisVariants = mp::mp_list<IS>;  };
struct C13 { using StaticAxisVariants = mp::mp_list<IO>;  };
struct C14 { using StaticAxisVariants = mp::mp_list<IOD>; };
struct C15 { using StaticAxisVariants = mp::mp_list<MG>;  };
struct C16 { using StaticAxisVariants = mp::mp_list<FL>;  };

using PilotEngine = ana::SearchAlgorithmPermutationEngine<
    C0, C1, C2, C3, C4, C5, C6, C7, C8, C9, C10, C11, C12, C13, C14, C15, C16>;

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: comdare-adhoc-emitter <output-dir>\n"
                     "  Emittiert pro Permutation des Pilot-Raums ein Modul-.cpp.\n";
        return 1;
    }
    auto const files = cg::emit_adhoc_modules<PilotEngine>(argv[1]);
    for (auto const& f : files) std::cout << f.string() << "\n";
    std::cerr << "comdare-adhoc-emitter: " << files.size()
              << " Permutations-Modul-.cpp geschrieben (count=" << PilotEngine::count() << ").\n";
    return files.empty() ? 2 : 0;
}
