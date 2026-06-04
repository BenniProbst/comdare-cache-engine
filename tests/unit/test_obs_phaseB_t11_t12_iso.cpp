// Phase B (2026-06-04) ISOLIERTE Verifikation T11 value_handle + T12 isa — UMGEHT abi_adapter.hpp (das wegen
// der noch in Arbeit befindlichen parallelen T7-prefetch-Hülle aktuell nicht baut). Prüft NUR meine Bausteine:
//   (1) die Observer-Hüllen ObservableValueHandle<S> / ObservableIsa<S> erfüllen ObservableAxis (snapshot_t +
//       statistics()) und tracken beim observe_* korrekt (strategie-charakteristisch);
//   (2) der echte Slot-Scan-Pfad über NodeChunkedStore::organ_observe_value_handle / organ_observe_isa +
//       ObservableComposedSearch::store_observe_value_handle / store_observe_isa liefert NICHT-leere Statistik;
//   (3) value_handle Indirektions-Charakteristik: Inline=0 indirect, ExternalPool/Versioned>0, ChainRef am höchsten.
// So ist MEINE Vervollständigung unabhängig vom Stand der anderen Phase-B-Agenten build- + verhaltens-verifiziert.

#include <anatomy/observer_aggregate.hpp>                               // ObservableAxis-Concept
#include <axes/value_handle_axis/axis_14_value_handle_observable.hpp>   // ObservableValueHandle + ValueHandleSnapshot
#include <axes/simd/axis_09_isa_observable.hpp>                         // ObservableIsa + IsaStatistics
#include <axes/value_handle_axis/axis_14_value_handle_inline.hpp>
#include <axes/value_handle_axis/axis_14_value_handle_external_pool.hpp>
#include <axes/value_handle_axis/axis_14_value_handle_chain_ref.hpp>
#include <axes/value_handle_axis/axis_14_value_handle_versioned_pointer.hpp>
#include <axes/simd/axis_09_isa_amd64.hpp>
#include <axes/node/axis_04_node_type_chunked_store.hpp>               // NodeChunkedStore (organ_observe_*)
#include <topics/traversal/axis_03a_search_algo/composable/composable_search.hpp>
#include <topics/traversal/axis_03a_search_algo/composable/observable_composed_search.hpp>  // ObservableComposedSearch
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_node4.hpp>

#include <cstdint>
#include <iostream>
#include <string>

namespace an   = ::comdare::cache_engine::anatomy;
namespace vh   = ::comdare::cache_engine::value_handle_axis;
namespace si   = ::comdare::cache_engine::simd;
namespace nd   = ::comdare::cache_engine::node;
namespace ml   = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace al   = ::comdare::cache_engine::allocator::axis_06_allocator;
namespace lk   = ::comdare::cache_engine::lookup::composable;

