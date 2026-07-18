// AP-7b-3/#27 -- SwissTable ISA group-match primitive (opt-in SIMD, default scalar).

#include <axes/lookup/composable/tier_to_organ_mapping.hpp>
#include <axes/lookup/axis_03a_search_algo_swisstable.hpp>
#include <axes/simd/axis_09_isa_aarch64.hpp>
#include <axes/simd/axis_09_isa_amd64.hpp>
#include <axes/simd/axis_09_isa_observable.hpp>
#include <axes/simd/axis_09_isa_powerpc.hpp>
#include <axes/simd/axis_09_isa_riscv.hpp>

#include <anatomy/observable_tier.hpp>
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>

#include <gtest/gtest.h>

#include <array>
#include <concepts>
#include <cstdint>
#include <map>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace anatomy = ::comdare::cache_engine::anatomy;
namespace lkc     = ::comdare::cache_engine::lookup::composable;
namespace simd    = ::comdare::cache_engine::simd;

namespace {

using Ctrl16 = std::array<std::uint8_t, 16>;

template <class I>
consteval std::uint16_t compile_mask(Ctrl16 ctrl, std::uint8_t needle) {
    return I::group_match_mask(ctrl.data(), needle);
}

template <class I>
[[nodiscard]] std::uint16_t runtime_mask(Ctrl16 const& ctrl, std::uint8_t needle) {
    return I::group_match_mask(ctrl.data(), needle);
}

consteval bool all_compile_masks_equal(Ctrl16 ctrl, std::uint8_t needle) {
    auto const scalar = compile_mask<lkc::ScalarGroupMatch>(ctrl, needle);
    return compile_mask<simd::Amd64Isa>(ctrl, needle) == scalar &&
           compile_mask<simd::Aarch64Isa>(ctrl, needle) == scalar &&
           compile_mask<simd::RiscVIsa>(ctrl, needle) == scalar &&
           compile_mask<simd::PowerPcIsa>(ctrl, needle) == scalar;
}

template <class Organ>
[[nodiscard]] std::map<std::uint64_t, std::uint64_t> dump_records(Organ const& organ) {
    std::map<std::uint64_t, std::uint64_t> out;
    std::size_t const                      visits = organ.for_each_record([&](std::uint64_t k, std::uint64_t v) {
        bool const inserted = out.emplace(k, v).second;
        EXPECT_TRUE(inserted) << "duplicate key=" << k;
    });
    EXPECT_EQ(visits, out.size());
    EXPECT_EQ(visits, organ.occupied_count());
    return out;
}

[[nodiscard]] std::uint64_t value_for(std::uint64_t key, std::uint64_t salt) {
    return (key * 0x9E3779B97F4A7C15ull) ^ (salt << 17u) ^ 0xD1B54A32D192ED03ull;
}

std::vector<std::uint64_t> collision_heavy_keys() {
    std::vector<std::uint64_t> keys;
    keys.reserve(384u);
    for (std::uint64_t h2 = 0; h2 < 8u; ++h2) {
        for (std::uint64_t n = 0; n < 48u; ++n) keys.push_back((1ull << 36u) + h2 + 128ull * (n * 3u + h2));
    }
    return keys;
}

template <class A, class B>
void expect_equivalent(A const& a, B const& b, std::vector<std::uint64_t> const& touched, std::string_view phase) {
    EXPECT_EQ(a.occupied_count(), b.occupied_count()) << phase;
    EXPECT_EQ(dump_records(a), dump_records(b)) << phase;
    for (std::uint64_t key : touched) EXPECT_EQ(a.lookup(key), b.lookup(key)) << phase << " key=" << key;
    for (std::uint64_t miss = 0; miss < 32u; ++miss) {
        std::uint64_t const key = (1ull << 60u) + miss * 129u;
        EXPECT_EQ(a.lookup(key), b.lookup(key)) << phase << " miss=" << key;
    }
}

} // namespace

