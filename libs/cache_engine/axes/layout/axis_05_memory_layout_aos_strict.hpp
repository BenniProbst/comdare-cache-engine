#pragma once
// V41.F.6.1.R7.1.b axis_05 AoSStrictMemoryLayout Wrapper

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

/// AoSStrictMemoryLayout — Array-of-Structs, packed (ohne Cache-Line Padding).
/// Vorteil: kompakte Daten. Nachteil: False-Sharing + schlechter Cache-Line-Hit
/// bei concurrent Schreibern. Typischer Wormhole-Layout (strict AoS).
class AoSStrictMemoryLayout : public MemoryLayoutStrategyBase<AoSStrictMemoryLayout> {
public:
    using topic_tag = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    using axis_tag  = subaxes::data_organization_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::aos_strict_enabled;

    [[nodiscard]] static constexpr std::size_t cache_line_size() noexcept { return 1; } // strict packed, kein alignment
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "memory_layout_aos_strict"; }

    // REALE Repraesentation (P-MD1-ERDUNG #167): strict-packed AoS — der Store legt [key|value] adjazent am
    // 16-B-Stride an (kein Padding). Der Key-only-Scan beruehrt jeden 16-B-Block → MEHR Lines als SoA, aber
    // weniger als das 64-B-gepaddete cache_line_aligned. CLU aus dem ECHTEN Store-Footprint (≈100 % der
    // beruehrten Linien sind durch Nutzdaten belegt). Die span ist die Nutzlast, kein Padding/Spread.
    [[nodiscard]] static constexpr RepresentationKind representation_kind() noexcept {
        return RepresentationKind::aos_interleaved_packed;
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "AoSStrictMemoryLayout (strict packed, no cache-line alignment, dense)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "AOS_STRICT"; }

    // V41.F.6.1 R5.B — verhaltens-tragende Laufzeit-API (Layout-Achse F15-operativ): AoS-strided
    // (Feld i bei i*record_size) — wie CacheLineAligned, aber strict-packed ohne Alignment.
    [[nodiscard]] static std::uint64_t scan_field_sum(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        std::uint64_t s = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * record_size, sizeof(v)); // AoS: strided
            s += v;
        }
        return s;
    }
};

} // namespace comdare::cache_engine::layout

namespace comdare::cache_engine::layout {
static_assert(concepts::MemoryLayoutStrategy<AoSStrictMemoryLayout>);
static_assert(concepts::CacheEnginePermutationStrategy<AoSStrictMemoryLayout>);
} // namespace comdare::cache_engine::layout
