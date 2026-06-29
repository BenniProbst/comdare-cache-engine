#pragma once
// V41.F.6.1.R7.5.d axis_14 InlineValueHandle (Goldstandard-Update)

#include "axis_14_value_handle_strategy_base.hpp"
#include "axis_14_value_handle_subaxes_vh1_to_vh3.hpp"
#include "concepts/axis_14_value_handle_cache_engine_permutation_concept.hpp"
#include <axes/value_handle_axis/axis_14_value_handle_flags.hpp>
#include <topics/value_handle/concepts/topic_value_handle_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::value_handle_axis {

/// InlineValueHandle — Default: value embedded direkt in Node-Slot.
/// Standard fuer kleine Values (uint64_t). Vermeidet Pointer-Indirektion +
/// Cache-Miss bei Lookup. Trade-off: groessere Nodes.
class InlineValueHandle : public ValueHandleStrategyBase<InlineValueHandle> {
public:
    using topic_tag = ::comdare::cache_engine::value_handle::concepts::ValueHandleTopicTag;
    using axis_tag  = subaxes::storage_location_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::inline_enabled;

    [[nodiscard]] static constexpr bool             is_inline() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "value_handle_inline"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "InlineValueHandle (value embedded in slot, no indirection)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "INLINE"; }

    // T11 value_handle F15-operativ (Pfad A, abi_adapter-Segment): strategie-charakteristische
    // Value-Zugriffs-SIMULATION auf einem flachen Record-Puffer. SIMULATION (kein echter Node-Slot):
    // Inline = Value direkt im Slot eingebettet -> EIN direkter 4-Byte-Read pro Record, KEINE
    // Pointer-Indirektion. Baseline-Variante der Achse (guenstigster Zugriff). Reale, von der
    // Strategie bestimmte Laufzeit (1 sequentieller Read/Record); KEIN konstanter Wert.
    [[nodiscard]] static std::uint64_t value_access_scan(unsigned char const* buf, std::size_t n,
                                                         std::size_t record_size) noexcept {
        std::uint64_t s = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * record_size, sizeof(v)); // inline: direkter Slot-Read, keine Indirektion
            s += v;
        }
        return s;
    }
};

} // namespace comdare::cache_engine::value_handle_axis

namespace comdare::cache_engine::value_handle_axis {
static_assert(concepts::ValueHandleStrategy<InlineValueHandle>);
static_assert(concepts::CacheEnginePermutationStrategy<InlineValueHandle>);
} // namespace comdare::cache_engine::value_handle_axis
