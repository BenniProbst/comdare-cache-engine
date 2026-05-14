// SPDX-License-Identifier: Apache-2.0
// REV 7.6 V14.2 — Tests fuer V13.2 + V13.3 ExperimentDriver-API
//
// Verifiziert die opt-in-Optionen aus V8.7 (jetzt operational in V13.2/V13.3):
//   - enable_runtime_codegen: phase3_hot_compile_missing
//   - enable_functional_tests: phase4b_functional_tests
// + V10.6 MessreihenMode-Defaults
//
// Diese Tests laufen ohne realen cmake-Subbuild oder geladene Module.
// E2E-Verifikation steht in messung_driver-Lauf an (V14.6 zukuenftig).

#include "../../cache_engine/builder/experiment_driver/experiment_driver.hpp"

#include <gtest/gtest.h>

#include <span>
#include <vector>

namespace cb = comdare::builder;

namespace {

class ExperimentDriverV13Fixture : public ::testing::Test {
protected:
    cb::ExperimentDriverOptions opts;
    void SetUp() override {
        // Defaults
        opts.config_dir   = std::filesystem::temp_directory_path() / "cev13_cfg";
        opts.output_dir   = std::filesystem::temp_directory_path() / "cev13_out";
        opts.comdare_root = std::filesystem::temp_directory_path() / "cev13_root";
        opts.verbose      = false;
        std::filesystem::create_directories(opts.output_dir);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Defaults-Sanity (V13.2 + V13.3 + V10.6)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExperimentDriverV13Fixture, EnableRuntimeCodegen_DefaultIsFalse) {
    EXPECT_FALSE(opts.enable_runtime_codegen);
}

TEST_F(ExperimentDriverV13Fixture, EnableFunctionalTests_DefaultIsFalse) {
    EXPECT_FALSE(opts.enable_functional_tests);
}

TEST_F(ExperimentDriverV13Fixture, MessreihenMode_DefaultIsFull) {
    EXPECT_EQ(opts.messreihen_mode,
              cb::ExperimentDriverOptions::MessreihenMode::Full);
}

TEST_F(ExperimentDriverV13Fixture, SotaProfileFilter_DefaultEmpty) {
    EXPECT_TRUE(opts.sota_profile_filter.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// V13.2 phase3_hot_compile_missing
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExperimentDriverV13Fixture, Phase3HotCompile_NoOpWhenFlagOff) {
    opts.enable_runtime_codegen = false;
    cb::ExperimentDriver driver{opts};
    std::vector<std::uint64_t> empty_fps;
    int rc = driver.phase3_hot_compile_missing(empty_fps);
    EXPECT_EQ(rc, cb::status_ok);
}

TEST_F(ExperimentDriverV13Fixture, Phase3HotCompile_OkWithEmptyFingerprintsEvenWhenEnabled) {
    opts.enable_runtime_codegen = true;
    cb::ExperimentDriver driver{opts};
    std::vector<std::uint64_t> empty_fps;
    int rc = driver.phase3_hot_compile_missing(empty_fps);
    EXPECT_EQ(rc, cb::status_ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// V13.3 phase4b_functional_tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExperimentDriverV13Fixture, Phase4bFunctionalTests_NoOpWhenFlagOff) {
    opts.enable_functional_tests = false;
    cb::ExperimentDriver driver{opts};
    std::vector<cb::loader::ModuleHandle> empty_handles;
    int rc = driver.phase4b_functional_tests(empty_handles);
    EXPECT_EQ(rc, cb::status_ok);
}

TEST_F(ExperimentDriverV13Fixture, Phase4bFunctionalTests_OkWithEmptyHandlesEvenWhenEnabled) {
    opts.enable_functional_tests = true;
    cb::ExperimentDriver driver{opts};
    std::vector<cb::loader::ModuleHandle> empty_handles;
    int rc = driver.phase4b_functional_tests(empty_handles);
    EXPECT_EQ(rc, cb::status_ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// V10.6 MessreihenMode + sota_profile_filter setzbar
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ExperimentDriverV13Fixture, MessreihenMode_DefinedFilterSettable) {
    opts.messreihen_mode = cb::ExperimentDriverOptions::MessreihenMode::Defined;
    opts.sota_profile_filter = {"art", "hot", "masstree"};
    EXPECT_EQ(opts.messreihen_mode,
              cb::ExperimentDriverOptions::MessreihenMode::Defined);
    EXPECT_EQ(opts.sota_profile_filter.size(), 3u);
    EXPECT_EQ(opts.sota_profile_filter[0], "art");
}

TEST_F(ExperimentDriverV13Fixture, BothFlagsCanBeOptInIndependently) {
    opts.enable_runtime_codegen  = true;
    opts.enable_functional_tests = true;
    EXPECT_TRUE(opts.enable_runtime_codegen);
    EXPECT_TRUE(opts.enable_functional_tests);
}

}  // anonymous namespace
