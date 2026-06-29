#pragma once
// C10 ITopologieEngine — Aggregation Live-Topologie (Cache + Core + Bus)
// Termin 7 / 13_md §1+§4+§6 + ILivePlatformModel

#include <cache_engine/platform/live_platform_model.hpp>

#include <cstdint>

namespace comdare::cache_engine::subsystems::topologie {

class ITopologieEngine {
public:
    virtual ~ITopologieEngine() = default;

    // Bind ans Live-Modell — wird vom CacheEngine im Setup aufgerufen
    virtual void bind_live_model(comdare::cache_engine::platform::ILivePlatformModel* m) = 0;

    [[nodiscard]] virtual comdare::cache_engine::platform::ILivePlatformModel* live_model() const noexcept = 0;

    // Re-Discovery anstossen (Hot-Plug oder geaenderte Plattform-Bedingungen)
    virtual void rediscover() = 0;

    [[nodiscard]] virtual std::uint64_t total_rediscoveries() const noexcept = 0;
};

} // namespace comdare::cache_engine::subsystems::topologie