static int g_fail = 0;
static void tr(std::string const& w, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n"; if (!c) ++g_fail; }

// ObservableAxis-Concept-Selbstbeweis (Compile-Time): die Hüllen tragen snapshot_t + statistics().
static_assert(an::ObservableAxis<vh::ObservableValueHandle<vh::InlineValueHandle>>);
static_assert(an::ObservableAxis<vh::ObservableValueHandle<vh::ChainRefValueHandle>>);
static_assert(an::ObservableAxis<si::ObservableIsa<si::Amd64Isa>>);
// POD-Cross-ABI-Pflicht (NUR uint64).
static_assert(std::is_standard_layout_v<vh::ValueHandleSnapshot> && std::is_trivially_copyable_v<vh::ValueHandleSnapshot>);
static_assert(std::is_standard_layout_v<si::IsaStatistics> && std::is_trivially_copyable_v<si::IsaStatistics>);

// Realer 3-Achsen-Store + komponierter Such-Container (identisch zur abi_adapter-container_t-Konstruktion).
using Store = nd::NodeChunkedStore<nd::Node4NodeType, ml::CacheLineAlignedMemoryLayout, al::MimallocAllocator>;
using Container = lk::ObservableComposedSearch<lk::SortedBinaryTraversal, Store>;

template <class VhStrategy>
static vh::ValueHandleSnapshot drive_vh(char const* name, std::uint64_t& indirect_out) {
    Container c;
    for (std::uint64_t k = 0; k < 500; ++k) (void)c.insert(k, k * 7u + 1u);
    vh::ObservableValueHandle<VhStrategy> organ;
    organ.reset();
    (void)c.store_observe_value_handle(organ);   // ECHTER Slot-Scan über die real gespeicherten Slots
    auto const s = organ.statistics();
    indirect_out = s.indirect_deref_count;
    std::cout << "  vh " << name << ": access=" << s.total_access_count
              << " indirect=" << s.indirect_deref_count << " vtag=" << s.version_tag_strips
              << " depth=" << s.peak_chain_depth << "\n";
    return s;
}

int main() {
    std::cout << "==== Phase B ISO: T11 value_handle + T12 isa (Hülle + echter Slot-Scan) ====\n";

    // ── T11 value_handle: echter Slot-Scan je Strategie + strategie-charakteristische Indirektion ──
    std::uint64_t in_inline = 99, in_pool = 0, in_chain = 0, in_ver = 0;
    auto s_inline = drive_vh<vh::InlineValueHandle>("Inline", in_inline);
    auto s_pool   = drive_vh<vh::ExternalPoolValueHandle>("ExternalPool", in_pool);
    auto s_chain  = drive_vh<vh::ChainRefValueHandle>("ChainRef", in_chain);
    auto s_ver    = drive_vh<vh::VersionedPointerValueHandle>("VersionedPointer", in_ver);

    tr("T11 Inline: total_access_count > 0",        s_inline.total_access_count > 0);
    tr("T11 Inline: indirect_deref_count == 0",     in_inline == 0);                 // Baseline (kein Pointer-Chase)
    tr("T11 Inline: peak_chain_depth == 1",         s_inline.peak_chain_depth == 1);
    tr("T11 ExternalPool: indirect_deref > 0",      in_pool > 0);                    // 1 abhängige Deref
    tr("T11 ExternalPool: peak_chain_depth == 2",   s_pool.peak_chain_depth == 2);
    tr("T11 ChainRef: indirect_deref > ExternalPool", in_chain > in_pool);          // 2 Derefs > 1 Deref
    tr("T11 ChainRef: peak_chain_depth == 3",       s_chain.peak_chain_depth == 3);
    tr("T11 VersionedPointer: version_tag_strips > 0", s_ver.version_tag_strips > 0); // MVCC-Tag-Strip
    tr("T11 access_count gleich über Strategien",   s_inline.total_access_count == s_chain.total_access_count);

    // ── T12 isa: echter SIMD-Feld-Scan über die Slot-Bytes ──
    Container c;
    for (std::uint64_t k = 0; k < 500; ++k) (void)c.insert(k, k * 7u + 1u);
    si::ObservableIsa<si::Amd64Isa> isa_organ;
    isa_organ.reset();
    (void)c.store_observe_isa(isa_organ);
    auto is = isa_organ.statistics();
    std::cout << "  isa Amd64: calls=" << is.simd_calls << " elems=" << is.elements_processed
              << " simd_iter=" << is.simd_iterations << " scalar=" << is.scalar_fallback_count
              << " checksum=" << is.last_checksum << "\n";
    tr("T12 isa: simd_calls > 0",          is.simd_calls > 0);
    tr("T12 isa: elements_processed > 0",  is.elements_processed > 0);
    // Amd64 auf x86_64-Build: SSE2 (lane_width 4) → vec_iters = elems/4, scalar_rest = elems%4. Σ deckt elems ab.
    tr("T12 isa: simd_iters*4 + scalar == elements (Schleifen-Treue)",
       is.simd_iterations * 4 + is.scalar_fallback_count == is.elements_processed);

    std::cout << "==== Phase B ISO T11/T12: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
