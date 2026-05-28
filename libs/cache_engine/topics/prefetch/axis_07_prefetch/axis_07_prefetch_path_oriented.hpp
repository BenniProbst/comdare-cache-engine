#pragma once
// V41.F.6.1.R7.5.a axis_07 PathOrientedPrefetch (PRT-ART)
//
// R7.6 Paper-Reference (Task #723):
// PRT-ART (Path-Oriented Prefetching ART) ist eigene Diplomarbeit-Arbeit
// (Probst, B. "PRT-ART: Active Cache-Aware Hardware Adaptation Cache Engine
// for Trie-Based Index Structures." Diplomarbeit TU Dresden, Prof. Habich,
// in Bearbeitung 2026).
//
// Verwandte Literatur:
// - Chen, S., Gibbons, P.B., Mowry, T.C. "Improving Index Performance through
//   Prefetching." Proceedings of ACM SIGMOD 2001. Heuristik fuer
//   Path-Bundle-Identifikation.
// - Mowry, T.C. "Tolerating Latency Through Software-Controlled Data
//   Prefetching." Stanford Tech-Report CSL-TR-94-628, 1994.
//
// Original-Pattern: Pre-loadet Knoten entlang vorhergesagtem Trie-Pfad bevor
// Suche startet. Bundle-Granularitaet: mehrere Cache-Lines pro Trigger.

#include "axis_07_prefetch_strategy_base.hpp"
#include "axis_07_prefetch_subaxes_pf1_to_pf3.hpp"
#include "concepts/axis_07_prefetch_cache_engine_permutation_concept.hpp"
#include "axis_07_prefetch_flags.hpp"
#include "../concepts/topic_prefetch_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::prefetch::axis_07_prefetch {

/// PathOrientedPrefetch — Pfad-orientierte Prefetch-Heuristik (PRT-ART).
/// Pre-loadet alle Knoten entlang vorhergesagtem Trie-Pfad bevor Suche
/// startet. Granularitaet: Bundle (mehrere Cache-Lines pro Trigger).
class PathOrientedPrefetch : public PrefetchStrategyBase<PathOrientedPrefetch> {
public:
    using topic_tag = ::comdare::cache_engine::prefetch::concepts::PrefetchTopicTag;
    using axis_tag  = subaxes::granularity_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::path_oriented_enabled;

    [[nodiscard]] static constexpr bool             is_active()    noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "prefetch_path_oriented"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "PathOrientedPrefetch (PRT-ART trie-path bundle-prefetch)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "PATH_ORIENTED"; }
};

}  // namespace

namespace comdare::cache_engine::prefetch::axis_07_prefetch {
    static_assert(concepts::PrefetchStrategy<PathOrientedPrefetch>);
    static_assert(concepts::CacheEnginePermutationStrategy<PathOrientedPrefetch>);
}
