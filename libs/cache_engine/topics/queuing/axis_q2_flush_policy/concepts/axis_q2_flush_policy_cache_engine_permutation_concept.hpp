#pragma once
// V41.F.6.1 axis_q2_flush_policy cache-engine-Pflicht-Concept (2026-05-26)
// @topic queuing @achse Q2

#include "axis_q2_flush_policy_concept.hpp"
#include "../axis_q2_flush_policy_subaxes_fs1_to_fs3.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::queuing::axis_q2_flush_policy::concepts {

/**
 * @brief CacheEngineFlushPolicyPermutationStrategy — cache-engine-Pflicht-API
 *
 * Sonderfall-Properties:
 *   - is_time_based()    — Time-Window-Policy?
 *   - is_threshold_based() — Watermark-Policy?
 *   - is_event_driven()  — Event-getrieben (eager/lazy)?
 *   - is_adaptive()      — Lernt aus Workload?
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
    };

}  // namespace
