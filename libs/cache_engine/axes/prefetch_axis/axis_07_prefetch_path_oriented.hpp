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
#include <axes/prefetch_axis/axis_07_prefetch_flags.hpp>
#include "axis_07_prefetch_path_oriented_impl.hpp" // V41.F.6.1.F.6 native Logik (prt-art-Migration)
#include <topics/prefetch/concepts/topic_prefetch_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::prefetch_axis {

/// PathOrientedPrefetch — Pfad-orientierte Prefetch-Heuristik (PRT-ART).
/// Pre-loadet alle Knoten entlang vorhergesagtem Trie-Pfad bevor Suche
/// startet. Granularitaet: Bundle (mehrere Cache-Lines pro Trigger).
class PathOrientedPrefetch : public PrefetchStrategyBase<PathOrientedPrefetch> {
public:
    using topic_tag = ::comdare::cache_engine::prefetch::concepts::PrefetchTopicTag;
    using axis_tag  = subaxes::granularity_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::path_oriented_enabled;

    [[nodiscard]] static constexpr bool             is_active() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "prefetch_path_oriented"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "PathOrientedPrefetch (PRT-ART trie-path bundle-prefetch)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "PATH_ORIENTED"; }

    // V41.F.6.1.F.6 — native Pfad-Tracking-Logik (prt-art REV 6 §5.17 + V11.1 Hot-Path).
    // Der Wrapper ist stateful: er verfolgt den aktiven Suchpfad und empfiehlt die naechste
    // Prefetch-Adresse. Forwarding auf die wiederverwendbare StrategyImpl.
    using impl_type                               = impl::PathOrientedImpl;
    static constexpr std::size_t kMaxTrackedSlots = impl_type::kMaxTrackedSlots;

    void                        enqueue(std::uint64_t addr) { tracker_.enqueue(addr); }
    [[nodiscard]] std::uint64_t suggest_next() const noexcept { return tracker_.suggest_next(); }
    [[nodiscard]] std::uint64_t total_enqueued() const noexcept { return tracker_.total_enqueued(); }
    [[nodiscard]] std::size_t   queue_depth() const noexcept { return tracker_.queue_depth(); }
    [[nodiscard]] std::vector<std::uint64_t> const& path() const noexcept { return tracker_.path(); }
    void                                            reset() noexcept { tracker_.reset(); }
    /// V11.1 Hot-Path-Hint aus rohen Schluessel-Bytes (z.B. binary_key_t aus SearchEngine-ABI).
    void note_hot_path_bytes(std::byte const* data, std::size_t bytes) noexcept {
        tracker_.note_hot_path_bytes(data, bytes);
    }
    [[nodiscard]] std::uint64_t total_hot_path_hints() const noexcept { return tracker_.total_hot_path_hints(); }

private:
    impl_type tracker_{};
};

} // namespace comdare::cache_engine::prefetch_axis

namespace comdare::cache_engine::prefetch_axis {
static_assert(concepts::PrefetchStrategy<PathOrientedPrefetch>);
static_assert(concepts::CacheEnginePermutationStrategy<PathOrientedPrefetch>);
} // namespace comdare::cache_engine::prefetch_axis
