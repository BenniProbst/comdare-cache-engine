#pragma once
// IStorageMedium + IPageCacheModel
// Termin 7 / REV 5 K07 (NVRAM, Persistent Memory, Page-Cache)

#include <cstdint>

namespace comdare::cache_engine::platform {

enum class StorageMediumKind : std::uint8_t {
    Dram        = 0,
    Hbm         = 1,    // P32/P33 Heterogene Memory
    Nvram       = 2,    // Persistent Memory (Optane-aehnlich, REV 5 K08)
    Cxl         = 3,    // CXL.mem
    SsdNvme     = 4,
    HddSpinning = 5,
};

class IStorageMedium {
public:
    virtual ~IStorageMedium() = default;

    [[nodiscard]] virtual StorageMediumKind kind()              const noexcept = 0;
    [[nodiscard]] virtual std::size_t       capacity_bytes()    const noexcept = 0;
    [[nodiscard]] virtual double            read_latency_ns()   const noexcept = 0;
    [[nodiscard]] virtual double            write_latency_ns()  const noexcept = 0;
    [[nodiscard]] virtual double            bandwidth_gbps()    const noexcept = 0;
    [[nodiscard]] virtual bool              is_persistent()     const noexcept = 0;
    [[nodiscard]] virtual bool              has_wearout()       const noexcept = 0;
};

class IPageCacheModel {
public:
    virtual ~IPageCacheModel() = default;

    [[nodiscard]] virtual std::size_t  page_size_bytes() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t resident_pages() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t total_capacity_pages() const noexcept = 0;

    virtual void note_access(std::uint64_t page_id) noexcept = 0;
    [[nodiscard]] virtual bool is_resident(std::uint64_t page_id) const noexcept = 0;
};

}  // namespace comdare::cache_engine::platform
