#pragma once
// V41.F.6.1.P2.D.tr.s3 OriginalSurfSearchAlgo S08 (2026-05-26)
//
// @paper P10 SuRF (Zhang/Lim/Andersen et al. SIGMOD 2018)
// @source ext/traversal/P10-SuRF/SuRF/include/surf.hpp
//
// 1/4 originall (nur lookupKey), 3 Lücken (insert/erase/clear).
// SuRF ist Read-Only Succinct-Range-Filter (Bulk-Loaded Trie).

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
#include "concepts/axis_03a_search_algo_original_code_mixin.hpp"
#include <topics/traversal/axis_03a_search_algo/legacy_code/paper_p10_surf_is_original.hpp>
#endif

#include <measurement/measurable_concept.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::lookup {

class OriginalSurfSearchAlgo : public SearchAlgoBase<OriginalSurfSearchAlgo>
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
    , public generated::p10_surf::OriginalCodeMixin
#endif
{
public:
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
    using generated::p10_surf::OriginalCodeMixin::get_compiler;
    using generated::p10_surf::OriginalCodeMixin::is_original_clear;
    using generated::p10_surf::OriginalCodeMixin::is_original_erase;
    using generated::p10_surf::OriginalCodeMixin::is_original_insert;
    using generated::p10_surf::OriginalCodeMixin::is_original_lookup;
    using generated::p10_surf::OriginalCodeMixin::is_original_module;
#endif

    static constexpr bool enabled = flags::original_surf_enabled;

    using key_type   = std::uint8_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 8>; // S08

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      max_fanout() noexcept { return 256; }
    [[nodiscard]] static constexpr std::string_view name() noexcept {
        return enabled ? std::string_view{"original_surf"} : std::string_view{"original_surf(disabled)"};
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "OriginalSurfSearchAlgo (SuRF Zhang/Lim/Andersen SIGMOD 2018, Read-Only-Index, 1/4 originall)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "ORIGINAL_SURF"; }

    [[nodiscard]] static constexpr bool supports_simd() noexcept {
        return false;
    } // LOUDS-Trie nicht SIMD-vectorisierbar
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // SuRF Pflicht-API lookupRange
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

    OriginalSurfSearchAlgo() noexcept = default;
    [[nodiscard]] bool operator==(OriginalSurfSearchAlgo const& other) const noexcept {
        return keys_.size() == other.keys_.size();
    }

    /// LUECKE: kein incremental Insert in SuRF (Bulk-Loaded) — Re-Impl als Sorted-Insert.
    void insert(key_type k, value_type v) {
        auto        it  = std::lower_bound(keys_.begin(), keys_.end(), k);
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        if (it != keys_.end() && *it == k)
            values_[idx] = v;
        else {
            keys_.insert(it, k);
            values_.insert(values_.begin() + idx, v);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (keys_.size() > stats_.peak_occupancy) stats_.peak_occupancy = keys_.size();
        observer_.notify(stats_);
#endif
    }

    /// Paper-API (originall via SuRF::lookupKey in s4).
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        if (it != keys_.end() && *it == k)
            ++stats_.total_hit_count;
        else
            ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        if (it == keys_.end() || *it != k) return std::nullopt;
        return values_[static_cast<std::size_t>(it - keys_.begin())];
    }

    /// LUECKE: SuRF ist Read-Only — Re-Impl als Tombstone-Set.
    bool erase(key_type k) {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        if (it == keys_.end() || *it != k) return false;
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        keys_.erase(it);
        values_.erase(values_.begin() + idx);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return keys_.size(); }
    [[nodiscard]] double density_percent() const noexcept { return 100.0 * static_cast<double>(keys_.size()) / 256.0; }
    /// LUECKE: kein Clear in SuRF Paper.
    void clear() noexcept {
        keys_.clear();
        values_.clear();
    }

    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        return concepts::DensityClass::Sparse; // Succinct-Filter
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
    std::vector<key_type>   keys_;
    std::vector<value_type> values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<OriginalSurfSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<OriginalSurfSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<OriginalSurfSearchAlgo>);
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
static_assert(OriginalSurfSearchAlgo::is_original_lookup(),
              "OriginalSurfSearchAlgo: lookup MUSS via lookupKey Paper-Bindung originall sein");
static_assert(!OriginalSurfSearchAlgo::is_original_insert(),
              "OriginalSurfSearchAlgo: insert ist Re-Impl Luecke (SuRF Read-Only) — MUSS false sein");
static_assert(!OriginalSurfSearchAlgo::is_original_erase(), "OriginalSurfSearchAlgo: erase ist Re-Impl Luecke");
static_assert(!OriginalSurfSearchAlgo::is_original_clear(), "OriginalSurfSearchAlgo: clear ist Re-Impl Luecke");
static_assert(!OriginalSurfSearchAlgo::is_original_module(),
              "OriginalSurfSearchAlgo: is_original_module MUSS false sein (3/4 Luecken)");
#else
static_assert(!OriginalSurfSearchAlgo::is_original_module(),
              "OriginalSurfSearchAlgo: is_original_module()=false wenn is_original-Codegen-Gate AUS ist");
#endif
} // namespace comdare::cache_engine::lookup
