// V41.F.6.1.R5.F — anatomy_codegen_tool Library-Tests
//
// Beweist:
// 1. known_compositions() liefert 11 Eintraege (6 CE-Reimpl + 5 PaperBinding)
// 2. find_composition(known_name) liefert non-null mit korrekten Properties
// 3. find_composition(unknown_name) liefert nullptr
// 4. select_compositions("") -> alle 11
// 5. select_compositions("art,hot,wormhole") -> 3 selected
// 6. select_compositions mit unknown sammelt Unbekannte separat
// 7. parse_library_type("SHARED")/"STATIC" funktioniert
// 8. parse_library_type(invalid) liefert nullptr
// 9. write_cmake_snippet erzeugt parsierbaren CMake-Snippet
// 10. Snippet enthaelt selected count + library_type + Pipe-Format
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §48
// @task #710 V41.F.6.1.R5.F

#include <gtest/gtest.h>
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <builder/anatomy_codegen_tool/anatomy_codegen_tool.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace tool = ::comdare::cache_engine::builder::codegen_tool;

namespace {

[[nodiscard]] std::string read_file(std::filesystem::path const& p) {
    std::ifstream      in{p};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

[[nodiscard]] std::filesystem::path tmp_snippet_path(std::string_view stem) {
    auto const dir = ::comdare::test::user_tmp_dir() / "comdare_r5f_test";
    std::filesystem::create_directories(dir);
    return dir / (std::string{stem} + ".cmake");
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// §1 — known_compositions Tabelle
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5F_Tool, KnownCompositionsHasElevenEntries) {
    auto const all = tool::known_compositions();
    EXPECT_EQ(all.size(), 11u);
}

TEST(R5F_Tool, KnownCompositionsContainsExpectedNames) {
    auto const all = tool::known_compositions();
    bool       art = false, hot = false, art_pb = false, surf_pb = false;
    for (auto const& c : all) {
        if (c.short_name == "art") art = true;
        if (c.short_name == "hot") hot = true;
        if (c.short_name == "art_pb") art_pb = true;
        if (c.short_name == "surf_pb") surf_pb = true;
    }
    EXPECT_TRUE(art);
    EXPECT_TRUE(hot);
    EXPECT_TRUE(art_pb);
    EXPECT_TRUE(surf_pb);
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — find_composition Lookup
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5F_Tool, FindCompositionKnownReturnsDescriptor) {
    auto const* art = tool::find_composition("art");
    ASSERT_NE(art, nullptr);
    EXPECT_EQ(art->short_name, std::string_view{"art"});
    EXPECT_EQ(art->cpp_type_name, std::string_view{"::comdare::cache_engine::compositions::ArtComposition"});
    EXPECT_EQ(art->header_include, std::string_view{"compositions/art_reference.hpp"});
}

TEST(R5F_Tool, FindCompositionUnknownReturnsNullptr) {
    EXPECT_EQ(tool::find_composition("does_not_exist"), nullptr);
    EXPECT_EQ(tool::find_composition(""), nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — select_compositions
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5F_Tool, SelectCompositionsEmptyReturnsAll) {
    auto const selected = tool::select_compositions("");
    EXPECT_EQ(selected.size(), 11u);
}

TEST(R5F_Tool, SelectCompositionsCsvReturnsSelected) {
    auto const selected = tool::select_compositions("art,hot,wormhole");
    ASSERT_EQ(selected.size(), 3u);
    EXPECT_EQ(selected[0]->short_name, std::string_view{"art"});
    EXPECT_EQ(selected[1]->short_name, std::string_view{"hot"});
    EXPECT_EQ(selected[2]->short_name, std::string_view{"wormhole"});
}

TEST(R5F_Tool, SelectCompositionsCollectsUnknown) {
    std::vector<std::string> unknown;
    auto const               selected = tool::select_compositions("art,bogus,hot,phantom", &unknown);
    EXPECT_EQ(selected.size(), 2u); // art + hot
    ASSERT_EQ(unknown.size(), 2u);
    EXPECT_EQ(unknown[0], "bogus");
    EXPECT_EQ(unknown[1], "phantom");
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — parse_library_type
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5F_Tool, ParseLibraryTypeShared) {
    auto const* t = tool::parse_library_type("SHARED");
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(*t, tool::LibraryType::Shared);
    EXPECT_EQ(tool::library_type_name(*t), std::string_view{"SHARED"});
}

TEST(R5F_Tool, ParseLibraryTypeStatic) {
    auto const* t = tool::parse_library_type("STATIC");
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(*t, tool::LibraryType::Static);
    EXPECT_EQ(tool::library_type_name(*t), std::string_view{"STATIC"});
}

TEST(R5F_Tool, ParseLibraryTypeInvalidReturnsNullptr) {
    EXPECT_EQ(tool::parse_library_type("shared"), nullptr); // case-sensitive
    EXPECT_EQ(tool::parse_library_type("MODULE"), nullptr);
    EXPECT_EQ(tool::parse_library_type(""), nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — write_cmake_snippet erzeugt parsierbaren Output
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5F_Tool, WriteCmakeSnippetContainsLibraryType) {
    auto const path     = tmp_snippet_path("library_type_check");
    auto const selected = tool::select_compositions("art");
    ASSERT_TRUE(tool::write_cmake_snippet(path, selected, tool::LibraryType::Shared));

    auto const content = read_file(path);
    EXPECT_NE(content.find("set(COMDARE_PERMUTATION_LIBRARY_TYPE \"SHARED\")"), std::string::npos)
        << "Snippet sollte LIBRARY_TYPE-Variable enthalten:\n"
        << content;
}

TEST(R5F_Tool, WriteCmakeSnippetContainsCompositionsList) {
    auto const path     = tmp_snippet_path("compositions_check");
    auto const selected = tool::select_compositions("art,hot");
    ASSERT_TRUE(tool::write_cmake_snippet(path, selected, tool::LibraryType::Shared));

    auto const content = read_file(path);
    EXPECT_NE(content.find("set(COMDARE_PERMUTATION_COMPOSITIONS"), std::string::npos);
    EXPECT_NE(content.find("ArtComposition|compositions/art_reference.hpp"), std::string::npos);
    EXPECT_NE(content.find("HotComposition|compositions/hot_reference.hpp"), std::string::npos);
}

TEST(R5F_Tool, WriteCmakeSnippetStaticLibraryTypeOption) {
    auto const path     = tmp_snippet_path("static_check");
    auto const selected = tool::select_compositions(""); // alle 11
    ASSERT_TRUE(tool::write_cmake_snippet(path, selected, tool::LibraryType::Static));

    auto const content = read_file(path);
    EXPECT_NE(content.find("set(COMDARE_PERMUTATION_LIBRARY_TYPE \"STATIC\")"), std::string::npos);
    // Alle 11 Compositions sollten enthalten sein
    EXPECT_NE(content.find("ArtComposition"), std::string::npos);
    EXPECT_NE(content.find("MasstreeComposition"), std::string::npos);
    EXPECT_NE(content.find("SurfPaperBindingComposition"), std::string::npos);
}

TEST(R5F_Tool, WriteCmakeSnippetCreatesParentDirectory) {
    auto const      tmp = ::comdare::test::user_tmp_dir() / "comdare_r5f_test" / "deep" / "sub" / "dir";
    std::error_code ec;
    std::filesystem::remove_all(tmp.parent_path(), ec); // sicherstellen dass nicht vorhanden

    auto const path     = tmp / "snippet.cmake";
    auto const selected = tool::select_compositions("art");
    EXPECT_TRUE(tool::write_cmake_snippet(path, selected, tool::LibraryType::Shared));
    EXPECT_TRUE(std::filesystem::exists(path));
}
