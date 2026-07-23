#pragma once
// V41.F.6.1 axis_03b_cache_traversal HashLookup CT02 (2026-05-26)
//
// @topic traversal @achse 03b @family CT02 HashLookup
// @subaxis CT2 hash_access
//
// **Algorithmus-Pattern:** Fibonacci-Hash open-addressing (Knuth TAOCP Vol 3,
// "Sorting and Searching", 2nd Ed. 1998 §6.4 Multiplikative Hashing).
// Konstante 11400714819323198485 ≈ 2^64 / golden_ratio. Linear Probing als
// Kollisions-Strategie (Knuth analyzed L1-cache-effizient).
//
// Standalone-Implementation: std::vector<std::optional<pair>> Buckets +
// power-of-2 mask + Fibonacci-Hash. Amortisiert O(1) bei load_factor < 0.7.
// Resize verdoppelt Capacity bei load > 0.7 (Power-of-2 Pflicht fuer Mask).
//
// **iterable_aspect_t:** initial_capacity ist iterable Aspekt fuer hybride
// Laufzeit-Permutation (kIterableInitialCapacities). PermutationEngine
// generiert 1 Binary mit Runtime-Loop ueber Capacity-Werte.
//
// Allocation: std::vector — [[allocation-failure-exception]]: register_entry
// + Resize koennen std::bad_alloc werfen.

#include "axis_03b_cache_traversal_base.hpp"
#include "axis_03b_cache_traversal_subaxes_ct1_to_ct2.hpp"
#include "concepts/axis_03b_cache_traversal_concept.hpp"
#include "concepts/axis_03b_cache_traversal_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03b_cache_traversal_hashed_strategy_concept.hpp"
#include "concepts/axis_03b_cache_traversal_iterable_aspect_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/cache_traversal/axis_03b_cache_traversal_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::cache_traversal {

class HashLookup : public CacheTraversalBase<HashLookup> {
public:
    static constexpr bool enabled = flags::hash_lookup_enabled;

    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::hash_access_tag;
    using family_id  = std::integral_constant<int, 2>; // CT02

    /// iterable_aspect_t (F.6.1.E hybride Laufzeit-Permutation):
    /// initial_capacity als Power-of-2. PermutationEngine erkennt via
    /// HasIterableAspect<V> und generiert 1 Binary mit Runtime-Loop.
    using iterable_aspect_t = std::size_t;
    static constexpr std::array<std::size_t, 5>                 kIterableInitialCapacities{8u, 16u, 64u, 256u, 1024u};
    [[nodiscard]] static constexpr std::span<std::size_t const> iterable_values() noexcept {
        return std::span<std::size_t const>{kIterableInitialCapacities.data(), kIterableInitialCapacities.size()};
    }

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "hash_lookup"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::cache_traversal::HashLookup",
                                  "axes/cache_traversal/axis_03b_cache_traversal_hash_lookup.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "HashLookup (Fibonacci-Hash open-addressing, prt-art LinearProbeHashSet-Pattern)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "HASH_LOOKUP"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    [[nodiscard]] static constexpr bool is_hashed() noexcept { return true; }
    [[nodiscard]] static constexpr bool has_collision_chains() noexcept { return true; } // Linear Probing
    [[nodiscard]] static constexpr bool amortized_o1() noexcept { return true; }

    static constexpr std::uint64_t kFibonacciMul    = 11400714819323198485ULL;
    static constexpr std::size_t   kInitialCapacity = 16; // Power-of-2 Default

    HashLookup() : HashLookup(kInitialCapacity) {}
    /// SONDERFALL [[zero-size-allocation-exception]]: cap=0 wirft std::invalid_argument
    /// (Mask-Modulo waere Division-By-Zero). cap MUSS Power-of-2 sein.
    explicit HashLookup(std::size_t initial_capacity)
        : capacity_mask_(validate_capacity(initial_capacity) - 1), buckets_(initial_capacity), size_(0) {}

    [[nodiscard]] bool operator==(HashLookup const& other) const noexcept { return size_ == other.size_; }

