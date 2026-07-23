#pragma once
// V41.F.6.1.R7.3 axis_08 HazardPointerConcurrency (Hazard Pointers, Reclamation)
// Klasse C: P30 NO LICENSE → eigene Re-Impl (vgl. C++26 P0233R4). is_original=false.

#include "axis_08_concurrency_strategy_base.hpp"
#include "axis_08_concurrency_subaxes_cc1_to_cc2.hpp"
#include "concepts/axis_08_concurrency_cache_engine_permutation_concept.hpp"
#include <axes/concurrency_axis/axis_08_concurrency_flags.hpp>
#include <topics/concurrency/concepts/topic_concurrency_concept.hpp>
#include <atomic>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::concurrency_axis {

/// HazardPointerConcurrency — Safe Memory Reclamation via Hazard Pointers:
/// pro Thread markierte "in-use"-Zeiger verhindern verfruehte Freigabe.
/// (Michael TPDS 2004.)
class HazardPointerConcurrency : public ConcurrencyStrategyBase<HazardPointerConcurrency> {
public:
    using topic_tag = ::comdare::cache_engine::concurrency::concepts::ConcurrencyTopicTag;
    using axis_tag  = subaxes::reclamation_scheme_tag;
    using family_id = std::integral_constant<int, 8>;

    static constexpr bool enabled = flags::hazard_ptr_enabled;

    [[nodiscard]] static constexpr concepts::ConcurrencyPattern concurrency_pattern() noexcept {
        return concepts::ConcurrencyPattern::HazardPtr;
    }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "concurrency_hazard_ptr"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::concurrency_axis::HazardPointerConcurrency",
                                  "axes/concurrency_axis/axis_08_concurrency_hazard_pointer.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "HazardPointerConcurrency (per-thread hazard pointers, safe reclamation)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "HAZARD_PTR"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    // V41 F15 Pfad-A — treibbare Concurrency-Op (acquire/release-Paar). Hazard Pointers (Michael
    // TPDS 2004): acquire() PUBLIZIERT einen "in-use"-Zeiger in den per-Thread-Hazard-Slot
    // (atomic store, SEQ_CST — die Reader-Seite braucht volle Sichtbarkeit, damit ein paralleler
    // Reclaimer den Slot scannen und die Freigabe verzoegern kann), release() LOESCHT den Slot
    // (store nullptr, RELEASE-Order). Reale, strategie-abhaengige Laufzeit: der SEQ_CST-Store auf
    // der acquire-Seite ist der charakteristische, teurere Reclamation-Overhead (StoreLoad-Barriere)
    // — distinkt von RCU (relaxed) und WaitFree (acq/rel). Slot thread_local-static (via slot_()).
    static void acquire() noexcept {
        // gepublishter Pseudo-Hazard-Zeiger (Slot-Selbst-Adresse als nicht-null in-use-Marker).
        slot_().store(reinterpret_cast<std::uintptr_t>(&slot_()), std::memory_order_seq_cst);
    }
    static void release() noexcept { slot_().store(0u, std::memory_order_release); }

private:
    [[nodiscard]] static std::atomic<std::uintptr_t>& slot_() noexcept {
        static thread_local std::atomic<std::uintptr_t> hp{0u};
        return hp;
    }
};

} // namespace comdare::cache_engine::concurrency_axis

namespace comdare::cache_engine::concurrency_axis {
static_assert(concepts::ConcurrencyStrategy<HazardPointerConcurrency>);
static_assert(concepts::CacheEnginePermutationStrategy<HazardPointerConcurrency>);
} // namespace comdare::cache_engine::concurrency_axis
