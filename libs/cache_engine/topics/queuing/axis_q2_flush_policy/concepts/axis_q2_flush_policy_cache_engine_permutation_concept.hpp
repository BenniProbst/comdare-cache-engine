#pragma once
// V41.F.6.1 axis_q2_flush_policy cache-engine-Pflicht-Concept (2026-05-26)
// @topic queuing @achse Q2

#include "axis_q2_flush_policy_concept.hpp"
#include "../axis_q2_flush_policy_subaxes_fs1_to_fs3.hpp"

#include <measurement/measurable_concept.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::queuing::axis_q2_flush_policy::concepts {

/**
 * @brief FlushPolicyStatistics — Pflicht-Felder fuer Flush-Policy Mess-Reihen
 *
 * Pro Policy zaehlbare Metriken: wie oft wurde FlushDecision ausgeloest,
 * welche Verteilung (Eager-100%, Watermark-irregular, Lazy-0%), avg Buffer-Fill
 * zum Flush-Zeitpunkt.
 */
struct FlushPolicyStatistics {
    std::uint64_t total_decisions_evaluated = 0;
    std::uint64_t full_flush_count          = 0;
    std::uint64_t partial_flush_count       = 0;
    std::uint64_t no_flush_count            = 0;
    std::uint64_t flush_complete_count      = 0;
    double        avg_fill_at_flush         = 0.0;
};

/**
 * @brief CacheEngineFlushPolicyPermutationStrategy — cache-engine-Pflicht-API
 *
 * Sonderfall-Properties:
 *   - is_time_based()    — Time-Window-Policy?
 *   - is_threshold_based() — Watermark-Policy?
 *   - is_event_driven()  — Event-getrieben (eager/lazy)?
 *   - is_adaptive()      — Lernt aus Workload?
 *
 * Mess-API (Pflicht wenn STATISTICS=ON, analog Q1 BufferStrategy):
 *   - statistics() / snapshot() / reset() / observer()
 */
template <typename P>
concept CacheEngineFlushPolicyPermutationStrategy =
    ::comdare::cache_engine::queuing::concepts::QueuingComponent<P>
    && requires {
        typename P::axis_tag;
        typename P::family_id;
        { P::name()                  } -> std::convertible_to<std::string_view>;
        { P::family_name()           } -> std::convertible_to<std::string_view>;
        { P::flag_suffix()           } -> std::convertible_to<std::string_view>;
        // Sonderfall-Properties
        { P::is_time_based()         } -> std::convertible_to<bool>;
        { P::is_threshold_based()    } -> std::convertible_to<bool>;
        { P::is_event_driven()       } -> std::convertible_to<bool>;
        { P::is_adaptive()           } -> std::convertible_to<bool>;
    }
#ifdef COMDARE_CE_ENABLE_STATISTICS
    && requires(P p, P const& pc) {
        { pc.statistics() } noexcept;
        { p.reset() }      noexcept;
    }
    && requires {
        typename P::snapshot_t;
        typename P::observer_t;
    }
    && std::same_as<typename P::observer_t,
                    ::comdare::cache_engine::measurement::MeasurableObserver<typename P::snapshot_t>>
    && requires(P const& pc) {
        { pc.observer() } noexcept -> std::same_as<typename P::observer_t const&>;
    }
#endif
    ;

}  // namespace