    /// SONDERFALL [[allocation-failure-exception]]: rehash kann std::bad_alloc werfen.
    void register_entry(key_type k, value_type v) {
        if ((size_ * 10) >= (capacity_mask_ + 1) * 7) { rehash((capacity_mask_ + 1) * 2); }
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
        std::size_t idx   = hash_index(k);
        std::size_t pos   = 0;
        bool        found = false;
        for (std::size_t i = 0; i < (capacity_mask_ + 1); ++i) {
            pos = (idx + i) & capacity_mask_;
            if (!buckets_[pos].has_value()) return false; // leerer Slot = Ende der Probe-Kette → nicht vorhanden
            if (buckets_[pos]->first == k) {
                found = true;
                break;
            }
        }
        if (!found) return false;

        // Knuth TAOCP Vol 3 §6.4 Algorithm R — Backward-Shift-Deletion fuer Linear Probing.
        // Ein blosses buckets_[pos].reset() wuerde die Probe-Kette zerreissen (Keys hinter der Luecke
        // ⇒ falsche Misses; Re-Insert ⇒ Duplikate). Stattdessen: Luecke bei `hole` halten und jedes
        // nachfolgende Element, dessen Home-Slot NICHT zyklisch im offenen Intervall (hole, j] liegt,
        // in die Luecke nachziehen. Erhaelt die Kontiguitaets-Invariante jeder Kette OHNE Tombstone
        // (passt zur std::optional-Belegung leer/besetzt) → resolve()/register_entry() bleiben korrekt.
        std::size_t hole = pos;
        buckets_[hole].reset();
        for (std::size_t j = (hole + 1) & capacity_mask_;; j = (j + 1) & capacity_mask_) {
            if (!buckets_[j].has_value()) break; // Ende des Clusters → fertig
            std::size_t const home = hash_index(buckets_[j]->first);
            // Liegt `home` zyklisch in (hole, j]? Dann darf das Element NICHT nachgezogen werden.
            bool const cannot_move = (hole <= j) ? (hole < home && home <= j) : (hole < home || home <= j);
            if (cannot_move) continue;
            buckets_[hole] = std::move(buckets_[j]); // Element in die Luecke ziehen
            buckets_[j].reset();                     // j wird zur neuen Luecke
            hole = j;
        }
        --size_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_unregister_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type tracked_count() const noexcept { return size_; }
    void                    clear() noexcept {
        for (auto& b : buckets_) b.reset();
        size_ = 0;
    }

    /// HashedTraversalStrategy [[hashed-strategy]]: Bucket-Count Accessor.
    [[nodiscard]] std::size_t bucket_count() const noexcept { return capacity_mask_ + 1; }

    /// HashedTraversalStrategy [[hashed-strategy]]: aktueller Load-Factor in [0.0, 1.0].
    [[nodiscard]] double load_factor() const noexcept {
        return static_cast<double>(size_) / static_cast<double>(bucket_count());
    }

    /// IterableAspectCacheTraversalStrategy [[iterable-aspect-strategy]]:
    /// Setter fuer Runtime-Capacity-Switch (vollstaendiger Rehash).
    /// SONDERFALL [[zero-size-allocation-exception]]: cap=0 oder nicht-Power-of-2 wirft.
    /// SONDERFALL [[allocation-failure-exception]]: rehash kann std::bad_alloc werfen.
    void set_iterable_aspect(std::size_t new_capacity) {
        std::size_t validated = validate_capacity(new_capacity);
        rehash(validated);
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::CacheTraversalStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }
#endif

private:
    static std::size_t validate_capacity(std::size_t cap) {
        if (cap == 0) {
            throw std::invalid_argument(
                "HashLookup: initial_capacity must be > 0 (Mask-Modulo would division-by-zero)");
        }
        if ((cap & (cap - 1)) != 0) {
            throw std::invalid_argument(
                "HashLookup: initial_capacity must be Power-of-2 (Fibonacci-Hash bitmask constraint)");
        }
        return cap;
    }

    [[nodiscard]] std::size_t hash_index(key_type k) const noexcept {
        return static_cast<std::size_t>((k * kFibonacciMul) & capacity_mask_);
    }

    void rehash(std::size_t new_capacity) {
        std::vector<std::optional<std::pair<key_type, value_type>>> old_buckets;
        old_buckets.swap(buckets_);
        buckets_.assign(new_capacity, std::nullopt);
        capacity_mask_       = new_capacity - 1;
        std::size_t old_size = size_;
        size_                = 0;
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

    std::size_t                                                 capacity_mask_;
    std::vector<std::optional<std::pair<key_type, value_type>>> buckets_;
    std::size_t                                                 size_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::CacheTraversalStatistics stats_{};
    mutable observer_t                         observer_{};
#endif
};

} // namespace comdare::cache_engine::cache_traversal

namespace comdare::cache_engine::cache_traversal {
static_assert(concepts::CacheTraversalVariant<HashLookup>);
static_assert(concepts::CacheEngineCacheTraversalPermutationStrategy<HashLookup>);
static_assert(concepts::HashedTraversalStrategy<HashLookup>);
static_assert(concepts::IterableAspectCacheTraversalStrategy<HashLookup>);
} // namespace comdare::cache_engine::cache_traversal