static_assert(lkc::SwissGroupMatcher<lkc::ScalarGroupMatch>);
static_assert(lkc::SwissGroupMatcher<simd::Amd64Isa>);
static_assert(lkc::SwissGroupMatcher<simd::Aarch64Isa>);
static_assert(lkc::SwissGroupMatcher<simd::RiscVIsa>);
static_assert(lkc::SwissGroupMatcher<simd::PowerPcIsa>);
static_assert(lkc::SwissGroupMatcher<simd::ObservableIsa<simd::Amd64Isa>>);
static_assert(lkc::SwissGroupProbeTraversal<lkc::SwissGroupProbeTraversalOrgan, lkc::SwissGroupPoolStore<>>);
static_assert(
    lkc::SwissGroupProbeTraversal<lkc::SwissGroupProbeTraversalOrgan, lkc::SwissGroupPoolStore<>, simd::Amd64Isa>);
static_assert(std::same_as<typename lkc::SwissTableOrgan::matcher_type, lkc::ScalarGroupMatch>);
static_assert(std::same_as<typename lkc::SwissTableOrganSimd<simd::Amd64Isa>::matcher_type, simd::Amd64Isa>);

constexpr Ctrl16 kCtrlA{0x80u, 1u, 2u, 1u, 0xFEu, 0u, 1u, 0x7Fu, 0x80u, 1u, 0xFEu, 2u, 3u, 1u, 4u, 1u};
constexpr Ctrl16 kCtrlB{0u, 0xFEu, 0x80u, 0x7Fu, 0x7Fu, 0x55u, 0x55u, 0x80u, 9u, 8u, 7u, 6u, 5u, 4u, 3u, 2u};

static_assert(compile_mask<lkc::ScalarGroupMatch>(kCtrlA, 1u) == 0xA24Au);
static_assert(compile_mask<lkc::ScalarGroupMatch>(kCtrlA, lkc::SwissGroupPoolStore<>::kEmpty) == 0x0101u);
static_assert(compile_mask<lkc::ScalarGroupMatch>(kCtrlA, lkc::SwissGroupPoolStore<>::kDeleted) == 0x0410u);
static_assert(all_compile_masks_equal(kCtrlA, 1u));
static_assert(all_compile_masks_equal(kCtrlA, lkc::SwissGroupPoolStore<>::kEmpty));
static_assert(all_compile_masks_equal(kCtrlA, lkc::SwissGroupPoolStore<>::kDeleted));
static_assert(all_compile_masks_equal(kCtrlB, 0x7Fu));
static_assert(all_compile_masks_equal(kCtrlB, 0x55u));

TEST(ComdareAP7b3SwissIsaGroupMatch, RuntimeMasksMatchScalarBitIdentically) {
    std::vector<Ctrl16> const patterns{
        Ctrl16{0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u,
               0x80u},
        kCtrlA,
        kCtrlB,
        Ctrl16{0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u},
        Ctrl16{0xFEu, 0x80u, 0x01u, 0xFEu, 0x02u, 0x80u, 0x03u, 0xFEu, 0x04u, 0x80u, 0x05u, 0xFEu, 0x06u, 0x80u, 0x07u,
               0xFEu},
    };
    std::vector<std::uint8_t> const needles{
        0u, 1u, 2u, 0x7Fu, 0x55u, lkc::SwissGroupPoolStore<>::kEmpty, lkc::SwissGroupPoolStore<>::kDeleted};

    for (auto const& ctrl : patterns) {
        for (std::uint8_t needle : needles) {
            std::uint16_t const scalar = runtime_mask<lkc::ScalarGroupMatch>(ctrl, needle);
            EXPECT_EQ(runtime_mask<simd::Amd64Isa>(ctrl, needle), scalar) << "needle=" << static_cast<int>(needle);
            EXPECT_EQ(runtime_mask<simd::Aarch64Isa>(ctrl, needle), scalar) << "needle=" << static_cast<int>(needle);
            EXPECT_EQ(runtime_mask<simd::RiscVIsa>(ctrl, needle), scalar) << "needle=" << static_cast<int>(needle);
            EXPECT_EQ(runtime_mask<simd::PowerPcIsa>(ctrl, needle), scalar) << "needle=" << static_cast<int>(needle);
        }
    }
}

