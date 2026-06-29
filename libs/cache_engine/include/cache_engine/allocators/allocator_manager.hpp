#pragma once
// AllocatorManager - Bindeglied zwischen CacheEngine + Allokator-Familien
//
// Verbindet eine Allokator-Konkretisierung (Family-Adapter) mit der CacheEngine's
// Visitor-Pattern + PermutationFlags + Telemetry-System.
// Pflicht-Komponente fuer Phase 6.2 (REV 7 §1.4) + Phase 6.3 ABI-Modul-Interface.

#include "allocator_permutation_flags.hpp"
#include "concepts/i_allocation_strategy.hpp"
#include "concepts/pmr_resource_concept.hpp"

#include <cstddef>
#include <memory>

namespace comdare::cache_engine::allocator {

// Strategy enthaelt typischerweise std::shared_mutex und ist daher nicht copy-/movable.
// AllocatorManager haelt deshalb eine Reference auf eine vom Caller besessene Strategy.
template <IAllocationStrategy Strategy>
class AllocatorManager {
public:
    explicit AllocatorManager(Strategy& strategy, AllocatorPermutationFlags flags) noexcept
        : strategy_{&strategy}, flags_{flags}, pmr_resource_{&strategy} {}

    AllocatorManager(AllocatorManager const&)            = delete;
    AllocatorManager& operator=(AllocatorManager const&) = delete;

    [[nodiscard]] Strategy&       strategy() noexcept { return *strategy_; }
    [[nodiscard]] Strategy const& strategy() const noexcept { return *strategy_; }

    [[nodiscard]] AllocatorPermutationFlags const& flags() const noexcept { return flags_; }

    [[nodiscard]] CacheEnginePmrResource<Strategy>& pmr_resource() noexcept { return pmr_resource_; }

    [[nodiscard]] std::pmr::memory_resource* as_pmr() noexcept { return &pmr_resource_; }

    [[nodiscard]] AllocationStatistics statistics() const noexcept { return strategy_->statistics(); }

private:
    Strategy*                        strategy_;
    AllocatorPermutationFlags        flags_;
    CacheEnginePmrResource<Strategy> pmr_resource_;
};

} // namespace comdare::cache_engine::allocator
