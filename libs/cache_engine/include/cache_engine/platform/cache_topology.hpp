#pragma once
// ICacheLevel + ICacheLine + ICacheTopology + ICacheResidency
// Termin 7 / 13_saeule_b_plattform_modell §1+§2+§3 (REV 3 K3.2 generisch)

#include <cstdint>
#include <map>
#include <vector>

namespace comdare::cache_engine::platform {

class ICacheLevel {
public:
    virtual ~ICacheLevel() = default;

    [[nodiscard]] virtual std::size_t   size_bytes()      const noexcept = 0;
    [[nodiscard]] virtual std::size_t   line_size_bytes() const noexcept = 0;
    [[nodiscard]] virtual int           associativity()   const noexcept = 0;
    [[nodiscard]] virtual double        latency_cycles()  const noexcept = 0;
    [[nodiscard]] virtual double        bandwidth_gbps()  const noexcept = 0;
};

enum class CacheLineState : std::uint8_t {
    Modified  = 0,   // M (MESI)
    Exclusive = 1,   // E
    Shared    = 2,   // S
    Invalid   = 3,   // I
    Owned     = 4,   // O (MOESI)
};

class ICacheLine {
public:
    virtual ~ICacheLine() = default;

    [[nodiscard]] virtual std::size_t      size_bytes() const noexcept = 0;
    [[nodiscard]] virtual ICacheLevel*     residency()  const noexcept = 0;
    [[nodiscard]] virtual CacheLineState   state()      const noexcept = 0;
};

class ICacheTopology {
public:
    using CcdId = std::uint16_t;

    virtual ~ICacheTopology() = default;

    [[nodiscard]] virtual std::vector<ICacheLevel*> all_levels() const = 0;
    [[nodiscard]] virtual bool is_asymmetric_l3() const noexcept = 0;
    [[nodiscard]] virtual std::map<CcdId, std::size_t> per_ccd_l3_bytes() const = 0;
};

enum class CacheResidencyTier : std::uint8_t {
    L1Cached     = 0,
    HeaderCached = 1,
    L2Cached     = 2,
    L3Cached     = 3,
    Uncached     = 4,
};

class ICacheResidency {
public:
    virtual ~ICacheResidency() = default;

    // Live: welche ICacheLine ist in welchem ICacheLevel?
    virtual void update_residency(std::uint64_t cache_line_addr,
                                  CacheResidencyTier tier) noexcept = 0;
    [[nodiscard]] virtual CacheResidencyTier query(std::uint64_t cache_line_addr) const noexcept = 0;
};

}  // namespace comdare::cache_engine::platform
