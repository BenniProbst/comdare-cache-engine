// V5-I7 — Zwei-Phasen-Treiber: Kern-Invariante (save→warmup→rollback→measure macht NUR via Mess-Op Fortschritt).
//
// Beweist (User-Direktive 2026-05-31, Mess-Architektur §4): da jede Warmup-Phase exakt zurückgerollt wird,
// ist der End-Zustand (Daten + ALLE Observer-Zähler) der Zwei-Phasen-Messung IDENTISCH zur Einphasen-Messung
// UND zur Kalt-Fallback-Messung (rollback==nullptr). Die Latenz-Samples sind „warm" (Warmup heizt Cache/BP),
// aber die LOGIK ist gegen denselben Vor-Zustand gemessen. Mock-Tier = std::map<uint64,uint64>-Hülle.

#include <anatomy/observable_tier.hpp>
#include <anatomy/rollbackable_tier.hpp>
#include <builder/anatomy_commands/tier_observe_trace_abi.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <optional>

namespace an = ::comdare::cache_engine::anatomy;
namespace ac = ::comdare::cache_engine::builder::anatomy_commands;

namespace {

// std::map-Mock-Tier: erfüllt IObservableTier (Antrieb + Observer) + IRollbackableTier (memento_all via Kopie).
struct MockTier final : an::IObservableTier, an::IRollbackableTier {
    std::map<std::uint64_t, std::uint64_t> data_;
    std::uint64_t                          ins_ = 0, ers_ = 0, peak_ = 0;
    mutable std::uint64_t                  lk_ = 0, hit_ = 0, miss_ = 0; // tier_lookup ist const → Stats mutable
    struct Snap {
        std::map<std::uint64_t, std::uint64_t> data;
        std::uint64_t                          ins{}, ers{}, peak{}, lk{}, hit{}, miss{};
    };
    std::optional<Snap> saved_;

    bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        bool const nu = data_.emplace(k, v).second;
        if (nu) {
            ++ins_;
            if (data_.size() > peak_) peak_ = data_.size();
        }
        return nu;
    }
    bool tier_lookup(std::uint64_t k, std::uint64_t* out) const noexcept override {
        ++lk_;
        auto const it = data_.find(k);
        if (it != data_.end()) {
            ++hit_;
            if (out) *out = it->second;
            return true;
        }
        ++miss_;
        return false;
    }
    bool tier_erase(std::uint64_t k) noexcept override {
        bool const e = data_.erase(k) != 0;
        if (e) ++ers_;
        return e;
    }
    void                        tier_clear() noexcept override { data_.clear(); }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return data_.size(); }
    // Die EINE konsolidierte tier_observe — search → axis_stats[0].
    void tier_observe(an::ComdareTierObserverSnapshot* o) const noexcept override {
        if (!o) return;
        o->axis_stats[0][3] = ins_;
        o->axis_stats[0][0] = lk_;
        o->axis_stats[0][1] = hit_;
        o->axis_stats[0][2] = miss_;
        o->axis_stats[0][4] = ers_;
        o->axis_stats[0][5] = peak_;
        o->tier_fill_level  = data_.size();
    }
    void tier_save_all() noexcept override { saved_ = Snap{data_, ins_, ers_, peak_, lk_, hit_, miss_}; }
    void tier_rollback_all() noexcept override {
        if (saved_) {
            data_ = saved_->data;
            ins_  = saved_->ins;
            ers_  = saved_->ers;
            peak_ = saved_->peak;
            lk_   = saved_->lk;
            hit_  = saved_->hit;
            miss_ = saved_->miss;
        }
    }
};

ac::AbiTierTraceConfig small_cfg() {
    ac::AbiTierTraceConfig cfg;
    cfg.fill_checkpoints       = {10, 50};
    cfg.lookups_per_checkpoint = 200;
    cfg.deletes_per_checkpoint = 20;
    cfg.seed                   = 7;
    return cfg;
}

void expect_same_final_state(MockTier const& x, MockTier const& y) {
    EXPECT_EQ(x.data_, y.data_);
    EXPECT_EQ(x.ins_, y.ins_);
    EXPECT_EQ(x.lk_, y.lk_);
    EXPECT_EQ(x.hit_, y.hit_);
    EXPECT_EQ(x.miss_, y.miss_);
    EXPECT_EQ(x.ers_, y.ers_);
    EXPECT_EQ(x.peak_, y.peak_);
}

} // namespace

// Zwei-Phasen-Messung (mit Rollback) lässt EXAKT denselben End-Zustand wie die Einphasen-Messung zurück.
TEST(V5TwoPhaseDriver, TwoPhaseEqualsSinglePhaseFinalState) {
    auto const cfg = small_cfg();
    MockTier   single;
    ac::drive_tier_observe_trace_abi(single, cfg);
    MockTier twophase;
    ac::drive_two_phase_tier_trace_abi(twophase, &twophase, cfg);
    expect_same_final_state(single, twophase);
}

// Kalt-Fallback (rollback==nullptr) verhält sich wie die Einphasen-Messung (keine Warmup-Ops).
TEST(V5TwoPhaseDriver, ColdFallbackEqualsSinglePhase) {
    auto const cfg = small_cfg();
    MockTier   single;
    ac::drive_tier_observe_trace_abi(single, cfg);
    MockTier cold;
    ac::drive_two_phase_tier_trace_abi(cold, nullptr, cfg);
    expect_same_final_state(single, cold);
}

// Füllstands-Trajektorie identisch (die gemessenen Ops bauen denselben Füllstand auf).
TEST(V5TwoPhaseDriver, FillLevelTrajectoryMatches) {
    auto const cfg = small_cfg();
    MockTier   single;
    auto const ts = ac::drive_tier_observe_trace_abi(single, cfg);
    MockTier   twophase;
    auto const tt = ac::drive_two_phase_tier_trace_abi(twophase, &twophase, cfg);
    ASSERT_EQ(ts.checkpoints.size(), tt.checkpoints.size());
    for (std::size_t i = 0; i < ts.checkpoints.size(); ++i) {
        EXPECT_EQ(ts.checkpoints[i].fill_level, tt.checkpoints[i].fill_level);
    }
}
