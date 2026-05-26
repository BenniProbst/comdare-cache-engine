#pragma once
// V41.F.6.1 axis_03a_search_algo Sub-Concept DensityClassifiedStrategy (2026-05-26)
//
// @topic traversal @achse 03a
//
// Sub-Concept fuer Such-Algorithmen die eine explizite Density-Klassifikation
// liefern (Pflicht-Pattern: jede Klasse mit `is_dense()` definiert MUSS
// auch density_class() abrufbar machen).

#include "axis_03a_search_algo_concept.hpp"

#include <concepts>
#include <cstdint>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::concepts {

/**
 * @brief DensityClass — Klassifikation der Search-Algo-Density
 *
 * V41.F.6.1 Topic traversal: ordinal Enum-Klassifikation (analog
 * ProgressGuarantee bei Allocator [[vendor-sonderfaelle-als-pflicht-property]]
 * Stufen-Pattern). Hoehere Stufen IMPLIZIEREN niedrigere Stufen.
 *
 * CacheEngineBuilder kann Vendor-Subsets bilden:
 *   - level() >= Dense          (Array256 only)
 *   - level() >= Balanced       (Array256 + Multilevel)
 *   - level() >= Sparse         (alle Strategien)
 *   - level() == AdaptiveTransition  (alle die zur Laufzeit umschalten)
 */
enum class DensityClass : int {
    Sparse              = 0,  // <30% fanout occupied (HOT, Patricia)
    Balanced            = 1,  // 30-70% (Masstree W=15)
    Dense               = 2,  // >70% (Array256, ART Node256)
    AdaptiveTransition  = 3,  // Threshold-Crossing (notify_density_threshold)
};

/**
 * @brief DensityClassifiedStrategy — Pflicht-API fuer density-klassifizierte Search-Algos
 *
 * Verfeinerung des SearchAlgoVariant-Basiskonzepts. Wrapper-Klassen muessen
 * `density_class() const noexcept -> DensityClass` anbieten.
 */
template <typename S>
concept DensityClassifiedStrategy =
    SearchAlgoVariant<S>
    && requires(S const& sc) {
        { sc.density_class() } noexcept -> std::convertible_to<DensityClass>;
    };

}  // namespace
