#pragma once
// D11 / L-76c (2026-06-02) — ViewAbiAdapter: Runtime-ABI-Adapter der VIEW-Gattung (Pflanze, non-owning), analog
// Sequence/Set/Container. Bridge ViewAnatomy<Composition> → IAnatomyBase + IViewTier. static_assert genus()==View.
// non-owning: tier_bind(externer Puffer) + tier_read — KEIN tier_insert/erase/clear (API-Asymmetrie absichtlich).

#include "anatomy_base.hpp"
#include "view_anatomy.hpp"
#include "view_tier.hpp"
#include "../execution_engine/execution_engine_base.hpp"

#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::anatomy {

template <AnatomyConcept A>
class ViewAbiAdapter final : public IAnatomyBase, public IViewTier {
    static_assert(A::genus() == AnatomyGenus::View,
                  "ViewAbiAdapter erwartet eine View-Gattung-Anatomie (AnatomyGenus::View). "
                  "Cross-Genus-Adapter sind type-system-mathematisch unmoeglich — Doku 14 §32.");

public:
    [[nodiscard]] std::string_view engine_name() const noexcept override { return A::composition_name(); }
    [[nodiscard]] ::comdare::cache_engine::execution_engine::EngineLifecycleState
    lifecycle_state() const noexcept override {
        return state_;
    }
    void warm_up() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Warming; }
    void run() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Running; }
    void reset() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle; }
    void shutdown() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Shutdown; }

    [[nodiscard]] std::string_view composition_name() const noexcept override { return A::composition_name(); }
    [[nodiscard]] std::string_view paper_id() const noexcept override { return A::paper_id(); }
    [[nodiscard]] AnatomyGenus     genus() const noexcept override { return A::genus(); }
    [[nodiscard]] std::size_t      organ_count() const noexcept override { return A::organ_count(); }

    void tier_bind(std::uint64_t const* data, std::uint64_t size) noexcept override {
        anatomy_.bind(data, static_cast<std::size_t>(size));
    }
    [[nodiscard]] bool tier_read(std::uint64_t index, std::uint64_t* out_value) const noexcept override {
        auto r = anatomy_.read(index);
        if (!r) return false;
        if (out_value != nullptr) *out_value = *r;
        return true;
    }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override {
        return static_cast<std::uint64_t>(anatomy_.size());
    }

    void tier_observe_view(ViewObserverSnapshotV1* out) const noexcept override {
        if (out == nullptr) return;
        ViewObserverSnapshot const s = anatomy_.observe_all();
        ViewObserverSnapshotV1     v{};
        v.read_count            = s.read_count;
        v.read_oob_count        = s.read_oob_count;
        v.bound_size            = s.bound_size;
        v.bind_count            = s.bind_count;
        v.observable_axis_count = 2; // R5.B ehrlich: axis_layout + axis_accessor real getrieben
        v.organ_count           = A::organ_count();
        *out                    = v;
    }

private:
    A                                                               anatomy_{};
    ::comdare::cache_engine::execution_engine::EngineLifecycleState state_{
        ::comdare::cache_engine::execution_engine::EngineLifecycleState::Uninitialized};
};

} // namespace comdare::cache_engine::anatomy
