// V41.F.6.1.R6.A — WorkloadGenerator Tests
//
// Beweist:
// 1. WorkloadConfig::is_valid validiert seed != 0, num_operations > 0,
//    key_max > key_min, op-mix non-negative + sum > 0
// 2. Konstruktor wirft std::invalid_argument bei invalid Config
// 3. Konstruktor normalisiert op-mix (Summe muss != 1.0 sein zur Konstruktion OK)
// 4. **Reproduzierbarkeit:** zwei Generators mit gleicher Config + N-mal next()
//    liefern IDENTISCHE Sequenz (User-Direktive Pflicht-Property)
// 5. reset() macht next() wieder vom Anfang
// 6. generate_all() liefert genau num_operations Ops
// 7. remaining()-Decrement nach next()
// 8. Op-Mix-Distribution stimmt grob (Chi-Square-locker)
// 9. Key-Range Einhaltung [key_min, key_max] fuer alle ops
// 10. Pre-built Profiles: insert_heavy/lookup_heavy/mixed_a/mixed_b
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §53
// @task #715 V41.F.6.1.R6.A

#include <gtest/gtest.h>

#include <builder/workload_driver/workload_config.hpp>
#include <builder/workload_driver/workload_generator.hpp>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace wd = ::comdare::cache_engine::builder::workload_driver;

// ─────────────────────────────────────────────────────────────────────────────
// §1 — WorkloadConfig::is_valid
// ─────────────────────────────────────────────────────────────────────────────

TEST(R6A_Config, DefaultConfigIsValid) {
    constexpr wd::WorkloadConfig cfg;
    static_assert(cfg.is_valid());
    SUCCEED();
}

TEST(R6A_Config, SeedZeroIsInvalid) {
    wd::WorkloadConfig cfg;
    cfg.seed = 0;
    EXPECT_FALSE(cfg.is_valid());
}

TEST(R6A_Config, ZeroOpsIsInvalid) {
    wd::WorkloadConfig cfg;
    cfg.num_operations = 0;
    EXPECT_FALSE(cfg.is_valid());
}

TEST(R6A_Config, EmptyKeyRangeIsInvalid) {
    wd::WorkloadConfig cfg;
    cfg.key_min = 10;
    cfg.key_max = 10;
    EXPECT_FALSE(cfg.is_valid());
    cfg.key_max = 9;
    EXPECT_FALSE(cfg.is_valid());
}

TEST(R6A_Config, NegativeOpMixIsInvalid) {
    wd::WorkloadConfig cfg;
    cfg.pct_lookup = -0.1;
    EXPECT_FALSE(cfg.is_valid());
}

