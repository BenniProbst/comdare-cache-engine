#pragma once
// V41.F.6.1.R7.5.f axis_io DirectIo (O_DIRECT, NVMe-optimal)

#include "axis_io_strategy_base.hpp"
#include "axis_io_subaxes_io1_to_io3.hpp"
#include "concepts/axis_io_cache_engine_permutation_concept.hpp"
#include <axes/io_dispatch/axis_io_flags.hpp>
#include <topics/io/concepts/topic_io_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::io_dispatch {

/// DirectIo — O_DIRECT bypass OS-Page-Cache.
/// Optimal fuer NVMe-SSD + DBMS-eigene Cache-Strategien (RocksDB, MySQL).
/// 512B/4KB aligned, kein double-buffering. Hoehere CPU-Cost, vorhersehbare
/// Latenz (kein Cache-Pollution durch Background-Activity).
class DirectIo : public IoStrategyBase<DirectIo> {
public:
    using topic_tag = ::comdare::cache_engine::io::concepts::IoTopicTag;
    using axis_tag  = subaxes::caching_strategy_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::direct_enabled;

    [[nodiscard]] static constexpr bool             is_in_memory_only() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "io_direct"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "DirectIo (O_DIRECT, bypass OS page-cache, NVMe-optimal)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "DIRECT"; }

    // V41.F.6.1 R5.B / T14 — verhaltens-tragende Mess-Op der io_dispatch-Achse (Pfad-A, F15-operativ).
    // EHRLICHKEIT: reine IN-MEMORY-Dispatch-SIMULATION, KEIN echtes IO. Die Op exerziert das
    // Dispatch-Profil der Strategie und erzeugt eine reale, strategie-abhaengige Laufzeit
    // (keine konstante/erfundene Zahl).
    // DirectIo = O_DIRECT-Pfad: jeder Zugriff muss auf eine 512-Byte-Sektorgrenze ausgerichtet werden
    // (Block-Device-Constraint). Pro Record wird der Record-Offset auf die Sektorgrenze
    // heruntergerundet und ab dort gelesen (Alignment-Adjust-Kosten + groesserer Lese-Footprint als
    // buffered/direct-baseline) -> realer, von record_size/512-Alignment abhaengiger Mehraufwand.
    [[nodiscard]] static std::uint64_t io_dispatch_scan(unsigned char const* buf, std::size_t n,
                                                        std::size_t record_size) noexcept {
        constexpr std::size_t kSector = 512;
        std::uint64_t         s       = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::size_t const offset = i * record_size;
            // O_DIRECT: Zugriff auf 512-Byte-Sektorgrenze ausrichten (sector-aligned read)
            std::size_t const aligned = offset & ~(kSector - 1u);
            std::uint32_t     v;
            std::memcpy(&v, buf + aligned, sizeof(v)); // direct: sector-aligned, bypass page-cache
            s += v + (offset - aligned);               // Alignment-Adjust-Kosten (within-sector offset)
        }
        return s;
    }
};

} // namespace comdare::cache_engine::io_dispatch

namespace comdare::cache_engine::io_dispatch {
static_assert(concepts::IoStrategy<DirectIo>);
static_assert(concepts::CacheEnginePermutationStrategy<DirectIo>);
} // namespace comdare::cache_engine::io_dispatch
