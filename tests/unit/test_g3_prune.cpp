// test_g3_prune -- G5 (P-B, Ledger Section 65/66): verify-then-prune-Entscheidung + Loeschsicherheit.
//
// Testet den mockbaren KERN ohne mc: prune_verdict (die Verify-Matrix: remote fehlt/mismatch/size-
// mismatch/ok/keine lokale .version -> nie loeschen ausser bei bewiesenem Spiegel) und prunable_artifacts
// (die EINZIGEN loeschbaren Dateien = Binary + 2 Sidecars -- NIEMALS Messdaten). Der reale
// verify_remote_then_prune-mc-Weg ist ein duenner Wrapper um genau diese Funktionen (wie der Rest der
// posix_spawn-mc-Methoden nicht unit-getestet).

#include "artifact_transport/artifact_cache.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

namespace at = comdare::cache_engine::builder::artifact_transport;

namespace {

struct TempDir {
    std::filesystem::path path;
    TempDir() {
        static std::atomic<unsigned> counter{0};
        path = std::filesystem::temp_directory_path() /
               ("g5_prune_" + std::to_string(counter.fetch_add(1)) + "_" + std::to_string(::time(nullptr)));
        std::filesystem::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

void touch(std::filesystem::path const& p, std::string const& content) {
    std::ofstream f{p, std::ios::binary | std::ios::trunc};
    f << content;
}

} // namespace

// ---------------------------------------------------------------------------
// prune_verdict-Matrix: Loeschen NUR bei bewiesenem Remote-Spiegel; jeder Zweifel -> behalten.
// ---------------------------------------------------------------------------
TEST(G5Prune, VerdictNoLocalVersion) {
    auto v = at::prune_verdict("", std::optional<std::string>{"v"}, 10, std::optional<std::uintmax_t>{10});
    EXPECT_FALSE(v.prune);
    EXPECT_EQ(v.reason, "no_local_version");
}

TEST(G5Prune, VerdictRemoteVersionMissing) {
    auto v = at::prune_verdict("v", std::nullopt, 10, std::optional<std::uintmax_t>{10});
    EXPECT_FALSE(v.prune);
    EXPECT_EQ(v.reason, "remote_version_missing");
}

TEST(G5Prune, VerdictVersionMismatch) {
    auto v =
        at::prune_verdict("v-local", std::optional<std::string>{"v-remote"}, 10, std::optional<std::uintmax_t>{10});
    EXPECT_FALSE(v.prune);
    EXPECT_EQ(v.reason, "version_mismatch");
}

TEST(G5Prune, VerdictRemoteSizeUnknown) {
    auto v = at::prune_verdict("v", std::optional<std::string>{"v"}, 10, std::nullopt);
    EXPECT_FALSE(v.prune);
    EXPECT_EQ(v.reason, "remote_size_unknown");
}

TEST(G5Prune, VerdictSizeMismatch) {
    auto v = at::prune_verdict("v", std::optional<std::string>{"v"}, 10, std::optional<std::uintmax_t>{20});
    EXPECT_FALSE(v.prune);
    EXPECT_EQ(v.reason, "size_mismatch");
}

TEST(G5Prune, VerdictVerifiedPrunes) {
    auto v = at::prune_verdict("version=m3v2\nfoo=bar", std::optional<std::string>{"version=m3v2\nfoo=bar"}, 428032,
                               std::optional<std::uintmax_t>{428032});
    EXPECT_TRUE(v.prune);
    EXPECT_EQ(v.reason, "verified");
}

// ---------------------------------------------------------------------------
// prunable_artifacts = genau die 3 Artefakt-Dateien.
// ---------------------------------------------------------------------------
TEST(G5Prune, PrunableArtifactsAreExactlyThree) {
    std::filesystem::path const bin = "/tmp/some/stem";
    auto const                  ps  = at::prunable_artifacts(bin);
    ASSERT_EQ(ps.size(), 3u);
    EXPECT_EQ(ps[0], bin / "perm.dll");
    EXPECT_EQ(ps[1], bin / "perm.dll.version");
    EXPECT_EQ(ps[2], bin / "perm.dll.algos");
}

// ---------------------------------------------------------------------------
// LOESCHSICHERHEIT (Messdaten-nie-loeschen, HART): ein Prune loescht NUR die 3 Artefakte; result.csv,
// measure_out und das prune.log bleiben unangetastet.
// ---------------------------------------------------------------------------
TEST(G5Prune, PruneNeverTouchesMeasureData) {
    TempDir tmp;
    auto    bin = tmp.path;
    touch(bin / "perm.dll", "BINARY");
    touch(bin / "perm.dll.version", "version=m3v2");
    touch(bin / "perm.dll.algos", "search_algo=k_ary@1.0.0");
    touch(bin / "result.csv", "binary_id;setting;ns_per_op\n1;a;5\n"); // MESSDATEN
    touch(bin / "measure_out.csv", "col\n1\n");                        // MESSDATEN
    touch(bin / "perm.prune.log", "[PRUNE] stem=x action=behalten\n"); // Log bleibt

    // Prune = prunable_artifacts loeschen (== verify_remote_then_prune bei verdict.prune).
    std::error_code ec;
    for (auto const& p : at::prunable_artifacts(bin)) std::filesystem::remove(p, ec);

    // Die 3 Artefakte sind weg ...
    EXPECT_FALSE(std::filesystem::exists(bin / "perm.dll"));
    EXPECT_FALSE(std::filesystem::exists(bin / "perm.dll.version"));
    EXPECT_FALSE(std::filesystem::exists(bin / "perm.dll.algos"));
    // ... die Messdaten + das Log bleiben.
    EXPECT_TRUE(std::filesystem::exists(bin / "result.csv"));
    EXPECT_TRUE(std::filesystem::exists(bin / "measure_out.csv"));
    EXPECT_TRUE(std::filesystem::exists(bin / "perm.prune.log"));
}
