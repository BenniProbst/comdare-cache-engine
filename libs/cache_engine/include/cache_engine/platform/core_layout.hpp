#pragma once
// ICpuCore + ICoreLayout + ICoreToThreadMap + IPinningPolicy
// Termin 7 / 13_saeule_b_plattform_modell §4 (REV 3 K3.2 generisch — keine Intel/Ryzen-Spez.)

#include <cstdint>
#include <map>
#include <thread>
#include <vector>

namespace comdare::cache_engine::platform {

using CoreId = std::uint32_t;

enum class CoreClass : std::uint8_t {
    Generic     = 0,
    HighIpc     = 1, // generisch fuer P-Cores / V-Cache-CCDs / "starke" Cores
    LowIpc      = 2, // generisch fuer E-Cores / "schwache" Cores
    Specialized = 3, // generisch fuer Tiles, PEs, RISC-Beschleuniger
};

class ICpuCore {
public:
    virtual ~ICpuCore() = default;

    [[nodiscard]] virtual CoreId    id() const noexcept            = 0;
    [[nodiscard]] virtual CoreClass klass() const noexcept         = 0;
    [[nodiscard]] virtual bool      hyperthreaded() const noexcept = 0;
};

class ICoreLayout {
public:
    virtual ~ICoreLayout() = default;

    [[nodiscard]] virtual std::vector<ICpuCore*> all_cores() const                 = 0;
    [[nodiscard]] virtual int                    total_cores() const noexcept      = 0;
    [[nodiscard]] virtual int                    total_threads() const noexcept    = 0;
    [[nodiscard]] virtual bool                   has_hybrid_cores() const noexcept = 0;
};

enum class PinningPolicyId : std::uint16_t {
    None             = 0,
    LargestL3Ccd     = 1, // generisch fuer X3D (REV 3 K3.2)
    HotPathOnHighIpc = 2, // generisch fuer Hybrid-CPU (REV 3 K3.2)
    NumaLocal        = 3,
    RoundRobin       = 4,
};

class IPinningPolicy {
public:
    virtual ~IPinningPolicy() = default;

    [[nodiscard]] virtual PinningPolicyId id() const noexcept                         = 0;
    [[nodiscard]] virtual CoreId          pin_for(std::thread::id tid) const noexcept = 0;
};

class ICoreToThreadMap {
public:
    virtual ~ICoreToThreadMap() = default;

    virtual void                                            assign(std::thread::id tid, CoreId core) noexcept = 0;
    [[nodiscard]] virtual CoreId                            core_for(std::thread::id tid) const noexcept      = 0;
    [[nodiscard]] virtual std::map<std::thread::id, CoreId> snapshot() const                                  = 0;
};

} // namespace comdare::cache_engine::platform
