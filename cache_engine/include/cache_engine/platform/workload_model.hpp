#pragma once
// IWorkloadModel + ICellProbeModel + IWordProbeModel
// Termin 7 / REV 5 K07 — Workload-Charakterisierung (Bender/P15 Survey/P28 Kuehn)

#include <cstdint>
#include <vector>

namespace comdare::cache_engine::platform {

enum class WorkloadDistribution : std::uint8_t {
    Uniform                = 0,
    Zipfian                = 1,
    Pareto                 = 2,
    SlightSkew             = 3,
    HeavySkew              = 4,
    PrefixHeavy            = 5,   // F8 Praefix-Heavy
    Sequential             = 6,
};

enum class WorkloadOpMix : std::uint8_t {
    PointReadOnly       = 0,
    RangeScanHeavy      = 1,
    InsertHeavy         = 2,
    UpdateHeavy         = 3,
    Mixed               = 4,
    YcsbA               = 10,
    YcsbB               = 11,
    YcsbC               = 12,
};

class IWorkloadModel {
public:
    virtual ~IWorkloadModel() = default;

    [[nodiscard]] virtual WorkloadDistribution distribution() const noexcept = 0;
    [[nodiscard]] virtual WorkloadOpMix         op_mix()       const noexcept = 0;
    [[nodiscard]] virtual double                point_to_scan_ratio() const noexcept = 0;
    [[nodiscard]] virtual std::vector<double>   access_probability_per_node() const = 0;   // P16 Bender
    [[nodiscard]] virtual std::uint64_t         total_keys() const noexcept = 0;
};

// Zellen-Probe-Modell (Theoretisches CPU-Modell fuer Layout-Theorie, P16/P17 Bender)
class ICellProbeModel {
public:
    virtual ~ICellProbeModel() = default;

    [[nodiscard]] virtual std::size_t cell_size_bytes()   const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t total_cells()     const noexcept = 0;
    virtual void note_probe(std::uint64_t cell_id)        noexcept = 0;
    [[nodiscard]] virtual std::uint64_t total_probes()    const noexcept = 0;
};

// Word-Probe-Modell (alternatives Theoretisches Modell)
class IWordProbeModel {
public:
    virtual ~IWordProbeModel() = default;

    [[nodiscard]] virtual std::size_t word_size_bytes()   const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t total_words()     const noexcept = 0;
    virtual void note_probe(std::uint64_t word_id)        noexcept = 0;
    [[nodiscard]] virtual std::uint64_t total_probes()    const noexcept = 0;
};

}  // namespace comdare::cache_engine::platform
