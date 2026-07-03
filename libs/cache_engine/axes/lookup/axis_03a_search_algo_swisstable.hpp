#pragma once
// AP-7a/#241 axis_03a_search_algo SwissTableSearchAlgo S22 (2026-07-03)
//
// @topic traversal @achse 03a @family S22 SwissTableSearchAlgo
// @subaxis SA2 sparse_access (UNGEORDNETE Swiss-Table-Hash-Struktur -- kein Range-Scan)
//
// **Algorithmus:** CE-native Re-Implementation des Swiss-Table-Prinzips aus Google/abseil
// raw_hash_set ("Swiss Tables", CppCon 2017): offene Adressierung mit einem Control-Byte je Slot
// (kEmpty=0x80, kDeleted=0xFE, sonst 7-Bit-H2-Fingerprint), Gruppen von 16 Slots, H1/H2-Split
// (H1 -> Gruppen-Startposition, H2 -> Fingerprint) und Tombstones fuer erase.
//
// **AP-7b:** Weg-B-Organ (SwissTableOrgan) fuer den echten Mess-Pfad noch offen -- bis dahin Flag
// Default-OFF (nur registriert/konform, wie S18-S21 per-K). Ohne AP-7b darf S22 nicht als echte
// SwissTable-Messung interpretiert werden.
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** C++23-Re-Impl nach dem veroeffentlichten
// Swiss-Table-Design, kein abseil/ext-Wrap, is_original=false. Allocation: std::vector --
// [[allocation-failure-exception]]: insert/rehash -> bad_alloc.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::lookup {

class SwissTableSearchAlgo : public SearchAlgoBase<SwissTableSearchAlgo> {
public:
    static constexpr bool enabled = flags::swisstable_enabled;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 22>; // S22

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      max_fanout() noexcept { return 65536; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "swisstable"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "SwissTableSearchAlgo (Google Swiss Tables, CppCon 2017 / abseil raw_hash_set -- "
               "CE-native Reimpl, is_original=false)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "SWISSTABLE"; }

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; } // AP-7-Follow: SIMD-Gruppen-Probe
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept {
        return false;
    } // UNGEORDNET -- Pool-Familie
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return false; }

    static constexpr std::uint64_t kFibonacciMul    = 11400714819323198485ULL;
    static constexpr std::size_t   kGroupWidth      = 16;
    static constexpr std::size_t   kInitialCapacity = 16; // Power-of-2, exakt eine Control-Gruppe
    static constexpr std::uint8_t  kEmpty           = 0x80u;
    static constexpr std::uint8_t  kDeleted         = 0xFEu;

    SwissTableSearchAlgo() : slots_(kInitialCapacity), ctrl_(kInitialCapacity, kEmpty), mask_(kInitialCapacity - 1) {}

    [[nodiscard]] bool operator==(SwissTableSearchAlgo const& other) const noexcept { return size_ == other.size_; }

    /// SONDERFALL [[allocation-failure-exception]]: rehash kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        if ((size_ + tombstones_ + 1u) * 8u >= (mask_ + 1u) * 7u) rehash((mask_ + 1u) * 2u);
        insert_impl(k, v, true);
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        std::uint64_t const       hash   = mixed_hash(k);
        std::uint8_t const        h2     = h2_fingerprint(hash);
        std::size_t const         groups = group_count();
        for (std::size_t probe = 0; probe < groups; ++probe) {
            std::size_t const group_start = group_start_for(hash, probe);
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (ctrl_[pos] == h2 && slots_[pos].key == k) {
                    result = slots_[pos].val;
                    goto done;
                }
            }
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                if (ctrl_[group_start + i] == kEmpty) goto done; // Probe-Kette zu Ende -> Miss
            }
        }
