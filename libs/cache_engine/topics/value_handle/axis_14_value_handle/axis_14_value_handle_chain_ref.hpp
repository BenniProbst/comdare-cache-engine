#pragma once
// V41.F.6.1 F.6 axis_14 ChainRefValueHandle (Multi-Value verkettete Referenz)
//
// F.6 Migration (Doku 19/20, Phase A): aus prt-art (comdare::prt_art::value_handle::ChainRefHandle)
// als 5. Value-Handle-STRATEGIE nach cache-engine. Bei Multi-Value-Schluesseln (mehrere Werte
// mit gleichem Key) verweist der Handle auf einen Linked-List-Head im Pool; jeder Chain-Knoten
// haelt (value_offset, next_offset). Die konkrete Pool-Offset-Laufzeit bleibt prt-art-Detail
// (optional_prt_art_impl) — diese Achsen-Strategie beschreibt die Wahl "chained external".

#include "axis_14_value_handle_strategy_base.hpp"
#include "axis_14_value_handle_subaxes_vh1_to_vh3.hpp"
#include "concepts/axis_14_value_handle_cache_engine_permutation_concept.hpp"
#include "axis_14_value_handle_flags.hpp"
#include "../concepts/topic_value_handle_concept.hpp"
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::value_handle::axis_14_value_handle {

/// ChainRefValueHandle — verkettete externe Referenz fuer Multi-Value-Schluessel.
/// Storage-Location-Strategie (wie ExternalPool extern, aber als Chain-Head-Offset).
class ChainRefValueHandle : public ValueHandleStrategyBase<ChainRefValueHandle> {
public:
    using topic_tag = ::comdare::cache_engine::value_handle::concepts::ValueHandleTopicTag;
    using axis_tag  = subaxes::storage_location_tag;
    using family_id = std::integral_constant<int, 5>;

    static constexpr bool enabled = flags::chain_ref_enabled;

    [[nodiscard]] static constexpr bool             is_inline()    noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "value_handle_chain_ref"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "ChainRefValueHandle (multi-value chained external reference, pool linked-list)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "CHAIN_REF"; }
};

}  // namespace

namespace comdare::cache_engine::value_handle::axis_14_value_handle {
    static_assert(concepts::ValueHandleStrategy<ChainRefValueHandle>);
    static_assert(concepts::CacheEnginePermutationStrategy<ChainRefValueHandle>);
}
