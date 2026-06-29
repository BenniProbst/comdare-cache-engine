#pragma once
// AllocatorPermutationFlags - Flag-Bank fuer alle 7 Allokator-Achsen (REV 7 §2.5)
//
// Ergaenzt die bestehenden 10 Banks der CacheEngine-PermutationFlags
// (siehe cache_engine/concepts/permutation_flags.hpp) um 7 Allokator-
// spezifische Banks AA1-AA7. Wird vom CacheEngineBuilder zur Compile-time
// gesetzt + von den Family-Adaptern zur Strategy-Wahl verwendet.

#include <cstdint>

namespace comdare::cache_engine::allocator {

// AA1 - FreeList-Topologie
enum class FreeListTopology : std::uint8_t {
    Unspecified                  = 0,
    FlatLifo_Dlmalloc            = 1,  // A20
    Superblock_Hoard             = 2,  // A01
    Slab_Bufctl                  = 3,  // A02
    LockFreeAnchor_Michael       = 4,  // A03
    PageSharded3List_Mimalloc    = 5,  // A04
    BitmapRun_Jemalloc           = 6,  // A05
    PerThreadCentral_Tcmalloc    = 7,  // A06
    BumpHybrid_Snmalloc          = 8,  // A07
    GlobalLockFreeLifo_Scalloc   = 9,  // A08
    NumaPerNode                  = 10, // A09
    Rpmalloc                     = 11, // A10
    LockFreeThreadCache_Lrmalloc = 12, // A11
    CacheSetDirected_Cama        = 13, // A12
    BitmapAvl_Starmalloc         = 14, // A13
    LifetimeAwareL8_Tcmalloc2024 = 15, // A14
    LocalShared_Hmalloc          = 16, // A15
    PimCoreLocal                 = 17, // A16
    BuddyBitmap                  = 18, // A19
    Ptmalloc_TcacheFastbin       = 19, // A21
    VmemPow2Rb                   = 20, // A23
    MagazineStack                = 21, // A23
    SinglePage_Exgen             = 22, // A18
};

// AA2 - Size-Class-Schema
enum class SizeClassSchema : std::uint8_t {
    Unspecified         = 0,
    PowerOf2            = 1,  // A19
    HoardPowB           = 2,  // A01
    ObjectCacheSlab     = 3,  // A02
    TieredJemalloc      = 4,  // A05
    Tcmalloc88          = 5,  // A06
    PageDirectMimalloc  = 6,  // A04
    VirtualSpanScalloc  = 7,  // A08
    Snmalloc3Tier       = 8,  // A07
    FineGrained8B_Exgen = 9,  // A18
    Dlmalloc            = 10, // A20
    PtmallocHierarchy   = 11, // A21
    PmrPoolOptions      = 12, // A22
};

// AA3 - Thread-Locality
enum class ThreadLocality : std::uint8_t {
    Unspecified          = 0,
    None_SingleThread    = 1, // A18, A19, A20
    PerProcessorHeap     = 2, // A01
    Procheap_Michael     = 3, // A03, A11
    TcachePerThread      = 4, // A04, A05, A06 (legacy), A07, A21
    PerCpuRseq           = 5, // A06 modern
    NumaOriginAware      = 6, // A09
    PerCorePim           = 7, // A16
    MagazinePerCpu       = 8, // A23
    HeterogeneousDynamic = 9, // A14
};

// AA4 - Synchronization
enum class Synchronization : std::uint8_t {
    Unspecified                 = 0,
    GlobalLock                  = 1,  // A19, A20 original
    PerHeapLock_Hoard           = 2,  // A01
    PerCacheLock_Slab           = 3,  // A02
    LockFreeCas_Michael         = 4,  // A03, A11
    NoLocks_Mimalloc            = 5,  // A04
    PerArenaLock_Jemalloc       = 6,  // A05, A21
    PerCpuLockless_Rseq         = 7,  // A06 modern
    MessagePassing_Snmalloc     = 8,  // A07
    LockFreeTreiber_Scalloc     = 9,  // A08
    PerNumaNodeLock             = 10, // A09
    LockFree_Rpmalloc           = 11, // A10
    VerifiedLockFree_Starmalloc = 12, // A13
    SharedMutexDefault_Comdare  = 13, // Comdare-Default
    ScopedLockPerPage_Comdare   = 14, // Comdare-Optional Cache-Page-Aware
    PmrSyncVariants             = 15, // A22
};

// AA5 - Allocation-Policy
enum class AllocationPolicy : std::uint8_t {
    Unspecified                = 0,
    FirstFit                   = 1,
    BestFitTree_Dlmalloc       = 2,  // A20
    FullestSuperblock_Hoard    = 3,  // A01
    ActivePartialNewSb_Michael = 4,  // A03, A11
    BumpPointerThenFreelist    = 5,  // A07
    InstantFitVmem             = 6,  // A23
    BestFitVmem                = 7,  // A23
    NextFitVmem                = 8,  // A23
    NumaOriginAware            = 9,  // A09
    CacheSetDirected_Cama      = 10, // A12
    TcacheFastFallthrough      = 11, // A04, A05, A06, A21
    SpanPrioritizationLifetime = 12, // A14
    PimMetadataPimExecuted     = 13, // A16
};

// AA6 - Reclamation
enum class Reclamation : std::uint8_t {
    Unspecified                   = 0,
    BoundaryTagCoalesce           = 1,  // A20, A21
    BuddyXorCoalesce              = 2,  // A19
    EmptyThresholdHoard           = 3,  // A01
    RefcountSlab                  = 4,  // A02
    LockFreeDescAvail_Michael     = 5,  // A03
    DeferredFree_Mimalloc         = 6,  // A04
    DecayBasedPurging             = 7,  // A05 modern
    PageRelease_Tcmalloc          = 8,  // A06
    BatchDispatch_Snmalloc        = 9,  // A07
    SpanFreedAtLastObject_Scalloc = 10, // A08
    NumaOriginReturn              = 11, // A09
    WaitFreeCrystalline           = 12, // A17
    QuarantineHardened            = 13, // A04 smimalloc, A13
    LifetimeAwareHugepageFiller   = 14, // A14
    MagazineResizeVmem            = 15, // A23
    MonotonicNoFree               = 16, // A22 monotonic_buffer_resource
    CoalescenceFree_Hmalloc       = 17, // A15
};

// AA7 - Fragmentation-Strategy
enum class FragmentationStrategy : std::uint8_t {
    Unspecified                    = 0,
    SlabColoring_Bonwick           = 1,  // A02
    BoundedBlowup_Hoard            = 2,  // A01
    CachePageAwareMultiWriter      = 3,  // Comdare + A12 inspired
    GuardPagesHardened             = 4,  // A04 smimalloc, A13
    BuddyInternal50pc              = 5,  // A19
    VirtualSpanReservation         = 6,  // A08
    HugepageFiller                 = 7,  // A06, A14
    IncrementalHugepageNuma        = 8,  // A09
    PageMetadataCompaction_Exgen   = 9,  // A18
    SnmallocSlabMetadata64bit      = 10, // A07
    NucaAwareTransfer_Tcmalloc2024 = 11, // A14
};

// Aggregierte Permutations-Konfiguration fuer einen Allokator-Build
struct AllocatorPermutationFlags {
    FreeListTopology      aa1 = FreeListTopology::Unspecified;
    SizeClassSchema       aa2 = SizeClassSchema::Unspecified;
    ThreadLocality        aa3 = ThreadLocality::Unspecified;
    Synchronization       aa4 = Synchronization::SharedMutexDefault_Comdare; // Default
    AllocationPolicy      aa5 = AllocationPolicy::Unspecified;
    Reclamation           aa6 = Reclamation::Unspecified;
    FragmentationStrategy aa7 = FragmentationStrategy::Unspecified;

