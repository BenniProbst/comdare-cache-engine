#pragma once
// i_executing_engine.hpp - REV 5 IExecutingEngine (Wurzel-Abstraktion)
// ISearchEngine + IFutureEngine erben hiervon.

#include "cache_engine/concepts/i_cache_engine.hpp"

#include <cstdint>

namespace comdare::prt_art {

/// EngineLifecycleState - Ablaufzustand der Engine
enum class EngineLifecycleState : std::uint8_t {
    Uninitialized, Warming, Running, Idle, Shutdown
};

/// IExecutingEngine - generische Wurzel, konsumiert cache_engine
class IExecutingEngine {
protected:
    comdare::cache_engine::ICacheEngine* cache_engine_ = nullptr;
    EngineLifecycleState lifecycle_state_ = EngineLifecycleState::Uninitialized;

public:
    virtual ~IExecutingEngine() = default;

    virtual void warm_up() = 0;
    virtual void reset() = 0;
    virtual void shutdown() = 0;

    void bind_cache_engine(comdare::cache_engine::ICacheEngine* engine) noexcept {
        cache_engine_ = engine;
    }
    void unbind_cache_engine() noexcept { cache_engine_ = nullptr; }

    [[nodiscard]] comdare::cache_engine::ICacheEngine* cache_engine() const noexcept {
        return cache_engine_;
    }

    [[nodiscard]] EngineLifecycleState lifecycle_state() const noexcept {
        return lifecycle_state_;
    }
};

}  // namespace comdare::prt_art
