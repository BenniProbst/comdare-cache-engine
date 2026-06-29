// SPDX-License-Identifier: Apache-2.0
// Tests fuer ycsb_cli (Phase C, 2026-05-13)

#include <gtest/gtest.h>

#include <comdare/workload_generator/workload_generator.hpp>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Re-declare ycsb_cli API (Lib hat YCSB_CLI_TEST_NO_MAIN=1, kein main())
namespace ycsb_cli {
namespace wg = comdare::workload_generator;
enum class OutputFormat : std::uint8_t { Binary = 0, Tsv = 1, Json = 2 };
struct CliConfig {
    wg::YcsbWorkload      workload      = wg::YcsbWorkload::C;
    std::uint64_t         num_keys      = 100000;
    std::uint64_t         num_ops       = 1000000;
    std::uint32_t         key_size      = 16;
    std::uint32_t         value_size    = 64;
    wg::KeyDistribution   key_dist      = wg::KeyDistribution::Zipfian;
    double                zipfian_theta = 0.99;
    std::uint64_t         seed          = 42;
    std::filesystem::path output        = "workload.bin";
    OutputFormat          format        = OutputFormat::Binary;
};
int parse_workload(std::string_view s, wg::YcsbWorkload& out) noexcept;
int parse_format(std::string_view s, OutputFormat& out) noexcept;
int parse_key_dist(std::string_view s, wg::KeyDistribution& out) noexcept;
int parse_args(int argc, char const* const* argv, CliConfig& cfg) noexcept;
int write_binary(std::filesystem::path const& p, std::span<wg::Operation const> ops) noexcept;
int write_tsv(std::filesystem::path const& p, std::span<wg::Operation const> ops) noexcept;
int write_json(std::filesystem::path const& p, std::span<wg::Operation const> ops) noexcept;
int generate_and_write(CliConfig const& cfg) noexcept;
} // namespace ycsb_cli

namespace wg = comdare::workload_generator;

// ─────────────────────────────────────────────────────────────────────────────
// Parser-Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(YcsbCli, ParseWorkloadAllLetters) {
    wg::YcsbWorkload w;
    EXPECT_EQ(ycsb_cli::parse_workload("A", w), 0);
    EXPECT_EQ(w, wg::YcsbWorkload::A);
    EXPECT_EQ(ycsb_cli::parse_workload("B", w), 0);
    EXPECT_EQ(w, wg::YcsbWorkload::B);
    EXPECT_EQ(ycsb_cli::parse_workload("c", w), 0);
    EXPECT_EQ(w, wg::YcsbWorkload::C);
    EXPECT_EQ(ycsb_cli::parse_workload("F", w), 0);
    EXPECT_EQ(w, wg::YcsbWorkload::F);
}

TEST(YcsbCli, ParseWorkloadInvalidReturnsError) {
    wg::YcsbWorkload w;
    EXPECT_EQ(ycsb_cli::parse_workload("X", w), 4);
    EXPECT_EQ(ycsb_cli::parse_workload("AB", w), 4);
    EXPECT_EQ(ycsb_cli::parse_workload("", w), 4);
}

TEST(YcsbCli, ParseFormatAllOptions) {
    ycsb_cli::OutputFormat f;
    EXPECT_EQ(ycsb_cli::parse_format("binary", f), 0);
    EXPECT_EQ(f, ycsb_cli::OutputFormat::Binary);
    EXPECT_EQ(ycsb_cli::parse_format("bin", f), 0);
    EXPECT_EQ(f, ycsb_cli::OutputFormat::Binary);
    EXPECT_EQ(ycsb_cli::parse_format("tsv", f), 0);
    EXPECT_EQ(f, ycsb_cli::OutputFormat::Tsv);
    EXPECT_EQ(ycsb_cli::parse_format("json", f), 0);
    EXPECT_EQ(f, ycsb_cli::OutputFormat::Json);
}

