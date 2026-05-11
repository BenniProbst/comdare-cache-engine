#pragma once
// CacheEngineMode — Block AP Forschungs-Mission als Operations-Modus
// Termin 7 / 02_uml_cache_engine §9 + REV 5 §3 (K03 Permutationsdimension)

#include <cstdint>

namespace comdare::cache_engine {

enum class CacheEngineMode : std::uint8_t {
    HEURISTIC_STATIC      = 0,   // StaticEngine, klassische Faustregeln (V1 Baseline F15)
    INFORMED_KALIBRIERT   = 1,   // Cache-Engine + Plattform-Kalibrierung (V3)
    AUTOMATIC_ADAPTIVE    = 2,   // DecisionLambdaTree-Steuerung waehrend Laufzeit (V4)
    BASELINE_NO_ENGINE    = 3,   // BaseEngineStrategy ohne CacheEngine (V1 ground truth)
};

// REV 5 K03: 4 Permutations-Varianten pro Search-Engine-Permutation
struct CacheEnginePermutationVariant {
    bool             use_cache_engine_strategy = false;   // false = BaseEngineStrategy
    CacheEngineMode  mode = CacheEngineMode::BASELINE_NO_ENGINE;
};

[[nodiscard]] constexpr CacheEnginePermutationVariant variant_v1_baseline() noexcept {
    return {false, CacheEngineMode::BASELINE_NO_ENGINE};
}
[[nodiscard]] constexpr CacheEnginePermutationVariant variant_v2_static() noexcept {
    return {true,  CacheEngineMode::HEURISTIC_STATIC};
}
[[nodiscard]] constexpr CacheEnginePermutationVariant variant_v3_informed() noexcept {
    return {true,  CacheEngineMode::INFORMED_KALIBRIERT};
}
[[nodiscard]] constexpr CacheEnginePermutationVariant variant_v4_adaptive() noexcept {
    return {true,  CacheEngineMode::AUTOMATIC_ADAPTIVE};
}

}  // namespace comdare::cache_engine
