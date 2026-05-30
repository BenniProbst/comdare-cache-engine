#pragma once
// V41.F.6.1.R7.1.b axis_05 SoAMemoryLayout Wrapper

#include "axis_05_memory_layout_strategy_base.hpp"
#include "axis_05_memory_layout_subaxes_hm1_to_hm4.hpp"
#include "concepts/axis_05_memory_layout_cache_engine_permutation_concept.hpp"
#include <axes/layout/axis_05_memory_layout_flags.hpp>
#include <topics/memory_layout/concepts/topic_memory_layout_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::layout {

/// SoAMemoryLayout — Struct-of-Arrays. Vorteil: SIMD-Vectorization, gute Cache-Density
/// bei Spalt-orientierten Zugriffen (z.B. nur Key-Scan ohne Value). Typischer
/// Layout fuer columnar OLAP-Indizes + LOUDS-Succinct.
class SoAMemoryLayout : public MemoryLayoutStrategyBase<SoAMemoryLayout> {
public:
    using topic_tag  = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    using axis_tag   = subaxes::data_organization_tag;
    using family_id  = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::soa_enabled;

    [[nodiscard]] static constexpr std::size_t      cache_line_size() noexcept { return 64; }
    [[nodiscard]] static constexpr std::string_view name()            noexcept { return "memory_layout_soa"; }
    [[nodiscard]] static constexpr std::string_view family_name()     noexcept { return "SoAMemoryLayout (Struct-of-Arrays, SIMD-friendly, column-scan optimal)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()     noexcept { return "SOA"; }

    // V41.F.6.1 R5.B — verhaltens-tragende Laufzeit-API (macht die Layout-Achse F15-operativ):
    // summiert je Datensatz ein 4-Byte-Feld aus `buf` im SoA-PATTERN — das Feld liegt CONTIGUOUS
    // (Feld i bei i*4), volle Cache-Line-Auslastung, ~16× weniger Cache-Lines als AoS-strided.
    // Genau der kanonische Vorteil columnarer Layouts bei Einzelfeld-Scans (echter Cache-Effekt).
    [[nodiscard]] static std::uint64_t scan_field_sum(unsigned char const* buf, std::size_t n,
                                                      std::size_t /*record_size*/) noexcept {
        std::uint64_t s = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * sizeof(std::uint32_t), sizeof(v));   // SoA: contiguous
            s += v;
        }
        return s;
    }
};

}  // namespace

namespace comdare::cache_engine::layout {
    static_assert(concepts::MemoryLayoutStrategy<SoAMemoryLayout>);
    static_assert(concepts::CacheEnginePermutationStrategy<SoAMemoryLayout>);
}