TEST(ComdareAP7b3SwissIsaGroupMatch, OptInSimdSwissTableMatchesDefaultScalarOrgan) {
    lkc::SwissTableOrgan                     scalar;
    lkc::SwissTableOrganSimd<simd::Amd64Isa> opt_in_simd;
    std::vector<std::uint64_t> const         keys = collision_heavy_keys();
    std::vector<std::uint64_t>               erased;

    for (std::size_t i = 0; i < keys.size(); ++i) {
        scalar.insert(keys[i], value_for(keys[i], i));
        opt_in_simd.insert(keys[i], value_for(keys[i], i));
    }
    expect_equivalent(scalar, opt_in_simd, keys, "after inserts");

    for (std::size_t i = 0; i < keys.size(); i += 7u) {
        scalar.insert(keys[i], value_for(keys[i], i + 10000u));
        opt_in_simd.insert(keys[i], value_for(keys[i], i + 10000u));
    }
    expect_equivalent(scalar, opt_in_simd, keys, "after updates");

    for (std::size_t i = 0; i < keys.size(); i += 5u) {
        bool const a = scalar.erase(keys[i]);
        bool const b = opt_in_simd.erase(keys[i]);
        EXPECT_EQ(a, b) << "erase key=" << keys[i];
        if (a) erased.push_back(keys[i]);
    }
    ASSERT_FALSE(erased.empty());
    EXPECT_EQ(scalar.erase(erased.front()), opt_in_simd.erase(erased.front()));
    expect_equivalent(scalar, opt_in_simd, keys, "after erases");

    for (std::size_t i = 0; i < erased.size(); i += 2u) {
        scalar.insert(erased[i], value_for(erased[i], i + 20000u));
        opt_in_simd.insert(erased[i], value_for(erased[i], i + 20000u));
    }
    expect_equivalent(scalar, opt_in_simd, keys, "after tombstone refill");

    scalar.clear();
    opt_in_simd.clear();
    expect_equivalent(scalar, opt_in_simd, keys, "after clear");
}

TEST(ComdareAP7b3SwissIsaGroupMatch, NeutralityGuardsStayIntact) {
    static_assert(std::is_trivially_copyable_v<anatomy::ComdareTierObserverSnapshot>);

    EXPECT_FALSE(::comdare::cache_engine::lookup::SwissTableSearchAlgo::supports_simd());
    EXPECT_EQ(COMDARE_ANATOMY_ABI_MAJOR, 6);
    EXPECT_EQ(sizeof(anatomy::ComdareTierObserverSnapshot), 1272u);
    EXPECT_EQ(anatomy::kTierObserverSnapshotVersionUnified, 7u);
}

#ifdef COMDARE_CE_ENABLE_STATISTICS
TEST(ComdareAP7b3SwissIsaGroupMatch, SwissStoreObserveIsaIsOptInAndNonEmpty) {
    lkc::SwissTableOrganSimd<simd::Amd64Isa> organ;
    for (std::uint64_t k = 0; k < 128u; ++k) organ.insert(k, value_for(k, k + 1u));

    simd::ObservableIsa<simd::Amd64Isa> isa;
    isa.reset();
    std::uint64_t const checksum = organ.store_observe_isa(isa);
    auto const          stats    = isa.statistics();

    EXPECT_NE(checksum, 0u);
    EXPECT_GT(stats.simd_calls, 0u);
    EXPECT_GT(stats.elements_processed, 0u);
#if defined(__x86_64__) || defined(_M_X64)
    EXPECT_EQ(stats.simd_iterations * 4u + stats.scalar_fallback_count, stats.elements_processed);
#endif
}
#endif
