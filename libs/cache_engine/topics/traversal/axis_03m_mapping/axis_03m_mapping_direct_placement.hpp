#pragma once
// V41.F.6.1 axis_03m_mapping DirectPlacement MP01 (2026-05-26)
//
// @topic traversal @achse 03m @family MP01 DirectPlacement
// @subaxis MP1 direct_access
//
// Direkter Slot-zu-Offset-Mapping (linear packing). Pro registriertem Slot
// wird der absolute Offset in einem std::vector gehalten. Inspiriert von
// prt-art-Legacy INode::placement_page_ Pattern (direkte Page-Pointer-
// Referenzen).
//
// Allocation: std::vector — [[allocation-failure-exception]].

#include "axis_03m_mapping_base.hpp"
#include "axis_03m_mapping_subaxes_mp1_to_mp2.hpp"
#include "concepts/axis_03m_mapping_concept.hpp"
#include "concepts/axis_03m_mapping_cache_engine_permutation_concept.hpp"
#include "../concepts/topic_traversal_concept.hpp"

#include <topics/traversal/axis_03m_mapping/axis_03m_mapping_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace comdare::cache_engine::traversal::axis_03m_mapping {

class DirectPlacement : public MappingBase<DirectPlacement> {
public:
    static constexpr bool enabled = flags::direct_placement_enabled;

    using slot_index_type = std::uint16_t;
    using offset_type     = std::size_t;
    using size_type       = std::size_t;
    using topic_tag       = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag        = subaxes::direct_access_tag;
    using family_id       = std::integral_constant<int, 1>;  // MP01

    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "direct_placement"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "DirectPlacement (prt-art INode::placement_page_ direct-Pointer)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "DIRECT_PLACEMENT"; }

    [[nodiscard]] static constexpr bool is_pool_relative()        noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_reverse_lookup() noexcept { return true; }  // linear-scan
    [[nodiscard]] static constexpr bool requires_pool_base()      noexcept { return false; }

    DirectPlacement() = default;

    [[nodiscard]] bool operator==(DirectPlacement const& other) const noexcept {
        return mappings_.size() == other.mappings_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: emplace_back kann std::bad_alloc werfen.
    void register_slot(slot_index_type s, offset_type o) {
        auto it = std::find_if(mappings_.begin(), mappings_.end(),
            [s](auto const& m) { return m.first == s; });
        if (it != mappings_.end()) {
            it->second = o;  // update
        } else {
            mappings_.emplace_back(s, o);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_register_count;
        if (mappings_.size() > stats_.peak_mapped) stats_.peak_mapped = mappings_.size();
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<offset_type> resolve_offset(slot_index_type s) const {
        auto it = std::find_if(mappings_.begin(), mappings_.end(),
            [s](auto const& m) { return m.first == s; });
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_resolve_count;
        if (it != mappings_.end()) ++stats_.total_resolve_hit_count;
        else                        ++stats_.total_resolve_miss_count;
        observer_.notify(stats_);
#endif
        if (it == mappings_.end()) return std::nullopt;
        return it->second;
    }

    [[nodiscard]] std::optional<slot_index_type> reverse_lookup(offset_type o) const {
        auto it = std::find_if(mappings_.begin(), mappings_.end(),
            [o](auto const& m) { return m.second == o; });
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_reverse_lookup_count;
        observer_.notify(stats_);
#endif
        if (it == mappings_.end()) return std::nullopt;
        return it->first;
    }

    [[nodiscard]] size_type mapped_count() const noexcept { return mappings_.size(); }
    void                    clear() noexcept { mappings_.clear(); }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::MappingStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot()   const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; observer_.notify(stats_); }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer()       noexcept { return observer_; }
#endif

private:
    std::vector<std::pair<slot_index_type, offset_type>> mappings_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::MappingStatistics stats_{};
    mutable observer_t                   observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::traversal::axis_03m_mapping {
    static_assert(concepts::MappingVariant<DirectPlacement>);
    static_assert(concepts::CacheEngineMappingPermutationStrategy<DirectPlacement>);
}
