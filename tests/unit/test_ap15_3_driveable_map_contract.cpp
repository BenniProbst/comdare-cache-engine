// AP15-3 (#263-Rest, Strang C) -- DriveableMapContract<Derived> direkt gegen std::map treiben.

#include "builder/measurement_snapshot.hpp"
#include "builder/pruef_dock/driveable_map_contract.hpp"

#include <anatomy/idriveable_tier.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/scannable_tier.hpp>
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace an = ::comdare::cache_engine::anatomy;
namespace b  = ::comdare::cache_engine::builder;
namespace pd = ::comdare::cache_engine::builder::pruef_dock;

namespace {

using U64 = std::uint64_t;

class MapTier : public an::IDriveableTier {
public:
    [[nodiscard]] bool tier_insert(U64 k, U64 v) noexcept override { return m_.insert_or_assign(k, v).second; }

    [[nodiscard]] bool tier_lookup(U64 k, U64* out) const noexcept override {
        auto const it = m_.find(k);
        if (it == m_.end()) return false;
        if (out != nullptr) *out = it->second;
        return true;
    }

    [[nodiscard]] bool tier_erase(U64 k) noexcept override { return m_.erase(k) != 0u; }

    void tier_clear() noexcept override { m_.clear(); }

    [[nodiscard]] U64 tier_size() const noexcept override { return m_.size(); }

protected:
    std::map<U64, U64> m_;
};

class ScanningMapTier final : public MapTier, public an::IScannableTier {
public:
    [[nodiscard]] U64 tier_scan(U64 start_key, U64 max_count, U64* out_checksum) const noexcept override {
        U64 visited = 0;
        U64 sum     = 0;
        for (auto it = m_.lower_bound(start_key); it != m_.end() && visited < max_count; ++it) {
            sum += it->second;
            ++visited;
        }
        if (out_checksum != nullptr) *out_checksum += sum;
        return visited;
    }
};

class ContractWithoutScan final : public pd::DriveableMapContract<ContractWithoutScan> {
public:
    [[nodiscard]] bool tier_insert(U64 k, U64 v) noexcept { return m_.insert_or_assign(k, v).second; }

    [[nodiscard]] bool tier_lookup(U64 k, U64* out) const noexcept {
        auto const it = m_.find(k);
        if (it == m_.end()) return false;
        if (out != nullptr) *out = it->second;
        return true;
    }

    [[nodiscard]] bool tier_erase(U64 k) noexcept { return m_.erase(k) != 0u; }

    void tier_clear() noexcept { m_.clear(); }

    [[nodiscard]] U64 tier_size() const noexcept { return m_.size(); }

private:
    std::map<U64, U64> m_;
};

template <class T>
concept HasLowerBound = requires(T const& c) { c.lower_bound(U64{0}, U64{1}); };

static_assert(!HasLowerBound<ContractWithoutScan>);

[[nodiscard]] U64 value_for(U64 key) noexcept { return key * 10u + 3u; }

void expect_lookup_matches(pd::DriveableMapView const& view, std::map<U64, U64> const& oracle, U64 key) {
    auto const it  = oracle.find(key);
    U64        got = 0;
    EXPECT_EQ(view.at(key, got), it != oracle.end());
    if (it != oracle.end()) { EXPECT_EQ(got, it->second); }
    EXPECT_EQ(view.contains(key), oracle.contains(key));
    EXPECT_EQ(view.count(key), oracle.count(key));
}

[[nodiscard]] std::pair<U64, U64> oracle_scan(std::map<U64, U64> const& oracle, U64 start_key, U64 max_count) {
    U64 expected_count = 0;
    U64 expected_sum   = 0;
    for (auto it = oracle.lower_bound(start_key); it != oracle.end() && expected_count < max_count; ++it) {
        expected_sum += it->second;
        ++expected_count;
    }
    return {expected_count, expected_sum};
}

template <class ScanResult>
void expect_scan_matches(ScanResult const& got, std::map<U64, U64> const& oracle, U64 start_key, U64 max_count) {
    auto const [expected_count, expected_sum] = oracle_scan(oracle, start_key, max_count);
    EXPECT_TRUE(got.available);
    EXPECT_EQ(got.count, expected_count);
    EXPECT_EQ(got.checksum, expected_sum);
}

[[nodiscard]] std::size_t count_cols(std::string const& csv_first_line) {
    return static_cast<std::size_t>(std::count(csv_first_line.begin(), csv_first_line.end(), ',') + 1);
}

[[nodiscard]] std::string first_line(std::string const& s) { return s.substr(0, s.find('\n')); }

} // namespace