TEST(R6A_Config, ZeroSumOpMixIsInvalid) {
    wd::WorkloadConfig cfg;
    cfg.pct_insert = 0.0;
    cfg.pct_lookup = 0.0;
    cfg.pct_erase  = 0.0;
    cfg.pct_clear  = 0.0;
    EXPECT_FALSE(cfg.is_valid());
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — Konstruktor wirft bei invalid Config
// ─────────────────────────────────────────────────────────────────────────────

TEST(R6A_Generator, ConstructorThrowsOnInvalidConfig) {
    wd::WorkloadConfig bad;
    bad.seed = 0;
    EXPECT_THROW({ wd::WorkloadGenerator g{bad}; }, std::invalid_argument);
}

TEST(R6A_Generator, ConstructorAcceptsValidConfig) {
    wd::WorkloadConfig good;
    EXPECT_NO_THROW({ wd::WorkloadGenerator g{good}; });
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — Op-Mix Normalisierung (Summe != 1.0)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R6A_Generator, ConstructorNormalizesOpMix) {
    wd::WorkloadConfig cfg;
    cfg.pct_insert = 2.0;
    cfg.pct_lookup = 8.0;  // Summe = 10.0
    cfg.pct_erase  = 0.0;
    cfg.pct_clear  = 0.0;
    wd::WorkloadGenerator g{cfg};
    auto const& normalized = g.config();
    EXPECT_DOUBLE_EQ(normalized.pct_insert, 0.2);
    EXPECT_DOUBLE_EQ(normalized.pct_lookup, 0.8);
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — KRITISCH Reproduzierbarkeit: identische Config → identische Sequenz
// ─────────────────────────────────────────────────────────────────────────────

TEST(R6A_Reproducibility, SameConfigProducesIdenticalSequence) {
    wd::WorkloadConfig cfg;
    cfg.num_operations = 500;

    wd::WorkloadGenerator g1{cfg};
    wd::WorkloadGenerator g2{cfg};

    for (std::size_t i = 0; i < cfg.num_operations; ++i) {
        auto const op1 = g1.next();
        auto const op2 = g2.next();
        ASSERT_EQ(op1, op2)
            << "Mismatch at index " << i
            << ": op1=(" << wd::op_kind_name(op1.kind)
            << ", k=" << op1.key << ", v=" << op1.value
            << ") op2=(" << wd::op_kind_name(op2.kind)
            << ", k=" << op2.key << ", v=" << op2.value << ")";
    }
}

TEST(R6A_Reproducibility, DifferentSeedsProduceDifferentSequences) {
    wd::WorkloadConfig cfg_a;  cfg_a.seed = 42;
    wd::WorkloadConfig cfg_b;  cfg_b.seed = 43;
    cfg_a.num_operations = cfg_b.num_operations = 100;

    wd::WorkloadGenerator g_a{cfg_a};
    wd::WorkloadGenerator g_b{cfg_b};

    bool any_diff = false;
    for (std::size_t i = 0; i < 100; ++i) {
        if (g_a.next() != g_b.next()) { any_diff = true; break; }
    }
    EXPECT_TRUE(any_diff) << "Verschiedene Seeds sollten unterschiedliche Sequenzen liefern";
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — reset() startet vom Anfang
// ─────────────────────────────────────────────────────────────────────────────

TEST(R6A_Reset, ResetMakesSequenceRestart) {
    wd::WorkloadConfig cfg;
    cfg.num_operations = 100;
    wd::WorkloadGenerator g{cfg};

    auto const first_run = g.generate_all();
    g.reset();
    EXPECT_EQ(g.generated_count(), 0u);
    EXPECT_EQ(g.remaining(), cfg.num_operations);

    auto const second_run = g.generate_all();
    ASSERT_EQ(first_run.size(), second_run.size());
    for (std::size_t i = 0; i < first_run.size(); ++i) {
        EXPECT_EQ(first_run[i], second_run[i]) << "Mismatch at index " << i;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — generate_all + remaining + generated_count
// ─────────────────────────────────────────────────────────────────────────────

TEST(R6A_BulkApi, GenerateAllReturnsCorrectNumberOfOps) {
    wd::WorkloadConfig cfg;
    cfg.num_operations = 12345;
    wd::WorkloadGenerator g{cfg};

    auto const ops = g.generate_all();
    EXPECT_EQ(ops.size(), cfg.num_operations);
    EXPECT_EQ(g.generated_count(), cfg.num_operations);
    EXPECT_EQ(g.remaining(), 0u);
}

TEST(R6A_BulkApi, RemainingDecrements) {
    wd::WorkloadConfig cfg;
    cfg.num_operations = 10;
    wd::WorkloadGenerator g{cfg};

    EXPECT_EQ(g.remaining(), 10u);
    g.next();
    EXPECT_EQ(g.remaining(), 9u);
    for (int i = 0; i < 9; ++i) g.next();
    EXPECT_EQ(g.remaining(), 0u);
    EXPECT_EQ(g.generated_count(), 10u);
}

// ─────────────────────────────────────────────────────────────────────────────
// §7 — Op-Mix-Distribution stimmt grob (statistische Verifikation)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R6A_Distribution, OpMixDistributionMatchesConfigApproximately) {
    wd::WorkloadConfig cfg;
    cfg.num_operations = 100'000;
    cfg.pct_insert = 0.5;
    cfg.pct_lookup = 0.4;
    cfg.pct_erase  = 0.1;
    cfg.pct_clear  = 0.0;

    wd::WorkloadGenerator g{cfg};
    std::array<std::size_t, 4> counts{};
    while (g.remaining() > 0) {
        auto const op = g.next();
        counts[static_cast<std::size_t>(op.kind)]++;
    }

    double const total = static_cast<double>(cfg.num_operations);
    double const pct_insert = static_cast<double>(counts[0]) / total;
    double const pct_lookup = static_cast<double>(counts[1]) / total;
    double const pct_erase  = static_cast<double>(counts[2]) / total;
    double const pct_clear  = static_cast<double>(counts[3]) / total;

    // Tolerance 2%: bei 100k Ops sollten Distribution-Werte recht stabil sein
    EXPECT_NEAR(pct_insert, 0.5, 0.02) << "insert ratio off";
    EXPECT_NEAR(pct_lookup, 0.4, 0.02) << "lookup ratio off";
    EXPECT_NEAR(pct_erase,  0.1, 0.02) << "erase ratio off";
    EXPECT_NEAR(pct_clear,  0.0, 0.005) << "clear ratio should be 0";
}

// ─────────────────────────────────────────────────────────────────────────────
// §8 — Key-Range Einhaltung
// ─────────────────────────────────────────────────────────────────────────────

TEST(R6A_KeyRange, AllKeysAreInConfiguredRange) {
    wd::WorkloadConfig cfg;
    cfg.num_operations = 10'000;
    cfg.key_min = 100;
    cfg.key_max = 200;

    wd::WorkloadGenerator g{cfg};
    while (g.remaining() > 0) {
        auto const op = g.next();
        EXPECT_GE(op.key, cfg.key_min);
        EXPECT_LE(op.key, cfg.key_max);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// §9 — Pre-built Workload-Profiles
// ─────────────────────────────────────────────────────────────────────────────

TEST(R6A_Profiles, InsertHeavyIs80_20) {
    constexpr auto cfg = wd::make_insert_heavy();
    static_assert(cfg.pct_insert == 0.80);
    static_assert(cfg.pct_lookup == 0.20);
    static_assert(cfg.name == std::string_view{"InsertHeavy_80_20"});
    SUCCEED();
}

TEST(R6A_Profiles, LookupHeavyIs95_5) {
    constexpr auto cfg = wd::make_lookup_heavy();
    static_assert(cfg.pct_insert == 0.05);
    static_assert(cfg.pct_lookup == 0.95);
    SUCCEED();
}

TEST(R6A_Profiles, MixedAIs50_50) {
    constexpr auto cfg = wd::make_mixed_a();
    static_assert(cfg.pct_insert == 0.50);
    static_assert(cfg.pct_lookup == 0.50);
    SUCCEED();
}

TEST(R6A_Profiles, MixedBIs5_95) {
    constexpr auto cfg = wd::make_mixed_b();
    static_assert(cfg.pct_insert == 0.05);
    static_assert(cfg.pct_lookup == 0.95);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §10 — op_kind_name fuer alle 4 Enums
// ─────────────────────────────────────────────────────────────────────────────

TEST(R6A_OpKindName, AllFourEnumsHaveStringName) {
    static_assert(wd::op_kind_name(wd::WorkloadOpKind::Insert) == std::string_view{"Insert"});
    static_assert(wd::op_kind_name(wd::WorkloadOpKind::Lookup) == std::string_view{"Lookup"});
    static_assert(wd::op_kind_name(wd::WorkloadOpKind::Erase)  == std::string_view{"Erase"});
    static_assert(wd::op_kind_name(wd::WorkloadOpKind::Clear)  == std::string_view{"Clear"});
    SUCCEED();
}
