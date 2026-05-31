#pragma once
// V41.F.6.1 R7.2 axis_03a_search_algo KArySearchAlgo S10 (2026-05-29)
//
// @topic traversal @achse 03a @family S10 KArySearchAlgo
// @subaxis SA2 sparse_access (sortierte Keys, Such-METHODE = k-ary statt binary)
//
// **Algorithmus:** k-ary search — Verallgemeinerung der Binärsuche, beschrieben in:
//   Benjamin Schlegel, Rainer Gemulla, Wolfgang Lehner: "k-ary search on modern
//   processors." DaMoN 2009 (5th Int. Workshop on Data Management on New Hardware),
//   Providence RI. DOI 10.1145/1565694.1565705.
//
// Binärsuche macht 1 Vergleich pro Iteration → Halbierung (log_2 n Iterationen).
// k-ary search macht K Vergleiche pro Iteration gegen K gleichverteilte Separatoren →
// Partition in K+1 Segmente (⌈log_(K+1) n⌉ Iterationen). Auf modernen Prozessoren
// amortisiert die parallele Vergleichsausführung (ILP/SIMD) die Mehrkosten pro Iteration,
// sodass die geringere Iterationszahl gewinnt — exakt das F15-Thema (Such-Methode im
// std::map-Innenleben → Performance). Die Arität K ist hier der iterable_aspect: die
// PermutationEngine misst K ∈ {2,4,8,16} (K=2 ≙ Binärsuche als Baseline).
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** Das Paper liefert die
// Algorithmen plattformunabhängig (Pseudocode) + Mess-Studie, KEINEN kanonischen
// permissiven Single-Repo-Code. Diese Implementierung ist eine originalgetreue
// C++23-Re-Implementierung der skalaren k-ary-Suche (sequential layout) →
// is_original = false (via AxisBase-Default). simd_lookup() markiert den
// data-level-parallel-Layout-Fast-Path (Paper §4); Pilot delegiert skalar.
//
// Erfuellt: SearchAlgoVariant, CacheEngineSearchAlgoPermutationStrategy,
//           DensityClassifiedStrategy, SimdCapableStrategy, IterableAspectSearchAlgoStrategy.
//
// Allocation: std::vector dynamisch — [[allocation-failure-exception]]: insert kann
// std::bad_alloc werfen.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include "concepts/axis_03a_search_algo_simd_capable_strategy_concept.hpp"
#include "concepts/axis_03a_search_algo_iterable_aspect_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::lookup {

class KArySearchAlgo : public SearchAlgoBase<KArySearchAlgo> {
public:
    static constexpr bool enabled = flags::k_ary_enabled;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 10>;  // S10

    /// iterable_aspect_t = Arität K (Such-METHODE). PermutationEngine erkennt HasIterableAspect<V>
    /// und generiert 1 Binary mit Runtime-Loop ueber kIterableArities statt 4 separate Binaries.
    /// K=2 ist die Binärsuch-Baseline; K∈{4,8,16} die echten k-ary-Varianten.
    using iterable_aspect_t = unsigned;
    static constexpr std::array<unsigned, 4> kIterableArities{2u, 4u, 8u, 16u};
    [[nodiscard]] static constexpr std::span<unsigned const> iterable_values() noexcept {
        return std::span<unsigned const>{kIterableArities.data(), kIterableArities.size()};
    }

    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_fanout()        noexcept { return 65536; }  // u16 Keyraum
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "k_ary"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "KArySearchAlgo (k-ary search — Schlegel/Gemulla/Lehner DaMoN 2009)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "K_ARY"; }

    [[nodiscard]] static constexpr bool supports_simd()            noexcept { return true; }   // data-parallel Layout (Paper §4)
    [[nodiscard]] static constexpr bool supports_range_scan()      noexcept { return true; }   // sortiert
    [[nodiscard]] static constexpr bool is_dense()                 noexcept { return false; }  // sparse sortiert
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

