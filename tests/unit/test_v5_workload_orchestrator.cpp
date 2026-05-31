// V5-I9 — WorkloadOrchestrator: host-seitiger generischer Lastprofil-Treiber.
//
// Beweist: (1) run_workload_profile fährt eine WorkloadConfig-Op-Sequenz über das Gattungs-ABI und respektiert
// die Zwei-Phasen-Invariante (2phase-End-Zustand == 1phase); (2) per-Op-Kind-Sample-Counts == Op-Vorkommen;
// (3) run_measurement_plan fährt MEHRERE Lastprofile je Binary (frischer Tier je Profil); (4) YCSB-C = 100%
// Lookup; (5) Lastprofil-Serialisierung. Mock-Tier = std::map<uint64,uint64>-Hülle (IObservableTier + IRollbackableTier).

#include <anatomy/observable_tier.hpp>
#include <anatomy/rollbackable_tier.hpp>
#include <builder/workload_driver/workload_orchestrator.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace an = ::comdare::cache_engine::anatomy;
namespace wd = ::comdare::cache_engine::builder::workload_driver;

namespace {

struct MockTier final : an::IObservableTier, an::IRollbackableTier {
    std::map<std::uint64_t, std::uint64_t> data_;
    std::uint64_t ins_ = 0, ers_ = 0, peak_ = 0;
    mutable std::uint64_t lk_ = 0, hit_ = 0, miss_ = 0;
    struct Snap { std::map<std::uint64_t, std::uint64_t> data; std::uint64_t ins, ers, peak, lk, hit, miss; };
    std::optional<Snap> saved_;

    bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        bool const nu = data_.emplace(k, v).second;
        if (nu) { ++ins_; if (data_.size() > peak_) peak_ = data_.size(); }
        return nu;
    }
    bool tier_lookup(std::uint64_t k, std::uint64_t* o) const noexcept override {
        ++lk_; auto const it = data_.find(k);
        if (it != data_.end()) { ++hit_; if (o) *o = it->second; return true; }
        ++miss_; return false;
    }
    bool tier_erase(std::uint64_t k) noexcept override { bool const e = data_.erase(k) != 0; if (e) ++ers_; return e; }
    void tier_clear() noexcept override { data_.clear(); }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return data_.size(); }
    void tier_observe(an::ComdareTierObserverSnapshotV1* o) const noexcept override {
        if (!o) return; an::ComdareTierObserverSnapshotV1 t{};
        t.search_insert_count = ins_; t.search_lookup_count = lk_; t.search_hit_count = hit_;
        t.search_miss_count = miss_; t.search_erase_count = ers_; t.search_peak_occupancy = peak_;
        t.tier_fill_level = data_.size(); *o = t;
    }
    void tier_save_all() noexcept override { saved_ = Snap{data_, ins_, ers_, peak_, lk_, hit_, miss_}; }
    void tier_rollback_all() noexcept override {
        if (saved_) { data_ = saved_->data; ins_ = saved_->ins; ers_ = saved_->ers; peak_ = saved_->peak;
                      lk_ = saved_->lk; hit_ = saved_->hit; miss_ = saved_->miss; }
    }
};

std::vector<wd::WorkloadOp> mixed_sequence() {
    using K = wd::WorkloadOpKind;
    return {
        {K::Insert, 1, 11}, {K::Insert, 2, 22}, {K::Insert, 3, 33}, {K::Lookup, 2, 0}, {K::Lookup, 9, 0},
        {K::Erase, 1, 0}, {K::Insert, 4, 44}, {K::Lookup, 4, 0}, {K::Clear, 0, 0},
        {K::Insert, 5, 55}, {K::Lookup, 5, 0}, {K::Erase, 5, 0}};
}

}  // namespace

// Zwei-Phasen-Lauf lässt EXAKT denselben End-Zustand wie der Einphasen-Lauf zurück (Warmups zurückgerollt).
TEST(V5WorkloadOrchestrator, TwoPhaseProfileEqualsSinglePhaseFinalState) {
    auto const ops = mixed_sequence();
    MockTier sp; wd::run_workload_profile(sp, nullptr, ops, "single");
    MockTier tp; wd::run_workload_profile(tp, &tp, ops, "twophase");
    EXPECT_EQ(sp.data_, tp.data_);
    EXPECT_EQ(sp.ins_, tp.ins_);
    EXPECT_EQ(sp.lk_, tp.lk_);
    EXPECT_EQ(sp.hit_, tp.hit_);
    EXPECT_EQ(sp.miss_, tp.miss_);
    EXPECT_EQ(sp.ers_, tp.ers_);
    EXPECT_EQ(sp.peak_, tp.peak_);
}

// Per-Op-Kind-Sample-Counts entsprechen den Op-Vorkommen in der Sequenz.
TEST(V5WorkloadOrchestrator, PerOpKindSampleCountsMatchSequence) {
    auto const ops = mixed_sequence();
    MockTier t; auto const r = wd::run_workload_profile(t, &t, ops, "p");
    EXPECT_EQ(r.insert_ns.size(), 5u);   // 5 Insert
    EXPECT_EQ(r.lookup_ns.size(), 4u);   // 4 Lookup
    EXPECT_EQ(r.erase_ns.size(), 2u);    // 2 Erase
    EXPECT_EQ(r.clear_ns.size(), 1u);    // 1 Clear
    EXPECT_EQ(r.op_count, ops.size());
}

// run_measurement_plan fährt MEHRERE Lastprofile je Binary (frischer Tier je Profil).
TEST(V5WorkloadOrchestrator, MeasurementPlanRunsMultipleProfiles) {
    wd::MeasurementPlan plan;
    plan.profiles = {wd::make_mixed_a(7, 200), wd::make_ycsb_c(7, 150)};
    MockTier t; auto const res = wd::run_measurement_plan(t, &t, plan);
    ASSERT_EQ(res.size(), 2u);
    EXPECT_EQ(res[0].op_count, 200u);
    EXPECT_EQ(res[1].op_count, 150u);
    EXPECT_EQ(res[0].profile_name, "MixedA_50_50");
    EXPECT_EQ(res[1].profile_name, "YCSB_C_read_only");
    // YCSB-C = 100% Lookup → nur lookup_ns gefüllt.
    EXPECT_EQ(res[1].lookup_ns.size(), 150u);
    EXPECT_TRUE(res[1].insert_ns.empty());
    EXPECT_TRUE(res[1].erase_ns.empty());
}

// Lastprofil-Serialisierung (Reproduzierbarkeits-Dokumentation).
TEST(V5WorkloadOrchestrator, SerializeWorkloadConfigFormat) {
    auto const s = wd::serialize_workload_config(wd::make_ycsb_c(42, 1000));
    EXPECT_EQ(s.rfind("YCSB_C_read_only|42|1000|1|1000000|", 0), 0u);
}
