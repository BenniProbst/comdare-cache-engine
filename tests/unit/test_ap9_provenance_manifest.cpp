#include "builder/provenance_manifest.hpp"

#include "builder/measurement_snapshot.hpp"
#include <anatomy/observable_tier.hpp>
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace b       = ::comdare::cache_engine::builder;
namespace anatomy = ::comdare::cache_engine::anatomy;
namespace prov    = ::comdare::cache_engine::provenance;

namespace {

[[nodiscard]] bool contains_key(std::string const& manifest, std::string_view key) {
    std::string needle{key};
    needle += '=';
    return manifest.find(needle) != std::string::npos;
}

[[nodiscard]] bool is_sha_or_unknown(std::string_view value) {
    if (value == "unknown") { return true; }
    if (value.size() != 40u) { return false; }
    return std::all_of(value.begin(), value.end(),
                       [](char c) { return std::isxdigit(static_cast<unsigned char>(c)) != 0; });
}

[[nodiscard]] std::size_t count_cols(std::string const& csv_first_line) {
    return static_cast<std::size_t>(std::count(csv_first_line.begin(), csv_first_line.end(), ',') + 1);
}

[[nodiscard]] std::string first_line(std::string const& s) { return s.substr(0, s.find('\n')); }

} // namespace

TEST(AP9ProvenanceManifest, SerializesAllRequiredKeys) {
    auto const manifest = b::serialize_provenance_manifest();
    std::cout << "AP9 provenance manifest\n" << manifest;

    EXPECT_TRUE(contains_key(manifest, "compiler_id"));
    EXPECT_TRUE(contains_key(manifest, "compiler_version"));
    EXPECT_TRUE(contains_key(manifest, "cxx_flags"));
    EXPECT_TRUE(contains_key(manifest, "isa_built_for"));
    EXPECT_TRUE(contains_key(manifest, "isa_ran_on"));
    EXPECT_TRUE(contains_key(manifest, "allocators"));
    EXPECT_TRUE(contains_key(manifest, "git_sha_super"));
    EXPECT_TRUE(contains_key(manifest, "git_sha_cache_engine"));
    EXPECT_TRUE(contains_key(manifest, "git_sha_prt_art"));
    EXPECT_TRUE(contains_key(manifest, "git_sha_thesis"));
}

TEST(AP9ProvenanceManifest, GitShasAreRealOrUnknown) {
    EXPECT_TRUE(is_sha_or_unknown(prov::git_sha_super));
    EXPECT_TRUE(is_sha_or_unknown(prov::git_sha_cache_engine));
    EXPECT_TRUE(is_sha_or_unknown(prov::git_sha_prt_art));
    EXPECT_TRUE(is_sha_or_unknown(prov::git_sha_thesis));
}

TEST(AP9ProvenanceManifest, NeutralityGuardsStayIntact) {
    static_assert(std::is_trivially_copyable_v<b::ComdareMeasurementSnapshotV1>);
    static_assert(std::is_trivially_copyable_v<anatomy::ComdareTierObserverSnapshot>);

    EXPECT_EQ(COMDARE_ANATOMY_ABI_MAJOR, 5);
    EXPECT_EQ(sizeof(anatomy::ComdareTierObserverSnapshot), 1416u);
    EXPECT_EQ(anatomy::kTierObserverSnapshotVersionUnified, 5u);

    std::vector<b::ComdareMeasurementSnapshotV1> rows(1);
    std::vector<std::string>                     ids{"neutrality_guard"};
    std::vector<std::string>                     workloads{"ap9"};

    auto const full_csv = b::serialize_measurements_csv(rows, ids, workloads);
    EXPECT_EQ(count_cols(first_line(full_csv)), 25u);

    auto const pipeline_csv = b::serialize_measurements_pipeline16_csv(rows, ids, workloads);
    EXPECT_EQ(count_cols(first_line(pipeline_csv)), 16u);
}