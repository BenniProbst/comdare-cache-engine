#pragma once
// D9.2 / L-76a (2026-06-02) — SetAbiAdapter: Runtime-ABI-Adapter der SET-Gattung (Vogel), analog
// SearchAlgorithmAbiAdapter / AdapterAbiAdapter. Bridge SetAnatomy<Composition> → IAnatomyBase + ISetTier.
// Eine Set-Permutations-.dll exportiert genau EINEN solchen via comdare_create_anatomy() (gibt IAnatomyBase*;
// der gattungs-agnostische Loader, der Set-Dock fragt dynamic_cast<ISetTier*>). static_assert genus()==Set (Doku 14 §32).

#include "anatomy_base.hpp"   // IAnatomyBase + AnatomyConcept
#include "set_anatomy.hpp"    // SetAnatomy / SetObserverSnapshot
#include "set_tier.hpp"       // ISetTier + SetObserverSnapshotV1
#include "../execution_engine/execution_engine_base.hpp"

#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::anatomy {

template <AnatomyConcept A>
class SetAbiAdapter final : public IAnatomyBase, public ISetTier {
    static_assert(A::genus() == AnatomyGenus::Set,
                  "SetAbiAdapter erwartet eine Set-Gattung-Anatomie (AnatomyGenus::Set). "
                  "Cross-Genus-Adapter sind type-system-mathematisch unmoeglich — Doku 14 §32.");

public:
    // ── IExecutionEngine (engine_kind() final = Anatomy in IAnatomyBase) ──
    [[nodiscard]] std::string_view engine_name() const noexcept override { return A::composition_name(); }
    [[nodiscard]] ::comdare::cache_engine::execution_engine::EngineLifecycleState
    lifecycle_state() const noexcept override { return state_; }
    void warm_up()  override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Warming; }
    void run()      override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Running; }
    void reset()    override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle; }
    void shutdown() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Shutdown; }

    // ── IAnatomyBase ──
    [[nodiscard]] std::string_view composition_name() const noexcept override { return A::composition_name(); }
    [[nodiscard]] std::string_view paper_id()         const noexcept override { return A::paper_id(); }
    [[nodiscard]] AnatomyGenus     genus()            const noexcept override { return A::genus(); }
    [[nodiscard]] std::size_t      organ_count()      const noexcept override { return A::organ_count(); }

    // ── ISetTier (K-only-Mengen-Antrieb + Observer über die ABI-Grenze) ──
    [[nodiscard]] bool tier_set_insert(std::uint64_t key) noexcept override {
        try { return anatomy_.insert(key); } catch (...) { return false; }
    }
    [[nodiscard]] bool tier_set_contains(std::uint64_t key) const noexcept override {
        try { return anatomy_.contains(key); } catch (...) { return false; }
    }
    [[nodiscard]] bool tier_set_erase(std::uint64_t key) noexcept override {
        try { return anatomy_.erase(key); } catch (...) { return false; }
    }
    [[nodiscard]] std::uint64_t tier_set_size() const noexcept override {
        return static_cast<std::uint64_t>(anatomy_.size());
    }
    void tier_set_clear() noexcept override { try { anatomy_.clear(); } catch (...) {} }

    void tier_observe_set(SetObserverSnapshotV1* out) const noexcept override {
        if (out == nullptr) return;
        SetObserverSnapshot const s = anatomy_.observe_all();
        SetObserverSnapshotV1 v{};
        v.insert_count          = s.insert_count;
        v.contains_count        = s.contains_count;
        v.contains_hit_count    = s.contains_hit_count;
        v.contains_miss_count   = s.contains_miss_count;
        v.erase_count           = s.erase_count;
        v.current_size          = s.current_size;
        v.peak_size             = s.peak_size;
        v.observable_axis_count = 1;   // R5.B ehrlich: real getrieben = search_algo-Kern-Organ
        v.organ_count           = A::organ_count();
        *out = v;
    }

private:
    A anatomy_{};
    ::comdare::cache_engine::execution_engine::EngineLifecycleState state_{
        ::comdare::cache_engine::execution_engine::EngineLifecycleState::Uninitialized};
};

}  // namespace comdare::cache_engine::anatomy
