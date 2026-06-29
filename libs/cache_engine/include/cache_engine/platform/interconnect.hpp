#pragma once
// IInterconnect + IBusTopology + IMemoryBandwidthModel
// Termin 7 / 13_saeule_b_plattform_modell §6

#include <cstdint>
#include <map>
#include <vector>

namespace comdare::cache_engine::platform {

enum class InterconnectClass : std::uint8_t {
    IntraCcd  = 0,
    InterCcd  = 1,
    MemoryBus = 2,
    NumaLink  = 3,
    PcIe      = 4,
    NvLink    = 5,
};

class IInterconnect {
public:
    virtual ~IInterconnect() = default;

    [[nodiscard]] virtual std::uint64_t     source_id() const noexcept      = 0;
    [[nodiscard]] virtual std::uint64_t     target_id() const noexcept      = 0;
    [[nodiscard]] virtual double            bandwidth_gbps() const noexcept = 0;
    [[nodiscard]] virtual double            latency_ns() const noexcept     = 0;
    [[nodiscard]] virtual InterconnectClass klass() const noexcept          = 0;
};

class IBusTopology {
public:
    virtual ~IBusTopology() = default;

    [[nodiscard]] virtual std::vector<IInterconnect*> all_interconnects() const = 0;
    [[nodiscard]] virtual bool has_inter_chiplet_link() const noexcept          = 0; // Block AO Ryzen Infinity Fabric
    [[nodiscard]] virtual bool has_hybrid_intra_cluster() const noexcept        = 0; // Block AO i9 Ring-Bus
};

class IMemoryBandwidthModel {
public:
    virtual ~IMemoryBandwidthModel() = default;

    [[nodiscard]] virtual std::map<IInterconnect*, double> per_link_capacity() const          = 0;
    [[nodiscard]] virtual std::map<IInterconnect*, double> current_utilization() const        = 0;
    [[nodiscard]] virtual IInterconnect*                   detect_bottleneck() const noexcept = 0;
};

} // namespace comdare::cache_engine::platform
