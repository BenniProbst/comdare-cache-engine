#pragma once
// V41.F.6.1 F.6 axis_08 OlcReservedBlocksConcurrency (OLC + reservierte Value-Blocks)
//
// F.6 Migration (Doku 19): prt-art OlcWithReservedValueBlocks als 9. Concurrency-STRATEGIE.
// OLC (Optimistic Lock Coupling) kombiniert mit reservierten, cache-line-ausgerichteten
// Value-Blocks pro Writer → vermeidet, dass MVCC-Versionen denselben Cache-Line teilen
// (verhindert Cache-Coherence-Storm). Die konkrete atomare Laufzeit-Impl (version_ +
// next_block_id_ + WriteGuard) bleibt prt-art-Detail (optional_prt_art_impl).

#include "axis_08_concurrency_strategy_base.hpp"
#include "axis_08_concurrency_subaxes_cc1_to_cc2.hpp"
#include "concepts/axis_08_concurrency_cache_engine_permutation_concept.hpp"
#include <axes/concurrency_axis/axis_08_concurrency_flags.hpp>
#include <topics/concurrency/concepts/topic_concurrency_concept.hpp>
#include <atomic>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::concurrency_axis {

/// OlcReservedBlocksConcurrency — OLC mit reservierten cache-line-aligned Value-Blocks
/// (Cache-Coherence-Storm-Vermeidung bei Multi-Writer). Spezialisierung des Optimistic-Patterns.
class OlcReservedBlocksConcurrency : public ConcurrencyStrategyBase<OlcReservedBlocksConcurrency> {
public:
    using topic_tag = ::comdare::cache_engine::concurrency::concepts::ConcurrencyTopicTag;
    using axis_tag  = subaxes::synchronization_pattern_tag;
    using family_id = std::integral_constant<int, 9>;

    static constexpr bool enabled = flags::olc_reserved_blocks_enabled;

    [[nodiscard]] static constexpr concepts::ConcurrencyPattern concurrency_pattern() noexcept {
        return concepts::ConcurrencyPattern::Optimistic;
    }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return "olc_reserved_blocks"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept { return "OlcReservedBlocksConcurrency (OLC + cache-line-reserved value blocks, anti-coherence-storm)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "OLC_RESERVED_BLOCKS"; }

    // V41 F15 Pfad-A — treibbare Concurrency-Op (acquire/release-Paar). OLC + reservierte Value-Blocks:
    // optimistischer Versions-Read (wie OlcOptimistic) PLUS eine atomare Reservierung eines neuen
    // Value-Blocks pro Writer (fetch_add auf next_block_id_ → modelliert das Vergeben einer cache-line-
    // ausgerichteten Block-ID). Reale, strategie-abhaengige Laufzeit: Versions-Load (acquire) + eine
    // zusaetzliche atomare RMW (fetch_add) → echt teurer als reines OLC, charakteristisch fuer das
    // Reserved-Block-Pattern (Doku 19). Single-Thread-Pfad-A: Validierung stets gueltig.
    static void acquire() noexcept {
        snapshot_() = version_().load(std::memory_order_acquire);
        // reservierte Block-ID vorruecken (anti-coherence-storm: jeder Writer eigener Block).
        (void) next_block_id_().fetch_add(1u, std::memory_order_acq_rel);
    }
    static void release() noexcept {
        unsigned const now = version_().load(std::memory_order_acquire);
        static volatile bool valid_sink = false;
        valid_sink = (now == snapshot_());
    }

private:
    [[nodiscard]] static std::atomic<unsigned>& version_() noexcept {
        static thread_local std::atomic<unsigned> v{0u};
        return v;
    }
    [[nodiscard]] static std::atomic<unsigned>& next_block_id_() noexcept {
        static thread_local std::atomic<unsigned> b{0u};
        return b;
    }
    [[nodiscard]] static unsigned& snapshot_() noexcept {
        static thread_local unsigned s = 0u;
        return s;
    }
};

}  // namespace

namespace comdare::cache_engine::concurrency_axis {
    static_assert(concepts::ConcurrencyStrategy<OlcReservedBlocksConcurrency>);
    static_assert(concepts::CacheEnginePermutationStrategy<OlcReservedBlocksConcurrency>);
}
