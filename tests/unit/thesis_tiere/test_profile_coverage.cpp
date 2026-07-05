#include "xml_config_parser/xml_config_parser.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifndef COMDARE_ALGORITHM_PROFILES_DIR
#error "COMDARE_ALGORITHM_PROFILES_DIR must point to libs/cache_engine/algorithm_profiles"
#endif

namespace {

namespace fs = std::filesystem;
namespace cx = ::comdare::builder::xml;

[[nodiscard]] std::string read_file(fs::path const& path) {
    std::ifstream      in{path};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

[[nodiscard]] std::string attr_value(std::string const& content, std::string const& attr) {
    std::regex  re{attr + R"ATTR(\s*=\s*"([^"]+)")ATTR"};
    std::smatch m;
    if (std::regex_search(content, m, re)) return m[1].str();
    return {};
}

[[nodiscard]] std::set<std::string> expected_refs(char prefix, int first, int last) {
    std::set<std::string> refs;
    for (int i = first; i <= last; ++i) {
        std::ostringstream os;
        os << prefix << std::setw(2) << std::setfill('0') << i;
        refs.insert(os.str());
    }
    return refs;
}

[[nodiscard]] std::vector<std::string> missing_refs(std::set<std::string> const& expected,
                                                    std::set<std::string> const& actual) {
    std::vector<std::string> missing;
    std::set_difference(expected.begin(), expected.end(), actual.begin(), actual.end(), std::back_inserter(missing));
    return missing;
}

[[nodiscard]] std::string join(std::vector<std::string> const& values) {
    std::ostringstream os;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) os << ", ";
        os << values[i];
    }
    return os.str();
}

[[nodiscard]] std::set<std::string> scan_allocator_family_refs(fs::path const& allocators_dir) {
    std::set<std::string> refs;
    if (!fs::is_directory(allocators_dir)) return refs;
    for (auto const& entry : fs::directory_iterator(allocators_dir)) {
        auto const& path = entry.path();
        if (path.extension() != ".xml" || path.filename().string().find(".profile.") == std::string::npos) continue;
        auto const family_ref = attr_value(read_file(path), "family_ref");
        if (!family_ref.empty()) refs.insert(family_ref);
    }
    return refs;
}

} // namespace

TEST(ProfileCoverage, SotaAndAllocatorReferencesAreComplete) {
    fs::path const profiles_dir{COMDARE_ALGORITHM_PROFILES_DIR};
    ASSERT_TRUE(fs::is_directory(profiles_dir)) << "profiles dir missing: " << profiles_dir.string();

    cx::XmlConfigParser parser;
    auto const          sota_profiles = parser.load_sota_profiles(profiles_dir / "sota");

    std::set<std::string> paper_refs;
    std::set<std::string> abstract_refs;
    for (auto const& profile : sota_profiles) {
        if (!profile.paper_ref.empty()) paper_refs.insert(profile.paper_ref);
        if (profile.pruefling_type == "abstract") abstract_refs.insert(profile.paper_ref);
    }

    auto const expected_papers   = expected_refs('P', 1, 33);
    auto const missing_papers    = missing_refs(expected_papers, paper_refs);
    auto const expected_abstract = std::set<std::string>{"P08", "P09", "P33"};

    ASSERT_TRUE(missing_papers.empty()) << "missing SOTA paper_ref(s): " << join(missing_papers);
    EXPECT_EQ(paper_refs.size(), 33u);
    EXPECT_EQ(abstract_refs, expected_abstract);

    auto const allocator_refs      = scan_allocator_family_refs(profiles_dir / "allocators");
    auto const expected_allocators = expected_refs('A', 1, 23);
    auto const missing_allocators  = missing_refs(expected_allocators, allocator_refs);

    ASSERT_TRUE(missing_allocators.empty()) << "missing allocator family_ref(s): " << join(missing_allocators);
    EXPECT_EQ(allocator_refs.size(), 23u);

    std::cout << "SOTA 33/33 (30 full + 3 abstract: P08/P09/P33) · Allokator 23/23\n";
}