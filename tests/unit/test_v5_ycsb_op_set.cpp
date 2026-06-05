// V5-#49-E/F — YCSB-Op-Set-Erweiterung: Range-Scan (E) + Read-Modify-Write (F) TREU abgebildet.
//
// Beweist (User-Direktive 2026-05-31, Task #49 Pfad A — YCSB treu einbinden, KEIN Fake-Proxy):
//   (1) make_ycsb_e generiert ~95% Scan + ~5% Insert (kein Lookup/Erase/Clear/RMW); Scan-Länge in [1,100].
//   (2) make_ycsb_f generiert ~50% Lookup + ~50% ReadModifyWrite (kein Insert/Erase/Clear/Scan).
//   (3) run_workload_profile misst Scan über IScannableTier (scan_ns gefüllt) — und überspringt Scan-Ops
//       EHRLICH, wenn kein IScannableTier übergeben wird (scan==nullptr → scan_ns leer, kein Fake-Sample).
//   (4) ReadModifyWrite verändert den Wert real (lookup → modify → upsert), ohne ABI-Erweiterung.
//   (5) tier_scan liefert geordnete Record-Zahl ab start_key + korrekte Wert-Prüfsumme.
// Mock-Tier = std::map<uint64,uint64>-Hülle (IObservableTier + IRollbackableTier + IScannableTier).

#include <anatomy/observable_tier.hpp>
#include <anatomy/rollbackable_tier.hpp>
#include <anatomy/scannable_tier.hpp>
#include <builder/workload_driver/workload_orchestrator.hpp>
#include <builder/workload_driver/workload_generator.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace an = ::comdare::cache_engine::anatomy;
namespace wd = ::comdare::cache_engine::builder::workload_driver;

namespace {

// std::map-Mock-Tier mit Scan-Fähigkeit (std::map ist natürlich key-sortiert → echter Range-Scan via lower_bound).
struct ScannableMockTier final : an::IObservableTier, an::IRollbackableTier, an::IScannableTier {
    std::map<std::uint64_t, std::uint64_t> data_;
    std::uint64_t ins_ = 0, ers_ = 0, peak_ = 0;
    mutable std::uint64_t lk_ = 0, hit_ = 0, miss_ = 0;
    struct Snap { std::map<std::uint64_t, std::uint64_t> data; std::uint64_t ins, ers, peak, lk, hit, miss; };
    std::optional<Snap> saved_;

    bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        auto const it = data_.find(k);
        if (it != data_.end()) { it->second = v; return false; }   // Upsert (Update bei Kollision)
        data_.emplace(k, v); ++ins_; if (data_.size() > peak_) peak_ = data_.size(); return true;
    }
    bool tier_lookup(std::uint64_t k, std::uint64_t* o) const noexcept override {
        ++lk_; auto const it = data_.find(k);
        if (it != data_.end()) { ++hit_; if (o) *o = it->second; return true; }
        ++miss_; return false;
    }
    bool tier_erase(std::uint64_t k) noexcept override { bool const e = data_.erase(k) != 0; if (e) ++ers_; return e; }
    void tier_clear() noexcept override { data_.clear(); }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return data_.size(); }
    void tier_observe(an::ComdareTierObserverSnapshot* o) const noexcept override {   // konsolidierter Observer: search → axis_stats[0]
        if (!o) return;
        o->axis_stats[0][3] = ins_; o->axis_stats[0][0] = lk_;  o->axis_stats[0][1] = hit_;
        o->axis_stats[0][2] = miss_; o->axis_stats[0][4] = ers_; o->axis_stats[0][5] = peak_;
        o->tier_fill_level = data_.size();
    }
    void tier_save_all() noexcept override { saved_ = Snap{data_, ins_, ers_, peak_, lk_, hit_, miss_}; }
    void tier_rollback_all() noexcept override {
        if (saved_) { data_ = saved_->data; ins_ = saved_->ins; ers_ = saved_->ers; peak_ = saved_->peak;
                      lk_ = saved_->lk; hit_ = saved_->hit; miss_ = saved_->miss; }
    }
    // IScannableTier: ab start_key bis max_count Records IN KEY-REIHENFOLGE (std::map = sortiert).
    std::uint64_t tier_scan(std::uint64_t start_key, std::uint64_t max_count,
                            std::uint64_t* out_checksum) const noexcept override {
        std::uint64_t visited = 0, sum = 0;
        for (auto it = data_.lower_bound(start_key); it != data_.end() && visited < max_count; ++it) {
            sum += it->second; ++visited;
        }
        if (out_checksum) *out_checksum += sum;
        return visited;
    }
};

struct OpCounts { std::size_t insert=0, lookup=0, erase=0, clear=0, scan=0, rmw=0; };

OpCounts count_ops(std::vector<wd::WorkloadOp> const& ops) {
    OpCounts c;
    for (auto const& op : ops) switch (op.kind) {
        case wd::WorkloadOpKind::Insert:          ++c.insert; break;
        case wd::WorkloadOpKind::Lookup:          ++c.lookup; break;
        case wd::WorkloadOpKind::Erase:           ++c.erase;  break;
        case wd::WorkloadOpKind::Clear:           ++c.clear;  break;
        case wd::WorkloadOpKind::Scan:            ++c.scan;   break;
        case wd::WorkloadOpKind::ReadModifyWrite: ++c.rmw;    break;
    }
    return c;
}

}  // namespace

