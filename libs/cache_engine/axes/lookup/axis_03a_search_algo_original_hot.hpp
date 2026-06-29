#pragma once
// V41.F.6.1.P2.D.tr.s2 OriginalHotSearchAlgo S05 (2026-05-26)
//
// @topic traversal @achse 03a @family S05 OriginalHotSearchAlgo
// @subaxis SA2 sparse_access (analog VectorU8U8SearchAlgo)
// @paper P02 HOT (Binna/Zangerle/Pichl/Specht, SIGMOD 2018; erw. ACM TODS 2022)
//
// **Habich-Compliance-Wrapper:** parallel zu VectorU8U8SearchAlgo (Re-Impl).
// get_compiler()="gcc-9.5" via Paper-Mixin.
// **Luecken-Markierung (2/4 originall):**
//   - insert: originall (HOTRowex::insert)
//   - lookup: originall (HOTRowex::lookup)
//   - erase:  LUECKE (gemappt auf HOTRowex, das append-only ist — Re-Impl, is_original_erase()=false).
//             Praezisierung (#43 s4): NUR die nebenlaeufige HOTRowex-Variante ist append-only; das
//             single-threaded HOT (HOTSingleThreaded::removeEntry) HAT ein erase. Da der Wrapper auf
//             rowex gemappt ist, bleibt is_original_erase()=false korrekt.
//   - clear:  LUECKE (kein clear im Paper — Re-Impl, is_original_clear()=false)
// is_original_module()=false (weil 2/4 Lücken).
//
// **Body-Strategie (s2):** Standalone Re-Impl analog VectorU8U8SearchAlgo (sorted lower_bound).
// s4: extern "C" Adapter zu HOTRowex<KeyType, std::uint64_t>::insert/lookup.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include "concepts/axis_03a_search_algo_simd_capable_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/axis_03a_search_algo_flags.hpp>

// V41.F.6.1.P2.D.tr.s2 Paper-Mixin (Tool-generated, SHA256-validiert gegen HOTRowex.hpp)
#include "concepts/axis_03a_search_algo_original_code_mixin.hpp"
#include <topics/traversal/axis_03a_search_algo/legacy_code/paper_p02_hot_is_original.hpp>

#include <measurement/measurable_concept.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::lookup {

class OriginalHotSearchAlgo : public SearchAlgoBase<OriginalHotSearchAlgo>,
                              public generated::p02_hot::OriginalCodeMixin {
public:
    // Diamond-Disambiguation: Mixin-Pfad wins
    using generated::p02_hot::OriginalCodeMixin::get_compiler;
    using generated::p02_hot::OriginalCodeMixin::is_original_clear;
    using generated::p02_hot::OriginalCodeMixin::is_original_erase;
    using generated::p02_hot::OriginalCodeMixin::is_original_insert;
    using generated::p02_hot::OriginalCodeMixin::is_original_lookup;
    using generated::p02_hot::OriginalCodeMixin::is_original_module;

    static constexpr bool enabled = flags::original_hot_enabled;

    using key_type   = std::uint8_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 5>; // S05

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      max_fanout() noexcept { return 256; } // sparse, theoretisch
    [[nodiscard]] static constexpr std::string_view name() noexcept {
        if constexpr (enabled) {
            return "original_hot";
        } else {
            return "original_hot(disabled)";
        }
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "OriginalHotSearchAlgo (HOT Binna PVLDB 2018, HOTRowex Paper-Bindung — 2/4 originall)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "ORIGINAL_HOT"; }

    [[nodiscard]] static constexpr bool supports_simd() noexcept {
        return true;
    } // SIMD-faehig (lower_bound + Bit-Mask)
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // sorted
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }           // Patricia sparse
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

    OriginalHotSearchAlgo() noexcept = default;

    [[nodiscard]] bool operator==(OriginalHotSearchAlgo const& other) const noexcept {
        return keys_.size() == other.keys_.size();
    }

    /// Paper-API (originall via HOTRowex::insert in s4).
    /// SONDERFALL [[allocation-failure-exception]]: std::vector::insert kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        if constexpr (enabled) {
            auto        it  = std::lower_bound(keys_.begin(), keys_.end(), k);
            std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
            if (it != keys_.end() && *it == k) {
                values_[idx] = v;
            } else {
                keys_.insert(it, k);
                values_.insert(values_.begin() + idx, v);
            }
        } else {
            (void)k;
            (void)v;
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (keys_.size() > stats_.peak_occupancy) stats_.peak_occupancy = keys_.size();
        observer_.notify(stats_);
#endif
    }

    /// Paper-API (originall via HOTRowex::lookup in s4).
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        bool hit = (it != keys_.end() && *it == k);
        if (hit)
            ++stats_.total_hit_count;
        else
            ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        if (it == keys_.end() || *it != k) return std::nullopt;
        return values_[static_cast<std::size_t>(it - keys_.begin())];
    }

    [[nodiscard]] std::optional<value_type> simd_lookup(key_type k) const { return lookup(k); }

    /// LUECKE [[paper-original-code-pattern]]: HOT ist append-only Trie — kein
    /// remove im Paper. Cache-Engine Re-Impl, is_original_erase() = false (via Mixin).
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

    /// LUECKE: kein clear im HOT Paper. Cache-Engine Re-Impl, is_original_clear() = false.
    void clear() noexcept {
        keys_.clear();
        values_.clear();
    }

    /// DensityClassifiedStrategy: HOT Patricia ist typisch sparse.
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        double dp = density_percent();
        if (dp > 60.0) return concepts::DensityClass::Dense;
        if (dp > 30.0) return concepts::DensityClass::Balanced;
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
    std::vector<key_type>   keys_;
    std::vector<value_type> values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<OriginalHotSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<OriginalHotSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<OriginalHotSearchAlgo>);
static_assert(concepts::SimdCapableStrategy<OriginalHotSearchAlgo>);
// Habich-Compliance: 2/4 originall (insert+lookup), 2/4 Lücken (erase+clear)
static_assert(OriginalHotSearchAlgo::is_original_insert(),
              "OriginalHotSearchAlgo: insert MUSS via HOTRowex Paper-Bindung originall sein");
static_assert(OriginalHotSearchAlgo::is_original_lookup(),
              "OriginalHotSearchAlgo: lookup MUSS via HOTRowex Paper-Bindung originall sein");
static_assert(
    !OriginalHotSearchAlgo::is_original_erase(),
    "OriginalHotSearchAlgo: erase ist Cache-Engine Re-Impl (HOT ist append-only) — is_original_erase MUSS false sein");
static_assert(!OriginalHotSearchAlgo::is_original_clear(),
              "OriginalHotSearchAlgo: clear ist Cache-Engine Re-Impl (kein clear in HOT Paper) — is_original_clear "
              "MUSS false sein");
static_assert(!OriginalHotSearchAlgo::is_original_module(),
              "OriginalHotSearchAlgo: is_original_module MUSS false sein (2/4 Lücken)");
} // namespace comdare::cache_engine::lookup
