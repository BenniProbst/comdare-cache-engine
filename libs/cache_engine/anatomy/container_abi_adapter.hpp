#pragma once
// D4b / L-75 (2026-06-02) — ContainerAbiAdapter: Runtime-ABI-Adapter der CONTAINER-Gattung (Adapter/Wirbelloses),
// analog SearchAlgorithmAbiAdapter für SearchAlgorithm. Bridge eine ContainerAnatomy<Composition> (Compile-Time-
// Concept) zur Runtime-ABI-Schicht (IAnatomyBase → IExecutionEngine) + dem Container-Antrieb (IContainerTier).
//
// Eine generierte Container-Permutations-.dll exportiert EXAKT EINEN solchen Adapter via comdare_create_anatomy()
// (gibt IAnatomyBase* — der Loader ist gattungs-agnostisch, der Container-Dock fragt dynamic_cast<IContainerTier*>).
// Cross-Genus-Adapter sind type-system-mathematisch unmöglich (Doku 14 §32) → static_assert genus()==Adapter.

#include "anatomy_base.hpp"        // IAnatomyBase + AnatomyConcept
#include "container_anatomy.hpp"   // ContainerAnatomy / ContainerObserverSnapshot
#include "container_tier.hpp"      // IContainerTier + ContainerObserverSnapshotV1
#include "../execution_engine/execution_engine_base.hpp"

#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::anatomy {

/// ContainerAbiAdapter<A> — generischer Runtime-ABI-Adapter für die Container-Gattung (Adapter in Tier-Metapher).
/// Vorbedingung: A erfüllt AnatomyConcept UND A::genus() == AnatomyGenus::Adapter (Compile-Zeit-Validierung).
///
/// Der Adapter konstruiert die Container-Anatomie mit kCapacity > 0 → die Q2-Flush-Policy ist über die DLL-Grenze
/// AKTIV (z.B. WatermarkFlush 75%); eine NoFlush-Komposition flusht trotzdem nie (policy-bestimmt, nicht cap).
template <AnatomyConcept A>
class ContainerAbiAdapter final : public IAnatomyBase, public IContainerTier {
    static_assert(A::genus() == AnatomyGenus::Adapter,
                  "ContainerAbiAdapter erwartet eine Container-Gattung-Anatomie (AnatomyGenus::Adapter). "
                  "Cross-Genus-Adapter sind type-system-mathematisch unmoeglich — Doku 14 §32.");

public:
    using element_type = typename A::element_type;
    /// Flush-Bezugsgröße der eingebauten Q2-Policy über die DLL-Grenze (Watermark 75% → Flush bei ~12 von 16).
    static constexpr std::size_t kCapacity = 16;

    // ── IExecutionEngine-Pflicht (Wurzel-Schicht; engine_kind() ist in IAnatomyBase final = Anatomy) ──
    [[nodiscard]] std::string_view engine_name() const noexcept override { return A::composition_name(); }

    [[nodiscard]] ::comdare::cache_engine::execution_engine::EngineLifecycleState
    lifecycle_state() const noexcept override { return state_; }

    void warm_up()  override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Warming; }
    void run()      override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Running; }
    void reset()    override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle; }
    void shutdown() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Shutdown; }

    // ── IAnatomyBase-Pflicht (Anatomie-Schicht) ──
    [[nodiscard]] std::string_view composition_name() const noexcept override { return A::composition_name(); }
    [[nodiscard]] std::string_view paper_id()         const noexcept override { return A::paper_id(); }
    [[nodiscard]] AnatomyGenus     genus()            const noexcept override { return A::genus(); }
    [[nodiscard]] std::size_t      organ_count()      const noexcept override { return A::organ_count(); }

    // ── IContainerTier-Pflicht (Container-Antrieb + Observer durch die ABI-Grenze) ──
    void tier_put(std::uint64_t value) noexcept override {
        anatomy_.put(static_cast<element_type>(value));
    }
    [[nodiscard]] bool tier_get(std::uint64_t* out_value) noexcept override {
        auto r = anatomy_.get();
        if (!r) return false;
        if (out_value != nullptr) *out_value = static_cast<std::uint64_t>(*r);
        return true;
    }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override {
        return static_cast<std::uint64_t>(anatomy_.size());
    }
    void tier_clear() noexcept override { anatomy_.clear(); }

    void tier_observe_container(ContainerObserverSnapshotV1* out) const noexcept override {
        if (out == nullptr) return;
        ContainerObserverSnapshot const s = anatomy_.observe_all();
        ContainerObserverSnapshotV1 v{};
        v.put_count                 = s.put_count;
        v.get_count                 = s.get_count;
        v.current_occupancy         = s.current_occupancy;
        v.peak_occupancy            = s.peak_occupancy;
        v.flush_decisions_evaluated = s.flush_decisions_evaluated;
        v.full_flush_count          = s.full_flush_count;
        v.flush_complete_count      = s.flush_complete_count;
        v.organ_count               = A::organ_count();
        *out = v;
    }

private:
    A anatomy_{kCapacity};   // capacity > 0 → Q2-Flush-Policy über die DLL-Grenze aktiv
    ::comdare::cache_engine::execution_engine::EngineLifecycleState state_{
        ::comdare::cache_engine::execution_engine::EngineLifecycleState::Uninitialized};
};

}  // namespace comdare::cache_engine::anatomy
