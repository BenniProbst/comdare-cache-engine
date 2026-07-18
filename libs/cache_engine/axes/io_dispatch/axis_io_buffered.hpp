#pragma once
// V41.F.6.1.R7.5.f axis_io BufferedIo (OS Page-Cache standard)

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

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::io_dispatch {

/// BufferedIo — Standard read()/write() via OS-Page-Cache.
/// Default fuer general-purpose DBMS. OS managed read-ahead + write-back.
/// Trade-off: bequem aber double-buffering (App-Cache + OS-Cache).
class BufferedIo : public IoStrategyBase<BufferedIo> {
public:
    using topic_tag = ::comdare::cache_engine::io::concepts::IoTopicTag;
    using axis_tag  = subaxes::caching_strategy_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::buffered_enabled;

    [[nodiscard]] static constexpr bool             is_in_memory_only() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "io_buffered"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::io_dispatch::BufferedIo",
                                  "axes/io_dispatch/axis_io_buffered.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "BufferedIo (OS page-cache, read-ahead + write-back, default)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "BUFFERED"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1";

    // V41.F.6.1 R5.B / T14 — verhaltens-tragende Mess-Op der io_dispatch-Achse (Pfad-A, F15-operativ).
    // EHRLICHKEIT: reine IN-MEMORY-Dispatch-SIMULATION, KEIN echtes IO. Die Op exerziert das
    // Dispatch-Profil der Strategie und erzeugt eine reale, strategie-abhaengige Laufzeit
    // (keine konstante/erfundene Zahl).
    // BufferedIo = OS-Page-Cache-Pfad: pro Record wird die 4-KiB-Page-Grenze geprueft (read-ahead/
    // write-back-Buchhaltung des Page-Cache). Records, die eine Page-Grenze kreuzen, erzeugen einen
    // zusaetzlichen (simulierten) Page-Touch -> realer Mehraufwand gegenueber InMemoryOnly, der von
    // record_size und Page-Alignment abhaengt.
    [[nodiscard]] static std::uint64_t io_dispatch_scan(unsigned char const* buf, std::size_t n,
                                                        std::size_t record_size) noexcept {
        constexpr std::size_t kPageSize = 4096;
        std::uint64_t         s         = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::size_t const offset = i * record_size;
            std::uint32_t     v;
            std::memcpy(&v, buf + offset, sizeof(v)); // buffered: read via OS-Page-Cache (simuliert)
            // Page-Cache-Buchhaltung: Page-Grenz-Check (read-ahead/write-back-Pfad)
            std::size_t const page_start = offset / kPageSize;
            std::size_t const page_end   = (offset + sizeof(v) - 1) / kPageSize;
            s += v + (page_end - page_start); // page-crossing -> zusaetzlicher Page-Touch
        }
        return s;
    }
};

} // namespace comdare::cache_engine::io_dispatch

namespace comdare::cache_engine::io_dispatch {
static_assert(concepts::IoStrategy<BufferedIo>);
static_assert(concepts::CacheEnginePermutationStrategy<BufferedIo>);
} // namespace comdare::cache_engine::io_dispatch
