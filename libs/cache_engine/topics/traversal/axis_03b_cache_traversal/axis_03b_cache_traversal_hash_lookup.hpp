#pragma once
// V41.F.6.1 axis_03b_cache_traversal HashLookup CT02 (2026-05-26)
//
// @topic traversal @achse 03b @family CT02 HashLookup
// @subaxis CT2 hash_access
//
// Hash-basierte Cache-Traversal mit Fibonacci-Hash + Open-Addressing (Linear
// Probing). Inspiriert von prt-art-Legacy LinearProbeHashSet. Amortisiert
// O(1) bei load_factor < 0.7.
//
// Resize-Pattern: bei load_factor > 0.7 wird capacity verdoppelt (Power-of-2).
//
// Allocation: std::vector — [[allocation-failure-exception]]: register_entry
// + Resize koennen std::bad_alloc werfen.

#include "axis_03b_cache_traversal_base.hpp"
#include "axis_03b_cache_traversal_subaxes_ct1_to_ct2.hpp"
#include "concepts/axis_03b_cache_traversal_concept.hpp"
#include "concepts/axis_03b_cache_traversal_cache_engine_permutation_concept.hpp"
#include "../concepts/topic_traversal_concept.hpp"

#include <topics/traversal/axis_03b_cache_traversal/axis_03b_cache_traversal_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::traversal::axis_03b_cache_traversal {

class HashLookup : public CacheTraversalBase<HashLookup> {
public:
    static constexpr bool enabled = flags::hash_lookup_enabled;

    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::hash_access_tag;
    using family_id  = std::integral_constant<int, 2>;  // CT02

    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "hash_lookup"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "HashLookup (Fibonacci-Hash open-addressing, prt-art LinearProbeHashSet-Pattern)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "HASH_LOOKUP"; }

    [[nodiscard]] static constexpr bool is_hashed()            noexcept { return true; }
    [[nodiscard]] static constexpr bool has_collision_chains() noexcept { return true; }  // Linear Probing
    [[nodiscard]] static constexpr bool amortized_o1()         noexcept { return true; }

    static constexpr std::uint64_t kFibonacciMul = 11400714819323198485ULL;
    static constexpr std::size_t   kInitialCapacity = 16;  // Power-of-2

    HashLookup() : capacity_mask_(kInitialCapacity - 1), buckets_(kInitialCapacity), size_(0) {}

    [[nodiscard]] bool operator==(HashLookup const& other) const noexcept {
        return size_ == other.size_;
    }

    /// SONDERFALL [[allocation-failure-exception]]: rehash kann std::bad_alloc werfen.
    void register_entry(key_type k, value_type v) {
        if ((size_ * 10) >= (capacity_mask_ + 1) * 7) {
            rehash((capacity_mask_ + 1) * 2);
        }
        std::size_t idx = hash_index(k);
        for (std::size_t i = 0; i < (capacity_mask_ + 1); ++i) {
            std::size_t pos = (idx + i) & capacity_mask_;
            if (!buckets_[pos].has_value()) {
                buckets_[pos] = std::pair{k, v};
                ++size_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
                ++stats_.total_register_count;
                if (size_ > stats_.peak_tracked) stats_.peak_tracked = size_;
                observer_.notify(stats_);
#endif
                return;
            }
            if (buckets_[pos]->first == k) {
                buckets_[pos]->second = v;
#ifdef COMDARE_CE_ENABLE_STATISTICS
                ++stats_.total_register_count;
                observer_.notify(stats_);
#endif
                return;
            }
        }
    }

    [[nodiscard]] std::optional<value_type> resolve(key_type k) const {
        std::size_t idx = hash_index(k);
        for (std::size_t i = 0; i < (capacity_mask_ + 1); ++i) {
            std::size_t pos = (idx + i) & capacity_mask_;
            if (!buckets_[pos].has_value()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
                ++stats_.total_resolve_count;
                ++stats_.total_resolve_miss_count;
                observer_.notify(stats_);
#endif
                return std::nullopt;
            }
            if (buckets_[pos]->first == k) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
                ++stats_.total_resolve_count;
                ++stats_.total_resolve_hit_count;
                observer_.notify(stats_);
#endif
                return buckets_[pos]->second;
            }
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_resolve_count;
        ++stats_.total_resolve_miss_count;
        observer_.notify(stats_);
#endif
        return std::nullopt;
    }

    bool unregister(key_type k) {
        std::size_t idx = hash_index(k);
        for (std::size_t i = 0; i < (capacity_mask_ + 1); ++i) {
            std::size_t pos = (idx + i) & capacity_mask_;
            if (!buckets_[pos].has_value()) return false;
            if (buckets_[pos]->first == k) {
                buckets_[pos].reset();
                --size_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
                ++stats_.total_unregister_count;
                observer_.notify(stats_);
#endif
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] size_type tracked_count() const noexcept { return size_; }
    void                    clear() noexcept {
        for (auto& b : buckets_) b.reset();
        size_ = 0;
    }

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
    [[nodiscard]] std::size_t hash_index(key_type k) const noexcept {
        return static_cast<std::size_t>((k * kFibonacciMul) & capacity_mask_);
    }

    void rehash(std::size_t new_capacity) {
        std::vector<std::optional<std::pair<key_type, value_type>>> old_buckets;
        old_buckets.swap(buckets_);
        buckets_.assign(new_capacity, std::nullopt);
        capacity_mask_ = new_capacity - 1;
        std::size_t old_size = size_;
        size_ = 0;
        for (auto const& slot : old_buckets) {
            if (slot.has_value()) {
                std::size_t idx = hash_index(slot->first);
                for (std::size_t i = 0; i < new_capacity; ++i) {
                    std::size_t pos = (idx + i) & capacity_mask_;
                    if (!buckets_[pos].has_value()) {
                        buckets_[pos] = slot;
                        ++size_;
                        break;
                    }
                }
            }
        }
        (void)old_size;
    }

    std::size_t capacity_mask_;
    std::vector<std::optional<std::pair<key_type, value_type>>> buckets_;
    std::size_t size_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::CacheTraversalStatistics stats_{};
    mutable observer_t                          observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::traversal::axis_03b_cache_traversal {
    static_assert(concepts::CacheTraversalVariant<HashLookup>);
    static_assert(concepts::CacheEngineCacheTraversalPermutationStrategy<HashLookup>);
}
