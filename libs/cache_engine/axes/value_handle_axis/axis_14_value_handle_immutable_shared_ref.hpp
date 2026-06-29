#pragma once
// V41.F.6.1.R7.5.d axis_14 ImmutableSharedRefValueHandle (RCU-style sharing)

#include "axis_14_value_handle_strategy_base.hpp"
#include "axis_14_value_handle_subaxes_vh1_to_vh3.hpp"
#include "concepts/axis_14_value_handle_cache_engine_permutation_concept.hpp"
#include <axes/value_handle_axis/axis_14_value_handle_flags.hpp>
#include <topics/value_handle/concepts/topic_value_handle_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::value_handle_axis {

/// ImmutableSharedRefValueHandle — Immutable Value shared via reference-counted
/// Pointer. Optimal fuer RCU-style Concurrency (Reader sieht stabile Snapshots,
/// Writer alloziert neuen Snapshot). Default fuer Persistent ART/Tries.
class ImmutableSharedRefValueHandle : public ValueHandleStrategyBase<ImmutableSharedRefValueHandle> {
public:
    using topic_tag = ::comdare::cache_engine::value_handle::concepts::ValueHandleTopicTag;
    using axis_tag  = subaxes::ownership_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::immutable_shared_ref_enabled;

    [[nodiscard]] static constexpr bool             is_inline() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "value_handle_immutable_shared_ref"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "ImmutableSharedRefValueHandle (RCU shared_ptr, snapshot-stable readers)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "IMMUTABLE_SHARED_REF"; }

    // T11 value_handle F15-operativ (Pfad A, abi_adapter-Segment): strategie-charakteristische
    // Value-Zugriffs-SIMULATION. SIMULATION (kein echter shared_ptr/atomic): RCU-Style geteilter
    // immutabler Value via referenz-gezaehltem Pointer. Pro Read-Zugriff: (a) externer Deref auf den
    // geteilten Snapshot PLUS (b) RefCount-Acquire/Release-Simulation (read-copy-update:
    // Reader inkrementiert beim Betreten, dekrementiert beim Verlassen den Zaehler). Mehraufwand
    // ggue. ExternalPool = die zwei RefCount-Updates pro Zugriff (kein echtes std::atomic, aber als
    // reale Daten-abhaengige Read-Modify-Write-Arithmetik gehalten, damit der Optimizer sie nicht
    // entfernt). Reale strategie-abhaengige Laufzeit; kein konstanter Wert.
    [[nodiscard]] static std::uint64_t value_access_scan(unsigned char const* buf, std::size_t n,
                                                         std::size_t record_size) noexcept {
        std::uint64_t const guard =
            (static_cast<std::uint64_t>(n) * record_size >= 3u ? static_cast<std::uint64_t>(n) * record_size - 3u : 1u);
        std::uint64_t s        = 0;
        std::uint64_t refcount = 0; // RCU-RefCount-Sim (acquire/release pro Reader-Zugriff)
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t handle;
            std::memcpy(&handle, buf + i * record_size, sizeof(handle)); // Slot: shared-ref-Offset
            refcount += 1;                                               // (b) RefCount-Acquire (Reader betritt)
            std::uint64_t off = (static_cast<std::uint64_t>(handle) % guard) & ~std::uint64_t{3};
            std::uint32_t v;
            std::memcpy(&v, buf + off, sizeof(v)); // (a) Deref auf geteilten Snapshot
            s += v + (refcount & 1u);              // RefCount fliesst ins Ergebnis (no-elide)
            refcount -= 1;                         // (b) RefCount-Release (Reader verlaesst)
        }
        return s + refcount;
    }
};

} // namespace comdare::cache_engine::value_handle_axis

namespace comdare::cache_engine::value_handle_axis {
static_assert(concepts::ValueHandleStrategy<ImmutableSharedRefValueHandle>);
static_assert(concepts::CacheEnginePermutationStrategy<ImmutableSharedRefValueHandle>);
} // namespace comdare::cache_engine::value_handle_axis
