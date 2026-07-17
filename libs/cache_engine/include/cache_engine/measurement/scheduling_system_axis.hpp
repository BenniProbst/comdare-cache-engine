// measurement/scheduling_system_axis.hpp -- Scheduling als CEB-Konfig-System-Achse (Bau-INC-1c, #37).
//
// Q2-Entscheid (User 2026-07-17): Scheduling ist eine compile-time-CRTP-System-Achse, unter der
// die CEB gebaut wird (kein Runtime-Switch, keine vtable). Die 5 Scheduling-Dimensionen kommen
// als static-constexpr-Properties via Derived (static-dispatch). Die historischen Enums werden
// aus concepts/scheduling_strategy.hpp WIEDERVERWENDET (Single-Source); die dortige DEPRECATED
// Runtime-vtable (ISchedulingStrategy, 0 Konsumenten) bleibt unberuehrt daneben stehen — ihr
// Ersatz ist der koordinierte Bau-INC-2.

#pragma once

#include <cache_engine/concepts/scheduling_strategy.hpp>
#include <cache_engine/measurement/ceb_system_axis.hpp>

#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

template <class Derived>
struct SchedulingSystemAxis : CebSystemAxis<Derived> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "scheduling"; }

    [[nodiscard]] static constexpr concepts::WorkerPoolLayout worker_pool_layout() noexcept {
        return Derived::do_worker_pool_layout();
    }
    [[nodiscard]] static constexpr std::size_t simd_worker_count_limit() noexcept {
        return Derived::do_simd_worker_count_limit();
    }
    [[nodiscard]] static constexpr concepts::HeteroCoreDispatch hetero_core_dispatch() noexcept {
        return Derived::do_hetero_core_dispatch();
    }
    [[nodiscard]] static constexpr concepts::CoRoutineStrategy co_routine_strategy() noexcept {
        return Derived::do_co_routine_strategy();
    }
    [[nodiscard]] static constexpr concepts::BatchGranularity batch_granularity() noexcept {
        return Derived::do_batch_granularity();
    }

protected:
    constexpr SchedulingSystemAxis() noexcept = default;
};

template <class A>
concept SchedulingSystemAxisConcept =
    CebSystemAxisConcept<A> && std::derived_from<A, SchedulingSystemAxis<A>> &&
    std::is_empty_v<SchedulingSystemAxis<A>> && (!std::is_polymorphic_v<SchedulingSystemAxis<A>>) && requires {
        { A::worker_pool_layout() } -> std::same_as<concepts::WorkerPoolLayout>;
        { A::simd_worker_count_limit() } -> std::same_as<std::size_t>;
        { A::hetero_core_dispatch() } -> std::same_as<concepts::HeteroCoreDispatch>;
        { A::co_routine_strategy() } -> std::same_as<concepts::CoRoutineStrategy>;
        { A::batch_granularity() } -> std::same_as<concepts::BatchGranularity>;
    };

/// Erste konkrete Auspraegung (Semantik des historischen DefaultSchedulingStrategy, jetzt
/// compile-time): ThreadPerCore, SIMD-Worker-Limit 2 (hardware-typisch 2 von N Cores),
/// kein Hetero-Dispatch, keine Co-Routinen, Einzel-Granularitaet.
struct DefaultSchedulingSystemAxis final : SchedulingSystemAxis<DefaultSchedulingSystemAxis> {
    [[nodiscard]] static constexpr concepts::WorkerPoolLayout do_worker_pool_layout() noexcept {
        return concepts::WorkerPoolLayout::ThreadPerCore;
    }
    [[nodiscard]] static constexpr std::size_t                  do_simd_worker_count_limit() noexcept { return 2; }
    [[nodiscard]] static constexpr concepts::HeteroCoreDispatch do_hetero_core_dispatch() noexcept {
        return concepts::HeteroCoreDispatch::None;
    }
    [[nodiscard]] static constexpr concepts::CoRoutineStrategy do_co_routine_strategy() noexcept {
        return concepts::CoRoutineStrategy::None;
    }
    [[nodiscard]] static constexpr concepts::BatchGranularity do_batch_granularity() noexcept {
        return concepts::BatchGranularity::Single;
    }
};

static_assert(SchedulingSystemAxisConcept<DefaultSchedulingSystemAxis>);

} // namespace comdare::cache_engine::measurement
