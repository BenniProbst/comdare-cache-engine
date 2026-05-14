#pragma once
// IScheduler — Plattform-Scheduling auf dem Live-Modell (REV 2 §2.6)
// Termin 7 / 22_architektur_skizze_REV5 §2.6

#include <cache_engine/concepts/event.hpp>
#include <cache_engine/platform/core_layout.hpp>

#include <thread>

namespace comdare::cache_engine::platform {

// Generischer Scheduler — entscheidet Thread-zu-Kern + Task-zu-Tier
class IScheduler {
public:
    virtual ~IScheduler() = default;

    virtual void on_event(comdare::cache_engine::Event const& event) noexcept = 0;
    virtual void rebalance() noexcept = 0;

    // Empfehlung: welcher Core soll diesen Thread bekommen?
    [[nodiscard]] virtual CoreId recommend_pin(std::thread::id tid) const noexcept = 0;
};

}  // namespace comdare::cache_engine::platform
