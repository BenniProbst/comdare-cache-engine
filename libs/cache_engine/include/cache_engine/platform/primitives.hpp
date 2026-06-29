#pragma once
// IRankSelectPrimitive + IBranchPredictorModel + IBitManipulationFeatureGate
// Termin 7 / REV 5 K07 (28 fehlende Saeule-B-Concepts)

#include <cstdint>

namespace comdare::cache_engine::platform {

// Rank/Select-Primitive (P09 Jacobson, P10 SuRF, P04 CoCo)
class IRankSelectPrimitive {
public:
    virtual ~IRankSelectPrimitive() = default;

    [[nodiscard]] virtual std::uint64_t rank1(std::uint64_t bit_pos) const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t rank0(std::uint64_t bit_pos) const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t select1(std::uint64_t k) const noexcept     = 0;
    [[nodiscard]] virtual std::uint64_t select0(std::uint64_t k) const noexcept     = 0;
    [[nodiscard]] virtual std::size_t   bit_size() const noexcept                   = 0;
};

// Branch-Predictor-Modell (Hankins/Patel B-Penalty)
class IBranchPredictorModel {
public:
    virtual ~IBranchPredictorModel() = default;

    [[nodiscard]] virtual double        estimated_misprediction_rate() const noexcept = 0;
    [[nodiscard]] virtual double        penalty_cycles_per_miss() const noexcept      = 0;
    [[nodiscard]] virtual std::uint64_t total_predictions() const noexcept            = 0;
    [[nodiscard]] virtual std::uint64_t total_mispredictions() const noexcept         = 0;
};

// Bit-Manipulation-Feature-Gate (BMI2 / popcount / CRC32 etc.)
class IBitManipulationFeatureGate {
public:
    virtual ~IBitManipulationFeatureGate() = default;

    [[nodiscard]] virtual bool has_pext() const noexcept     = 0;
    [[nodiscard]] virtual bool has_pdep() const noexcept     = 0;
    [[nodiscard]] virtual bool has_popcount() const noexcept = 0;
    [[nodiscard]] virtual bool has_crc32() const noexcept    = 0;
    [[nodiscard]] virtual bool has_clmul() const noexcept    = 0;
};

} // namespace comdare::cache_engine::platform
