#pragma once
/**
 * @file SIMDCapabilities.hpp
 * @brief Runtime SIMD capability query and usage tracking (HR9-017-sub-01)
 *
 * Part of comdare-simd (ModuleBaseline0-Foundation)
 * Provides:
 *   - Compile-time capability flags (which backends are compiled in)
 *   - Runtime call counters per backend (which backends are actually USED)
 *   - Human-readable capability string
 */

#include <cstdint>
#include <atomic>

namespace comdare::simd {

// =============================================================================
// Backend Enum
// =============================================================================

enum class Backend : uint8_t {
    Scalar  = 0,
    AVX2    = 1,
    AVX512  = 2,
    NEON    = 3,
    Count   = 4
};

inline const char* backend_name(Backend b) {
    switch (b) {
        case Backend::Scalar: return "scalar";
        case Backend::AVX2:   return "avx2";
        case Backend::AVX512: return "avx512";
        case Backend::NEON:   return "neon";
        default:              return "unknown";
    }
}

// =============================================================================
// Compile-Time Capability Flags
// =============================================================================

struct Capabilities {
    bool has_avx512 = false;
    bool has_avx2   = false;
    bool has_neon   = false;
    bool has_scalar = true;  // always available

    /// Which backend is the primary dispatch target?
    Backend active_backend() const {
#ifdef __AVX512F__
        return Backend::AVX512;
#elif defined(__AVX2__)
        return Backend::AVX2;
#elif defined(COMDARE_SIMD_NEON)
        return Backend::NEON;
#else
        return Backend::Scalar;
#endif
    }
};

/// Query compile-time capabilities of this binary
inline Capabilities query_capabilities() {
    Capabilities cap;
#ifdef __AVX512F__
    cap.has_avx512 = true;
    cap.has_avx2   = true;  // AVX-512 implies AVX2
#endif
#ifdef __AVX2__
    cap.has_avx2 = true;
#endif
#if defined(__ARM_NEON) || defined(__aarch64__)
    cap.has_neon = true;
#endif
    return cap;
}

/// Human-readable string of active SIMD backend
inline const char* capabilities_string() {
#ifdef __AVX512F__
    return "AVX-512F (+AVX2 +Scalar)";
#elif defined(__AVX2__)
    return "AVX2 (+Scalar)";
#elif defined(COMDARE_SIMD_NEON)
    return "ARM NEON (+Scalar)";
#else
    return "Scalar-only";
#endif
}

// =============================================================================
// Runtime Call Counters (HR9-017-sub-01 core feature)
// =============================================================================

/**
 * Tracks how many times each backend was dispatched to.
 * Uses relaxed atomics for low-overhead counting.
 * Thread-safe for concurrent increment, snapshot reads are approximate.
 */
struct CallCounters {
    std::atomic<uint64_t> scalar_calls{0};
    std::atomic<uint64_t> avx2_calls{0};
    std::atomic<uint64_t> avx512_calls{0};
    std::atomic<uint64_t> neon_calls{0};

    void increment(Backend b) {
        switch (b) {
            case Backend::Scalar: scalar_calls.fetch_add(1, std::memory_order_relaxed); break;
            case Backend::AVX2:   avx2_calls.fetch_add(1, std::memory_order_relaxed); break;
            case Backend::AVX512: avx512_calls.fetch_add(1, std::memory_order_relaxed); break;
            case Backend::NEON:   neon_calls.fetch_add(1, std::memory_order_relaxed); break;
            default: break;
        }
    }

    uint64_t count(Backend b) const {
        switch (b) {
            case Backend::Scalar: return scalar_calls.load(std::memory_order_relaxed);
            case Backend::AVX2:   return avx2_calls.load(std::memory_order_relaxed);
            case Backend::AVX512: return avx512_calls.load(std::memory_order_relaxed);
            case Backend::NEON:   return neon_calls.load(std::memory_order_relaxed);
            default: return 0;
        }
    }

    uint64_t total() const {
        return scalar_calls.load(std::memory_order_relaxed)
             + avx2_calls.load(std::memory_order_relaxed)
             + avx512_calls.load(std::memory_order_relaxed)
             + neon_calls.load(std::memory_order_relaxed);
    }

    void reset() {
        scalar_calls.store(0, std::memory_order_relaxed);
        avx2_calls.store(0, std::memory_order_relaxed);
        avx512_calls.store(0, std::memory_order_relaxed);
        neon_calls.store(0, std::memory_order_relaxed);
    }
};

/// Global call counters — tracks which backends are actually USED at runtime
inline CallCounters& global_counters() {
    static CallCounters counters;
    return counters;
}

} // namespace comdare::simd
