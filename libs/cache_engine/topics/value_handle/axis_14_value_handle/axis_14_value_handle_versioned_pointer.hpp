#pragma once
// V41.F.6.1.R7.5.d axis_14 VersionedPointerValueHandle (MVCC)

#include "axis_14_value_handle_strategy_base.hpp"
#include "axis_14_value_handle_subaxes_vh1_to_vh3.hpp"
#include "concepts/axis_14_value_handle_cache_engine_permutation_concept.hpp"
#include "axis_14_value_handle_flags.hpp"
#include "../concepts/topic_value_handle_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::value_handle::axis_14_value_handle {

/// VersionedPointerValueHandle — Pointer mit Version-Tag (MVCC).
/// Standard fuer Multi-Version-Concurrency-Control: Reader bekommt
/// Snapshot-Version, Writer erstellt neue Version. Tombstone-Erkennung via
/// Version-Bit. Verwendet in Masstree + SMART.
class VersionedPointerValueHandle : public ValueHandleStrategyBase<VersionedPointerValueHandle> {
public:
    using topic_tag = ::comdare::cache_engine::value_handle::concepts::ValueHandleTopicTag;
    using axis_tag  = subaxes::versioning_tag;
    using family_id = std::integral_constant<int, 4>;

    static constexpr bool enabled = flags::versioned_pointer_enabled;

    [[nodiscard]] static constexpr bool             is_inline()    noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "value_handle_versioned_pointer"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "VersionedPointerValueHandle (MVCC version-tagged pointer, Masstree/SMART)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "VERSIONED_POINTER"; }
};

}  // namespace

namespace comdare::cache_engine::value_handle::axis_14_value_handle {
    static_assert(concepts::ValueHandleStrategy<VersionedPointerValueHandle>);
    static_assert(concepts::CacheEnginePermutationStrategy<VersionedPointerValueHandle>);
}
