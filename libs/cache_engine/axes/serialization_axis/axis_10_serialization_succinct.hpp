#pragma once
// V41.F.6.1.R7.5.c axis_10 SuccinctSerialization (LOUDS/SuRF)

#include "axis_10_serialization_strategy_base.hpp"
#include "axis_10_serialization_subaxes_sr1_to_sr3.hpp"
#include "concepts/axis_10_serialization_cache_engine_permutation_concept.hpp"
#include <axes/serialization_axis/axis_10_serialization_flags.hpp>
#include <topics/serialization/concepts/topic_serialization_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::serialization_axis {

/// SuccinctSerialization — Bit-Packed Succinct Encoding (LOUDS/SuRF).
/// Approaches information-theoretic lower bound (n*H + o(n) bits).
/// Read-only nach Build, optimal fuer Approx-Filter (SuRF) + Read-Heavy.
class SuccinctSerialization : public SerializationStrategyBase<SuccinctSerialization> {
public:
    using topic_tag = ::comdare::cache_engine::serialization::concepts::SerializationTopicTag;
    using axis_tag  = subaxes::density_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::succinct_enabled;

    [[nodiscard]] static constexpr bool             supports_compression() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "serialization_succinct"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "SuccinctSerialization (LOUDS/SuRF bit-packed, n*H + o(n) bits)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "SUCCINCT"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    // R5.B: behaviorale Laufzeit-API (s. RawBinarySerialization). Succinct = Bit-für-Bit-Packing jedes Werts
    // mit seiner minimalen Bit-Breite in einen 64-Bit-Akkumulator (information-theoretisch dichte Kodierung) —
    // der HÖCHSTE Per-Element-CPU-Aufwand der Achse (echte Bit-Manipulation pro Bit).
    [[nodiscard]] static std::uint64_t serialize_scan(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        std::uint64_t s = 0, acc = 0;
        unsigned      filled = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * record_size, sizeof(v));
            unsigned w = 1;
            for (std::uint32_t t = v; t > 1u; t >>= 1) ++w; // minimale Bit-Breite
            for (unsigned b = 0; b < w; ++b) {              // Bit-für-Bit packen
                acc = (acc << 1) | ((v >> b) & 1u);
                if (++filled == 64u) {
                    s += acc;
                    acc    = 0;
                    filled = 0;
                }
            }
        }
        return s + acc + filled;
    }
};

} // namespace comdare::cache_engine::serialization_axis

namespace comdare::cache_engine::serialization_axis {
static_assert(concepts::SerializationStrategy<SuccinctSerialization>);
static_assert(concepts::CacheEnginePermutationStrategy<SuccinctSerialization>);
} // namespace comdare::cache_engine::serialization_axis
