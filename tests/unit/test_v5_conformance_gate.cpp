// V5-I4 — Konformitäts-Gate Tests: das Gate erkennt konforme vs nicht-konforme std::map-Hüllen.
#include "builder/pruef_dock/conformance_gate.hpp"

#include <anatomy/scannable_tier.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>

namespace pd = comdare::cache_engine::builder::pruef_dock;
namespace an = comdare::cache_engine::anatomy;

// Korrekte std::map-Hülle (Referenz-konform), inkl. optionalem geordnetem Scan.
class GoodTier : public an::IDriveableTier, public an::IScannableTier {
public:
    bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        auto const r = m_.insert({k, v});
        if (!r.second) { r.first->second = v; }
        return r.second; // true = NEUER Key
    }
    bool tier_lookup(std::uint64_t k, std::uint64_t* o) const noexcept override {
        auto const it = m_.find(k);
        if (it == m_.end()) { return false; }
        if (o != nullptr) { *o = it->second; }
        return true;
    }
    bool          tier_erase(std::uint64_t k) noexcept override { return m_.erase(k) > 0; }
    void          tier_clear() noexcept override { m_.clear(); }
    std::uint64_t tier_size() const noexcept override { return m_.size(); }

    std::uint64_t tier_scan(std::uint64_t start_key, std::uint64_t max_count,
                            std::uint64_t* out_checksum) const noexcept override {
        std::uint64_t visited = 0;
        std::uint64_t sum     = 0;
        for (auto it = m_.lower_bound(start_key); it != m_.end() && visited < max_count; ++it) {
            sum += it->second;
            ++visited;
        }
        if (out_checksum != nullptr) { *out_checksum += sum; }
        return visited;
    }

protected:
    std::map<std::uint64_t, std::uint64_t> m_;
};

// Defekte Hülle: insert meldet IMMER "neu" (auch bei Update) → Konformitäts-Bruch (RF3).
class BrokenInsertTier : public GoodTier {
public:
    bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        (void)GoodTier::tier_insert(k, v);
        return true; // BUG: Update wird faelschlich als "neu" gemeldet
    }
};

// Defekte Hülle: size immer +1 (Off-by-one) → Größen-Konsistenz-Bruch.
class BrokenSizeTier : public GoodTier {
public:
    std::uint64_t tier_size() const noexcept override { return GoodTier::tier_size() + 1; } // BUG
};

// Defekte Hülle: synthetisches try_emplace ueberschreibt durch einen falschen Existenz-Miss den ALT-Wert.
class BrokenTryEmplaceTier : public GoodTier {
public:
    bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        auto const inserted = GoodTier::tier_insert(k, v);
        if (k == 19u && v == 190u) {
            try_key_present_      = true;
            try_key_lookup_count_ = 0;
        }
        return inserted;
    }

    bool tier_lookup(std::uint64_t k, std::uint64_t* o) const noexcept override {
        if (k == 19u && try_key_present_) {
            ++try_key_lookup_count_;
            if (try_key_lookup_count_ == 2u) { return false; }
        }
        return GoodTier::tier_lookup(k, o);
    }

private:
    bool                  try_key_present_      = false;
    mutable std::uint64_t try_key_lookup_count_ = 0;
};

// Defekte Hülle: empty-Synthese ueber size luegt bei leerem Zustand.
class BrokenEmptyTier : public GoodTier {
public:
    std::uint64_t tier_size() const noexcept override {
        auto const n = GoodTier::tier_size();
        return n == 0u ? 1u : n;
    }
};

// Defekte Hülle: tier_scan laeuft in Insert-Reihenfolge statt in Key-Reihenfolge.
class BrokenOrderTier : public GoodTier {
public:
    bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        bool const inserted = GoodTier::tier_insert(k, v);
        if (inserted) { insertion_order_.push_back(k); }
        return inserted;
    }
    bool tier_erase(std::uint64_t k) noexcept override {
        bool const erased = GoodTier::tier_erase(k);
        if (erased) {
            insertion_order_.erase(std::remove(insertion_order_.begin(), insertion_order_.end(), k),
                                   insertion_order_.end());
        }
        return erased;
    }
    void tier_clear() noexcept override {
        GoodTier::tier_clear();
        insertion_order_.clear();
    }
    std::uint64_t tier_scan(std::uint64_t start_key, std::uint64_t max_count,
                            std::uint64_t* out_checksum) const noexcept override {
        std::uint64_t visited = 0;
        std::uint64_t sum     = 0;
        for (auto const key : insertion_order_) {
            if (key < start_key || visited >= max_count) { continue; }
            std::uint64_t value = 0;
            if (GoodTier::tier_lookup(key, &value)) {
                sum += value;
                ++visited;
            }
        }
        if (out_checksum != nullptr) { *out_checksum += sum; }
        return visited;
    }

private:
    std::vector<std::uint64_t> insertion_order_;
};

TEST(V5ConformanceGate, GoodTierPassesAllCases) {
    GoodTier   t;
    auto const r = pd::run_conformance_gate(t);
    EXPECT_TRUE(r.passed());
    EXPECT_GT(r.cases_total, 0u);
    EXPECT_EQ(r.cases_passed, r.cases_total);
    EXPECT_EQ(r.first_fail, 0u);
}

TEST(V5ConformanceGate, BrokenInsertDetected) {
    BrokenInsertTier t;
    auto const       r = pd::run_conformance_gate(t);
    EXPECT_FALSE(r.passed());
    EXPECT_GT(r.first_fail, 0u);
    EXPECT_LT(r.cases_passed, r.cases_total);
}

TEST(V5ConformanceGate, BrokenSizeDetected) {
    BrokenSizeTier t;
    auto const     r = pd::run_conformance_gate(t);
    EXPECT_FALSE(r.passed());
    EXPECT_GT(r.first_fail, 0u);
}

TEST(V5ConformanceGate, BrokenTryEmplaceDetected) {
    BrokenTryEmplaceTier t;
    auto const           r = pd::run_conformance_gate(t);
    EXPECT_FALSE(r.passed());
    EXPECT_GT(r.first_fail, 0u);
}

TEST(V5ConformanceGate, BrokenEmptyDetected) {
    BrokenEmptyTier t;
    auto const      r = pd::run_conformance_gate(t);
    EXPECT_FALSE(r.passed());
    EXPECT_GT(r.first_fail, 0u);
}

TEST(V5ConformanceGate, BrokenOrderDetected) {
    BrokenOrderTier t;
    auto const      r = pd::run_conformance_gate(t);
    EXPECT_FALSE(r.passed());
    EXPECT_GT(r.first_fail, 0u);
}

// Determinismus: gleicher Seed → identisches Ergebnis (reproduzierbares Gate).
TEST(V5ConformanceGate, Deterministic) {
    GoodTier   a, b;
    auto const ra = pd::run_conformance_gate(a, 1234, 500);
    auto const rb = pd::run_conformance_gate(b, 1234, 500);
    EXPECT_EQ(ra.cases_total, rb.cases_total);
    EXPECT_EQ(ra.cases_passed, rb.cases_passed);
}
