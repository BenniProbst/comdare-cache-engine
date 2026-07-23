#pragma once
// P33-VAMPIR Nachbau Option A: VampirNfpAllocator (A24)
//
// Source scope: P33 poster plus review describe VAMPIR as "Virtualized
// Non-Functional Memory Properties for Data-Pipeline Scheduling", but provide
// no code, pseudocode, cost formula, scheduling algorithm, or migration API.
// This organ is therefore a structural reconstruction of the buildable
// memory-type-virtualization part only: an allocator front routes allocations
// through an NFP-tagged memory tier and exposes the NFP descriptor metadata.
//
// Faithfulness:
// - is_original=false: no paper-original-code mixin, no paper-code gate, no hash lock.
// - NFP descriptor fields are taken from the review enumeration only:
//   bandwidth_class, latency_class, capacity. The poster is not the source for
//   this enumeration.
// - Energy is omitted; cache-engine has no energy counter here, so adding one
//   would be a fabricated empty metric.
// - Out of scope and intentionally not implemented: Compensation, transparent
//   OS migration/replacement, pipeline ordering, global schedule/negotiation
//   for query scheduling, energy as a metric, and ongoing dynamic replacement.
// - "Micro-allocator", "memory decorator", and "multidimensional" are not
//   claimed VAMPIR source terms; if used around this organ, they are derived
//   cache-engine pattern terms only.
// - This is unrelated to TelemetryVampirOtf2. That type names an OTF2 trace
//   telemetry variant, not the VAMPIR NFP memory virtualization concept.
//
// Hardware note: real DRAM/HBM/NVRAM binding is platform and hardware specific.
// This portable organ preserves the VAMPIR interface substance by carrying the
// tier tag and descriptor, then allocates via cache-engine portable_aligned_alloc.

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include <topics/allocator/concepts/topic_allocator_concept.hpp>

#include <cstddef>
#include <string_view>
#include <type_traits>

#include <axes/alloc/axis_06_allocator_flags.hpp>
#include <cache_engine/allocators/portable_aligned_alloc.hpp>
#include <cache_engine/concepts/cache_recommendation.hpp>
#include <measurement/measurable_concept.hpp>

namespace comdare::cache_engine::alloc {

class VampirNfpAllocator : public AllocatorStrategyBase<VampirNfpAllocator> {
public:
    using value_type = std::byte;
    using size_type  = std::size_t;

    static constexpr bool enabled = flags::vampir_nfp_enabled;

    using topic_tag = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag  = subaxes::allocation_policy_tag;
    using family_id = std::integral_constant<int, 24>;

    struct NfpDescriptor {
        std::string_view bandwidth_class;
        std::string_view latency_class;
        std::size_t      capacity;
    };

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return true; }
    [[nodiscard]] static constexpr bool        supports_pmr() noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t max_alignment() noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept { return "vampir_nfp"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "VAMPIR NFP memory-type virtualization allocator";
    }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view               algo_version = "v1.0.0";
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "VAMPIR_NFP"; }

    [[nodiscard]] static constexpr bool                        has_native_aligned_alloc() noexcept { return true; }
    [[nodiscard]] static constexpr bool                        requires_explicit_init() noexcept { return false; }
    [[nodiscard]] static constexpr bool                        supports_numa_node_hint() noexcept { return false; }
    [[nodiscard]] static constexpr bool                        supports_thread_local_cache() noexcept { return false; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }
    [[nodiscard]] static constexpr bool requires_specialized_hardware() noexcept { return false; }

    [[nodiscard]] static constexpr ::comdare::cache_engine::MemoryAllocationHint::TierKind target_tier() noexcept {
        return ::comdare::cache_engine::MemoryAllocationHint::TierKind::Dram;
    }

    [[nodiscard]] static constexpr NfpDescriptor nfp_descriptor() noexcept {
        return NfpDescriptor{std::string_view{"unspecified"}, std::string_view{"unspecified"}, 0u};
    }

    [[nodiscard]] bool operator==(VampirNfpAllocator const&) const noexcept { return true; }

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        void* p = allocate_on_tier(target_tier(), bytes, alignment);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t const aligned_bytes = aligned_size(bytes, alignment);
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += aligned_bytes;
            stats_.total_bytes_in_use += aligned_bytes;
        } else {
            ++stats_.failure_count;
        }
        observer_.notify(stats_);
#endif
        return p;
    }

    void deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
        if (p == nullptr) return;
        ::comdare::cache_engine::allocator::portable_aligned_free(p);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t const aligned_bytes = aligned_size(bytes, alignment);
        ++stats_.deallocation_count;
        if (aligned_bytes <= stats_.total_bytes_in_use) {
            stats_.total_bytes_in_use -= aligned_bytes;
        } else {
            stats_.total_bytes_in_use = 0;
        }
        observer_.notify(stats_);
#else
        (void)bytes;
        (void)alignment;
#endif
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::AllocationStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;

    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }
#endif

private:
    [[nodiscard]] static void* allocate_on_tier(::comdare::cache_engine::MemoryAllocationHint::TierKind tier,
                                                std::size_t bytes, std::size_t alignment) noexcept {
        (void)tier;
        (void)nfp_descriptor();
        return ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, bytes);
    }

    [[nodiscard]] static constexpr std::size_t aligned_size(std::size_t bytes, std::size_t alignment) noexcept {
        if (alignment == 0u) return bytes;
        return ((bytes + alignment - 1u) / alignment) * alignment;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::AllocationStatistics stats_{};
    observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::alloc

namespace comdare::cache_engine::alloc {
static_assert(concepts::AllocatorStrategy<VampirNfpAllocator>,
              "VampirNfpAllocator must satisfy the axis_06 AllocatorStrategy contract");
static_assert(concepts::CacheEnginePermutationStrategy<VampirNfpAllocator>,
              "VampirNfpAllocator must satisfy the cache-engine permutation strategy contract");
} // namespace comdare::cache_engine::alloc