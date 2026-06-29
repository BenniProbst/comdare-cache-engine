#pragma once
// V41.F.6.1.R7.3 axis_08 BlockingConcurrency (coarse-grained mutex / pessimistic)

#include "axis_08_concurrency_strategy_base.hpp"
#include "axis_08_concurrency_subaxes_cc1_to_cc2.hpp"
#include "concepts/axis_08_concurrency_cache_engine_permutation_concept.hpp"
#include <axes/concurrency_axis/axis_08_concurrency_flags.hpp>
#include <topics/concurrency/concepts/topic_concurrency_concept.hpp>
#include <mutex>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::concurrency_axis {

/// BlockingConcurrency — Pessimistic: globaler/coarse-grained Mutex (std::mutex).
/// Einfach + korrekt, aber serialisiert alle Zugriffe. Vergleichs-Obergrenze fuer
/// Locking-Overhead.
class BlockingConcurrency : public ConcurrencyStrategyBase<BlockingConcurrency> {
public:
    using topic_tag = ::comdare::cache_engine::concurrency::concepts::ConcurrencyTopicTag;
    using axis_tag  = subaxes::synchronization_pattern_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::blocking_enabled;

    [[nodiscard]] static constexpr concepts::ConcurrencyPattern concurrency_pattern() noexcept {
        return concepts::ConcurrencyPattern::Blocking;
    }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "concurrency_blocking"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "BlockingConcurrency (coarse-grained mutex, pessimistic)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "BLOCKING"; }

    // V41 F15 Pfad-A — treibbare Concurrency-Op (acquire/release-Paar). Blocking = pessimistisch:
    // ECHTER std::mutex lock()/unlock(). Reale, strategie-abhaengige Laufzeit (Mutex-Bahn auch
    // unkontendiert = teuerster Wrapper, Obergrenze des Locking-Overheads). Das Lock-Primitiv ist
    // thread_local-static (EINE Instanz via lock_(), von acquire UND release geteilt), damit der
    // Wrapper-Typ leer/trivial bleibt (keine Kopier-/ABI-Regression) — die Op exerziert ehrlich
    // genau EINE Lock/Unlock-Bahn (unkontendiert, single-thread-Pfad-A; korrekt gepaart).
    static void acquire() noexcept { lock_().lock(); }
    static void release() noexcept { lock_().unlock(); }

private:
    [[nodiscard]] static std::mutex& lock_() noexcept {
        static thread_local std::mutex m;
        return m;
    }
};

} // namespace comdare::cache_engine::concurrency_axis

namespace comdare::cache_engine::concurrency_axis {
static_assert(concepts::ConcurrencyStrategy<BlockingConcurrency>);
static_assert(concepts::CacheEnginePermutationStrategy<BlockingConcurrency>);
} // namespace comdare::cache_engine::concurrency_axis
