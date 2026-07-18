#pragma once
// V41.F.6.1.P2.D.tr.s2 OriginalArtSearchAlgo S04 (2026-05-26)
//
// @topic traversal @achse 03a @family S04 OriginalArtSearchAlgo
// @subaxis SA1 dense_access (analog Array256SearchAlgo)
// @paper P01 ART (Leis/Kemper/Neumann, ICDE 2013)
//
// **Habich-Compliance-Wrapper:** parallel zu Array256SearchAlgo (Re-Impl).
// get_compiler()="gcc-9.5" via Paper-Mixin (Tool-validierte Source-Identity ueber SHA256).
// is_original_module()=true (alle 4 Functions: insert/lookup/erase/clear originall).
//
// **Body-Strategie (s2):** Standalone Re-Impl analog Array256SearchAlgo (std::array<optional<u64>,256>).
// Compile-Time-Switch via `enabled = flags::original_art_enabled`. Wenn aktiviert,
// signalisiert die Wrapper-Klasse die Paper-Bindung via Mixin-Properties. Tatsaechliches
// Linking gegen unodb::db erfolgt in s4 (Library-Build mit Original-Compiler via
// cmake/compiler_cache.cmake + cmake/paper_binary.cmake).

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include "concepts/axis_03a_search_algo_simd_capable_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/axis_03a_search_algo_flags.hpp>

#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
// V41.F.6.1.P2.D.tr.s2 Paper-Mixin (Tool-generated, SHA256-validiert gegen art.hpp)
#include "concepts/axis_03a_search_algo_original_code_mixin.hpp"
#include <topics/traversal/axis_03a_search_algo/legacy_code/paper_p01_art_is_original.hpp>
#endif

#include <measurement/measurable_concept.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::lookup {

class OriginalArtSearchAlgo : public SearchAlgoBase<OriginalArtSearchAlgo>
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
    ,
                              public generated::p01_art::OriginalCodeMixin // Habich-Compliance Mixin
#endif
{
public:
    // Diamond-Disambiguation: Mixin-Pfad wins fuer get_compiler/is_original_*
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
    using generated::p01_art::OriginalCodeMixin::get_compiler;
    using generated::p01_art::OriginalCodeMixin::is_original_clear;
    using generated::p01_art::OriginalCodeMixin::is_original_erase;
    using generated::p01_art::OriginalCodeMixin::is_original_insert;
    using generated::p01_art::OriginalCodeMixin::is_original_lookup;
    using generated::p01_art::OriginalCodeMixin::is_original_module;
#endif

    static constexpr bool enabled = flags::original_art_enabled;

    using key_type   = std::uint8_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::dense_access_tag;
    using family_id  = std::integral_constant<int, 4>; // S04

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; } // unodb::db (NICHT olc_db)
    [[nodiscard]] static constexpr std::size_t max_fanout() noexcept { return 256; }
    [[nodiscard]] static constexpr std::string_view name() noexcept {
        if constexpr (enabled) {
            return "original_art";
        } else {
            return "original_art(disabled)";
        }
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "OriginalArtSearchAlgo (ART Leis ICDE 2013, unodb::db Paper-Bindung)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "ORIGINAL_ART"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return true; }
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; }
    [[nodiscard]] static constexpr bool is_dense() noexcept { return true; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

    OriginalArtSearchAlgo() noexcept : count_(0) {}

    [[nodiscard]] bool operator==(OriginalArtSearchAlgo const& other) const noexcept { return count_ == other.count_; }

    void insert(key_type k, value_type v) {
        if constexpr (enabled) {
            // s2 Standalone-Body (Array256SearchAlgo-Pattern). s4 wird ueber extern "C" Adapter
            // unodb::db<key,value>::insert_internal aufrufen.
            if (!data_[k].has_value()) ++count_;
            data_[k] = v;
        } else {
            (void)k;
            (void)v;
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (count_ > stats_.peak_occupancy) stats_.peak_occupancy = count_;
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        if (data_[k].has_value())
            ++stats_.total_hit_count;
        else
            ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        if constexpr (enabled) {
            return data_[k];
        } // s4: unodb::db::get(...)
        else {
            return std::nullopt;
        }
    }

    /// SIMD-Fast-Path (Sub-Concept Pflicht). ART Node256 ist O(1) direct addressed.
    [[nodiscard]] std::optional<value_type> simd_lookup(key_type k) const { return data_[k]; }

    bool erase(key_type k) {
        if constexpr (enabled) {
            if (!data_[k].has_value()) return false;
            data_[k].reset();
            --count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.total_erase_count;
            observer_.notify(stats_);
#endif
            return true;
        } else {
            (void)k;
            return false;
        }
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return count_; }
    [[nodiscard]] double    density_percent() const noexcept { return 100.0 * static_cast<double>(count_) / 256.0; }
    void                    clear() noexcept {
        if constexpr (enabled) {
            for (auto& slot : data_) slot.reset();
            count_ = 0;
        }
    }

    /// DensityClassifiedStrategy: ART Node256 ist per Konstruktion DENSE.
    [[nodiscard]] concepts::DensityClass density_class() const noexcept { return concepts::DensityClass::Dense; }

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
    std::array<std::optional<value_type>, 256> data_{};
    std::size_t                                count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<OriginalArtSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<OriginalArtSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<OriginalArtSearchAlgo>);
static_assert(concepts::SimdCapableStrategy<OriginalArtSearchAlgo>);
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
// Habich-Compliance: alle 4 Functions sind im Paper originall (P01 ART 4/4)
static_assert(OriginalArtSearchAlgo::is_original_module(),
              "OriginalArtSearchAlgo MUSS is_original_module()=true liefern (4/4 ART-API originall)");
#else
static_assert(!OriginalArtSearchAlgo::is_original_module(),
              "OriginalArtSearchAlgo: is_original_module()=false wenn is_original-Codegen-Gate AUS ist");
#endif
} // namespace comdare::cache_engine::lookup
