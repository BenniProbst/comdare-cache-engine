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

// ---------------------------------------------------------------------------
// FIX A (2026-07-26) -- REGRESSIONSTEST zur stillen Wirkungslosigkeit des Prune.
//
// BEFUND, den dieser Test festnagelt: push_tier_binary legt unter cache_key_prefix(<PER-PERM build_version>)/<stem>
// ab -- die Facade haengt je Permutation "+cxx=...+opt=...+ext=..." an. Der Prune-Aufrufer kannte diesen Suffix nicht
// und reichte den nackten Basis-Wert herein; der Praefix zeigte damit ins Leere, mc_remote_exists war IMMER false,
// jeder Stem endete auf "behalten" -- dauerhaft und unauffaellig, weil der Schritt nicht-fatal ist.
//
// Der mc-Weg selbst ist hier nicht testbar (posix_spawn). Getestet wird die Stelle, die falsch war: die Bildung des
// Remote-Praefix. prune_key_base ist genau diese Stelle und wird von verify_remote_then_prune benutzt.
// ---------------------------------------------------------------------------

namespace {
// Deterministischer Praefix: cache_key_prefix mischt COMDARE_MEASUREMENT_COMBO ein (measurement_combo_).
struct ScopedComboEnv {
    explicit ScopedComboEnv(char const* value) { ::setenv("COMDARE_MEASUREMENT_COMBO", value, 1); }
    ~ScopedComboEnv() { ::unsetenv("COMDARE_MEASUREMENT_COMBO"); }
};

// Die real vorkommende Form: Basis + der per-Perm-Suffix, den profile_run_entry.hpp anhaengt.
constexpr char const* kBaseVersion = "m3v2";
constexpr char const* kPermVersion = "m3v2+cxx=g++-16+opt=O3+ext=avx2";
} // namespace

TEST(G5PruneKeyBase, PrefixComesFromLocalVersionSidecarNotFromParameter) {
    ScopedComboEnv const combo{"wallclock"};
    auto const           ac = at::ArtifactCache::from_env();

    TempDir const               tmp;
    std::filesystem::path const bin = tmp.path / "perm_00042";
    std::filesystem::create_directories(bin);
    touch(bin / "perm.dll", "BINARY");
    touch(bin / "perm.dll.version", kPermVersion); // so stempelt write_version_sidecar real

    std::string const key = ac.prune_key_base(bin, kBaseVersion);

    // (1) POSITIV: der Praefix folgt der .version -- also dem Wert, unter dem push_tier_binary abgelegt hat.
    EXPECT_EQ(key, ac.cache_key_prefix(kPermVersion) + "/perm_00042");

    // (2) REGRESSION: er ist NICHT der aus dem Parameter gebildete. Faellt Fix A zurueck, wird genau das hier rot.
    EXPECT_NE(key, ac.cache_key_prefix(kBaseVersion) + "/perm_00042")
        << "Alt-Verhalten: Praefix aus dem uebergebenen Basis-Wert -> zeigt ins Leere -> Prune ewig no-op";

    // (3) Der Suffix ueberlebt vollstaendig -- er ist der Teil, der vorher fehlte.
    EXPECT_NE(key.find("+opt=O3"), std::string::npos);
    EXPECT_NE(key.find("+ext=avx2"), std::string::npos);
    EXPECT_NE(key.find("+cxx=g++-16"), std::string::npos);
}

TEST(G5PruneKeyBase, ParameterIsOnlyTheFallbackForStemsWithoutSidecar) {
    ScopedComboEnv const combo{"wallclock"};
    auto const           ac = at::ArtifactCache::from_env();

    TempDir const               tmp;
    std::filesystem::path const bin = tmp.path / "perm_00007";
    std::filesystem::create_directories(bin);
    touch(bin / "perm.dll", "BINARY"); // KEINE .version

    // Fehlt die Sidecar-Datei, greift der Parameter -- das Alt-Verhalten bleibt als Rueckfall erhalten.
    EXPECT_EQ(ac.prune_key_base(bin, kBaseVersion), ac.cache_key_prefix(kBaseVersion) + "/perm_00007");

    // Leere Sidecar-Datei zaehlt wie "fehlt" (ein leerer Praefix waere ein Objekt-Store-Wurzeltreffer).
    touch(bin / "perm.dll.version", "");
    EXPECT_EQ(ac.prune_key_base(bin, kBaseVersion), ac.cache_key_prefix(kBaseVersion) + "/perm_00007");
}

TEST(G5PruneKeyBase, SidecarContentIsUsedRawWithoutNormalisation) {
    ScopedComboEnv const combo{"wallclock"};
    auto const           ac = at::ArtifactCache::from_env();

    TempDir const               tmp;
    std::filesystem::path const bin = tmp.path / "perm_00013";
    std::filesystem::create_directories(bin);
    touch(bin / "perm.dll", "BINARY");
    touch(bin / "perm.dll.version", kPermVersion);

    // BEWUSST kein Trim/keine Normalisierung: write_version_sidecar schreibt den String roh und ohne Zeilenende,
    // push_tier_binary nutzt denselben String. Wuerde hier normalisiert, waere das eine zweite Wahrheit, die vom
    // Push abweichen kann. Der Praefix beginnt daher exakt mit dem Dateiinhalt.
    EXPECT_TRUE(ac.prune_key_base(bin, kBaseVersion).starts_with(std::string{kPermVersion} + "+ceb="))
        << "Praefix beginnt mit dem ROHEN Sidecar-Inhalt";
}
