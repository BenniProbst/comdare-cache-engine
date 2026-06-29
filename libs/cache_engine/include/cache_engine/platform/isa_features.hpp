#pragma once
// IHardwareExtension + IIsaFeatureSet
// Termin 7 / 13_saeule_b_plattform_modell §5

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace comdare::cache_engine::platform {

enum class HardwareExtensionFamily : std::uint8_t {
    BaseISA          = 0,
    SimdExtension    = 1,
    BitManipulation  = 2,
    MemoryHints      = 3,
    Synchronization  = 4,
    Security         = 5,
    DatabaseSpecific = 6, // P31 Ungethuem WAH/PLWAH/COMPAX, Hash, MergeSort etc.
};

enum class OperationKind : std::uint8_t {
    Lookup      = 0,
    Insert      = 1,
    Compare     = 2,
    Prefetch    = 3,
    Atomic      = 4,
    Sort        = 5,
    HashCompute = 6,
};

class IHardwareExtension {
public:
    virtual ~IHardwareExtension() = default;

    [[nodiscard]] virtual std::string                     feature_name() const          = 0;
    [[nodiscard]] virtual int                             bit_position() const noexcept = 0;
    [[nodiscard]] virtual HardwareExtensionFamily         family() const noexcept       = 0;
    [[nodiscard]] virtual std::map<OperationKind, double> cost_model() const            = 0;
};

class IIsaFeatureSet {
public:
    virtual ~IIsaFeatureSet() = default;

    [[nodiscard]] virtual std::vector<IHardwareExtension*> extensions() const = 0;

    [[nodiscard]] virtual bool          has_avx2() const noexcept               = 0;
    [[nodiscard]] virtual bool          has_avx512f() const noexcept            = 0;
    [[nodiscard]] virtual bool          has_bmi2() const noexcept               = 0;
    [[nodiscard]] virtual bool          has_popcount() const noexcept           = 0;
    [[nodiscard]] virtual bool          has_neon() const noexcept               = 0;
    [[nodiscard]] virtual bool          has_sve() const noexcept                = 0;
    [[nodiscard]] virtual bool          has_riscv_v() const noexcept            = 0;
    [[nodiscard]] virtual bool          has_software_prefetch() const noexcept  = 0;
    [[nodiscard]] virtual bool          has_branch_free_cmov() const noexcept   = 0;
    [[nodiscard]] virtual bool          has_simd_compare() const noexcept       = 0;
    [[nodiscard]] virtual std::uint16_t virtual_address_bits() const noexcept   = 0;
    [[nodiscard]] virtual std::uint32_t atomic_int64_supported() const noexcept = 0;
};

} // namespace comdare::cache_engine::platform
