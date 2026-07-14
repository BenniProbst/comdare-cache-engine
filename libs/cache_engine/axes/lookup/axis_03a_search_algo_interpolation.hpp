#pragma once
// V41.F.6.1 R7.2 axis_03a_search_algo InterpolationSearchAlgo S11 (2026-05-29)
//
// @topic traversal @achse 03a @family S11 InterpolationSearchAlgo
// @subaxis SA2 sparse_access (sortierte Keys, Such-METHODE = interpolierend statt halbierend)
//
// **Algorithmus:** Interpolationssuche — schaetzt die wahrscheinliche Position des Ziels per
// linearer Interpolation zwischen den Intervall-Endpunkten statt blinder Halbierung. Beschrieben in:
//   Y. Perl, A. Itai, H. Avni: "Interpolation search — a log log N search."
//   Communications of the ACM 21(7), 1978, S. 550-553. DOI 10.1145/359545.359557.
//
// Positionsschaetzung in [lo,hi]:  pos = lo + (key - keys[lo]) * (hi - lo) / (keys[hi] - keys[lo]).
// Bei gleichverteilten Keys O(log log N) im Mittel (vs. O(log N) Binärsuche); Worst-Case O(N)
// bei stark schiefer Verteilung. Distinkt von KArySearchAlgo (partitionsbasiert) und den
// lower_bound-Wrappern (halbierend): VERTEILUNGS-bewusste Such-METHODE — trifft das F15-Thema
// (Such-Methode + Key-Verteilung im std::map-Innenleben → Performance). Datenabhaengiger Index
// → NICHT SIMD-vektorisierbar (supports_simd=false, kein simd_lookup, kein SimdCapableStrategy).
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** klassischer Lehrbuch-Algorithmus,
// kein kanonischer permissiver Repo-Code → originalgetreue C++23-Re-Implementierung,
// is_original = false (via AxisBase-Default).
//
// Erfuellt: SearchAlgoVariant, CacheEngineSearchAlgoPermutationStrategy, DensityClassifiedStrategy.
//
// Allocation: std::vector dynamisch — [[allocation-failure-exception]]: insert kann
// std::bad_alloc werfen.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
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

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::lookup {

class InterpolationSearchAlgo : public SearchAlgoBase<InterpolationSearchAlgo> {
public:
    static constexpr bool enabled = flags::interpolation_enabled;
    // (E-Welle-A2 / Befund-2 / A2.4-S1) Array-Familie (Interpolationssuche über sortiert-aufsteigenden flachen Slot-Store)
    // → store-traversierbar: Suche über DENSELBEN LayoutAwareChunkedStore (node/layout/allocator wirken real). G3-klassifiziert.
    static constexpr bool axis_03a_store_traversable = true;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 11>; // S11

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      max_fanout() noexcept { return 65536; } // u16 Keyraum
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "interpolation"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::InterpolationSearchAlgo",
                                  "axes/lookup/axis_03a_search_algo_interpolation.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "InterpolationSearchAlgo (interpolation search — Perl/Itai/Avni CACM 1978)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "INTERPOLATION"; }

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; }      // datenabhaengiger Index
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // sortiert
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }           // sparse sortiert
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

    InterpolationSearchAlgo() noexcept = default;

    [[nodiscard]] bool operator==(InterpolationSearchAlgo const& other) const noexcept {
        return keys_.size() == other.keys_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: insert kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        auto        it  = std::lower_bound(keys_.begin(), keys_.end(), k);
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        if (it != keys_.end() && *it == k) {
            values_[idx] = v; // update
        } else {
            keys_.insert(it, k);
            values_.insert(values_.begin() + static_cast<std::ptrdiff_t>(idx), v);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (keys_.size() > stats_.peak_occupancy) stats_.peak_occupancy = keys_.size();
        observer_.notify(stats_);
#endif
    }

    /// Interpolationssuche (Perl/Itai/Avni 1978): Positionsschaetzung per linearer Interpolation.
    /// Keys sind eindeutig + sortiert → keys[hi] > keys[lo] solange lo < hi (Divisor >= 1).
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        std::size_t const         n      = keys_.size();
        if (n != 0) {
            std::size_t lo = 0, hi = n - 1;
            while (lo <= hi && k >= keys_[lo] && k <= keys_[hi]) {
                if (lo == hi) {
                    if (keys_[lo] == k) result = values_[lo];
                    break;
                }
                key_type const key_lo = keys_[lo];
                key_type const key_hi = keys_[hi]; // > key_lo (eindeutig sortiert, lo < hi)
                // pos = lo + (k - key_lo) * (hi - lo) / (key_hi - key_lo); Produkt passt in u64.
                std::uint64_t const num =
                    static_cast<std::uint64_t>(static_cast<unsigned>(k) - static_cast<unsigned>(key_lo)) *
                    static_cast<std::uint64_t>(hi - lo);
                std::size_t const pos =
                    lo + static_cast<std::size_t>(num / static_cast<std::uint64_t>(static_cast<unsigned>(key_hi) -
                                                                                   static_cast<unsigned>(key_lo)));
                // pos in [lo, hi] (k <= key_hi ⇒ num/divisor <= hi - lo).
                key_type const at = keys_[pos];
                if (at == k) {
                    result = values_[pos];
                    break;
                }
                if (at < k)
                    lo = pos + 1; // pos < hi hier (sonst at == key_hi >= k)
                else
                    hi = pos - 1; // pos > lo hier (sonst at == key_lo <= k) ⇒ kein Underflow
            }
        }
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
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        if (it == keys_.end() || *it != k) return false;
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        keys_.erase(it);
        values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(idx));
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
    void clear() noexcept {
        keys_.clear();
        values_.clear();
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]]: Belegungs-basierte Klassifikation.
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        std::size_t const n = keys_.size();
        if (n > 1024) return concepts::DensityClass::Dense;
        if (n > 64) return concepts::DensityClass::Balanced;
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
static_assert(concepts::SearchAlgoVariant<InterpolationSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<InterpolationSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<InterpolationSearchAlgo>);
} // namespace comdare::cache_engine::lookup