TEST(YcsbCli, ParseFormatInvalidReturnsError) {
    ycsb_cli::OutputFormat f;
    EXPECT_EQ(ycsb_cli::parse_format("xml", f), 4);
    EXPECT_EQ(ycsb_cli::parse_format("", f), 4);
}

TEST(YcsbCli, ParseKeyDistAllOptions) {
    wg::KeyDistribution d;
    EXPECT_EQ(ycsb_cli::parse_key_dist("uniform", d), 0);
    EXPECT_EQ(d, wg::KeyDistribution::Uniform);
    EXPECT_EQ(ycsb_cli::parse_key_dist("zipfian", d), 0);
    EXPECT_EQ(d, wg::KeyDistribution::Zipfian);
    EXPECT_EQ(ycsb_cli::parse_key_dist("sequential", d), 0);
    EXPECT_EQ(d, wg::KeyDistribution::Sequential);
    EXPECT_EQ(ycsb_cli::parse_key_dist("latest", d), 0);
    EXPECT_EQ(d, wg::KeyDistribution::Latest);
}

TEST(YcsbCli, ParseArgsHappyPath) {
    char const*         argv[] = {"ycsb_cli",     "--workload=A",     "--num-keys=1000",    "--num-ops=5000",
                                  "--key-size=8", "--value-size=128", "--key-dist=uniform", "--zipfian-theta=0.8",
                                  "--seed=123",   "--output=foo.bin", "--format=json"};
    ycsb_cli::CliConfig cfg;
    int                 status = ycsb_cli::parse_args(11, argv, cfg);
    EXPECT_EQ(status, 0);
    EXPECT_EQ(cfg.workload, wg::YcsbWorkload::A);
    EXPECT_EQ(cfg.num_keys, 1000u);
    EXPECT_EQ(cfg.num_ops, 5000u);
    EXPECT_EQ(cfg.key_size, 8u);
    EXPECT_EQ(cfg.value_size, 128u);
    EXPECT_EQ(cfg.key_dist, wg::KeyDistribution::Uniform);
    EXPECT_DOUBLE_EQ(cfg.zipfian_theta, 0.8);
    EXPECT_EQ(cfg.seed, 123u);
    EXPECT_EQ(cfg.output.string(), "foo.bin");
    EXPECT_EQ(cfg.format, ycsb_cli::OutputFormat::Json);
}

TEST(YcsbCli, ParseArgsHelpFlag) {
    char const*         argv[] = {"ycsb_cli", "--help"};
    ycsb_cli::CliConfig cfg;
    EXPECT_EQ(ycsb_cli::parse_args(2, argv, cfg), -1);
}

TEST(YcsbCli, ParseArgsUnknownFlagReturnsError) {
    char const*         argv[] = {"ycsb_cli", "--bogus=42"};
    ycsb_cli::CliConfig cfg;
    EXPECT_EQ(ycsb_cli::parse_args(2, argv, cfg), 4);
}

TEST(YcsbCli, ParseArgsMissingEqualsReturnsError) {
    char const*         argv[] = {"ycsb_cli", "not-a-flag"};
    ycsb_cli::CliConfig cfg;
    EXPECT_EQ(ycsb_cli::parse_args(2, argv, cfg), 4);
}

// ─────────────────────────────────────────────────────────────────────────────
// Writer-Tests (Roundtrip)
// ─────────────────────────────────────────────────────────────────────────────

namespace {

std::vector<wg::Operation> make_sample_ops() {
    return {
        {wg::OperationKind::Read, 42, 0},
        {wg::OperationKind::Insert, 43, 0},
        {wg::OperationKind::Update, 44, 0},
        {wg::OperationKind::Scan, 45, 10},
        {wg::OperationKind::ReadModifyWrite, 46, 0},
        {wg::OperationKind::Erase, 47, 0},
    };
}

} // namespace