done:
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        if (result)
            ++stats_.total_hit_count;
        else
            ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        return result;
    }

    bool erase(key_type k) {
        std::uint64_t const hash   = mixed_hash(k);
        std::uint8_t const  h2     = h2_fingerprint(hash);
        std::size_t const   groups = group_count();
        for (std::size_t probe = 0; probe < groups; ++probe) {
            std::size_t const group_start = group_start_for(hash, probe);
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (ctrl_[pos] == h2 && slots_[pos].key == k) {
                    ctrl_[pos]  = kDeleted; // Tombstone -- Probe-Kette bleibt intakt
                    slots_[pos] = Slot{};
                    --size_;
                    ++tombstones_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
                    ++stats_.total_erase_count;
                    observer_.notify(stats_);
#endif
                    return true;
                }
            }
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                if (ctrl_[group_start + i] == kEmpty) return false;
            }
        }
        return false;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return size_; }
    [[nodiscard]] double    density_percent() const noexcept { return 100.0 * static_cast<double>(size_) / 65536.0; }
    void                    clear() noexcept {
        for (auto& s : slots_) s = Slot{};
        std::fill(ctrl_.begin(), ctrl_.end(), kEmpty);
        size_       = 0;
        tombstones_ = 0;
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]].
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        if (size_ > 1024) return concepts::DensityClass::Dense;
        if (size_ > 64) return concepts::DensityClass::Balanced;
        return concepts::DensityClass::Sparse;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::SearchAlgoStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    }
    // CoW-Memento (#142/Audit-K3): Stat-POD-Restore -> organ_cow_capable_v aktiv (spiegelt Observable-Huelle).
    void restore_statistics(snapshot_t const& s) noexcept {
        stats_ = s;
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }
#endif

private:
    struct Slot {
        key_type   key{};
        value_type val{};
    };
    static constexpr std::size_t kNpos = static_cast<std::size_t>(-1);

    [[nodiscard]] static constexpr std::uint64_t mixed_hash(key_type k) noexcept {
        return static_cast<std::uint64_t>(k) * kFibonacciMul;
    }

    [[nodiscard]] static constexpr std::uint8_t h2_fingerprint(std::uint64_t hash) noexcept {
        return static_cast<std::uint8_t>(hash & 0x7Fu);
    }

    [[nodiscard]] std::size_t group_count() const noexcept { return (mask_ + 1u) / kGroupWidth; }

    [[nodiscard]] std::size_t group_start_for(std::uint64_t hash, std::size_t probe) const noexcept {
        std::size_t const group_mask = group_count() - 1u;
        std::size_t const h1_group   = static_cast<std::size_t>(hash >> 7u) & group_mask;
        return ((h1_group + probe) & group_mask) * kGroupWidth;
    }

    void insert_impl(key_type k, value_type v, bool notify) {
        std::uint64_t const hash          = mixed_hash(k);
        std::uint8_t const  h2            = h2_fingerprint(hash);
        std::size_t const   groups        = group_count();
        std::size_t         first_deleted = kNpos;
        for (std::size_t probe = 0; probe < groups; ++probe) {
            std::size_t const group_start = group_start_for(hash, probe);
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (ctrl_[pos] == h2 && slots_[pos].key == k) {
                    slots_[pos].val = v;
                    if (notify) notify_insert();
                    return;
                }
            }
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (ctrl_[pos] == kDeleted && first_deleted == kNpos) first_deleted = pos;
            }
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (ctrl_[pos] == kEmpty) {
                    std::size_t const target = (first_deleted != kNpos) ? first_deleted : pos;
                    if (first_deleted != kNpos) --tombstones_; // Tombstone wiederverwendet
                    ctrl_[target]  = h2;
                    slots_[target] = Slot{k, v};
                    ++size_;
                    if (notify) notify_insert();
                    return;
                }
            }
        }

        rehash((mask_ + 1u) * 2u);
        insert_impl(k, v, notify);
    }

    void rehash(std::size_t new_capacity) {
        std::vector<Slot>         old_slots;
        std::vector<std::uint8_t> old_ctrl;
        old_slots.swap(slots_);
        old_ctrl.swap(ctrl_);
        slots_.assign(new_capacity, Slot{});
        ctrl_.assign(new_capacity, kEmpty);
        mask_       = new_capacity - 1u;
        size_       = 0;
        tombstones_ = 0;
        for (std::size_t i = 0; i < old_ctrl.size(); ++i) {
            if (old_ctrl[i] == kEmpty || old_ctrl[i] == kDeleted) continue;
            insert_impl(old_slots[i].key, old_slots[i].val, false);
        }
    }

    void notify_insert() noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (size_ > stats_.peak_occupancy) stats_.peak_occupancy = size_;
        observer_.notify(stats_);
#endif
    }

    std::vector<Slot>         slots_;
    std::vector<std::uint8_t> ctrl_;
    std::size_t              mask_;
    std::size_t              size_       = 0;
    std::size_t              tombstones_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<SwissTableSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<SwissTableSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<SwissTableSearchAlgo>);
} // namespace comdare::cache_engine::lookup
