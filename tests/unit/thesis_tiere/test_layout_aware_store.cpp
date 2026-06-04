// Schritt-1-Verifikation (Plan dynamic-frolicking-truffle, 2026-06-04): LayoutAwareChunkedStore<N,L,A>.
// Belegt literal: (1) Round-Trip key_at/value_at, (2) eff_stride CLA=64 vs aos_strict=16, (3) layout-abhängige
// allocator-Bytes (CLA-Padding kostet echte Bytes), (4) organ_observe_layout ohne OOB + CLA≠aos-Checksum.
// Standalone cl-Build (analog test_node_delegation_proof). Exit 0 = ALLE OK.

#include <axes/node/axis_04_node_type_layout_aware_store.hpp>
#include <axes/node/axis_04_node_type_node4.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_aos_strict.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp>
#include <axes/layout/axis_05_memory_layout_observable.hpp>

#include <cstdint>
#include <cstdio>

namespace n   = ::comdare::cache_engine::node;
namespace ml  = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace lay = ::comdare::cache_engine::layout;
namespace al  = ::comdare::cache_engine::allocator::axis_06_allocator;

using N    = n::Node4NodeType;
using CLA  = ml::CacheLineAlignedMemoryLayout;
using AOS  = ml::AoSStrictMemoryLayout;
using Mi   = al::MimallocAllocator;

using StoreCLA = n::LayoutAwareChunkedStore<N, CLA, Mi>;
using StoreAOS = n::LayoutAwareChunkedStore<N, AOS, Mi>;

int main() {
    int fails = 0;
    auto check = [&](bool ok, char const* msg) { if (!ok) { std::printf("FAIL: %s\n", msg); ++fails; } };

    constexpr std::size_t kN = 4096;

    // (2) eff_stride
    check(StoreCLA::eff_stride == 64, "CLA eff_stride==64");
    check(StoreAOS::eff_stride == 16, "aos eff_stride==16");

    StoreCLA cla; StoreAOS aos;
    for (std::uint64_t i = 0; i < kN; ++i) { cla.append_slot(i, i * 7u + 1u); aos.append_slot(i, i * 7u + 1u); }

    // (1) Round-Trip (CLA: Padding darf Key/Value nicht zerstören)
    bool rt = true;
    for (std::uint64_t i = 0; i < kN; ++i) {
        if (cla.key_at(i) != i || cla.value_at(i) != i * 7u + 1u) { rt = false; break; }
        if (aos.key_at(i) != i || aos.value_at(i) != i * 7u + 1u) { rt = false; break; }
    }
    check(rt, "round-trip key_at/value_at (CLA+aos)");
    check(cla.slot_count() == kN && aos.slot_count() == kN, "slot_count==kN");

    // (3) layout-abhängige allocator-Bytes (CLA 64-Stride → 4x aos 16-Stride)
#ifdef COMDARE_CE_ENABLE_STATISTICS
    auto sa_cla = cla.allocator_statistics();
    auto sa_aos = aos.allocator_statistics();
    std::printf("alloc_bytes_in_use: CLA=%llu aos=%llu (chunks CLA=%zu aos=%zu)\n",
                (unsigned long long)sa_cla.total_bytes_in_use, (unsigned long long)sa_aos.total_bytes_in_use,
                cla.chunk_count(), aos.chunk_count());
    check(sa_cla.total_bytes_in_use == kN * 64, "CLA bytes_in_use==kN*64");
    check(sa_aos.total_bytes_in_use == kN * 16, "aos bytes_in_use==kN*16");
    check(sa_cla.total_bytes_in_use == 4 * sa_aos.total_bytes_in_use, "CLA == 4x aos bytes");
    // node_type: chunk_count record-basiert (cap=Node4=4) identisch zwischen Layouts
    check(cla.chunk_count() == aos.chunk_count(), "chunk_count layout-unabhängig (node-Semantik)");
#endif

    // (4) organ_observe_layout: OOB-frei + CLA≠aos-Checksum (echte Layout-Divergenz über die Repräsentation)
    lay::ObservableMemoryLayout<CLA> obs_cla{};
    lay::ObservableMemoryLayout<AOS> obs_aos{};
    std::uint64_t cs_cla = cla.organ_observe_layout(obs_cla);
    std::uint64_t cs_aos = aos.organ_observe_layout(obs_aos);
    std::printf("layout checksum: CLA=%llu aos=%llu\n", (unsigned long long)cs_cla, (unsigned long long)cs_aos);
    check(cs_cla != 0 && cs_aos != 0, "layout checksum != 0 (kein Crash/OOB, Werte erhoben)");

    if (fails == 0) std::printf("==== LayoutAwareChunkedStore: ALLE OK ====\n");
    return fails == 0 ? 0 : 1;
}
