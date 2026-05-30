#pragma once
// V41.F.6.1 R7.2 axis_03b_cache_traversal BinarySearchFanout CT03 (2026-05-29)
//
// @topic traversal @achse 03b @family CT03 BinarySearchFanout
// @subaxis CT1 linear_access (geordnet — binary search statt linear/hash)
//
// **Algorithmus-Pattern:** sortierte Fanout-Eintraege + Binärsuche (std::lower_bound) zur
// Routing-Resolution — der klassische B+-Tree-Inner-Node-Search (Bayer/McCreight, Acta Informatica
// 1972) in seiner sortiert-binären Variante. O(log N) Resolve vs. LinearFanout (O(N) linear-scan)
// und HashLookup (O(1) gehasht, aber ungeordnet + Hash-Overhead). Vorteil: geordnete Traversierung
// + cache-freundlich bei mittlerem Fanout, ohne Hash-Kosten; behaelt die Schluessel-Ordnung (wichtig
// fuer Range-faehige Routing-Layer). Dritte distinkte cache_traversal-Strategie der Achse.
//
// Standalone C++23-Re-Impl (sortierter std::vector<pair> + lower_bound), is_original=false.
//
// Allocation: std::vector — [[allocation-failure-exception]]: register_entry kann std::bad_alloc.

#include "axis_03b_cache_traversal_base.hpp"
#include "axis_03b_cache_traversal_subaxes_ct1_to_ct2.hpp"
#include "concepts/axis_03b_cache_traversal_concept.hpp"
#include "concepts/axis_03b_cache_traversal_cache_engine_permutation_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/cache_traversal/axis_03b_cache_traversal_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace comdare::cache_engine::cache_traversal {

class BinarySearchFanout : public CacheTraversalBase<BinarySearchFanout> {
public:
    static constexpr bool enabled = flags::binary_search_fanout_enabled;

    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::linear_access_tag;
    using family_id  = std::integral_constant<int, 3>;  // CT03

    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "binary_search_fanout"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "BinarySearchFanout (sorted fanout + lower_bound, B+ inner-node binary search, Bayer/McCreight 1972)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "BINARY_SEARCH_FANOUT"; }

    [[nodiscard]] static constexpr bool is_hashed()            noexcept { return false; }
    [[nodiscard]] static constexpr bool has_collision_chains() noexcept { return false; }
    [[nodiscard]] static constexpr bool amortized_o1()         noexcept { return false; }  // O(log N) resolve

    BinarySearchFanout() = default;

    [[nodiscard]] bool operator==(BinarySearchFanout const& other) const noexcept {
        return entries_.size() == other.entries_.size();
    }

    /// Sortiert-eingefuegt (lower_bound) → entries_ bleibt nach key geordnet. Update bei Treffer.
    /// SONDERFALL [[allocation-failure-exception]]: insert kann std::bad_alloc werfen.
    void register_entry(key_type k, value_type v) {
        auto it = std::lower_bound(entries_.begin(), entries_.end(), k,
            [](auto const& e, key_type key) { return e.first < key; });
        if (it != entries_.end() && it->first == k) {
            it->second = v;
        } else {
            entries_.insert(it, std::pair<key_type, value_type>{k, v});
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_register_count;
        if (entries_.size() > stats_.peak_tracked) stats_.peak_tracked = entries_.size();
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<value_type> resolve(key_type k) const {
        auto it = std::lower_bound(entries_.begin(), entries_.end(), k,
            [](auto const& e, key_type key) { return e.first < key; });
        bool const hit = (it != entries_.end() && it->first == k);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_resolve_count;
        if (hit) ++stats_.total_resolve_hit_count; else ++stats_.total_resolve_miss_count;
        observer_.notify(stats_);
#endif
        if (!hit) return std::nullopt;
        return it->second;
    }

    bool unregister(key_type k) {
        auto it = std::lower_bound(entries_.begin(), entries_.end(), k,
            [](auto const& e, key_type key) { return e.first < key; });
        if (it == entries_.end() || it->first != k) return false;
        entries_.erase(it);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_unregister_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type tracked_count() const noexcept { return entries_.size(); }
    void                    clear() noexcept { entries_.clear(); }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::CacheTraversalStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot()   const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; observer_.notify(stats_); }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer()       noexcept { return observer_; }
#endif

private:
    std::vector<std::pair<key_type, value_type>> entries_;  // nach key sortiert
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::CacheTraversalStatistics stats_{};
    mutable observer_t                          observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::cache_traversal {
    static_assert(concepts::CacheTraversalVariant<BinarySearchFanout>);
    static_assert(concepts::CacheEngineCacheTraversalPermutationStrategy<BinarySearchFanout>);
}
