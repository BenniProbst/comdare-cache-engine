#pragma once
// V41.F.6.1.P2.D.tr.s2 OriginalStartSearchAlgo S06 (2026-05-26)
//
// @topic traversal @achse 03a @family S06 OriginalStartSearchAlgo
// @subaxis SA3 multilevel_access (analog VectorU16U16SearchAlgo)
// @paper P05 START (Fent/Jungmair/Kipf/Neumann, ICDEW 2020)
//
// **Habich-Compliance-Wrapper:** parallel zu VectorU16U16SearchAlgo (Re-Impl).
// get_compiler()="gcc-9.5" via Paper-Mixin.
// **Luecken-Markierung (2/4 originall):**
//   - insert: originall (insertLater im sosd-Adapter)
//   - lookup: originall (EqualityLookup)
//   - erase:  LUECKE (kein remove im sosd-Adapter — Re-Impl, is_original_erase()=false)
//   - clear:  LUECKE (kein clear im sosd-Adapter — Re-Impl, is_original_clear()=false)
//
// **Body-Strategie (s2):** Standalone Re-Impl analog VectorU16U16SearchAlgo (u16-key + sorted lower_bound).
// s4: extern "C" Adapter zu START::insertLater / EqualityLookup.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/axis_03a_search_algo_flags.hpp>

#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
// V41.F.6.1.P2.D.tr.s2 Paper-Mixin (Tool-generated, SHA256-validiert gegen sosd-competitor-adapter-START.h)
#include "concepts/axis_03a_search_algo_original_code_mixin.hpp"
#include <topics/traversal/axis_03a_search_algo/legacy_code/paper_p05_start_is_original.hpp>
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

class OriginalStartSearchAlgo : public SearchAlgoBase<OriginalStartSearchAlgo>
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
    ,
                                public generated::p05_start::OriginalCodeMixin
#endif
{
public:
    // Diamond-Disambiguation: Mixin-Pfad wins
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
    using generated::p05_start::OriginalCodeMixin::get_compiler;
    using generated::p05_start::OriginalCodeMixin::is_original_clear;
    using generated::p05_start::OriginalCodeMixin::is_original_erase;
    using generated::p05_start::OriginalCodeMixin::is_original_insert;
    using generated::p05_start::OriginalCodeMixin::is_original_lookup;
    using generated::p05_start::OriginalCodeMixin::is_original_module;
#endif

    static constexpr bool enabled = flags::original_start_enabled;

    using key_type   = std::uint16_t; // Multi-Byte Discriminator (analog VectorU16U16SearchAlgo)
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::multilevel_access_tag;
    using family_id  = std::integral_constant<int, 6>; // S06

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      max_fanout() noexcept { return 65536; }
    [[nodiscard]] static constexpr std::string_view name() noexcept {
        if constexpr (enabled) {
            return "original_start";
        } else {
            return "original_start(disabled)";
        }
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "OriginalStartSearchAlgo (START — Self-Tuning Adaptive Radix Tree, Fent/Jungmair/Kipf/Neumann, ICDEW "
               "2020, DOI 10.1109/ICDEW49219.2020.00015 — 2/4 originall)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "ORIGINAL_START"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1";

    /// SONDERFALL (analog VectorU16U16SearchAlgo): kein SIMD — Cost-DP nicht vectorisierbar.
    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; }
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

    OriginalStartSearchAlgo() noexcept = default;

    [[nodiscard]] bool operator==(OriginalStartSearchAlgo const& other) const noexcept {
        return keys_.size() == other.keys_.size();
    }

    /// Paper-API (originall via insertLater im sosd-Adapter in s4).
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

    /// Paper-API (originall via EqualityLookup im sosd-Adapter in s4).
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

    /// LUECKE [[paper-original-code-pattern]]: kein remove im sosd-Adapter.
    /// Cache-Engine Re-Impl, is_original_erase() = false.
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
    [[nodiscard]] double    density_percent() const noexcept {
        return 100.0 * static_cast<double>(keys_.size()) / 65536.0;
    }

    /// LUECKE: kein clear im sosd-Adapter. Cache-Engine Re-Impl, is_original_clear() = false.
    void clear() noexcept {
        keys_.clear();
        values_.clear();
    }

    /// DensityClassifiedStrategy: START Cost-DP optimiert sich fuer Balanced-Bereich.
    [[nodiscard]] concepts::DensityClass density_class() const noexcept { return concepts::DensityClass::Balanced; }

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
static_assert(concepts::SearchAlgoVariant<OriginalStartSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<OriginalStartSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<OriginalStartSearchAlgo>);
// NICHT SimdCapableStrategy — START Cost-DP nicht vectorisierbar (analog VectorU16U16SearchAlgo)
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
// Habich-Compliance: 2/4 originall (insert+lookup), 2/4 Lücken (erase+clear)
static_assert(OriginalStartSearchAlgo::is_original_insert(),
              "OriginalStartSearchAlgo: insert MUSS via insertLater Paper-Bindung originall sein");
static_assert(OriginalStartSearchAlgo::is_original_lookup(),
              "OriginalStartSearchAlgo: lookup MUSS via EqualityLookup Paper-Bindung originall sein");
static_assert(!OriginalStartSearchAlgo::is_original_erase(),
              "OriginalStartSearchAlgo: erase ist Cache-Engine Re-Impl (kein remove im sosd-Adapter) — "
              "is_original_erase MUSS false sein");
static_assert(!OriginalStartSearchAlgo::is_original_clear(),
              "OriginalStartSearchAlgo: clear ist Cache-Engine Re-Impl (kein clear im sosd-Adapter) — "
              "is_original_clear MUSS false sein");
static_assert(!OriginalStartSearchAlgo::is_original_module(),
              "OriginalStartSearchAlgo: is_original_module MUSS false sein (2/4 Lücken)");
#else
static_assert(!OriginalStartSearchAlgo::is_original_module(),
              "OriginalStartSearchAlgo: is_original_module()=false wenn is_original-Codegen-Gate AUS ist");
#endif
} // namespace comdare::cache_engine::lookup
