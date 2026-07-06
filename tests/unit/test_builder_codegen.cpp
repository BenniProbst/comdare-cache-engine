// SPDX-License-Identifier: Apache-2.0
// Tests fuer Phase 7.2: CodegenEngine + Aggregator-CMakeLists.txt
//
// Diese Tests laufen ohne realen cmake-Subbuild — sie verifizieren nur, dass:
//   1. generate_module() die erwarteten Dateien schreibt
//   2. generate_aggregate_cmake() einen korrekten Aggregator-CMakeLists.txt
//      erzeugt, der alle Module include-t und das comdare_all_permutations-
//      Custom-Target referenziert
//
// Ein End-to-End-Subbuild-Test wuerde cmake auf dem Test-Host benoetigen +
// wird in einer separaten CI-Phase abgedeckt (verifiziert manuell mit
// example_configs/ → 54 DLLs).

#include "../../cache_engine/builder/codegen/codegen.hpp"
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace cg  = comdare::builder::codegen;
namespace xml = comdare::builder::xml;

namespace {

class CodegenFixture : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = ::comdare::test::user_tmp_dir() /
                   ("comdare_codegen_test_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()));
        std::filesystem::create_directories(tmp_dir_);

        cg::CodegenOptions opts;
        opts.output_root  = tmp_dir_ / "generated";
        opts.comdare_root = "/path/to/comdare";
        engine_           = std::make_unique<cg::CodegenEngine>(opts);
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(tmp_dir_, ec);
    }

    std::filesystem::path              tmp_dir_;
    std::unique_ptr<cg::CodegenEngine> engine_;
};

[[nodiscard]] std::string read_file(std::filesystem::path const& p) {
    std::ifstream     in{p};
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// generate_module: schreibt .cpp + _CMakeLists.txt
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CodegenFixture, GenerateModuleCreatesSourceFile) {
    xml::PermutationEntry ce{"ce_test", {}};
    xml::PermutationEntry sa{"sa_test", {}};
    xml::PermutationEntry al{"al_test", {}};
    std::uint64_t const   fp = 0xC0FFEE12345678ABULL;

    engine_->generate_module(ce, sa, al, fp);

    auto src_path = engine_->module_source_path(fp);
    EXPECT_TRUE(std::filesystem::exists(src_path)) << src_path.string();

    auto content = read_file(src_path);
    EXPECT_NE(content.find("comdare_get_module_v1"), std::string::npos);
    EXPECT_NE(content.find("c0ffee12345678ab"), std::string::npos);
    EXPECT_NE(content.find("ce_test"), std::string::npos);
}

TEST_F(CodegenFixture, GenerateModuleCreatesCMakeListsFile) {
    xml::PermutationEntry ce{"ce_a", {}};
    xml::PermutationEntry sa{"sa_a", {}};
    xml::PermutationEntry al{"al_a", {}};
    std::uint64_t const   fp = 0xDEADBEEFCAFEBABEULL;

    engine_->generate_module(ce, sa, al, fp);

    auto cm_path = engine_->module_cmake_path(fp);
    EXPECT_TRUE(std::filesystem::exists(cm_path));

    auto content = read_file(cm_path);
    EXPECT_NE(content.find("add_library(comdare_perm_deadbeefcafebabe SHARED"), std::string::npos);
    EXPECT_NE(content.find("target_compile_features(comdare_perm_deadbeefcafebabe PRIVATE cxx_std_23)"),
              std::string::npos);
    // Pfad muss Forward-Slashes verwenden, NICHT Backslashes
    EXPECT_EQ(content.find('\\'), std::string::npos) << "CMakeLists.txt enthaelt Backslashes (Escape-Probleme!)";
}

// ─────────────────────────────────────────────────────────────────────────────
// generate_aggregate_cmake: zentraler Aggregator
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CodegenFixture, AggregateCmakeEmptyListProducesValidFile) {
    engine_->generate_aggregate_cmake({});

    auto agg_path = engine_->aggregate_cmake_path();
    EXPECT_TRUE(std::filesystem::exists(agg_path));

    auto content = read_file(agg_path);
    EXPECT_NE(content.find("cmake_minimum_required"), std::string::npos);
    EXPECT_NE(content.find("project(comdare_permutations"), std::string::npos);
    EXPECT_NE(content.find("add_custom_target(comdare_all_permutations"), std::string::npos);
}

TEST_F(CodegenFixture, AggregateCmakeListsAllFingerprints) {
    std::array<std::uint64_t, 3> fps{0xAAAA, 0xBBBB, 0xCCCC};
    engine_->generate_aggregate_cmake(fps);

    auto content = read_file(engine_->aggregate_cmake_path());
    EXPECT_NE(content.find("module_aaaa_CMakeLists.txt"), std::string::npos);
    EXPECT_NE(content.find("module_bbbb_CMakeLists.txt"), std::string::npos);
    EXPECT_NE(content.find("module_cccc_CMakeLists.txt"), std::string::npos);
    EXPECT_NE(content.find("add_dependencies(comdare_all_permutations comdare_perm_aaaa)"), std::string::npos);
    EXPECT_NE(content.find("add_dependencies(comdare_all_permutations comdare_perm_bbbb)"), std::string::npos);
    EXPECT_NE(content.find("add_dependencies(comdare_all_permutations comdare_perm_cccc)"), std::string::npos);
}

TEST_F(CodegenFixture, AggregateCmakeUsesCxxStd23) {
    std::array<std::uint64_t, 1> fps{0x1234};
    engine_->generate_aggregate_cmake(fps);

    auto content = read_file(engine_->aggregate_cmake_path());
    EXPECT_NE(content.find("set(CMAKE_CXX_STANDARD 23)"), std::string::npos);
    EXPECT_NE(content.find("set(CMAKE_CXX_STANDARD_REQUIRED ON)"), std::string::npos);
}

TEST_F(CodegenFixture, AggregateCmakeUsesPositionIndependentCode) {
    std::array<std::uint64_t, 1> fps{0x5678};
    engine_->generate_aggregate_cmake(fps);
    auto content = read_file(engine_->aggregate_cmake_path());
    EXPECT_NE(content.find("CMAKE_POSITION_INDEPENDENT_CODE ON"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// End-to-End ohne Subbuild: 2 Perms generieren + aggregaten
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CodegenFixture, TwoPermsEndToEndGeneration) {
    xml::PermutationEntry ce{"ce", {}};
    xml::PermutationEntry sa{"sa", {}};
    xml::PermutationEntry al{"al", {}};

    std::array<std::uint64_t, 2> fps{0x111, 0x222};
    engine_->generate_module(ce, sa, al, fps[0]);
    engine_->generate_module(ce, sa, al, fps[1]);
    engine_->generate_aggregate_cmake(fps);

    EXPECT_TRUE(std::filesystem::exists(engine_->module_source_path(0x111)));
    EXPECT_TRUE(std::filesystem::exists(engine_->module_source_path(0x222)));
    EXPECT_TRUE(std::filesystem::exists(engine_->module_cmake_path(0x111)));
    EXPECT_TRUE(std::filesystem::exists(engine_->module_cmake_path(0x222)));
    EXPECT_TRUE(std::filesystem::exists(engine_->aggregate_cmake_path()));

    auto agg = read_file(engine_->aggregate_cmake_path());
    EXPECT_NE(agg.find("Aggregator fuer 2 Permutationen"), std::string::npos);
}
