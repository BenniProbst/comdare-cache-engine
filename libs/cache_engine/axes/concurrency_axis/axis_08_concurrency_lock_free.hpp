#pragma once
// V41.F.6.1.R7.3 axis_08 LockFreeConcurrency (CAS-basiert, lock-free)

#include "axis_08_concurrency_strategy_base.hpp"
#include "axis_08_concurrency_subaxes_cc1_to_cc2.hpp"
#include "concepts/axis_08_concurrency_cache_engine_permutation_concept.hpp"
#include <axes/concurrency_axis/axis_08_concurrency_flags.hpp>
#include <topics/concurrency/concepts/topic_concurrency_concept.hpp>
#include <atomic>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::concurrency_axis {

/// LockFreeConcurrency — Lock-free via Compare-And-Swap (std::atomic CAS-Loops).
/// System-weiter Fortschritt garantiert (mindestens ein Thread macht Fortschritt).
class LockFreeConcurrency : public ConcurrencyStrategyBase<LockFreeConcurrency> {
public:
    using topic_tag = ::comdare::cache_engine::concurrency::concepts::ConcurrencyTopicTag;
    using axis_tag  = subaxes::synchronization_pattern_tag;
    using family_id = std::integral_constant<int, 5>;

    static constexpr bool enabled = flags::lock_free_enabled;

    [[nodiscard]] static constexpr concepts::ConcurrencyPattern concurrency_pattern() noexcept {
        return concepts::ConcurrencyPattern::LockFree;
    }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "concurrency_lock_free"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "LockFreeConcurrency (CAS-based, lock-free progress guarantee)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "LOCK_FREE"; }

    // V41 F15 Pfad-A — treibbare Concurrency-Op (acquire/release-Paar). LockFree = ECHTE
    // Compare-And-Swap-Schleife: acquire spinnt 0→1 (compare_exchange_weak, acquire-Order),
    // release stored 0 (release-Order). Reale, strategie-abhaengige Laufzeit (atomare CAS-Bahn:
    // billiger als Mutex-Syscall-Bahn, teurer als no-op → distinkt). Single-thread-Pfad-A: das
    // CAS gelingt im ersten Versuch (kein Live-Lock), exerziert aber die echte atomare RMW-Op.
    // Flag thread_local-static (EINE Instanz via flag_(), von acquire UND release geteilt).
    static void acquire() noexcept {
        std::atomic<unsigned>& f = flag_();
        unsigned expected = 0;
        while (!f.compare_exchange_weak(expected, 1u, std::memory_order_acquire,
                                        std::memory_order_relaxed)) {
            expected = 0;  // CAS-Retry (im Pfad-A-Single-Thread im 1. Versuch erfolgreich)
        }
    }
    static void release() noexcept { flag_().store(0u, std::memory_order_release); }

private:
    [[nodiscard]] static std::atomic<unsigned>& flag_() noexcept {
        static thread_local std::atomic<unsigned> f{0u};
        return f;
    }
};

}  // namespace

namespace comdare::cache_engine::concurrency_axis {
    static_assert(concepts::ConcurrencyStrategy<LockFreeConcurrency>);
    static_assert(concepts::CacheEnginePermutationStrategy<LockFreeConcurrency>);
}
