// test_184_dataset_loader_wiring — #184 (2026-07-07): der XML-`dataset_source`->Registry-loader_id->Loader->
// Operation-Pfad ist end-to-end HERMETISCH verdrahtet. Beweist LITERAL ueber ein KLEINES COMMITTETES Fixture
// (kein gitignored on-demand-Fetch): load_or_generate_ycsb(dataset_source="string_corpus", dataset_id=<pfad>)
//   (1) trifft den Registry-Loader (NICHT den YCSB-Fallback);
//   (2) liefert genau eine Read-Operation je Zeile;
//   (3) key_id == FNV-1a-64 des Zeilen-Strings, deterministisch (gleicher String -> gleicher Key);
//   (4) dataset_akte (dieselbe FNV-1a-Mechanik) ueber das Fixture ist stabil/reproduzierbar.
// Leerer dataset_source faellt sauber auf den YCSB-Generator zurueck (Kontrast-Beweis).

#include <comdare/measurement/dataset_loader/dataset_akte.hpp>
#include <comdare/measurement/dataset_loader/dataset_loader.hpp>
#include <comdare/measurement/dataset_loader/loaders/string_corpus_loader.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace dl = comdare::measurement::dataset_loader;
namespace wg = comdare::workload_generator;

namespace {

// Unabhaengige Standard-FNV-1a-64-Referenz (Offset-Basis 14695981039346656037, Prime 1099511628211) —
// muss das ergeben, was der Loader intern rechnet (Verdrahtungs-Kreuzcheck ueber die ECHTEN Keys).
[[nodiscard]] std::uint64_t fnv1a64(std::string const& s) {
    std::uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

[[nodiscard]] std::string fixture_path() {
    return std::string{COMDARE_TEST_FIXTURE_DIR} + "/dataset_loader/mini_corpus.txt";
}

} // namespace

TEST(Dataset184Wiring, StringCorpusSourceReachesLoaderNotYcsb) {
    wg::WorkloadConfig cfg{};
    cfg.num_operations = 999; // absichtlich != 5: ein YCSB-Fallback haette 999 Ops, der Loader genau 5
    auto const ops     = dl::load_or_generate_ycsb("string_corpus", fixture_path(), cfg, /*seed=*/7u);
    ASSERT_EQ(ops.size(), 5u) << "dataset_source-Verdrahtung traf nicht den Loader (YCSB-Fallback?)";
    for (auto const& op : ops) EXPECT_EQ(static_cast<int>(op.op), static_cast<int>(wg::OperationKind::Read));
}

TEST(Dataset184Wiring, KeyIdsAreDeterministicFnv1aOfLines) {
    wg::WorkloadConfig cfg{};
    auto const         ops = dl::load_or_generate_ycsb("string_corpus", fixture_path(), cfg, /*seed=*/0u);
    ASSERT_EQ(ops.size(), 5u);
    EXPECT_EQ(ops[0].key_id, fnv1a64("apple"));
    EXPECT_EQ(ops[1].key_id, fnv1a64("banana"));
    EXPECT_EQ(ops[2].key_id, fnv1a64("cherry"));
    EXPECT_EQ(ops[3].key_id, fnv1a64("apple"));
    EXPECT_EQ(ops[4].key_id, fnv1a64("date"));
    // Gleicher String -> gleicher Key (Option-A-Vertrag): Zeile 0 und 3 ("apple") sind identisch.
    EXPECT_EQ(ops[0].key_id, ops[3].key_id);
}

TEST(Dataset184Wiring, EmptySourceFallsBackToYcsbGenerator) {
    wg::WorkloadConfig cfg{};
    cfg.num_operations = 32;
    auto const ops     = dl::load_or_generate_ycsb("", "A", cfg, /*seed=*/1u);
    EXPECT_FALSE(ops.empty()); // YCSB-Generator, NICHT der Corpus-Loader
    EXPECT_NE(ops.size(), 5u);
}

TEST(Dataset184Wiring, AkteOverFixtureIsStable) {
    auto const a = dl::compute_dataset_akte("mini_corpus", fixture_path(), "none");
    auto const b = dl::compute_dataset_akte("mini_corpus", fixture_path(), "none");
    EXPECT_EQ(a.checksum, b.checksum);
    EXPECT_EQ(a.line_count, 5u); // 5 Zeilen im Fixture
    EXPECT_NE(a.checksum, 0u);
}
