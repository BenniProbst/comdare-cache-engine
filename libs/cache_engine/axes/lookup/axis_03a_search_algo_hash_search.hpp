#pragma once
// V41.F.6.1 R7.2 axis_03a_search_algo HashSearchAlgo S14 (2026-05-29)
//
// @topic traversal @achse 03a @family S14 HashSearchAlgo
// @subaxis SA2 sparse_access (UNGEORDNETE Hash-Struktur — kein Range-Scan)
//
// **Algorithmus:** Open-Addressing-Hashtabelle mit Linear Probing + multiplikativem (Fibonacci-)
// Hashing, beschrieben in: Donald E. Knuth, "The Art of Computer Programming, Vol. 3: Sorting and
// Searching", 2nd Ed. 1998, §6.4 (Hashing). Konstante 2^64/φ ≈ 11400714819323198485.
//
// Das fundamentale Such-Paradigma, das axis_03a noch fehlte: WEDER Array (Array256/65535) noch
// sortierter Vektor (VectorU8/U16) noch Such-METHODE auf Sortierung (k-ary/interpolation/eytzinger)
// noch verzeigerte geordnete Struktur (skip-list) noch Radix-Trie (ART/HOT/…), sondern eine
// UNGEORDNETE Hashtabelle: O(1) erwartet bei load_factor < 0.7, KEINE Schluessel-Ordnung
// (supports_range_scan=false — die distinkte Klassifikation). Komplettiert die Such-Bibliothek.
//
// **KORREKTES erase** ([[algorithm-correctness-when-named]]): Tombstone-Markierung (NICHT bloßes
// Leeren — das wuerde bei Linear Probing die Probe-Kette nachfolgender Schluessel brechen). Tombstones
// werden bei insert wiederverwendet + beim Rehash (Resize) entfernt.
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** Lehrbuch-Algorithmus → C++23-Re-Impl,
// is_original=false. Allocation: std::vector — [[allocation-failure-exception]]: insert/rehash → bad_alloc.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::lookup {

class HashSearchAlgo : public SearchAlgoBase<HashSearchAlgo> {
public:
    static constexpr bool enabled = flags::hash_search_enabled;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 14>;  // S14

    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_fanout()        noexcept { return 65536; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "hash_search"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "HashSearchAlgo (open-addressing Fibonacci hash, linear probing — Knuth TAOCP 3 §6.4)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "HASH_SEARCH"; }

    [[nodiscard]] static constexpr bool supports_simd()            noexcept { return false; }  // Hash-Probing
    [[nodiscard]] static constexpr bool supports_range_scan()      noexcept { return false; }  // UNGEORDNET — Kern-Unterschied
    [[nodiscard]] static constexpr bool is_dense()                 noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return false; }

    static constexpr std::uint64_t kFibonacciMul   = 11400714819323198485ULL;
    static constexpr std::size_t   kInitialCapacity = 16;  // Power-of-2

    HashSearchAlgo() : buckets_(kInitialCapacity), mask_(kInitialCapacity - 1) {}

    [[nodiscard]] bool operator==(HashSearchAlgo const& other) const noexcept {
        return size_ == other.size_;
    }

