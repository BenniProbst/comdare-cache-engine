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
#include <axes/value_handle_axis/axis_14_value_handle_flags.hpp>
#include <topics/value_handle/concepts/topic_value_handle_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::value_handle_axis {

/// ChainRefValueHandle — verkettete externe Referenz fuer Multi-Value-Schluessel.
/// Storage-Location-Strategie (wie ExternalPool extern, aber als Chain-Head-Offset).
class ChainRefValueHandle : public ValueHandleStrategyBase<ChainRefValueHandle> {
public:
    using topic_tag = ::comdare::cache_engine::value_handle::concepts::ValueHandleTopicTag;
    using axis_tag  = subaxes::storage_location_tag;
    using family_id = std::integral_constant<int, 5>;

    static constexpr bool enabled = flags::chain_ref_enabled;

    [[nodiscard]] static constexpr bool             is_inline() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "value_handle_chain_ref"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::value_handle_axis::ChainRefValueHandle",
                                  "axes/value_handle_axis/axis_14_value_handle_chain_ref.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "ChainRefValueHandle (multi-value chained external reference, pool linked-list)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "CHAIN_REF"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1";

    // T11 value_handle F15-operativ (Pfad A, abi_adapter-Segment): strategie-charakteristische
    // Value-Zugriffs-SIMULATION. SIMULATION (kein echter Pool-Linked-List): Multi-Value-Schluessel
    // -> der Slot haelt einen CHAIN-HEAD-Offset, der Chain-Knoten haelt (value_offset, next_offset).
    // Zugriff = 2x indirekt: (1) Head-Deref liefert den Chain-Knoten, (2) value_offset-Deref liefert
    // den Value. Teuerste Variante der Achse (doppeltes pointer-chasing) — charakteristisch fuer
    // verkettete externe Referenzen. Reale strategie-abhaengige Laufzeit durch die 2 abhaengigen
    // Lasten; kein konstanter Wert.
    [[nodiscard]] static std::uint64_t value_access_scan(unsigned char const* buf, std::size_t n,
                                                         std::size_t record_size) noexcept {
        std::uint64_t const span  = static_cast<std::uint64_t>(n) * record_size;
        std::uint64_t const guard = (span >= 3u ? span - 3u : 1u);
        std::uint64_t       s     = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t head;
            std::memcpy(&head, buf + i * record_size, sizeof(head)); // Slot: Chain-Head-Offset
            std::uint64_t head_off = (static_cast<std::uint64_t>(head) % guard) & ~std::uint64_t{3};
            std::uint32_t node;
            std::memcpy(&node, buf + head_off, sizeof(node)); // (1) Head-Deref -> Chain-Knoten
            std::uint64_t val_off = (static_cast<std::uint64_t>(node) % guard) & ~std::uint64_t{3};
            std::uint32_t v;
            std::memcpy(&v, buf + val_off, sizeof(v)); // (2) value_offset-Deref -> Value
            s += v;
        }
        return s;
    }
};

} // namespace comdare::cache_engine::value_handle_axis

namespace comdare::cache_engine::value_handle_axis {
static_assert(concepts::ValueHandleStrategy<ChainRefValueHandle>);
static_assert(concepts::CacheEnginePermutationStrategy<ChainRefValueHandle>);
} // namespace comdare::cache_engine::value_handle_axis
