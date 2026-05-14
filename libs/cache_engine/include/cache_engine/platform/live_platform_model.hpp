#pragma once
// ILivePlatformModel + ILiveCpuModel
// Termin 7 / 02_uml_cache_engine §6 + 13_saeule_b_plattform_modell

#include <cache_engine/platform/cache_topology.hpp>
#include <cache_engine/platform/core_layout.hpp>
#include <cache_engine/platform/interconnect.hpp>
#include <cache_engine/platform/isa_features.hpp>

#include <chrono>
#include <map>

namespace comdare::cache_engine::platform {

// Echtzeit-Modell der Plattform (von der CacheEngine gepflegt)
class ILivePlatformModel {
public:
    virtual ~ILivePlatformModel() = default;

    [[nodiscard]] virtual ICacheTopology*       cache_topology()  const noexcept = 0;
    [[nodiscard]] virtual ICacheResidency*      cache_residency() const noexcept = 0;
    [[nodiscard]] virtual ICoreLayout*          core_layout()     const noexcept = 0;
    [[nodiscard]] virtual ICoreToThreadMap*     core_thread_map() const noexcept = 0;
    [[nodiscard]] virtual IIsaFeatureSet*       isa_features()    const noexcept = 0;
    [[nodiscard]] virtual IBusTopology*         bus_topology()    const noexcept = 0;
    [[nodiscard]] virtual IMemoryBandwidthModel* bandwidth_model() const noexcept = 0;

    // Live-Update-Mechanik
    [[nodiscard]] virtual std::chrono::milliseconds update_period() const noexcept = 0;
    virtual void update_now() noexcept = 0;
    [[nodiscard]] virtual std::uint64_t last_update_count() const noexcept = 0;
};

// Live-Sicht auf Per-Core-Lasten
class ILiveCpuModel {
public:
    virtual ~ILiveCpuModel() = default;

    [[nodiscard]] virtual std::map<CoreId, double> per_core_utilization()     const = 0;
    [[nodiscard]] virtual std::map<CoreId, double> per_core_cache_pressure()  const = 0;
    [[nodiscard]] virtual std::map<CoreId, double> per_core_hot_path_score()  const = 0;
    [[nodiscard]] virtual double                   total_utilization()        const noexcept = 0;
};

}  // namespace comdare::cache_engine::platform