    static constexpr unsigned kDefaultArity = 4;  // 5-Wege-Partition (Paper-Beispiel)

    KArySearchAlgo() noexcept : arity_(kDefaultArity) {}
    explicit KArySearchAlgo(unsigned arity) noexcept : arity_(arity < 2u ? 2u : arity) {}

    [[nodiscard]] bool operator==(KArySearchAlgo const& other) const noexcept {
        return keys_.size() == other.keys_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: insert kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        if (it != keys_.end() && *it == k) {
            values_[idx] = v;  // update
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

    /// k-ary search (Schlegel DaMoN 2009): pro Iteration werden bis zu K gleichverteilte
    /// Separatoren verglichen → Partition in K+1 Segmente. Bei Restbreite <= K linearer Scan.
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        std::size_t const n = keys_.size();
        std::size_t lo = 0, hi = n;  // Halb-offenes Intervall [lo, hi)
        std::size_t const K = arity_;
        while (hi - lo > K) {
            std::size_t const width = hi - lo;
            std::size_t new_lo = lo, new_hi = hi;
            bool narrowed = false;
            for (std::size_t j = 1; j <= K; ++j) {
                std::size_t const pos = lo + (width * j) / (K + 1);  // Separator-Position in [lo, hi)
                key_type const sep = keys_[pos];
                if (sep == k) {  // Direkt-Treffer auf Separator
#ifdef COMDARE_CE_ENABLE_STATISTICS
                    ++stats_.total_lookup_count; ++stats_.total_hit_count; observer_.notify(stats_);
#endif
                    return values_[pos];
                }
                if (k < sep) { new_hi = pos; narrowed = true; break; }  // Ziel im Segment vor pos
                new_lo = pos + 1;                                        // Ziel hinter diesem Separator
            }
            lo = new_lo;
            if (narrowed) hi = new_hi;  // sonst: k > alle Separatoren → [letzter_sep+1, hi)
        }
        // Rest-Segment (Breite <= K): linearer Scan
        for (std::size_t i = lo; i < hi; ++i) {
            if (keys_[i] == k) { result = values_[i]; break; }
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        if (result) ++stats_.total_hit_count; else ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        return result;
    }

    /// SIMD-Fast-Path ([[simd-capable-strategy]]): data-level-parallel Layout (Paper §4) vergleicht
    /// die K Separatoren in einem SIMD-Register. Pilot delegiert skalar an lookup().
    [[nodiscard]] std::optional<value_type> simd_lookup(key_type k) const { return lookup(k); }

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
    void                    clear() noexcept { keys_.clear(); values_.clear(); }

    /// DensityClassifiedStrategy [[density-classified-strategy]]: Belegungs-basierte Klassifikation
    /// (k-ary search lohnt erst ab groesseren sortierten Regionen).
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        std::size_t const n = keys_.size();
        if (n > 1024) return concepts::DensityClass::Dense;
        if (n > 64)   return concepts::DensityClass::Balanced;
        return concepts::DensityClass::Sparse;
    }

    /// IterableAspectSearchAlgoStrategy [[iterable-aspect-strategy]]: Setter fuer die Arität K
    /// (konsolidierte Laufzeit-Permutation analog 03a-Schablone).
    void set_iterable_aspect(unsigned new_arity) noexcept { arity_ = (new_arity < 2u ? 2u : new_arity); }

    /// Accessor (Diagnostik): aktuelle Arität K.
    [[nodiscard]] unsigned arity() const noexcept { return arity_; }

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
    unsigned                arity_;
    std::vector<key_type>   keys_;
    std::vector<value_type> values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                      observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::lookup {
    static_assert(concepts::SearchAlgoVariant<KArySearchAlgo>);
    static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<KArySearchAlgo>);
    static_assert(concepts::DensityClassifiedStrategy<KArySearchAlgo>);
    static_assert(concepts::SimdCapableStrategy<KArySearchAlgo>);
    static_assert(concepts::IterableAspectSearchAlgoStrategy<KArySearchAlgo>);
}
