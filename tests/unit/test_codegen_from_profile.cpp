// SPDX-License-Identifier: Apache-2.0
// REV 7.6 V10.4 — Tests fuer CodegenEngine::generate_module_from_profile (V9.3)
//
// Verifiziert:
//   1. Aus AlgorithmProfile wird korrekt eine .cpp-Datei generiert
//   2. CMakeLists pro Profil-Modul wird geschrieben
//   3. Profil-Metadaten (id, paper_ref, axes) erscheinen im generierten Output
//   4. Mehrere Profile mit unterschiedlichen Fingerprints kollidieren nicht
//   5. Generated Source enthaelt comdare_get_module_v1 + ABI-Bindings

#include "../../cache_engine/builder/codegen/codegen.hpp"
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace cg  = comdare::builder::codegen;
namespace xml = comdare::builder::xml;
namespace fs  = std::filesystem;

namespace {

class CodegenFromProfileFixture : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = ::comdare::test::user_tmp_dir() / "comdare_v10_4_codegen_test";
        fs::remove_all(tmp_dir_);
        fs::create_directories(tmp_dir_);

        opts_.output_root  = tmp_dir_ / "generated";
        opts_.comdare_root = tmp_dir_; // egal — Test scannt nur File-Inhalt
        engine_            = std::make_unique<cg::CodegenEngine>(opts_);
    }
    void TearDown() override {
        std::error_code ec;
        fs::remove_all(tmp_dir_, ec);
    }

    [[nodiscard]] static std::string read_file(fs::path const& p) {
        std::ifstream      f{p};
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    [[nodiscard]] static xml::AlgorithmProfile sample_profile_art() {
        xml::AlgorithmProfile p;
        p.id                  = "art";
        p.paper_ref           = "P01";
        p.key_types           = "std::uint64_t";
        p.value_types         = "std::string";
        p.axes["page"]        = "DENSEBYTE_ART256";
        p.axes["node"]        = "SPARSE_NODE4_ART";
        p.axes["traversal"]   = "STANDARD";
        p.axes["concurrency"] = "OPTIMISTIC_LOCK_COUPLING";
        p.axes["allocator"]   = "MIMALLOC";
        return p;
    }

    [[nodiscard]] static xml::AlgorithmProfile sample_profile_hot() {
        xml::AlgorithmProfile p;
        p.id                = "hot";
        p.paper_ref         = "P02";
        p.key_types         = "std::uint64_t";
        p.value_types       = "std::string";
        p.axes["page"]      = "HOT_MULTIBYTE";
        p.axes["node"]      = "HOT_BIT_NODE";
        p.axes["traversal"] = "HOT_PATH";
        return p;
    }

    fs::path                           tmp_dir_;
    cg::CodegenOptions                 opts_;
    std::unique_ptr<cg::CodegenEngine> engine_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: generate_module_from_profile schreibt die Source-Datei
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(CodegenFromProfileFixture, GeneratesSourceFile) {
    auto                    profile = sample_profile_art();
    constexpr std::uint64_t fp      = 0xC0FFEE0000000001ULL;
    engine_->generate_module_from_profile(profile, fp);

    auto src_path = opts_.output_root / "module_profile_art_c0ffee0000000001.cpp";
    EXPECT_TRUE(fs::exists(src_path)) << "Erwartet: " << src_path.string();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: generate_module_from_profile schreibt die CMakeLists-Datei
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(CodegenFromProfileFixture, GeneratesCMakeListsFile) {
    auto                    profile = sample_profile_art();
    constexpr std::uint64_t fp      = 0xC0FFEE0000000001ULL;
    engine_->generate_module_from_profile(profile, fp);

    auto cm_path = opts_.output_root / "module_profile_art_c0ffee0000000001_CMakeLists.txt";
    EXPECT_TRUE(fs::exists(cm_path)) << "Erwartet: " << cm_path.string();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: Generated Source enthaelt Profil-Metadaten + ABI-Bindings
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(CodegenFromProfileFixture, SourceContainsProfileMetadata) {
    auto                    profile = sample_profile_art();
    constexpr std::uint64_t fp      = 0xDEADBEEF00000002ULL;
    engine_->generate_module_from_profile(profile, fp);

    auto src_path = opts_.output_root / "module_profile_art_deadbeef00000002.cpp";
    auto content  = read_file(src_path);

    EXPECT_NE(content.find("Profile id   : art"), std::string::npos);
    EXPECT_NE(content.find("Paper-Ref    : P01"), std::string::npos);
    EXPECT_NE(content.find("page = DENSEBYTE_ART256"), std::string::npos);
    EXPECT_NE(content.find("node = SPARSE_NODE4_ART"), std::string::npos);
    EXPECT_NE(content.find("comdare_get_module_v1"), std::string::npos);
    EXPECT_NE(content.find("COMDARE_MODULE_EXPORT"), std::string::npos);
    EXPECT_NE(content.find("comdare_permutation_module_v1"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 4: Mehrere Profile generieren eindeutige Datei-Pfade
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(CodegenFromProfileFixture, MultipleProfilesGenerateDistinctFiles) {
    auto art_prof = sample_profile_art();
    auto hot_prof = sample_profile_hot();

    constexpr std::uint64_t fp_art = 0xC0FFEE0000000010ULL;
    constexpr std::uint64_t fp_hot = 0xC0FFEE0000000020ULL;

    engine_->generate_module_from_profile(art_prof, fp_art);
    engine_->generate_module_from_profile(hot_prof, fp_hot);

    EXPECT_TRUE(fs::exists(opts_.output_root / "module_profile_art_c0ffee0000000010.cpp"));
    EXPECT_TRUE(fs::exists(opts_.output_root / "module_profile_hot_c0ffee0000000020.cpp"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 5: CMakeLists referenziert das richtige Library-Target
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(CodegenFromProfileFixture, CMakeListsReferencesCorrectTarget) {
    auto                    profile = sample_profile_art();
    constexpr std::uint64_t fp      = 0xCAFE000000000003ULL;
    engine_->generate_module_from_profile(profile, fp);

    auto cm_path = opts_.output_root / "module_profile_art_cafe000000000003_CMakeLists.txt";
    auto content = read_file(cm_path);

    EXPECT_NE(content.find("comdare_perm_profile_art_cafe000000000003"), std::string::npos);
    EXPECT_NE(content.find("add_library"), std::string::npos);
    EXPECT_NE(content.find("SHARED"), std::string::npos);
    EXPECT_NE(content.find("cxx_std_23"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// REV 7.6 V17.3 — Tests fuer Template-Substitution (V17.1)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CodegenFromProfileFixture, TemplateSubstitution_NoOpWhenTemplateMissing) {
    // Profile-id "nonexistent_xyz" hat kein Template
    xml::AlgorithmProfile p;
    p.id          = "nonexistent_xyz";
    p.paper_ref   = "P99";
    p.key_types   = "int";
    p.value_types = "int";

    constexpr std::uint64_t fp = 0xC0FFEE9999999999ULL;
    engine_->generate_module_from_profile(p, fp);

    auto src_path = opts_.output_root / "module_profile_nonexistent_xyz_c0ffee9999999999.cpp";
    ASSERT_TRUE(fs::exists(src_path));
    auto content = read_file(src_path);

    // Skelett-Body ist enthalten (kein Template)
    EXPECT_NE(content.find("ProfileModule_v1"), std::string::npos);
    EXPECT_NE(content.find("Template     : no (skelett body)"), std::string::npos);
    // Kein Template-#include
    EXPECT_EQ(content.find("_body.hpp.template"), std::string::npos);
}

TEST_F(CodegenFromProfileFixture, TemplateSubstitution_IncludesTemplateWhenPresent) {
    // Tmp-Template anlegen unter <tmp>/cache_engine/builder/codegen/templates/test_id_body.hpp.template
    auto tpl_dir = tmp_dir_ / "cache_engine" / "builder" / "codegen" / "templates";
    fs::create_directories(tpl_dir);
    {
        std::ofstream f(tpl_dir / "test_id_body.hpp.template");
        f << "// Mock-Template V17.3\n"
          << "#pragma once\n"
          << "namespace comdare::cache_engine::builder::generated {\n"
          << "struct ProfileModuleBody { void run_workload(...) {} void pull_live_counters(...) {} };\n"
          << "}\n";
    }

    // CodegenEngine mit comdare_root auf das Tmp-Verzeichnis (sodass Template gefunden wird)
    cg::CodegenOptions opts2;
    opts2.output_root  = tmp_dir_ / "generated2";
    opts2.comdare_root = tmp_dir_;
    cg::CodegenEngine engine2{opts2};

    xml::AlgorithmProfile p;
    p.id                       = "test_id";
    p.paper_ref                = "PXX";
    constexpr std::uint64_t fp = 0xCAFE000000000017ULL;
    engine2.generate_module_from_profile(p, fp);

    auto src_path = opts2.output_root / "module_profile_test_id_cafe000000000017.cpp";
    ASSERT_TRUE(fs::exists(src_path));
    auto content = read_file(src_path);

    // Template wird inkludiert
    EXPECT_NE(content.find("test_id_body.hpp.template"), std::string::npos);
    EXPECT_NE(content.find("Template     : yes"), std::string::npos);
    // ABI-Glue verwendet TemplateBody alias
    EXPECT_NE(content.find("using TemplateBody = ::comdare::cache_engine::builder::generated::ProfileModuleBody;"),
              std::string::npos);
    // Kein Skelett-Body
    EXPECT_EQ(content.find("ProfileModule_v1"), std::string::npos);
}

// REV 7.6 V18.3 — Multi-Path-Lookup-Tests (cache-engine + prt-art Templates)
TEST_F(CodegenFromProfileFixture, MultiPath_PrtArtTemplateFoundWhenSotaMissing) {
    // Mock-Template NUR im prt_art_root, nicht in comdare_root
    auto prt_art_tpl_dir = tmp_dir_ / "prt_art" / "codegen" / "templates";
    fs::create_directories(prt_art_tpl_dir);
    {
        std::ofstream f(prt_art_tpl_dir / "prtart_body.hpp.template");
        f << "// Mock prtart Template V18.3\n"
          << "namespace comdare::cache_engine::builder::generated {\n"
          << "struct ProfileModuleBody { void run_workload(...) {} void pull_live_counters(...) {} };\n"
          << "}\n";
    }

    cg::CodegenOptions opts2;
    opts2.output_root  = tmp_dir_ / "generated_v18_3a";
    opts2.comdare_root = tmp_dir_; // hat KEIN prtart-Template hier
    opts2.prt_art_root = tmp_dir_; // hat prtart-Template
    cg::CodegenEngine engine2{opts2};

    xml::AlgorithmProfile p;
    p.id        = "prtart";
    p.paper_ref = "PRTART";
    engine2.generate_module_from_profile(p, 0xAB);

    auto src = read_file(opts2.output_root / "module_profile_prtart_ab.cpp");
    EXPECT_NE(src.find("Template     : yes"), std::string::npos);
    EXPECT_NE(src.find("prtart_body.hpp.template"), std::string::npos);
}

TEST_F(CodegenFromProfileFixture, MultiPath_SotaPriorityOverPrtArt) {
    auto sota_tpl_dir    = tmp_dir_ / "cache_engine" / "builder" / "codegen" / "templates";
    auto prt_art_tpl_dir = tmp_dir_ / "prt_art" / "codegen" / "templates";
    fs::create_directories(sota_tpl_dir);
    fs::create_directories(prt_art_tpl_dir);
    {
        std::ofstream f(sota_tpl_dir / "shared_id_body.hpp.template");
        f << "namespace comdare::cache_engine::builder::generated {\n"
          << "struct ProfileModuleBody { void run_workload(...) {} void pull_live_counters(...) {} };\n"
          << "}\n";
    }
    {
        std::ofstream f(prt_art_tpl_dir / "shared_id_body.hpp.template");
        f << "// pruefling fallback (sollte NICHT verwendet werden)\n";
    }

    cg::CodegenOptions opts2;
    opts2.output_root  = tmp_dir_ / "generated_v18_3b";
    opts2.comdare_root = tmp_dir_;
    opts2.prt_art_root = tmp_dir_;
    cg::CodegenEngine engine2{opts2};

    xml::AlgorithmProfile p;
    p.id = "shared_id";
    engine2.generate_module_from_profile(p, 0xCD);

    auto src = read_file(opts2.output_root / "module_profile_shared_id_cd.cpp");
    EXPECT_NE(src.find("/cache_engine/builder/codegen/templates/shared_id_body.hpp.template"), std::string::npos);
    EXPECT_EQ(src.find("/prt_art/codegen/templates/shared_id_body.hpp.template"), std::string::npos);
}

TEST_F(CodegenFromProfileFixture, MultiPath_NoOpWhenPrtArtRootEmpty) {
    auto prt_art_tpl_dir = tmp_dir_ / "prt_art" / "codegen" / "templates";
    fs::create_directories(prt_art_tpl_dir);
    {
        std::ofstream f(prt_art_tpl_dir / "alone_body.hpp.template");
        f << "irrelevant\n";
    }

    cg::CodegenOptions opts2;
    opts2.output_root  = tmp_dir_ / "generated_v18_3c";
    opts2.comdare_root = tmp_dir_;
    // opts2.prt_art_root bleibt leer
    cg::CodegenEngine engine2{opts2};

    xml::AlgorithmProfile p;
    p.id = "alone";
    engine2.generate_module_from_profile(p, 0xEF);

    auto src = read_file(opts2.output_root / "module_profile_alone_ef.cpp");
    EXPECT_NE(src.find("Template     : no (skelett body)"), std::string::npos);
}

TEST_F(CodegenFromProfileFixture, TemplateSubstitution_AbiGlueDelegatesToTemplate) {
    auto tpl_dir = tmp_dir_ / "cache_engine" / "builder" / "codegen" / "templates";
    fs::create_directories(tpl_dir);
    std::ofstream f(tpl_dir / "abc_body.hpp.template");
    f << "namespace comdare::cache_engine::builder::generated {\n"
      << "struct ProfileModuleBody { void run_workload(...) {} void pull_live_counters(...) {} };\n"
      << "}\n";
    f.close();

    cg::CodegenOptions opts2;
    opts2.output_root  = tmp_dir_ / "generated3";
    opts2.comdare_root = tmp_dir_;
    cg::CodegenEngine engine2{opts2};

    xml::AlgorithmProfile p;
    p.id        = "abc";
    p.paper_ref = "PXX";
    engine2.generate_module_from_profile(p, 0x42);

    auto src_path = opts2.output_root / "module_profile_abc_42.cpp";
    ASSERT_TRUE(fs::exists(src_path));
    auto content = read_file(src_path);

    EXPECT_NE(content.find("static_cast<TemplateBody*>(inst)->run_workload"), std::string::npos);
    EXPECT_NE(content.find("static_cast<TemplateBody*>(inst)->pull_live_counters"), std::string::npos);
    EXPECT_NE(content.find("delete static_cast<TemplateBody*>(inst)"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 6: Profile mit leerem axes-Map erzeugt trotzdem valide Output
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(CodegenFromProfileFixture, EmptyAxesProduceValidOutput) {
    xml::AlgorithmProfile minimal_profile;
    minimal_profile.id          = "minimal";
    minimal_profile.paper_ref   = "P00";
    minimal_profile.key_types   = "int";
    minimal_profile.value_types = "int";
    // axes-Map bleibt leer

    constexpr std::uint64_t fp = 0x0000000000001234ULL; // simple valid hex
    (void)fp;
    EXPECT_NO_THROW(engine_->generate_module_from_profile(minimal_profile, 0x1234ULL));

    auto src_path = opts_.output_root / "module_profile_minimal_1234.cpp";
    EXPECT_TRUE(fs::exists(src_path));
    auto content = read_file(src_path);
    EXPECT_NE(content.find("Profile id   : minimal"), std::string::npos);
    EXPECT_NE(content.find("comdare_get_module_v1"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// V19.4 — Test fuer expected_workload-Parsing aus Profile-XML
// Verifiziert: parse_profile liest <expected_workload>YCSB_X</expected_workload>
// und setzt AlgorithmProfile::expected_workload entsprechend.
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(CodegenFromProfileFixture, ParsesExpectedWorkloadTag) {
    auto profile_xml = tmp_dir_ / "test.profile.xml";
    {
        std::ofstream f{profile_xml};
        f << R"(<?xml version="1.0" encoding="UTF-8"?>
<comdare_algorithm_profile id="testw" paper_ref="PT01">
  <axes>
    <traversal>STANDARD</traversal>
  </axes>
  <key_value_signature>
    <key_types>std::uint64_t</key_types>
    <value_types>std::string</value_types>
  </key_value_signature>
  <expected_workload>YCSB_F</expected_workload>
</comdare_algorithm_profile>)";
    }
    xml::XmlConfigParser parser;
    auto                 p = parser.parse_profile(profile_xml);
    EXPECT_EQ(p.id, "testw");
    EXPECT_EQ(p.expected_workload, "YCSB_F");
}

// V19.4 — expected_workload optional: leer bei fehlendem Tag
TEST_F(CodegenFromProfileFixture, ExpectedWorkloadOptionalEmpty) {
    auto profile_xml = tmp_dir_ / "test_no_workload.profile.xml";
    {
        std::ofstream f{profile_xml};
        f << R"(<?xml version="1.0" encoding="UTF-8"?>
<comdare_algorithm_profile id="nowl" paper_ref="PT02">
  <axes><traversal>STANDARD</traversal></axes>
  <key_value_signature>
    <key_types>std::uint64_t</key_types>
    <value_types>std::string</value_types>
  </key_value_signature>
</comdare_algorithm_profile>)";
    }
    xml::XmlConfigParser parser;
    auto                 p = parser.parse_profile(profile_xml);
    EXPECT_EQ(p.id, "nowl");
    EXPECT_TRUE(p.expected_workload.empty());
}

// V29.A — allocator_override Tag (analog expected_workload)
TEST_F(CodegenFromProfileFixture, ParsesAllocatorOverrideTag) {
    auto profile_xml = tmp_dir_ / "test_alloc_override.profile.xml";
    {
        std::ofstream f{profile_xml};
        f << R"(<?xml version="1.0" encoding="UTF-8"?>
<comdare_algorithm_profile id="testao" paper_ref="PT03">
  <axes>
    <allocator>MIMALLOC</allocator>
  </axes>
  <key_value_signature>
    <key_types>std::uint64_t</key_types>
    <value_types>std::string</value_types>
  </key_value_signature>
  <expected_workload>YCSB_C</expected_workload>
  <allocator_override>jemalloc</allocator_override>
</comdare_algorithm_profile>)";
    }
    xml::XmlConfigParser parser;
    auto                 p = parser.parse_profile(profile_xml);
    EXPECT_EQ(p.id, "testao");
    EXPECT_EQ(p.expected_workload, "YCSB_C");
    EXPECT_EQ(p.allocator_override, "jemalloc");
    // axes/allocator bleibt MIMALLOC (Override nur per Builder anzuwenden)
    EXPECT_EQ(p.axes.at("allocator"), "MIMALLOC");
}

// V29.A — allocator_override optional: leer bei fehlendem Tag
TEST_F(CodegenFromProfileFixture, AllocatorOverrideOptionalEmpty) {
    auto profile_xml = tmp_dir_ / "test_no_alloc_override.profile.xml";
    {
        std::ofstream f{profile_xml};
        f << R"(<?xml version="1.0" encoding="UTF-8"?>
<comdare_algorithm_profile id="noao" paper_ref="PT04">
  <axes><allocator>MIMALLOC</allocator></axes>
  <key_value_signature>
    <key_types>std::uint64_t</key_types>
    <value_types>std::string</value_types>
  </key_value_signature>
</comdare_algorithm_profile>)";
    }
    xml::XmlConfigParser parser;
    auto                 p = parser.parse_profile(profile_xml);
    EXPECT_TRUE(p.allocator_override.empty());
}

} // anonymous namespace
