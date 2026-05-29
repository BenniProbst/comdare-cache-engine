#pragma once
// V41.F.6.1.R7.1.b axis_05 CacheLineAlignedMemoryLayout Default-Wrapper (Goldstandard-Update)

#include "axis_05_memory_layout_strategy_base.hpp"
#include "axis_05_memory_layout_subaxes_hm1_to_hm4.hpp"
#include "concepts/axis_05_memory_layout_cache_engine_permutation_concept.hpp"
#include "axis_05_memory_layout_flags.hpp"
#include "../concepts/topic_memory_layout_concept.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {

/// CacheLineAlignedMemoryLayout — Default: 64-byte aligned AoS layout.
/// Standard fuer ART/HOT/Masstree/START. Vermeidet False-Sharing,
/// optimal fuer concurrent Schreiber.
class CacheLineAlignedMemoryLayout : public MemoryLayoutStrategyBase<CacheLineAlignedMemoryLayout> {
public:
    using topic_tag  = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    using axis_tag   = subaxes::alignment_strategy_tag;
    using family_id  = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::cache_line_aligned_enabled;

    [[nodiscard]] static constexpr std::size_t      cache_line_size() noexcept { return 64; }
    [[nodiscard]] static constexpr std::string_view name()            noexcept { return "memory_layout_cache_line_aligned"; }
    [[nodiscard]] static constexpr std::string_view family_name()     noexcept { return "CacheLineAlignedMemoryLayout (64-byte AoS, standard cache architectures)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()     noexcept { return "CACHE_LINE_ALIGNED"; }

    // V41.F.6.1 R5.B — verhaltens-tragende Laufzeit-API (macht die Layout-Achse F15-operativ):
    // summiert je Datensatz ein 4-Byte-Feld aus `buf` (>= n*record_size Bytes) im AoS-PATTERN
    // (Feld i bei i*record_size, Stride = record_size) → STRIDED-Zugriff, je Datensatz eine eigene
    // Cache-Line, geringe Cache-Line-Auslastung. Kontrast zu SoA (contiguous). Echter Cache-Effekt.
    [[nodiscard]] static std::uint64_t scan_field_sum(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        std::uint64_t s = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * record_size, sizeof(v));   // AoS: strided
            s += v;
        }
        return s;
    }
};

}  // namespace

namespace comdare::cache_engine::memory_layout::axis_05_memory_layout {
    static_assert(concepts::MemoryLayoutStrategy<CacheLineAlignedMemoryLayout>);
    static_assert(concepts::CacheEnginePermutationStrategy<CacheLineAlignedMemoryLayout>);
}
