#pragma once
// V41.F.6.1.R7.1.b axis_05_memory_layout CRTP-Basis (Goldstandard-Nachruestung)

#include "concepts/axis_05_memory_layout_concept.hpp"
#include "concepts/axis_05_memory_layout_cache_engine_permutation_concept.hpp"
#include <topics/axis_base.hpp>
#include <axes/cacheline/cacheline_config.hpp> // KF-5: per-Organ Cache-Line-Unterachse
#include <type_traits>

namespace comdare::cache_engine::layout {

/// RepresentationKind (P-MD1-ERDUNG, #167, 2026-06-18) — die 5 REALEN, byte-distinkten Speicher-
/// Repraesentationen, die der LayoutAwareChunkedStore compile-time-dispatched (`if constexpr`) PHYSISCH
/// anlegt. Lehrbuch-Benennung (web-verifiziert): AoS (Array-of-Structs), SoA (Struct-of-Arrays),
/// AoSoA (Array-of-Structs-of-Arrays / SIMD-tiled), succinct hot/cold column-split (succinct data structures).
/// JEDE Strategie deklariert genau eine; der Store leitet daraus store_record/load_key/load_value UND den
/// REALEN Key-Scan-Footprint (field_bytes/cache_lines) ab — NICHT mehr der entkoppelte Deskriptor (P-MD1).
enum class RepresentationKind {
    aos_interleaved_packed,  ///< AoS dicht: [key|value] adjazent, 16-B-Stride (aos_strict).
    aos_interleaved_padded,  ///< AoS gepaddet: [key|value|pad] auf 64-B-Cache-Line-Stride (cache_line_aligned).
    soa_split_columns,       ///< SoA: keys[]-Spalte gefolgt von values[]-Spalte (zwei getrennte Arrays).
    aosoa_blocked_columns,   ///< AoSoA: pro Block B keys dann B values, Bloecke als Array (SIMD-tiled).
    succinct_hot_cold_split, ///< succinct: 2-B-Hot-Key-Spalte (Scan) + 6-B-Cold-Residue + values (packed_bitmap).
};

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
template <typename Derived, ::comdare::cache_engine::cacheline::CacheLineConfig CacheLineCfg =
                                ::comdare::cache_engine::cacheline::CacheLineConfig{}>
class MemoryLayoutStrategyBase : public ::comdare::cache_engine::topics::AxisBase,
                                 public ::comdare::cache_engine::cacheline::CacheLineAware<CacheLineCfg> {
public:
    /// Die durch die Cache-Line-Unterachse gewählte Linien-Größe in Bytes (permutierbar; distinkt vom
    /// intrinsischen cache_line_size() des Wrappers). Default 64 (B64).
    [[nodiscard]] static constexpr std::size_t cacheline_subaxis_line_bytes() noexcept {
        return static_cast<std::size_t>(CacheLineCfg.line_size);
    }

    // ── REALE Repraesentation (P-MD1-ERDUNG, #167, 2026-06-18) ──────────────────────────────────────────
    // Jede Strategie deklariert ihre RepresentationKind; der LayoutAwareChunkedStore dispatcht darauf
    // (`if constexpr`) und legt die Records PHYSISCH gemaess der Repraesentation an. CLU + field_bytes +
    // cache_lines kommen aus dem REALEN, byte-genau vermessenen Store-Footprint (NICHT mehr aus einem
    // entkoppelten record_useful_bytes/record_line_span-Deskriptor — das war das Phantom-Muster P-MD1).
    // Default = dichte AoS (aos_interleaved_packed); jede Strategie ueberschreibt sie.
    [[nodiscard]] static constexpr RepresentationKind representation_kind() noexcept {
        return RepresentationKind::aos_interleaved_packed;
    }

protected:
    MemoryLayoutStrategyBase() noexcept {
        static_assert(concepts::MemoryLayoutStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

} // namespace comdare::cache_engine::layout
