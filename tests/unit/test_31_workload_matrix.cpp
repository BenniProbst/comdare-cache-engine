// #31-Schritt-1 (F7 Option A, Hybrid): Trait-Test der additiven compile-time-Workload-Achse über den
// bestehenden runtime-profile_by_name. Beweist: die 8 Profile sind compile-time-iterierbar (Matrix-Achse W)
// UND config_for delegiert bit-identisch an profile_by_name (golden-neutral, kein Umbau).

#include <builder/workload_driver/workload_matrix.hpp>

#include <boost/mp11.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

namespace wd = ::comdare::cache_engine::builder::workload_driver;
namespace mp = ::boost::mp11;

static_assert(wd::profile_count == 8, "8 Workload-Profile compile-time.");

TEST(WorkloadMatrixHybrid, EightProfilesCompileTimeIterable) {
    std::size_t n = 0;
    mp::mp_for_each<wd::profile_list>([&](auto tag) {
        ++n;
        (void)tag;
    });
    EXPECT_EQ(n, 8u);
}

TEST(WorkloadMatrixHybrid, CompileTimeTagBridgesToRuntimeProfileBitIdentical) {
    // Hybrid-Brücke: jeder compile-time-Tag → config_for == profile_by_name(token) (Delegation, golden-neutral)
    // und liefert eine gültige (nicht-leere) Config.
    const std::uint64_t seed      = 42;
    const std::size_t   ops       = 1000;
    bool                all_valid = true;
    bool                all_match = true;
    mp::mp_for_each<wd::profile_list>([&]<class Tag>(Tag) {
        constexpr wd::WorkloadProfile P        = Tag::value;
        auto                          via_tag  = wd::config_for(P, seed, ops);
        auto                          via_name = wd::profile_by_name(wd::profile_token(P), seed, ops);
        if (via_tag.name.empty()) { all_valid = false; }
        if (via_tag.name != via_name.name) { all_match = false; }
    });
    EXPECT_TRUE(all_valid); // jeder compile-time-Tag → gültiges runtime-Profil
    EXPECT_TRUE(all_match); // config_for delegiert bit-identisch an profile_by_name (Hybrid, kein Drift)
}