TEST(AP153DriveableMapContract, BasicDaggerOpsMatchStdMap) {
    MapTier              tier;
    pd::DriveableMapView view{tier};
    std::map<U64, U64>   oracle;

    EXPECT_TRUE(view.empty());
    EXPECT_EQ(view.size(), 0u);
    expect_lookup_matches(view, oracle, 7u);

    EXPECT_EQ(view.insert_if_absent(17u, 170u), oracle.insert({17u, 170u}).second);
    expect_lookup_matches(view, oracle, 17u);
    EXPECT_EQ(view.insert_if_absent(17u, 171u), oracle.insert({17u, 171u}).second);
    expect_lookup_matches(view, oracle, 17u);

    EXPECT_EQ(view.insert_or_assign(20u, 200u), oracle.insert_or_assign(20u, 200u).second);
    expect_lookup_matches(view, oracle, 20u);
    EXPECT_EQ(view.insert_or_assign(20u, 201u), oracle.insert_or_assign(20u, 201u).second);
    expect_lookup_matches(view, oracle, 20u);

    U64        bracket = 999u;
    auto const op_new  = oracle[21u];
    EXPECT_TRUE(view.bracket(21u, bracket));
    EXPECT_EQ(bracket, op_new);
    EXPECT_EQ(view.size(), oracle.size());

    EXPECT_EQ(view.insert_or_assign(22u, 220u), oracle.insert_or_assign(22u, 220u).second);
    auto const op_old = oracle[22u];
    EXPECT_TRUE(view.bracket(22u, bracket));
    EXPECT_EQ(bracket, op_old);
    EXPECT_EQ(view.size(), oracle.size());

    EXPECT_EQ(view.bracket(23u), oracle[23u]);
    EXPECT_EQ(view.size(), oracle.size());

    U64 at_hit = 0;
    EXPECT_TRUE(view.at(22u, at_hit));
    EXPECT_EQ(at_hit, oracle.at(22u));

    bool oracle_at_miss_threw = false;
    try {
        (void)oracle.at(222u);
    } catch (std::out_of_range const&) { oracle_at_miss_threw = true; }
    EXPECT_TRUE(oracle_at_miss_threw);
    EXPECT_FALSE(view.at(222u, at_hit));

    EXPECT_EQ(view.erase(17u), oracle.erase(17u) != 0u);
    expect_lookup_matches(view, oracle, 17u);
    view.clear();
    oracle.clear();
    EXPECT_TRUE(view.empty());
    EXPECT_EQ(view.size(), oracle.size());
    expect_lookup_matches(view, oracle, 22u);
}

TEST(AP153DriveableMapContract, OrderingOpsWithScannerMatchStdMap) {
    ScanningMapTier      tier;
    pd::DriveableMapView view{tier};
    std::map<U64, U64>   oracle;

    for (U64 const key : std::array<U64, 5>{40u, 10u, 30u, 20u, 50u}) {
        EXPECT_EQ(view.insert_or_assign(key, value_for(key)), oracle.insert_or_assign(key, value_for(key)).second);
    }

    EXPECT_TRUE(view.has_scanner());
    EXPECT_TRUE(view.has_ordering_interface());

    for (U64 prefix = 0; prefix <= oracle.size() + 1u; ++prefix) {
        expect_scan_matches(view.lower_bound(0u, prefix), oracle, 0u, prefix);
    }
    expect_scan_matches(view.lower_bound(60u, 4u), oracle, 60u, 4u);

    for (U64 const key : std::array<U64, 6>{5u, 10u, 25u, 30u, 50u, 60u}) {
        expect_scan_matches(view.lower_bound(key, 1u), oracle, key, 1u);

        auto const key_present = oracle.contains(key);
        auto const upper_start = key_present ? key + 1u : key;
        expect_scan_matches(view.upper_bound(key, key_present, 1u), oracle, upper_start, 1u);

        auto const eq = view.equal_range(key, key_present);
        expect_scan_matches(eq.first, oracle, key, 1u);
        expect_scan_matches(eq.second, oracle, upper_start, 1u);
    }
}

TEST(AP153DriveableMapContract, OrderingOpsUnavailableWithoutScanner) {
    MapTier              tier;
    pd::DriveableMapView view{tier};

    EXPECT_TRUE(view.insert_or_assign(10u, 103u));
    EXPECT_FALSE(view.has_scanner());
    EXPECT_FALSE(view.has_ordering_interface());

    auto const lower = view.lower_bound(0u, 1u);
    EXPECT_FALSE(lower.available);
    EXPECT_EQ(lower.count, 0u);
    EXPECT_EQ(lower.checksum, 0u);

    auto const eq = view.equal_range(10u, true);
    EXPECT_FALSE(eq.first.available);
    EXPECT_FALSE(eq.second.available);
}

TEST(AP153DriveableMapContract, NeutralityGuardsStayIntact) {
    static_assert(std::is_trivially_copyable_v<b::ComdareMeasurementSnapshotV1>);
    static_assert(std::is_trivially_copyable_v<an::ComdareTierObserverSnapshot>);

    EXPECT_EQ(COMDARE_ANATOMY_ABI_MAJOR, 5);
    EXPECT_EQ(sizeof(an::ComdareTierObserverSnapshot), 1344u);
    EXPECT_EQ(an::kTierObserverSnapshotVersionUnified, 6u);

    std::vector<b::ComdareMeasurementSnapshotV1> rows(1);
    std::vector<std::string>                     ids{"neutrality_guard"};
    std::vector<std::string>                     workloads{"ap15_3"};

    auto const full_csv = b::serialize_measurements_csv(rows, ids, workloads);
    EXPECT_EQ(count_cols(first_line(full_csv)), 25u);

    auto const pipeline_csv = b::serialize_measurements_pipeline16_csv(rows, ids, workloads);
    EXPECT_EQ(count_cols(first_line(pipeline_csv)), 16u);
}
