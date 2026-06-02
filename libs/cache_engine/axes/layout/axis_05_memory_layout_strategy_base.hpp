#pragma once
// V41.F.6.1.R7.1.b axis_05_memory_layout CRTP-Basis (Goldstandard-Nachruestung)

#include "concepts/axis_05_memory_layout_concept.hpp"
#include "concepts/axis_05_memory_layout_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>
#include <axes/cacheline/cacheline_config.hpp>  // KF-5: per-Organ Cache-Line-Unterachse
#include <type_traits>

namespace comdare::cache_engine::layout {

/// MemoryLayoutStrategyBase — CRTP-Wurzel fuer alle Memory-Layout-Strategien.
///
/// 3-fach Concept-Check im Konstruktor (Goldstandard-Pattern analog
/// axis_06_allocator_strategy_base.hpp):
///   1. MemoryLayoutStrategy (Achsen-eigenes Strategie-Concept)
///   2. CacheEnginePermutationStrategy (Cross-Achsen Permutations-API)
///   3. AxisBaseConcept (Cross-Achsen Mess-API via AxisBase Wurzel)
///
/// KF-5 (2026-06-02, User-Entscheidung „die 4 + axis_05_memory_layout"): defaulted NTTP CacheLineCfg +
/// Erbe von CacheLineAware<Cfg>. HARMONISIERUNG mit dem bestehenden per-Wrapper-`cache_line_size()`:
///   • `cacheline_config()` (von CacheLineAware) = die PERMUTIERBARE Cache-Line-Unterachse (size/align/sw_hint,
///     compile-time pro Binary gewählt) — der untersuchte Freiheitsgrad.
///   • das vorhandene `cache_line_size()` der Layout-Wrapper (z.B. 64 bei CacheLineAlignedMemoryLayout) = der
///     INTRINSISCHE Design-Deskriptor der Strategie.
/// Beide koexistieren konfliktfrei (Default Cfg = {} = None → unverändert). `cacheline_subaxis_line_bytes()`
/// macht die gewählte Unterachsen-Linien-Größe explizit abfragbar (die Run-Body-Nutzung folgt mit KF-6).
template <typename Derived,
          ::comdare::cache_engine::cacheline::CacheLineConfig CacheLineCfg = ::comdare::cache_engine::cacheline::CacheLineConfig{}>
class MemoryLayoutStrategyBase
    : public ::comdare::cache_engine::topics::AxisBase
    , public ::comdare::cache_engine::cacheline::CacheLineAware<CacheLineCfg> {
public:
    /// Die durch die Cache-Line-Unterachse gewählte Linien-Größe in Bytes (permutierbar; distinkt vom
    /// intrinsischen cache_line_size() des Wrappers). Default 64 (B64).
    [[nodiscard]] static constexpr std::size_t cacheline_subaxis_line_bytes() noexcept {
        return static_cast<std::size_t>(CacheLineCfg.line_size);
    }
protected:
    MemoryLayoutStrategyBase() noexcept {
        static_assert(concepts::MemoryLayoutStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
