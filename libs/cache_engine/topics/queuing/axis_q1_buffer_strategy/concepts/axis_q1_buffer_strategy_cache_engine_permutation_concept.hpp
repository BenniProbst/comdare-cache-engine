#pragma once
// V41.F.6.1 axis_q1_buffer_strategy cache-engine-Pflicht-Concept (2026-05-26)
//
// @topic queuing
// @achse Q1 buffer_strategy
//
// Parallel zu BufferStrategy: cache-engine-spezifische Pflicht-API analog
// CacheEnginePermutationStrategy bei Allocator. CacheEngineBuilder kann
// daraus pro Vendor Metadaten lesen.
//
// Pflicht-API (IMMER):
//   - typename axis_tag    (QS1-QS6 Subaxis-Tag)
//   - typename family_id   (Q01-Q13 Compile-Time-ID)
//   - static constexpr bool        is_thread_safe()
//   - static constexpr bool        is_bounded()        // FIFO unbounded, Ring bounded
//   - static constexpr std::size_t default_capacity()  // 0 = unbounded oder unbegrenzt-default
//   - static constexpr std::string_view name()
//   - static constexpr std::string_view family_name()
//   - static constexpr std::string_view flag_suffix()  // F.6.1.G CLI
//
// Sonderfall-Properties (analog Allocator [[vendor-sonderfaelle-als-pflicht-property]]):
//   - supports_concurrent_producers()  // SPSC: false, MPMC: true
//   - supports_concurrent_consumers()
//   - supports_priority_ordering()     // PriorityHeap: true
//   - is_versioned()                   // CoW/Epoch/Delta: true
//   - progress_guarantee()             // Stufen-Enum analog Allocator
//
// Mess-API (Pflicht WENN COMDARE_CE_ENABLE_STATISTICS=ON):
//   - statistics() noexcept           -> BufferStatistics
//   - snapshot()   const noexcept     -> snapshot_t
//   - reset()      noexcept           // Statistik-Reset
//   - typename snapshot_t / observer_t
//   - observer() const& -> observer_t const&

#include "axis_q1_buffer_strategy_concept.hpp"
#include "../axis_q1_buffer_strategy_subaxes_qs1_to_qs6.hpp"

#include <measurement/measurable_concept.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::queuing::axis_q1_buffer_strategy::concepts {

/**
 * @brief BufferStatistics — Pflicht-Felder fuer Buffer-Mess-Reihen
 *
 * Welch's t-Test (V41.B3) auslesbar pro Buffer-Vendor.
 */
struct BufferStatistics {
    std::uint64_t total_put_count   = 0;
    std::uint64_t total_get_count   = 0;
    std::uint64_t overflow_count    = 0;
    std::uint64_t underflow_count   = 0;
    std::uint64_t peak_size         = 0;
    double        avg_occupancy     = 0.0;
};

/**
 * @brief ProgressGuarantee — Stufen-Klassifikation analog Allocator-Achse
 *
 * Wiederverwendung des Patterns aus axis_06_allocator (User-Direktive
 * [[vendor-sonderfaelle-als-pflicht-property]] Stufen-Pattern).
 */
enum class ProgressGuarantee : int {
    Blocking        = 0,
    ObstructionFree = 1,
    LockFree        = 2,
    WaitFree        = 3,
};

/**
 * @brief CacheEngineBufferPermutationStrategy — cache-engine-Pflicht-API
 */
template <typename B>
concept CacheEngineBufferPermutationStrategy =
    ::comdare::cache_engine::queuing::concepts::QueuingComponent<B>
    && requires {
        typename B::axis_tag;
        typename B::family_id;
        { B::is_thread_safe()       } -> std::convertible_to<bool>;
        { B::is_bounded()           } -> std::convertible_to<bool>;
        { B::default_capacity()     } -> std::convertible_to<std::size_t>;
        { B::name()                 } -> std::convertible_to<std::string_view>;
        { B::family_name()          } -> std::convertible_to<std::string_view>;
        { B::flag_suffix()          } -> std::convertible_to<std::string_view>;
        // Sonderfall-Properties Pflicht ([[vendor-sonderfaelle-als-pflicht-property]])
        { B::supports_concurrent_producers() } -> std::convertible_to<bool>;
        { B::supports_concurrent_consumers() } -> std::convertible_to<bool>;
        { B::supports_priority_ordering()    } -> std::convertible_to<bool>;
        { B::is_versioned()                  } -> std::convertible_to<bool>;
        { B::progress_guarantee()            } -> std::convertible_to<ProgressGuarantee>;
    }
#ifdef COMDARE_CE_ENABLE_STATISTICS
    && requires(B b, B const& bc) {
        { bc.statistics() } noexcept;
        { b.reset()       } noexcept;
    }
    && requires {
        typename B::snapshot_t;
        typename B::observer_t;
    }
    && std::same_as<typename B::observer_t,
                    ::comdare::cache_engine::measurement::MeasurableObserver<typename B::snapshot_t>>
    && requires(B const& bc) {
        { bc.observer() } noexcept -> std::same_as<typename B::observer_t const&>;
    }
#endif
    ;

}  // namespace
