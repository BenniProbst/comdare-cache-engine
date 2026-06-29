#pragma once
// OLCMechanic — P01/P08 Optimistic Lock Coupling (5-Schritt-Protokoll)
// Termin 7 / 02_uml_cache_engine §3

#include <cache_engine/concepts/i_concurrency_mechanic.hpp>

#include <atomic>

namespace comdare::cache_engine {

class OLCMechanic final : public IConcurrencyMechanic {
public:
    [[nodiscard]] ConcurrencyMechanicKind kind() const noexcept override { return ConcurrencyMechanicKind::OLC; }

    void begin_read() noexcept override { captured_version_ = global_version_.load(std::memory_order_acquire); }

    void end_read() noexcept override {
        // Validate: version unveraendert?
        std::uint64_t current = global_version_.load(std::memory_order_acquire);
        valid_                = (current == captured_version_);
    }

    void begin_write() noexcept override {
        // Sperre erwerben (vereinfacht — atomic increment fuer Write-Marker)
        global_version_.fetch_add(1, std::memory_order_acq_rel);
    }

    void end_write() noexcept override { global_version_.fetch_add(1, std::memory_order_acq_rel); }

    [[nodiscard]] bool          last_read_valid() const noexcept { return valid_; }
    [[nodiscard]] std::uint64_t version() const noexcept { return global_version_.load(std::memory_order_relaxed); }

private:
    std::atomic<std::uint64_t> global_version_{0};
    std::uint64_t              captured_version_ = 0;
    bool                       valid_            = true;
};

} // namespace comdare::cache_engine
