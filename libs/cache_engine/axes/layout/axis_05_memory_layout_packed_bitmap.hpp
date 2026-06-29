#pragma once
// V41.F.6.1.R7.1.b axis_05 PackedBitmapMemoryLayout Wrapper

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

/// PackedBitmapMemoryLayout — Bit-packed (1 Bit pro Slot), succinct.
/// Typischer Layout fuer LOUDS / SuRF Filter-Trees. Sehr kompakt
/// (n * lg(sigma) bits), aber hoeherer Decode-Aufwand pro Lookup.
class PackedBitmapMemoryLayout : public MemoryLayoutStrategyBase<PackedBitmapMemoryLayout> {
public:
    using topic_tag = ::comdare::cache_engine::memory_layout::concepts::MemoryLayoutTopicTag;
    using axis_tag  = subaxes::packing_density_tag;
    using family_id = std::integral_constant<int, 4>;

    static constexpr bool enabled = flags::packed_bitmap_enabled;

    [[nodiscard]] static constexpr std::size_t      cache_line_size() noexcept { return 8; } // 64 bit = 1 word
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "memory_layout_packed_bitmap"; }

    // REALE Repraesentation (P-MD1-ERDUNG #167): succinct hot/cold column-split (succinct data structures,
    // LOUDS/SuRF-Geist). Der Store legt PRO Chunk drei Regionen an: eine 2-B-HOT-Key-Spalte (low16 jedes Keys),
    // eine 6-B-COLD-Residue-Spalte (high48) und die values. Der Key-only-Scan beruehrt NUR die 2-B-Hot-Spalte
    // (n*2 B KONTIGUIERLICH) → der WENIGSTE Byte-/Line-Footprint aller 5 Reps (ceil(n*2/64) statt ceil(n*8/64)).
    // load_key rekonstruiert VERLUSTFREI low16|(high48<<16) (voller uint64-Round-Trip) — Decode beim Load. CLU
    // damit byte-distinkt + plausibel aus dem ECHTEN Hot-Spalten-Footprint (kein 3/8-Deskriptor-Modell mehr).
    [[nodiscard]] static constexpr RepresentationKind representation_kind() noexcept {
        return RepresentationKind::succinct_hot_cold_split;
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "PackedBitmapMemoryLayout (bit-packed succinct, LOUDS/SuRF, n*lg(sigma) bits)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "PACKED_BITMAP"; }

    // V41.F.6.1 R5.B — verhaltens-tragende Laufzeit-API (Layout-Achse F15-operativ): succinct/packed
    // → kontiguierliche 2-Byte-Felder (i*2), die DICHTESTE Variante (n*2 Bytes, ~32× weniger
    // Cache-Lines als AoS-strided), entsprechend der bit-packed-Charakteristik dieses Layouts.
    [[nodiscard]] static std::uint64_t scan_field_sum(unsigned char const* buf, std::size_t n,
                                                      std::size_t /*record_size*/) noexcept {
        std::uint64_t s = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint16_t v;
            std::memcpy(&v, buf + i * sizeof(std::uint16_t), sizeof(v)); // packed: contiguous 2-byte
            s += v;
        }
        return s;
    }
};

} // namespace comdare::cache_engine::layout

namespace comdare::cache_engine::layout {
static_assert(concepts::MemoryLayoutStrategy<PackedBitmapMemoryLayout>);
static_assert(concepts::CacheEnginePermutationStrategy<PackedBitmapMemoryLayout>);
} // namespace comdare::cache_engine::layout
