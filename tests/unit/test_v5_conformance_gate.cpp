// V5-I4 — Konformitäts-Gate Tests: das Gate erkennt konforme vs nicht-konforme std::map-Hüllen.
#include "builder/pruef_dock/conformance_gate.hpp"

#include <gtest/gtest.h>
#include <cstdint>
#include <map>

namespace pd = comdare::cache_engine::builder::pruef_dock;
namespace an = comdare::cache_engine::anatomy;

// Korrekte std::map-Hülle (Referenz-konform).
class GoodTier : public an::IDriveableTier {
    std::map<std::uint64_t, std::uint64_t> m_;
public:
    bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        auto const r = m_.insert({k, v});
        if (!r.second) { r.first->second = v; }
        return r.second;  // true = NEUER Key
    }
    bool tier_lookup(std::uint64_t k, std::uint64_t* o) const noexcept override {
        auto const it = m_.find(k);
        if (it == m_.end()) { return false; }
        if (o != nullptr) { *o = it->second; }
        return true;
    }
    bool tier_erase(std::uint64_t k) noexcept override { return m_.erase(k) > 0; }
    void tier_clear() noexcept override { m_.clear(); }
    std::uint64_t tier_size() const noexcept override { return m_.size(); }
};

// Defekte Hülle: insert meldet IMMER "neu" (auch bei Update) → Konformitäts-Bruch (RF3).
class BrokenInsertTier : public GoodTier {
public:
    bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        (void)GoodTier::tier_insert(k, v);
        return true;  // BUG: Update wird faelschlich als "neu" gemeldet
    }
};

// Defekte Hülle: size immer +1 (Off-by-one) → Größen-Konsistenz-Bruch.
class BrokenSizeTier : public GoodTier {
public:
    std::uint64_t tier_size() const noexcept override { return GoodTier::tier_size() + 1; }  // BUG
};

TEST(V5ConformanceGate, GoodTierPassesAllCases) {
    GoodTier t;
    auto const r = pd::run_conformance_gate(t);
    EXPECT_TRUE(r.passed());
    EXPECT_GT(r.cases_total, 0u);
    EXPECT_EQ(r.cases_passed, r.cases_total);
    EXPECT_EQ(r.first_fail, 0u);
}

TEST(V5ConformanceGate, BrokenInsertDetected) {
    BrokenInsertTier t;
    auto const r = pd::run_conformance_gate(t);
    EXPECT_FALSE(r.passed());
    EXPECT_GT(r.first_fail, 0u);
    EXPECT_LT(r.cases_passed, r.cases_total);
}

TEST(V5ConformanceGate, BrokenSizeDetected) {
    BrokenSizeTier t;
    auto const r = pd::run_conformance_gate(t);
    EXPECT_FALSE(r.passed());
    EXPECT_GT(r.first_fail, 0u);
}

// Determinismus: gleicher Seed → identisches Ergebnis (reproduzierbares Gate).
TEST(V5ConformanceGate, Deterministic) {
    GoodTier a, b;
    auto const ra = pd::run_conformance_gate(a, 1234, 500);
    auto const rb = pd::run_conformance_gate(b, 1234, 500);
    EXPECT_EQ(ra.cases_total, rb.cases_total);
    EXPECT_EQ(ra.cases_passed, rb.cases_passed);
}
