#pragma once
// V41.F.6.1.R7.1.b axis_05 CacheLineAlignedMemoryLayout Default-Wrapper (Goldstandard-Update)

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

/// CacheLineAlignedMemoryLayout — Default: 64-byte aligned AoS layout.
/// Standard fuer ART/HOT/Masstree/START. Vermeidet False-Sharing,
/// optimal fuer concurrent Schreiber.
class CacheLineAlignedMemoryLayout : public MemoryLayoutStrategyBase<CacheLineAlignedMemoryLayout> {
public:
    using topic_tag = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    using axis_tag  = subaxes::alignment_strategy_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::cache_line_aligned_enabled;

    [[nodiscard]] static constexpr std::size_t      cache_line_size() noexcept { return 64; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "memory_layout_cache_line_aligned"; }

    // REALE Repraesentation (P-MD1-ERDUNG #167): jeder Record ist im Store auf eine VOLLE 64-B-Cache-Line
    // gepaddet ([key|value|48 B pad], Stride 64). Der Key-only-Scan beruehrt damit PRO Record eine eigene
    // Linie, von der nur 8 Key-Bytes nuetzlich sind → die NIEDRIGSTE CLU der 5 Layouts (bewusstes
    // False-Sharing-Vermeidungs-Padding kostet Cache-Line-Effizienz). CLU aus dem ECHTEN 64-B-Store-Stride.
    [[nodiscard]] static constexpr RepresentationKind representation_kind() noexcept {
        return RepresentationKind::aos_interleaved_padded;
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "CacheLineAlignedMemoryLayout (64-byte AoS, standard cache architectures)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "CACHE_LINE_ALIGNED"; }

    // V41.F.6.1 R5.B — verhaltens-tragende Laufzeit-API (macht die Layout-Achse F15-operativ).
    // LAYOUT-FIX (X-§4, 2026-06-04): cache_line_aligned modelliert auf 64-Byte-Grenzen GEPADDETE Records →
    // effektiver Stride = round_up(record_size, 64). Bei record_size < 64 (z.B. dem Mess-Build record_size=48)
    // entsteht Padding (16 B) → größerer Stride als aos_strict (kompakt-gepackt, Stride = record_size) → MEHR
    // Cache-Lines/Record → messbar höhere, vom Layout VERSCHIEDENE Latenz. (Frühere Fassung war byte-identisch
    // zu aos_strict — der cache_line_size()-Wert ging nicht in den Scan ein; das war ein Duplikat-Bug.)
    // Bei record_size==64 (oder jedem 64-Vielfachen) fällt aligned_stride==record_size → dann (korrekt) wieder
    // identisch zu aos_strict; der Mess-Build wählt daher kRecordSize=48 (abi_adapter.hpp), damit die
    // Layout-Achse aos_strict vs cache_line_aligned real differenziert. Echter Cache-Effekt, kein synthetischer Wert.
    [[nodiscard]] static std::uint64_t scan_field_sum(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        constexpr std::size_t kCacheLine     = 64;
        std::size_t const     aligned_stride = (record_size + kCacheLine - 1u) & ~(kCacheLine - 1u); // round_up auf 64
        std::uint64_t         s              = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * aligned_stride, sizeof(v)); // CLA: cache-line-gepaddeter Stride
            s += v;
        }
        return s;
    }
};

} // namespace comdare::cache_engine::layout

namespace comdare::cache_engine::layout {
static_assert(concepts::MemoryLayoutStrategy<CacheLineAlignedMemoryLayout>);
static_assert(concepts::CacheEnginePermutationStrategy<CacheLineAlignedMemoryLayout>);
} // namespace comdare::cache_engine::layout
