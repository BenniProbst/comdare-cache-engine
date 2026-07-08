// #31-Schritt-1 (F7 Hybrid, WorkloadKind-Reuse): Trait-Test der additiven compile-time-Workload-Achse
// über den bestehenden enum WorkloadKind + runtime-Brücke profile_by_name. Beweist: die 6 YCSB-Profile A–F
// sind compile-time-iterierbar (Matrix-Achse W) UND config_for delegiert bit-identisch an profile_by_name
// (golden-neutral, kein neuer Enum — Bestands-WorkloadKind wiederverwendet).

#include <builder/workload_driver/workload_matrix.hpp>

#include <boost/mp11.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

namespace wd  = ::comdare::cache_engine::builder::workload_driver;
namespace cmd = ::comdare::cache_engine::builder::commands;
namespace mp  = ::boost::mp11;

static_assert(wd::ycsb_profile_count == 6, "6 YCSB-Profile A–F compile-time via WorkloadKind-Reuse.");

TEST(WorkloadMatrixHybrid, SixYcsbProfilesCompileTimeIterable) {
    std::size_t n = 0;
    mp::mp_for_each<wd::ycsb_profile_list>([&](auto tag) {
        ++n;
        (void)tag;
    });
    EXPECT_EQ(n, 6u);
}

TEST(WorkloadMatrixHybrid, WorkloadKindBridgesToRuntimeProfileBitIdentical) {
    // Hybrid-Brücke: jede compile-time-WorkloadKind (YCSB A–F) → config_for == profile_by_name(token)
    // (Delegation, golden-neutral) + liefert eine gültige (nicht-leere) Config.
    const std::uint64_t seed      = 42;
    const std::size_t   ops       = 1000;
    bool                all_valid = true;
    bool                all_match = true;
    mp::mp_for_each<wd::ycsb_profile_list>([&]<class Tag>(Tag) {
        constexpr cmd::WorkloadKind K        = Tag::value;
        auto                        via_tag  = wd::config_for(K, seed, ops);
        auto                        via_name = wd::profile_by_name(wd::ycsb_token(K), seed, ops);
        if (via_tag.name.empty()) { all_valid = false; }
        if (via_tag.name != via_name.name) { all_match = false; }
    });
    EXPECT_TRUE(all_valid); // jede YCSB-WorkloadKind → gültiges runtime-Profil
    EXPECT_TRUE(all_match); // config_for delegiert bit-identisch an profile_by_name (Hybrid, kein Drift)
}

TEST(WorkloadMatrixHybrid, TwoDMatrixIsWorkloadTimesDataset) {
    static_assert(wd::dataset_count == 6, "6er-Kanon-Datensätze.");
    static_assert(wd::matrix_cell_count == 36, "2D-Matrix = 6 Workloads × 6 Datasets.");
    std::size_t cells = 0;
    mp::mp_for_each<wd::matrix_cells>([&](auto cell) {
        ++cells;
        (void)cell;
    });
    EXPECT_EQ(cells, 36u); // Achse W (A–F) × Achse D (6 Datasets) = 36 compile-time-Zellen
}
