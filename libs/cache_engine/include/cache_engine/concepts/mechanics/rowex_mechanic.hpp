#pragma once
// ROWEXMechanic — P02/P08 Read-Optimized Write Exclusion (4-Schritt-Protokoll)
// Termin 7 / 02_uml_cache_engine §3

#include <cache_engine/concepts/i_concurrency_mechanic.hpp>

#include <atomic>

namespace comdare::cache_engine {

class RowexMechanic final : public IConcurrencyMechanic {
public:
    [[nodiscard]] ConcurrencyMechanicKind kind() const noexcept override { return ConcurrencyMechanicKind::ROWEX; }

    void begin_read() noexcept override {
        // Reader haben KEINE atomaren Operationen, KEINE Locks, KEINE Versions-Reads
        ++read_count_;
    }
    void end_read() noexcept override {}

    void begin_write() noexcept override {
        // Writer ist exclusive: Lock erwerben (vereinfacht — exchange auf flag)
        bool expected = false;
        while (
            !writer_lock_.compare_exchange_weak(expected, true, std::memory_order_acquire, std::memory_order_relaxed)) {
            expected = false;
        }
    }

    void end_write() noexcept override { writer_lock_.store(false, std::memory_order_release); }

    [[nodiscard]] std::size_t read_count() const noexcept { return read_count_; }

private:
    std::atomic<bool> writer_lock_{false};
    std::size_t       read_count_ = 0;
};

} // namespace comdare::cache_engine
