#pragma once
// V41.F.6.1 axis_03b_cache_traversal LinearFanout CT01 (2026-05-26)
//
// @topic traversal @achse 03b @family CT01 LinearFanout
// @subaxis CT1 linear_access
//
// **Algorithmus-Pattern:** Linear-scan auf kleinen Fanout-Arrays. Klassischer
// B+ Tree Inner-Node-Search (Bayer/McCreight, Acta Informatica 1972) ohne
// Hash-Overhead. Cache-freundlich fuer kleine N (<32), CPU-Branch-Predictor-
// friendly bei sortierten Eintraegen.
//
// Standalone-Implementation: std::vector<pair<key, value>> + std::find_if.
//
// Allocation: std::vector — [[allocation-failure-exception]]: register_entry
// kann std::bad_alloc werfen.

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

class LinearFanout : public CacheTraversalBase<LinearFanout> {
public:
    static constexpr bool enabled = flags::linear_fanout_enabled;

    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::linear_access_tag;
    using family_id  = std::integral_constant<int, 1>;  // CT01

    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "linear_fanout"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "LinearFanout (prt-art IFanout::lookup_page linear-scan-Fallback)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "LINEAR_FANOUT"; }

    [[nodiscard]] static constexpr bool is_hashed()            noexcept { return false; }
    [[nodiscard]] static constexpr bool has_collision_chains() noexcept { return false; }
    [[nodiscard]] static constexpr bool amortized_o1()         noexcept { return false; }  // O(N) linear

    LinearFanout() = default;

    [[nodiscard]] bool operator==(LinearFanout const& other) const noexcept {
        return entries_.size() == other.entries_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: push_back kann std::bad_alloc werfen.
    void register_entry(key_type k, value_type v) {
        auto it = std::find_if(entries_.begin(), entries_.end(),
            [k](auto const& e) { return e.first == k; });
        if (it != entries_.end()) {
            it->second = v;
        } else {
            entries_.emplace_back(k, v);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_register_count;
        if (entries_.size() > stats_.peak_tracked) stats_.peak_tracked = entries_.size();
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<value_type> resolve(key_type k) const {
        auto it = std::find_if(entries_.begin(), entries_.end(),
            [k](auto const& e) { return e.first == k; });
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_resolve_count;
        if (it != entries_.end()) ++stats_.total_resolve_hit_count;
        else                       ++stats_.total_resolve_miss_count;
        observer_.notify(stats_);
#endif
        if (it == entries_.end()) return std::nullopt;
        return it->second;
    }

    bool unregister(key_type k) {
        auto it = std::find_if(entries_.begin(), entries_.end(),
            [k](auto const& e) { return e.first == k; });
        if (it == entries_.end()) return false;
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
    std::vector<std::pair<key_type, value_type>> entries_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::CacheTraversalStatistics stats_{};
    mutable observer_t                          observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::cache_traversal {
    static_assert(concepts::CacheTraversalVariant<LinearFanout>);
    static_assert(concepts::CacheEngineCacheTraversalPermutationStrategy<LinearFanout>);
}
