#pragma once
// V41.F.6.1.R7.5.f axis_io MmapIo (mmap-based, Persistent Memory)

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

/// MmapIo — mmap()-based File-Backed Memory.
/// Optimal fuer Persistent Memory (Intel Optane) + Read-Heavy Workloads.
/// OS pageret automatisch. Kein Copy zwischen User+Kernel-Space.
/// Trade-off: page-fault-Latenz spike vs Reuse-friendly.
class MmapIo : public IoStrategyBase<MmapIo> {
public:
    using topic_tag = ::comdare::cache_engine::io::concepts::IoTopicTag;
    using axis_tag  = subaxes::persistence_tag;
    using family_id = std::integral_constant<int, 4>;

    static constexpr bool enabled = flags::mmap_enabled;

    [[nodiscard]] static constexpr bool             is_in_memory_only() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "io_mmap"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::io_dispatch::MmapIo", "axes/io_dispatch/axis_io_mmap.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "MmapIo (mmap file-backed, Persistent Memory, read-heavy)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "MMAP"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    // V41.F.6.1 R5.B / T14 — verhaltens-tragende Mess-Op der io_dispatch-Achse (Pfad-A, F15-operativ).
    // EHRLICHKEIT: reine IN-MEMORY-Dispatch-SIMULATION, KEIN echtes IO (kein echtes mmap/page-fault).
    // Die Op exerziert das Dispatch-Profil der Strategie und erzeugt eine reale, strategie-abhaengige
    // Laufzeit (keine konstante/erfundene Zahl).
    // MmapIo = file-backed mmap-Pfad: der Zugriff geht ueber einen volatile-Deref. Damit kann der
    // Compiler den Lesewert NICHT als RAM-cachebar wegoptimieren (modelliert den nicht-elidierbaren,
    // page-fault-getriebenen Speicherzugriff einer mmap-Region) -> realer, von n abhaengiger
    // Mehraufwand gegenueber dem direkten InMemoryOnly-Pfad.
    [[nodiscard]] static std::uint64_t io_dispatch_scan(unsigned char const* buf, std::size_t n,
                                                        std::size_t record_size) noexcept {
        std::uint64_t s = 0;
        for (std::size_t i = 0; i < n; ++i) {
            // mmap: page-fault-getriebener Zugriff -> volatile-Deref, nicht wegoptimierbar
            unsigned char const volatile* p = buf + i * record_size;
            std::uint32_t v = static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
                              (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
            s += v;
        }
        return s;
    }
};

} // namespace comdare::cache_engine::io_dispatch

namespace comdare::cache_engine::io_dispatch {
static_assert(concepts::IoStrategy<MmapIo>);
static_assert(concepts::CacheEnginePermutationStrategy<MmapIo>);
} // namespace comdare::cache_engine::io_dispatch
