#include <comdare/measurement/dataset_loader/dataset_akte.hpp>
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen
#include <comdare/measurement/dataset_loader/loaders/string_corpus_loader.hpp>

#include "builder/measurement_snapshot.hpp"
#include <anatomy/observable_tier.hpp>
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace b          = ::comdare::cache_engine::builder;
namespace anatomy    = ::comdare::cache_engine::anatomy;
namespace dl         = ::comdare::measurement::dataset_loader;
namespace dl_loaders = ::comdare::measurement::dataset_loader::loaders;
namespace wg         = ::comdare::workload_generator;

namespace {

[[nodiscard]] bool contains_key(std::string const& manifest, std::string_view key) {
    std::string needle{key};
    needle += '=';
    return manifest.find(needle) != std::string::npos;
}

[[nodiscard]] std::size_t count_cols(std::string const& csv_first_line) {
    return static_cast<std::size_t>(std::count(csv_first_line.begin(), csv_first_line.end(), ',') + 1);
}

[[nodiscard]] std::string first_line(std::string const& s) { return s.substr(0, s.find('\n')); }

[[nodiscard]] std::filesystem::path write_temp_file(std::string_view name, std::string const& content) {
    auto const    path = ::comdare::test::user_tmp_dir() / std::string{name};
    std::ofstream out{path, std::ios::binary | std::ios::trunc};
    EXPECT_TRUE(static_cast<bool>(out));
    out << content;
    EXPECT_TRUE(static_cast<bool>(out));
    return path;
}

[[nodiscard]] std::vector<std::uint64_t> key_ids(std::vector<wg::Operation> const& ops) {
    std::vector<std::uint64_t> keys;
    keys.reserve(ops.size());
    for (auto const& op : ops) { keys.push_back(op.key_id); }
    return keys;
}

void print_keys(std::vector<std::uint64_t> const& keys) {
    for (std::size_t i = 0; i < keys.size(); ++i) {
        if (i != 0u) { std::cout << ','; }
        std::cout << keys[i];
    }
}

} // namespace

TEST(AP10DatasetAkte, StringCorpusLoaderMapsDeterministicallyToUint64ReadOps) {
    auto const corpus_path = write_temp_file("comdare_ap10_string_corpus.txt", "alpha\nbeta\ngamma\nalpha\ndelta\n");

    EXPECT_TRUE(dl_loaders::kStringCorpusLoaderRegistered);
    ASSERT_TRUE(dl::DatasetLoaderRegistry::instance().has("string_corpus"));

    auto const first  = dl::DatasetLoaderRegistry::instance().try_load("string_corpus", corpus_path.string(), 1);
    auto const second = dl::DatasetLoaderRegistry::instance().try_load("string_corpus", corpus_path.string(), 99);

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    ASSERT_EQ(first->size(), 5u);
    ASSERT_EQ(second->size(), 5u);

    for (auto const& op : *first) {
        EXPECT_EQ(op.op, wg::OperationKind::Read);
        EXPECT_EQ(op.scan_length, 0u);
    }

    auto const first_keys  = key_ids(*first);
    auto const second_keys = key_ids(*second);
    EXPECT_EQ(first_keys, second_keys);
    ASSERT_EQ(first_keys.size(), 5u);

    EXPECT_EQ(first_keys[0], first_keys[3]);
    EXPECT_NE(first_keys[0], first_keys[1]);
    EXPECT_NE(first_keys[1], first_keys[2]);
    EXPECT_NE(first_keys[2], first_keys[4]);

    std::cout << "AP10 loader determinism\n";
    std::cout << "same input with different seeds -> identical key_ids: true\n";
    std::cout << "key_ids=";
    print_keys(first_keys);
    std::cout << '\n';
}

TEST(AP10DatasetAkte, ComputesStableAkteAndSerializesKeys) {
    auto const corpus_path =
        write_temp_file("comdare_ap10_dataset_akte_corpus.txt", "alpha\nbeta\ngamma\nalpha\ndelta\n");

    auto const first  = dl::compute_dataset_akte("ap10_temp_corpus", corpus_path, "test:raw-lines");
    auto const second = dl::compute_dataset_akte("ap10_temp_corpus", corpus_path, "test:raw-lines");

    EXPECT_EQ(first.id, "ap10_temp_corpus");
    EXPECT_EQ(first.source_path, corpus_path.string());
    EXPECT_EQ(first.preprocessing, "test:raw-lines");
    EXPECT_EQ(first.line_count, 5u);
    EXPECT_EQ(first.checksum, second.checksum);
    EXPECT_EQ(first.line_count, second.line_count);

    auto const defaulted = dl::compute_dataset_akte("ap10_temp_corpus_default", corpus_path);
    EXPECT_EQ(defaulted.preprocessing, "none");

    auto const manifest = dl::serialize_dataset_akte(first);
    std::cout << "AP10 dataset akte example\n" << manifest;

    EXPECT_TRUE(contains_key(manifest, "id"));
    EXPECT_TRUE(contains_key(manifest, "source_path"));
    EXPECT_TRUE(contains_key(manifest, "checksum"));
    EXPECT_TRUE(contains_key(manifest, "line_count"));
    EXPECT_TRUE(contains_key(manifest, "preprocessing"));

    auto const manifest_path = ::comdare::test::user_tmp_dir() / "comdare_ap10_dataset_akte_manifest.txt";
    EXPECT_TRUE(dl::write_dataset_akte(manifest_path, first));
}

TEST(AP10DatasetAkte, NeutralityGuardsStayIntact) {
    static_assert(std::is_trivially_copyable_v<b::ComdareMeasurementSnapshotV1>);
    static_assert(std::is_trivially_copyable_v<anatomy::ComdareTierObserverSnapshot>);

    EXPECT_EQ(COMDARE_ANATOMY_ABI_MAJOR, 4);
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