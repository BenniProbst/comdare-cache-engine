#pragma once
// C12 IFilterEngine — Approximate-Membership-Filter (F29 SuRF + Bloom + Cuckoo + ARF)
// Termin 7 / 11_md F29

#include <cstdint>

namespace comdare::cache_engine::subsystems::filter {

enum class FilterKind : std::uint8_t {
    SurfBase      = 0, // P10 SuRF-Base
    SurfHash      = 1, // P10 SuRF-Hash
    SurfReal      = 2, // P10 SuRF-Real
    SurfMixed     = 3, // P10 SuRF-Mixed
    Bloom         = 4,
    CountingBloom = 5,
    Cuckoo        = 6,
    Arf           = 7,
};

class IFilterEngine {
public:
    virtual ~IFilterEngine() = default;

    [[nodiscard]] virtual FilterKind kind() const noexcept = 0;

    // Insert/Lookup. Filter haben One-Sided-Error-Garantie (kein false negative).
    virtual void               insert(std::uint64_t key) noexcept            = 0;
    [[nodiscard]] virtual bool may_contain(std::uint64_t key) const noexcept = 0;

    // Konfigurierbare False-Positive-Rate
    [[nodiscard]] virtual double      current_fpr_estimate() const noexcept = 0;
    [[nodiscard]] virtual std::size_t bit_size() const noexcept             = 0;
};

} // namespace comdare::cache_engine::subsystems::filter