    [[nodiscard]] constexpr bool is_specified() const noexcept {
        return aa1 != FreeListTopology::Unspecified && aa2 != SizeClassSchema::Unspecified &&
               aa3 != ThreadLocality::Unspecified && aa4 != Synchronization::Unspecified &&
               aa5 != AllocationPolicy::Unspecified && aa6 != Reclamation::Unspecified &&
               aa7 != FragmentationStrategy::Unspecified;
    }
};

// Pflicht-Permutationen aus Allokator_Matrix.txt (REV 7)
namespace permutations {

constexpr AllocatorPermutationFlags kHoardStyle{
    .aa1 = FreeListTopology::Superblock_Hoard,
    .aa2 = SizeClassSchema::HoardPowB,
    .aa3 = ThreadLocality::PerProcessorHeap,
    .aa4 = Synchronization::PerHeapLock_Hoard,
    .aa5 = AllocationPolicy::FullestSuperblock_Hoard,
    .aa6 = Reclamation::EmptyThresholdHoard,
    .aa7 = FragmentationStrategy::BoundedBlowup_Hoard,
};

constexpr AllocatorPermutationFlags kMimallocStyle{
    .aa1 = FreeListTopology::PageSharded3List_Mimalloc,
    .aa2 = SizeClassSchema::PageDirectMimalloc,
    .aa3 = ThreadLocality::TcachePerThread,
    .aa4 = Synchronization::NoLocks_Mimalloc,
    .aa5 = AllocationPolicy::TcacheFastFallthrough,
    .aa6 = Reclamation::DeferredFree_Mimalloc,
    .aa7 = FragmentationStrategy::SnmallocSlabMetadata64bit, // analog
};

constexpr AllocatorPermutationFlags kTcmallocModern{
    .aa1 = FreeListTopology::PerThreadCentral_Tcmalloc,
    .aa2 = SizeClassSchema::Tcmalloc88,
    .aa3 = ThreadLocality::PerCpuRseq,
    .aa4 = Synchronization::PerCpuLockless_Rseq,
    .aa5 = AllocationPolicy::TcacheFastFallthrough,
    .aa6 = Reclamation::PageRelease_Tcmalloc,
    .aa7 = FragmentationStrategy::HugepageFiller,
};

constexpr AllocatorPermutationFlags kPrtArtPool{
    .aa1 = FreeListTopology::Superblock_Hoard,
    .aa2 = SizeClassSchema::ObjectCacheSlab,
    .aa3 = ThreadLocality::TcachePerThread,
    .aa4 = Synchronization::SharedMutexDefault_Comdare,
    .aa5 = AllocationPolicy::FullestSuperblock_Hoard,
    .aa6 = Reclamation::RefcountSlab,
    .aa7 = FragmentationStrategy::SlabColoring_Bonwick,
};

constexpr AllocatorPermutationFlags kHardenedComdare{
    .aa1 = FreeListTopology::Superblock_Hoard,
    .aa2 = SizeClassSchema::ObjectCacheSlab,
    .aa3 = ThreadLocality::TcachePerThread,
    .aa4 = Synchronization::SharedMutexDefault_Comdare,
    .aa5 = AllocationPolicy::FullestSuperblock_Hoard,
    .aa6 = Reclamation::QuarantineHardened,
    .aa7 = FragmentationStrategy::GuardPagesHardened,
};

constexpr AllocatorPermutationFlags kLocklessComdare{
    .aa1 = FreeListTopology::LockFreeAnchor_Michael,
    .aa2 = SizeClassSchema::PowerOf2,
    .aa3 = ThreadLocality::Procheap_Michael,
    .aa4 = Synchronization::LockFreeCas_Michael,
    .aa5 = AllocationPolicy::ActivePartialNewSb_Michael,
    .aa6 = Reclamation::WaitFreeCrystalline,
    .aa7 = FragmentationStrategy::BoundedBlowup_Hoard,
};

constexpr AllocatorPermutationFlags kNumaComdare{
    .aa1 = FreeListTopology::NumaPerNode,
    .aa2 = SizeClassSchema::Tcmalloc88,
    .aa3 = ThreadLocality::NumaOriginAware,
    .aa4 = Synchronization::PerNumaNodeLock,
    .aa5 = AllocationPolicy::NumaOriginAware,
    .aa6 = Reclamation::NumaOriginReturn,
    .aa7 = FragmentationStrategy::IncrementalHugepageNuma,
};

constexpr AllocatorPermutationFlags kRealtimeComdare{
    .aa1 = FreeListTopology::CacheSetDirected_Cama,
    .aa2 = SizeClassSchema::ObjectCacheSlab,
    .aa3 = ThreadLocality::None_SingleThread,
    .aa4 = Synchronization::SharedMutexDefault_Comdare,
    .aa5 = AllocationPolicy::CacheSetDirected_Cama,
    .aa6 = Reclamation::BoundaryTagCoalesce,
    .aa7 = FragmentationStrategy::SlabColoring_Bonwick,
};

constexpr AllocatorPermutationFlags kSingleThreadOptimum{
    .aa1 = FreeListTopology::SinglePage_Exgen,
    .aa2 = SizeClassSchema::FineGrained8B_Exgen,
    .aa3 = ThreadLocality::None_SingleThread,
    .aa4 = Synchronization::GlobalLock, // no contention
    .aa5 = AllocationPolicy::FirstFit,
    .aa6 = Reclamation::BoundaryTagCoalesce,
    .aa7 = FragmentationStrategy::PageMetadataCompaction_Exgen,
};

} // namespace permutations

} // namespace comdare::cache_engine::allocator
