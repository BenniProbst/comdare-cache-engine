#pragma once
// V41.F.6.1.F.6 axis_03a_search_algo Array65535SearchAlgo S09 (2026-05-29)
//
// @topic traversal @achse 03a @family S09 Array65535SearchAlgo
// @subaxis SA4 direct_multibyte_access
//
// **Herkunft:** F.6-Migration aus prt-art `internal_search/array_65535.hpp`
// (REV 6 §5.17 — "Density 25-50 %"). Direkt-adressiertes Array mit
// std::uint16_t-Diskriminator, hoehere Verzweigungs-Tiefe als Array256
// (Single-Byte) und das direkt-adressierte Gegenstueck zu VectorU16U16
// (sortiert) im selben uint16-Fanout-Bereich.
//
// **Algorithmus-Pattern:** ART-artige direkte Adressierung, aber auf
// 2-Byte-Diskriminator erweitert (Mid-Density-Tier zwischen Array256 dense
// und sparse Patricia). Kein externes Paper — prt-art-Eigenentwurf.
//
// **Korrektheit ggü. prt-art-Original:** prt-art nutzte `kCapacity = 65535`
// mit `slots_[discriminator]` — ein uint16-Diskriminator kann jedoch 0..65535
// annehmen (65536 Werte), d.h. `discriminator == 65535` war im Original
// Out-of-Bounds (UB). Hier auf 65536 Slots korrigiert (voller uint16-Bereich),
// konsistent mit VectorU16U16 (Nenner 65536). Der Klassen-/Familienname bleibt
// "Array65535" als etablierte Taxonomie-Bezeichnung des ~64K-Fanout-Tiers.
//
// Erfuellt:
//   - SearchAlgoVariant (Pflicht-API)
//   - CacheEngineSearchAlgoPermutationStrategy (cache-engine-spec)
//   - DensityClassifiedStrategy (DensityClass::Balanced — Mid-Density 25-50 %)
//   - **NICHT** SimdCapableStrategy (direkter O(1)-Index, kein SIMD-Vorteil;
//     65536-Slot-Scan waere zudem zu breit fuer sinnvolle Vektorisierung)
//
// Allocation: zwei std::vector (Werte + Presence-Bits) heap-alloziert im
// Konstruktor — [[allocation-failure-exception]] (std::bad_alloc moeglich).
// Presence-Vektor statt Sentinel-Wert, damit jeder value_type (inkl. ~0ull)
// als gueltiger Slot-Wert darstellbar bleibt (keine Sentinel-Kollision).

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/composable/capacity_constraint.hpp>
#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::lookup {

class Array65535SearchAlgo : public SearchAlgoBase<Array65535SearchAlgo> {
public:
    static constexpr bool enabled = flags::array65535_enabled;
    // #188-4c-ii: faithful Flach-Store-Pfad via DirectAddressTraversal; #217-2b: Wrapper-key_type bleibt heute u16.
    static constexpr bool axis_03a_store_traversable = true;

    /// Voller uint16-Diskriminator-Bereich [0, 65535] = 65536 Slots (Korrektur
    /// der prt-art-65535-Off-by-one). Density-Zielband aus REV 6 §5.17.
    static constexpr std::size_t kCapacity          = 65536;
    static constexpr double      kDensityMinPercent = 25.0;
    static constexpr double      kDensityMaxPercent = 50.0;

    using key_type   = std::uint16_t; // #217-2b: native Wrapper-Breite bleibt heute; Umbau spaeter.
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::direct_multibyte_access_tag;
    using family_id  = std::integral_constant<int, 9>; // S09

    static_assert(static_cast<std::uint64_t>(std::numeric_limits<key_type>::max()) >=
                      static_cast<std::uint64_t>(kCapacity - 1u),
                  "Array65535SearchAlgo key_type muss jeden deklarierten Static-Slot adressieren koennen");

    [[nodiscard]] static constexpr bool                           is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t                    max_fanout() noexcept { return kCapacity; }
    [[nodiscard]] static constexpr composable::CapacityConstraint container_capacity() noexcept {
        return {0, kCapacity, composable::CapacityKind::Static};
    }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "array65535"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "Array65535SearchAlgo (prt-art REV6 §5.17 mid-density direct-addressed uint16)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "ARRAY65535"; }

    /// SONDERFALL: kein SIMD — direkter O(1)-Index, kein Vektorisierungs-Vorteil.
    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // index-geordnet
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }           // Mid-Density
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

    /// SONDERFALL [[allocation-failure-exception]]: zwei vector(kCapacity) koennen std::bad_alloc werfen.
    Array65535SearchAlgo() : data_(kCapacity), present_(kCapacity, 0u), count_(0) {}

    [[nodiscard]] bool operator==(Array65535SearchAlgo const& other) const noexcept { return count_ == other.count_; }

    void insert(key_type k, value_type v) {
        if (present_[k] == 0u) ++count_;
        data_[k]    = v;
        present_[k] = 1u;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (count_ > stats_.peak_occupancy) stats_.peak_occupancy = count_;
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        bool hit = (present_[k] != 0u);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        if (hit)
            ++stats_.total_hit_count;
        else
            ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        if (!hit) return std::nullopt;
        return data_[k];
    }

    bool erase(key_type k) {
        if (present_[k] == 0u) return false;
        present_[k] = 0u;
        --count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return count_; }
    [[nodiscard]] double    density_percent() const noexcept {
        return 100.0 * static_cast<double>(count_) / static_cast<double>(kCapacity);
    }
    void clear() noexcept {
        for (auto& p : present_) p = 0u;
        count_ = 0;
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]]:
    /// Mid-Density-Tier (Zielband 25-50 %) — Balanced.
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
    std::vector<value_type>   data_;
    std::vector<std::uint8_t> present_;
    std::size_t               count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<Array65535SearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<Array65535SearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<Array65535SearchAlgo>);
// NICHT: SimdCapableStrategy (direkter O(1)-Index, kein SIMD-Vorteil)
} // namespace comdare::cache_engine::lookup
