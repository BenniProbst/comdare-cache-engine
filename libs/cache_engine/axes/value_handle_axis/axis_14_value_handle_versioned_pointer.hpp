#pragma once
// V41.F.6.1.R7.5.d axis_14 VersionedPointerValueHandle (MVCC)

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

    [[nodiscard]] static constexpr bool             is_inline() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "value_handle_versioned_pointer"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::value_handle_axis::VersionedPointerValueHandle",
                                  "axes/value_handle_axis/axis_14_value_handle_versioned_pointer.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "VersionedPointerValueHandle (MVCC version-tagged pointer, Masstree/SMART)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "VERSIONED_POINTER"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    // T11 value_handle F15-operativ (Pfad A, abi_adapter-Segment): strategie-charakteristische
    // Value-Zugriffs-SIMULATION. SIMULATION (kein echter MVCC-Tag): der Slot haelt einen
    // version-getaggten Pointer (oberste Bits = Version/MVCC-Tag, untere Bits = Offset). Pro
    // Record: Tag-Maskierung (Version abspalten via Bit-Maske, Tombstone-/Version-Bit pruefen),
    // dann Deref auf den maskierten Offset. Mehraufwand ggue. Inline = die Tag-Strip-Arithmetik vor
    // jedem Deref (Masstree/SMART-charakteristisch). Reale strategie-abhaengige Laufzeit; kein
    // konstanter Wert.
    [[nodiscard]] static std::uint64_t value_access_scan(unsigned char const* buf, std::size_t n,
                                                         std::size_t record_size) noexcept {
        constexpr std::uint32_t kVersionMask = 0xFF000000u; // obere 8 Bit = MVCC-Version/Tag
        constexpr std::uint32_t kOffsetMask  = 0x00FFFFFFu; // untere 24 Bit = Pointer-Offset
        std::uint64_t const     span         = static_cast<std::uint64_t>(n) * record_size;
        std::uint64_t           s            = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t tagged;
            std::memcpy(&tagged, buf + i * record_size, sizeof(tagged)); // version-getaggter Pointer
            std::uint32_t const version = (tagged & kVersionMask) >> 24; // MVCC-Tag abspalten
            std::uint32_t const raw_off = (tagged & kOffsetMask);        // reiner Offset
            std::uint64_t       off =
                (static_cast<std::uint64_t>(raw_off) % (span >= 3u ? span - 3u : 1u)) & ~std::uint64_t{3};
            std::uint32_t v;
            std::memcpy(&v, buf + off, sizeof(v)); // versioned: Deref nach Tag-Strip
            s += v + version;                      // Version fliesst in Ergebnis (Snapshot-Sichtbarkeit)
        }
        return s;
    }
};

} // namespace comdare::cache_engine::value_handle_axis

namespace comdare::cache_engine::value_handle_axis {
static_assert(concepts::ValueHandleStrategy<VersionedPointerValueHandle>);
static_assert(concepts::CacheEnginePermutationStrategy<VersionedPointerValueHandle>);
} // namespace comdare::cache_engine::value_handle_axis
