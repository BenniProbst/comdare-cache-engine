#pragma once
// V41.F.6.1.R7.5.d axis_14 ExternalPoolValueHandle (Wormhole)

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

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::value_handle_axis {

/// ExternalPoolValueHandle — Value extern in Pool, Node speichert nur Pool-Offset.
/// Standard fuer Wormhole (Wu EuroSys 2019): kompakte Nodes + Variable-Size
/// Values via Pool. Pointer-Indirektion kostet 1 Cache-Miss pro Lookup.
class ExternalPoolValueHandle : public ValueHandleStrategyBase<ExternalPoolValueHandle> {
public:
    using topic_tag = ::comdare::cache_engine::value_handle::concepts::ValueHandleTopicTag;
    using axis_tag  = subaxes::storage_location_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::external_pool_enabled;

    [[nodiscard]] static constexpr bool             is_inline() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "value_handle_external_pool"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::value_handle_axis::ExternalPoolValueHandle",
                                  "axes/value_handle_axis/axis_14_value_handle_external_pool.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "ExternalPoolValueHandle (Wormhole pool-offset, variable-size values)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "EXTERNAL_POOL"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1";

    // T11 value_handle F15-operativ (Pfad A, abi_adapter-Segment): strategie-charakteristische
    // Value-Zugriffs-SIMULATION. SIMULATION (kein echter Pool): External-Pool speichert im Slot nur
    // einen Pool-OFFSET; der eigentliche Value liegt extern -> 1 zusaetzliche, daten-abhaengige
    // (pointer-chasing) Dereferenzierung pro Record gegenueber Inline (1 Cache-Miss/Lookup laut
    // Wormhole). Hier: Slot-Read liefert Offset, dieser indiziert (modulo) erneut in den Puffer
    // (Pool-Deref). Reale strategie-abhaengige Mehrlaufzeit durch die 2. abhaengige Last; kein
    // konstanter Wert.
    [[nodiscard]] static std::uint64_t value_access_scan(unsigned char const* buf, std::size_t n,
                                                         std::size_t record_size) noexcept {
        std::uint64_t const span = static_cast<std::uint64_t>(n) * record_size;
        std::uint64_t       s    = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t handle;
            std::memcpy(&handle, buf + i * record_size, sizeof(handle)); // Slot haelt nur Pool-Offset
            // Pool-Deref: Offset indiziert extern in den Pool (modulo, bounds-safe, 4-Byte-aligned)
            std::uint64_t off =
                (static_cast<std::uint64_t>(handle) % (span >= 3u ? span - 3u : 1u)) & ~std::uint64_t{3};
            std::uint32_t v;
            std::memcpy(&v, buf + off, sizeof(v)); // external pool: 2. abhaengiger Read (pointer chase)
            s += v;
        }
        return s;
    }
};

} // namespace comdare::cache_engine::value_handle_axis

namespace comdare::cache_engine::value_handle_axis {
static_assert(concepts::ValueHandleStrategy<ExternalPoolValueHandle>);
static_assert(concepts::CacheEnginePermutationStrategy<ExternalPoolValueHandle>);
} // namespace comdare::cache_engine::value_handle_axis
