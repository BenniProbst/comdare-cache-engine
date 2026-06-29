#pragma once
// C06 IAllocationEngine — Allocator-Pool-Verwaltung (F4 + REV 3 K3.2 generisch)
// Termin 7 / 01_md §7 + cache_hierarchy_manager.hpp

#include <cache_engine/concepts/cache_hierarchy_manager.hpp>

#include <cstdint>

namespace comdare::cache_engine::subsystems::allocation {

class IAllocationEngine {
public:
    virtual ~IAllocationEngine() = default;

    [[nodiscard]] virtual void* allocate(comdare::cache_engine::DataTemperature temp, std::size_t bytes,
                                         std::size_t alignment) = 0;

    virtual void deallocate(void* ptr, std::size_t bytes) noexcept = 0;

    // Re-Routing (z.B. bei TierWechsel von Hot → Ultra)
    virtual void migrate(void* ptr, comdare::cache_engine::AllocatorTier from,
                         comdare::cache_engine::AllocatorTier to) noexcept = 0;

    [[nodiscard]] virtual std::uint64_t total_allocated_bytes() const noexcept   = 0;
    [[nodiscard]] virtual std::uint64_t total_deallocated_bytes() const noexcept = 0;
};

} // namespace comdare::cache_engine::subsystems::allocation