    /// SONDERFALL [[allocation-failure-exception]]: rehash kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        if ((size_ + tombstones_) * 10 >= (mask_ + 1) * 7) rehash((mask_ + 1) * 2);
        std::size_t const cap = mask_ + 1;
        std::size_t const start = hash_index(k);
        std::size_t first_deleted = kNpos;
        for (std::size_t i = 0; i < cap; ++i) {
            std::size_t const pos = (start + i) & mask_;
            Slot& s = buckets_[pos];
            if (s.state == SlotState::Empty) {
                std::size_t const target = (first_deleted != kNpos) ? first_deleted : pos;
                if (first_deleted != kNpos) --tombstones_;  // Tombstone wiederverwendet
                buckets_[target] = Slot{k, v, SlotState::Occupied};
                ++size_;
                notify_insert();
                return;
            }
            if (s.state == SlotState::Deleted) {
                if (first_deleted == kNpos) first_deleted = pos;
            } else if (s.key == k) {  // Occupied + gleicher Key → Update
                s.val = v;
                notify_insert();
                return;
            }
        }
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        std::size_t const cap = mask_ + 1;
        std::size_t const start = hash_index(k);
        for (std::size_t i = 0; i < cap; ++i) {
            std::size_t const pos = (start + i) & mask_;
            Slot const& s = buckets_[pos];
            if (s.state == SlotState::Empty) break;                       // Kette zu Ende → Miss
            if (s.state == SlotState::Occupied && s.key == k) { result = s.val; break; }
            // Deleted oder Occupied(anderer Key) → weiter proben
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        if (result) ++stats_.total_hit_count; else ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        return result;
    }

    bool erase(key_type k) {
        std::size_t const cap = mask_ + 1;
        std::size_t const start = hash_index(k);
        for (std::size_t i = 0; i < cap; ++i) {
            std::size_t const pos = (start + i) & mask_;
            Slot& s = buckets_[pos];
            if (s.state == SlotState::Empty) return false;
            if (s.state == SlotState::Occupied && s.key == k) {
                s.state = SlotState::Deleted;  // Tombstone — Probe-Kette bleibt intakt
                --size_;
                ++tombstones_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
                ++stats_.total_erase_count; observer_.notify(stats_);
#endif
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return size_; }
    [[nodiscard]] double    density_percent() const noexcept {
        return 100.0 * static_cast<double>(size_) / 65536.0;
    }
    void clear() noexcept {
        for (auto& s : buckets_) s = Slot{};
        size_ = 0;
        tombstones_ = 0;
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]].
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        if (size_ > 1024) return concepts::DensityClass::Dense;
        if (size_ > 64)   return concepts::DensityClass::Balanced;
        return concepts::DensityClass::Sparse;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::SearchAlgoStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot()   const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; observer_.notify(stats_); }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer()       noexcept { return observer_; }
#endif

private:
    enum class SlotState : std::uint8_t { Empty, Occupied, Deleted };
    struct Slot { key_type key{}; value_type val{}; SlotState state{SlotState::Empty}; };
    static constexpr std::size_t kNpos = static_cast<std::size_t>(-1);

    [[nodiscard]] std::size_t hash_index(key_type k) const noexcept {
        return static_cast<std::size_t>((static_cast<std::uint64_t>(k) * kFibonacciMul)) & mask_;
    }

    void rehash(std::size_t new_capacity) {
        std::vector<Slot> old;
        old.swap(buckets_);
        buckets_.assign(new_capacity, Slot{});
        mask_ = new_capacity - 1;
        size_ = 0;
        tombstones_ = 0;
        for (auto const& s : old) {
            if (s.state != SlotState::Occupied) continue;  // Tombstones entfallen beim Rehash
            std::size_t const start = hash_index(s.key);
            for (std::size_t i = 0; i < new_capacity; ++i) {
                std::size_t const pos = (start + i) & mask_;
                if (buckets_[pos].state == SlotState::Empty) {
                    buckets_[pos] = Slot{s.key, s.val, SlotState::Occupied};
                    ++size_;
                    break;
                }
            }
        }
    }

    void notify_insert() noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (size_ > stats_.peak_occupancy) stats_.peak_occupancy = size_;
        observer_.notify(stats_);
#endif
    }

    std::vector<Slot> buckets_;
    std::size_t       mask_;
    std::size_t       size_ = 0;
    std::size_t       tombstones_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                      observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::lookup {
    static_assert(concepts::SearchAlgoVariant<HashSearchAlgo>);
    static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<HashSearchAlgo>);
    static_assert(concepts::DensityClassifiedStrategy<HashSearchAlgo>);
}
