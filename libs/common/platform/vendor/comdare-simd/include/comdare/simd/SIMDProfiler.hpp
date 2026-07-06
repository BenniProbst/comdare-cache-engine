#pragma once
/**
 * @file SIMDProfiler.hpp
 * @brief Runtime execution path profiling for SIMD operations (HR9-017-sub-02)
 *
 * Part of comdare-simd (ModuleBaseline0-Foundation)
 * Provides:
 *   - Per-call execution tracing (function, backend, element count)
 *   - Opt-in profiling (zero overhead when disabled)
 *   - RAII scope guard for automatic timing
 *   - Summary report generation
 */

#include "SIMDCapabilities.hpp"

#include <chrono>
#include <vector>
#include <mutex>
#include <cstdint>

namespace comdare::simd {

// =============================================================================
// Call Trace Record
// =============================================================================

struct CallTrace {
    const char* function_name;   ///< e.g. "add_u64", "bitwise_and_u64"
    Backend     backend_used;    ///< Which backend executed this call
    uint64_t    element_count;   ///< Number of elements processed
    uint64_t    duration_ns;     ///< Execution time in nanoseconds
};

// =============================================================================
// SIMD Profiler
// =============================================================================

/**
 * Opt-in profiler for SIMD dispatch tracing.
 * When disabled (default), all operations are no-ops with zero overhead.
 * When enabled, records per-call traces with timing information.
 */
class SIMDProfiler {
public:
    void enable()  { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool is_enabled() const { return enabled_; }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        traces_.clear();
    }

    void record(const char* func, Backend backend,
                uint64_t count, uint64_t duration_ns) {
        if (!enabled_) return;
        std::lock_guard<std::mutex> lock(mutex_);
        traces_.push_back({func, backend, count, duration_ns});
    }

    /// Get a snapshot of all traces (thread-safe copy)
    std::vector<CallTrace> traces() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return traces_;
    }

    size_t trace_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return traces_.size();
    }

    /// Total nanoseconds spent in a specific backend
    uint64_t total_ns_for_backend(Backend b) const {
        std::lock_guard<std::mutex> lock(mutex_);
        uint64_t total = 0;
        for (const auto& t : traces_) {
            if (t.backend_used == b) total += t.duration_ns;
        }
        return total;
    }

    /// Total elements processed by a specific backend
    uint64_t total_elements_for_backend(Backend b) const {
        std::lock_guard<std::mutex> lock(mutex_);
        uint64_t total = 0;
        for (const auto& t : traces_) {
            if (t.backend_used == b) total += t.element_count;
        }
        return total;
    }

private:
    bool enabled_ = false;
    mutable std::mutex mutex_;
    std::vector<CallTrace> traces_;
};

/// Global profiler instance
inline SIMDProfiler& global_profiler() {
    static SIMDProfiler profiler;
    return profiler;
}

// =============================================================================
// RAII Scope Guard for Profiling
// =============================================================================

/**
 * RAII guard that measures execution time and records a trace on destruction.
 * When profiling is disabled, construction and destruction are trivially cheap.
 */
class ProfileScope {
public:
    ProfileScope(const char* func, Backend backend, uint64_t count)
        : func_(func), backend_(backend), count_(count),
          active_(global_profiler().is_enabled())
    {
        if (active_) {
            start_ = std::chrono::steady_clock::now();
        }
    }

    ~ProfileScope() {
        if (active_) {
            auto end = std::chrono::steady_clock::now();
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                end - start_).count();
            global_profiler().record(func_, backend_, count_,
                                     static_cast<uint64_t>(ns));
        }
    }

    // Non-copyable, non-movable
    ProfileScope(const ProfileScope&) = delete;
    ProfileScope& operator=(const ProfileScope&) = delete;

private:
    const char* func_;
    Backend backend_;
    uint64_t count_;
    bool active_;
    std::chrono::steady_clock::time_point start_;
};

} // namespace comdare::simd
