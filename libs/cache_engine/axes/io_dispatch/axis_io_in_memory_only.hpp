#pragma once
// V41.F.6.1.R7.5.f axis_io InMemoryOnly (Goldstandard-Update)

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

/// InMemoryOnly — Default: kein IO, alles im RAM (Pure In-Memory Index).
/// Baseline fuer Mess-Reihen ohne Persistence-Overhead.
class InMemoryOnly : public IoStrategyBase<InMemoryOnly> {
public:
    using topic_tag = ::comdare::cache_engine::io::concepts::IoTopicTag;
    using axis_tag  = subaxes::persistence_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::in_memory_only_enabled;

    [[nodiscard]] static constexpr bool             is_in_memory_only() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "io_in_memory_only"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::io_dispatch::InMemoryOnly",
                                  "axes/io_dispatch/axis_io_in_memory_only.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "InMemoryOnly (no IO, RAM-only baseline)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "IN_MEMORY_ONLY"; }

    // V41.F.6.1 R5.B / T14 — verhaltens-tragende Mess-Op der io_dispatch-Achse (Pfad-A, F15-operativ).
    // EHRLICHKEIT: reine IN-MEMORY-Dispatch-SIMULATION, KEIN echtes IO (echte Disk-IO im DRAM-
    // Benchmark waere unsinnig). Die Op exerziert das Dispatch-Profil der Strategie und erzeugt
    // eine reale, strategie-abhaengige Laufzeit (keine konstante/erfundene Zahl).
    // InMemoryOnly = Baseline: direkter sequentieller Record-Zugriff ohne Dispatch-Overhead,
    // analog read() aus dem RAM-Index. Schnellster Pfad (Vergleichs-Nullpunkt der Achse).
    [[nodiscard]] static std::uint64_t io_dispatch_scan(unsigned char const* buf, std::size_t n,
                                                        std::size_t record_size) noexcept {
        std::uint64_t s = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * record_size, sizeof(v)); // direct: RAM-only, kein Dispatch-Overhead
            s += v;
        }
        return s;
    }
};

} // namespace comdare::cache_engine::io_dispatch

namespace comdare::cache_engine::io_dispatch {
static_assert(concepts::IoStrategy<InMemoryOnly>);
static_assert(concepts::CacheEnginePermutationStrategy<InMemoryOnly>);
} // namespace comdare::cache_engine::io_dispatch
