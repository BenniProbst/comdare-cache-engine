#pragma once
// D10 / L-76b (2026-06-02) — SequenceAnatomy: die SEQUENCE-GATTUNG (Reptil, genus()==Sequence, V-indexed).
// Hält ein V-Speicher-Organ (wachsendes Array) und treibt die axis_growth-Policy der Komposition REAL
// (next_capacity steuert reserve → growth_events). API: push_back(v)/at(idx)→value/size/clear. Doku 14 §27.2/§28/§32.

#include "anatomy_base.hpp"         // AnatomyGenus
#include "sequence_composition.hpp" // SequenceComposition / GrowthPolicy

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::anatomy {

/// In-process Sequence-Observer (reich; SequenceObserverSnapshotV1 ist die cross-boundary-Spiegelung).
struct SequenceObserverSnapshot {
    std::uint64_t push_count    = 0;
    std::uint64_t at_count      = 0;
    std::uint64_t at_oob_count  = 0;
    std::uint64_t current_size  = 0;
    std::uint64_t peak_size     = 0;
    std::uint64_t growth_events = 0;
};

/// SequenceAnatomy<Composition> — genus()==Sequence; V-indexed-API über ein wachsendes Array; treibt die
/// axis_growth-Policy (Composition::growth_policy) real (Kapazitäts-Wachstum).
template <class Composition>
class SequenceAnatomy {
public:
    using composition_t = Composition;
    using growth_t      = typename Composition::growth_policy;
    using element_type  = std::uint64_t;

    static constexpr std::string_view composition_name() noexcept { return Composition::name; }
    static constexpr std::string_view paper_id() noexcept { return Composition::paper_id; }
    static constexpr AnatomyGenus     genus() noexcept { return AnatomyGenus::Sequence; }        // Reptil
    static constexpr std::size_t      organ_count() noexcept { return Composition::slot_count; } // 11

    // ── Sequence-Gattungs-API (V-indexed) — treibt das V-Organ + die axis_growth-Policy ──
    void push_back(element_type v) {
        if (data_.size() == capacity_) { // Überlauf → Growth-Policy befragen
            capacity_ = growth_.next_capacity(capacity_, data_.size() + 1);
            data_.reserve(capacity_);
            ++obs_.growth_events;
        }
        data_.push_back(v);
        ++obs_.push_count;
        obs_.current_size = static_cast<std::uint64_t>(data_.size());
        if (obs_.current_size > obs_.peak_size) obs_.peak_size = obs_.current_size;
    }
    [[nodiscard]] std::optional<element_type> at(std::uint64_t index) const {
        ++obs_.at_count;
        if (index >= data_.size()) {
            ++obs_.at_oob_count;
            return std::nullopt;
        }
        return data_[static_cast<std::size_t>(index)];
    }
    [[nodiscard]] std::size_t size() const noexcept { return data_.size(); }
    void                      clear() noexcept {
        data_.clear();
        obs_.current_size = 0;
    }

    [[nodiscard]] SequenceObserverSnapshot observe_all() const noexcept { return obs_; }

private:
    std::vector<element_type>        data_{};
    growth_t                         growth_{};
    std::size_t                      capacity_ = 0;
    mutable SequenceObserverSnapshot obs_{}; // mutable: at() ist const, zählt aber Zugriffe
};

} // namespace comdare::cache_engine::anatomy