TEST(YcsbCli, WriteBinaryRoundtrip) {
    auto ops = make_sample_ops();
    auto tmp = std::filesystem::temp_directory_path() / "ycsb_test.bin";
    EXPECT_EQ(ycsb_cli::write_binary(tmp, ops), 0);

    std::ifstream in{tmp, std::ios::binary};
    ASSERT_TRUE(in.is_open());
    std::uint32_t magic, version;
    std::uint64_t n;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    in.read(reinterpret_cast<char*>(&n), sizeof(n));
    EXPECT_EQ(magic, 0xC0FFEE01u);
    EXPECT_EQ(version, 1u);
    EXPECT_EQ(n, 6u);

    std::uint8_t  k;
    std::uint64_t key;
    std::uint32_t sl;
    in.read(reinterpret_cast<char*>(&k), sizeof(k));
    in.read(reinterpret_cast<char*>(&key), sizeof(key));
    in.read(reinterpret_cast<char*>(&sl), sizeof(sl));
    EXPECT_EQ(k, static_cast<std::uint8_t>(wg::OperationKind::Read));
    EXPECT_EQ(key, 42u);

    in.close();
    std::filesystem::remove(tmp);
}

TEST(YcsbCli, WriteTsvHasHeader) {
    auto ops = make_sample_ops();
    auto tmp = std::filesystem::temp_directory_path() / "ycsb_test.tsv";
    EXPECT_EQ(ycsb_cli::write_tsv(tmp, ops), 0);

    std::ifstream in{tmp};
    std::string   header;
    std::getline(in, header);
    EXPECT_EQ(header, "op\tkey\tscan_length");

    std::string first_row;
    std::getline(in, first_row);
    EXPECT_EQ(first_row, "read\t42\t0");

    in.close();
    std::filesystem::remove(tmp);
}

TEST(YcsbCli, WriteJsonContainsCount) {
    auto ops = make_sample_ops();
    auto tmp = std::filesystem::temp_directory_path() / "ycsb_test.json";
    EXPECT_EQ(ycsb_cli::write_json(tmp, ops), 0);

    std::ifstream in{tmp};
    std::string   content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("\"num_ops\": 6"), std::string::npos);
    EXPECT_NE(content.find("\"op\": \"read\""), std::string::npos);
    EXPECT_NE(content.find("\"key\": 42"), std::string::npos);

    in.close();
    std::filesystem::remove(tmp);
}

// ─────────────────────────────────────────────────────────────────────────────
// End-to-End: generate_and_write
// ─────────────────────────────────────────────────────────────────────────────

TEST(YcsbCli, GenerateAndWriteSmallWorkload) {
    ycsb_cli::CliConfig cfg;
    cfg.workload = wg::YcsbWorkload::C;
    cfg.num_keys = 100;
    cfg.num_ops  = 50;
    cfg.output   = std::filesystem::temp_directory_path() / "ycsb_e2e.bin";
    cfg.format   = ycsb_cli::OutputFormat::Binary;

    EXPECT_EQ(ycsb_cli::generate_and_write(cfg), 0);
    EXPECT_TRUE(std::filesystem::exists(cfg.output));
    EXPECT_GT(std::filesystem::file_size(cfg.output), 0u);

    std::filesystem::remove(cfg.output);
}

TEST(YcsbCli, GenerateAndWriteAllYcsbVariants) {
    for (auto w : {wg::YcsbWorkload::A, wg::YcsbWorkload::B, wg::YcsbWorkload::C, wg::YcsbWorkload::D,
                   wg::YcsbWorkload::E, wg::YcsbWorkload::F}) {
        ycsb_cli::CliConfig cfg;
        cfg.workload = w;
        cfg.num_keys = 50;
        cfg.num_ops  = 25;
        cfg.output   = std::filesystem::temp_directory_path() / "ycsb_variant.tsv";
        cfg.format   = ycsb_cli::OutputFormat::Tsv;
        EXPECT_EQ(ycsb_cli::generate_and_write(cfg), 0) << "Workload variant failed: " << static_cast<int>(w);
        std::filesystem::remove(cfg.output);
    }
}
