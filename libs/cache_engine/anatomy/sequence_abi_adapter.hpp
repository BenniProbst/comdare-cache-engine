#pragma once
// D10 / L-76b (2026-06-02) — SequenceAbiAdapter: Runtime-ABI-Adapter der SEQUENCE-Gattung (Reptil), analog
// Set/Container. Bridge SequenceAnatomy<Composition> → IAnatomyBase + ISequenceTier. static_assert genus()==Sequence.

#include "anatomy_base.hpp"
#include "sequence_anatomy.hpp"
#include "sequence_tier.hpp"
#include "../execution_engine/execution_engine_base.hpp"

#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::anatomy {

template <AnatomyConcept A>
class SequenceAbiAdapter final : public IAnatomyBase, public ISequenceTier {
    static_assert(A::genus() == AnatomyGenus::Sequence,
                  "SequenceAbiAdapter erwartet eine Sequence-Gattung-Anatomie (AnatomyGenus::Sequence). "
                  "Cross-Genus-Adapter sind type-system-mathematisch unmoeglich — Doku 14 §32.");

public:
    [[nodiscard]] std::string_view engine_name() const noexcept override { return A::composition_name(); }
    [[nodiscard]] ::comdare::cache_engine::execution_engine::EngineLifecycleState
    lifecycle_state() const noexcept override { return state_; }
    void warm_up()  override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Warming; }
    void run()      override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Running; }
    void reset()    override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Idle; }
    void shutdown() override { state_ = ::comdare::cache_engine::execution_engine::EngineLifecycleState::Shutdown; }

    [[nodiscard]] std::string_view composition_name() const noexcept override { return A::composition_name(); }
    [[nodiscard]] std::string_view paper_id()         const noexcept override { return A::paper_id(); }
    [[nodiscard]] AnatomyGenus     genus()            const noexcept override { return A::genus(); }
    [[nodiscard]] std::size_t      organ_count()      const noexcept override { return A::organ_count(); }

    void tier_push_back(std::uint64_t value) noexcept override {
        try { anatomy_.push_back(value); } catch (...) {}
    }
    [[nodiscard]] bool tier_at(std::uint64_t index, std::uint64_t* out_value) const noexcept override {
        try {
            auto r = anatomy_.at(index);
            if (!r) return false;
            if (out_value != nullptr) *out_value = *r;
            return true;
        } catch (...) { return false; }
    }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override {
        return static_cast<std::uint64_t>(anatomy_.size());
    }
    void tier_clear() noexcept override { try { anatomy_.clear(); } catch (...) {} }

    void tier_observe_sequence(SequenceObserverSnapshotV1* out) const noexcept override {
        if (out == nullptr) return;
        SequenceObserverSnapshot const s = anatomy_.observe_all();
        SequenceObserverSnapshotV1 v{};
        v.push_count            = s.push_count;
        v.at_count              = s.at_count;
        v.at_oob_count          = s.at_oob_count;
        v.current_size          = s.current_size;
        v.peak_size             = s.peak_size;
        v.growth_events         = s.growth_events;
        v.observable_axis_count = 2;   // R5.B ehrlich: V-Speicher-Organ + axis_growth real getrieben
        v.organ_count           = A::organ_count();
        *out = v;
    }

private:
    A anatomy_{};
    ::comdare::cache_engine::execution_engine::EngineLifecycleState state_{
        ::comdare::cache_engine::execution_engine::EngineLifecycleState::Uninitialized};
};

}  // namespace comdare::cache_engine::anatomy