// ── (1) YCSB-E: ~95% Scan + ~5% Insert, sonst nichts; Scan-Länge ∈ [1,100] ──
TEST(V5YcsbOpSet, YcsbE_MostlyScan_PlusInsert) {
    wd::WorkloadGenerator gen{wd::make_ycsb_e(7, 4000)};
    auto const ops = gen.generate_all();
    auto const c = count_ops(ops);
    EXPECT_EQ(c.lookup, 0u);
    EXPECT_EQ(c.erase, 0u);
    EXPECT_EQ(c.clear, 0u);
    EXPECT_EQ(c.rmw, 0u);
    EXPECT_GT(c.scan, c.insert);                    // Scan dominiert klar (95 vs 5)
    EXPECT_GT(c.scan, static_cast<std::size_t>(4000 * 0.90));   // ~95% Scan (Toleranz)
    EXPECT_GT(c.insert, 0u);                        // 5% Insert real vorhanden
    // Scan-Länge wird in op.value kodiert: ∈ [1, scan_length_max=100].
    for (auto const& op : ops) if (op.kind == wd::WorkloadOpKind::Scan) {
        EXPECT_GE(op.value, 1u);
        EXPECT_LE(op.value, 100u);
    }
}

// ── (2) YCSB-F: ~50% Lookup + ~50% RMW, sonst nichts ──
TEST(V5YcsbOpSet, YcsbF_HalfReadHalfRmw) {
    wd::WorkloadGenerator gen{wd::make_ycsb_f(7, 4000)};
    auto const c = count_ops(gen.generate_all());
    EXPECT_EQ(c.insert, 0u);
    EXPECT_EQ(c.erase, 0u);
    EXPECT_EQ(c.clear, 0u);
    EXPECT_EQ(c.scan, 0u);
    EXPECT_GT(c.lookup, static_cast<std::size_t>(4000 * 0.40));   // ~50% (Toleranz)
    EXPECT_GT(c.rmw,    static_cast<std::size_t>(4000 * 0.40));
}

// ── (5) tier_scan: geordnete Record-Zahl ab start_key + korrekte Prüfsumme ──
TEST(V5YcsbOpSet, TierScanOrderedCountAndChecksum) {
    ScannableMockTier t;
    for (std::uint64_t k = 1; k <= 10; ++k) (void)t.tier_insert(k, k * 10);   // (1..10) -> (10..100)
    std::uint64_t cs = 0;
    auto const n = t.tier_scan(/*start*/4, /*max*/3, &cs);
    EXPECT_EQ(n, 3u);                  // Keys 4,5,6
    EXPECT_EQ(cs, 40u + 50u + 60u);    // Werte 40+50+60
    // Scan über das Ende hinaus → nur bis zur Füllung.
    std::uint64_t cs2 = 0;
    EXPECT_EQ(t.tier_scan(9, 100, &cs2), 2u);   // nur Keys 9,10
    EXPECT_EQ(cs2, 90u + 100u);
}

// ── (3) run_workload_profile misst Scan über IScannableTier — und überspringt es ehrlich ohne ──
TEST(V5YcsbOpSet, ScanMeasuredWithInterface_SkippedWithout) {
    ScannableMockTier t;
    for (std::uint64_t k = 1; k <= 50; ++k) (void)t.tier_insert(k, k);
    std::vector<wd::WorkloadOp> const ops{
        {wd::WorkloadOpKind::Scan, 5, 10}, {wd::WorkloadOpKind::Scan, 20, 5}, {wd::WorkloadOpKind::Scan, 1, 8}};
    // mit IScannableTier → 3 Scan-Samples
    auto const r_with = wd::run_workload_profile(t, nullptr, ops, "scan", &t);
    EXPECT_EQ(r_with.scan_ns.size(), 3u);
    EXPECT_GT(r_with.read_sink, 0u);
    // ohne (scan==nullptr) → KEINE Scan-Samples (ehrlich übersprungen, kein Fake)
    auto const r_without = wd::run_workload_profile(t, nullptr, ops, "scan_noscan");
    EXPECT_TRUE(r_without.scan_ns.empty());
}

// ── (4) ReadModifyWrite verändert den Wert real (lookup → modify → upsert) ──
TEST(V5YcsbOpSet, RmwModifiesValue) {
    ScannableMockTier t;
    (void)t.tier_insert(5, 100);
    std::vector<wd::WorkloadOp> const ops{{wd::WorkloadOpKind::ReadModifyWrite, 5, 7}};
    auto const r = wd::run_workload_profile(t, nullptr, ops, "rmw");   // rollback=nullptr → genau EINE Anwendung
    EXPECT_EQ(r.rmw_ns.size(), 1u);
    std::uint64_t out = 0;
    ASSERT_TRUE(t.tier_lookup(5, &out));
    EXPECT_EQ(out, 100u ^ 7u);          // modifiziert = alt XOR op.value
    EXPECT_EQ(t.tier_size(), 1u);       // kein neuer Key (Upsert, nicht Insert)
}
