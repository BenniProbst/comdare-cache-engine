#pragma once
// ComdareRcuMechanic — F2 eigene RCU-Implementation (Task #104)
// statt liburcu, Konzept-Quelle: P29 McKenney
// Termin 7 / 02_uml_cache_engine §3

#include <cache_engine/concepts/i_concurrency_mechanic.hpp>

#include <atomic>
#include <thread>

namespace comdare::cache_engine {

class ComdareRcuMechanic final : public IConcurrencyMechanic {
public:
    [[nodiscard]] ConcurrencyMechanicKind kind() const noexcept override { return ConcurrencyMechanicKind::ComdareRcu; }

    void register_thread() noexcept override {
        ++registered_threads_;
        thread_quiescent_state_ = false;
    }

    void deregister_thread() noexcept override {
        if (registered_threads_ > 0) --registered_threads_;
    }

    void begin_read() noexcept override {
        thread_quiescent_state_ = false;
        ++active_readers_;
    }

    void end_read() noexcept override {
        if (active_readers_ > 0) --active_readers_;
        // QSBR: Lese-Ende = Quiescent State
        qsbr_quiescent_state();
    }

    void begin_write() noexcept override {}
    void end_write() noexcept override {}

    void synchronize() noexcept override { synchronize_rcu(); }

    void qsbr_register_thread() noexcept { register_thread(); }
    void qsbr_quiescent_state() noexcept {
        thread_quiescent_state_ = true;
        ++quiescent_states_;
    }

    void synchronize_rcu() noexcept {
        // Vereinfachte synchronize: warte bis alle Reader Quiescent State erreicht haben
        // (Echte Implementierung in Task #104)
        while (active_readers_ != 0) { std::this_thread::yield(); }
    }

    [[nodiscard]] std::size_t active_readers() const noexcept {
        return active_readers_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] std::size_t registered_threads() const noexcept {
        return registered_threads_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] std::uint64_t quiescent_states() const noexcept {
        return quiescent_states_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<std::size_t>        active_readers_{0};
    std::atomic<std::size_t>        registered_threads_{0};
    std::atomic<std::uint64_t>      quiescent_states_{0};
    thread_local static inline bool thread_quiescent_state_ = false;
};

} // namespace comdare::cache_engine
