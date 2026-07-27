// measurement/scheduling_system_axis.hpp -- Scheduling als CEB-Konfig-System-Achse (Bau-INC-1c, #37).
//
// Q2-Entscheid (User 2026-07-17): Scheduling ist eine compile-time-CRTP-System-Achse, unter der
// die CEB gebaut wird (kein Runtime-Switch, keine vtable). Die 5 Scheduling-Dimensionen kommen
// als static-constexpr-Properties via Derived (static-dispatch). Die historischen Enums werden
// aus concepts/scheduling_strategy.hpp WIEDERVERWENDET (Single-Source); die dortige DEPRECATED
// Runtime-vtable (ISchedulingStrategy, 0 Konsumenten) bleibt unberuehrt daneben stehen — ihr
// Ersatz ist der koordinierte Bau-INC-2.

// O-8 Schritt 6 (Bauplan IV.2.2 / Owner OD-2): scheduling ist KEINE System-HAUPT-Achse mehr, sondern
// UNTER-Achse des target_isa-Komplex-Wrappers ("der Komplex-Wrapper erhaelt allein die zugehoerigen
// target_isa Unter-System-Achsen, die zuvor vereinbart waren"). Die Abgangs-Wache in
// abi/system_axis_order.hpp haelt fest, dass sie nicht still als Haupt-Achse zurueckkehrt.
// Der Eltern-Bezug ist ein TYP (CebSubAxis, Paket A1), das parent_axis_label() daraus ABGELEITET --
// die 5 Sub-Dimensionen und ihre Semantik bleiben davon voellig unberuehrt.

#pragma once

#include <cache_engine/concepts/scheduling_strategy.hpp>
#include <cache_engine/measurement/ceb_sub_axis.hpp>
#include <cache_engine/measurement/target_isa_complex_axis.hpp>

#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

template <class Derived>
struct SchedulingSystemAxis : CebSubAxis<Derived, TargetIsaAxisTag> {
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

// O-8 Schritt 6: das Concept fordert jetzt zusaetzlich die UNTER-Achsen-Schicht (CebSubAxisConcept).
// Damit ist der Eltern-Bezug Teil der Zusicherung: eine scheduling-Auspraegung, die nicht mehr am
// target_isa-Komplex haengt, erfuellt das Concept nicht mehr.
template <class A>
concept SchedulingSystemAxisConcept =
    CebSubAxisConcept<A> && std::derived_from<A, SchedulingSystemAxis<A>> && std::is_empty_v<SchedulingSystemAxis<A>> &&
    (!std::is_polymorphic_v<SchedulingSystemAxis<A>>) && requires {
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
// O-8 Schritt 6 EINHAENGUNGS-WACHE: scheduling haengt am target_isa-Komplex, ueber den TYP.
static_assert(DefaultSchedulingSystemAxis::parent_axis_label() == std::string_view{"target_isa"});
static_assert(std::same_as<DefaultSchedulingSystemAxis::parent_axis, TargetIsaAxisTag>);
static_assert(axis_depth_v<DefaultSchedulingSystemAxis> == 1);

} // namespace comdare::cache_engine::measurement
