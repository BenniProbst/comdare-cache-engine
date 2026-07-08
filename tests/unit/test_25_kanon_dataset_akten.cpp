// test_25_kanon_dataset_akten — #25 (2026-07-08): verifiziert die vier lokal im ce-Repo committeten
// 6er-Kanon-Datensatz-Akten (url/protein/tpcds-id/trec-terms) gegen die ECHTEN CoCo-trie-preprocessed
// Dateien via der offiziellen compute_dataset_akte-Mechanik (FNV-1a-64 ueber Datei-Bytes + \n-Zaehlung).
//
// Zweck: beweist, dass die in Code/test_data_xml/{url,protein,tpcds-id,trec-terms}.test_data.xml
// committeten <dataset_akte>-Werte (checksum/line_count) REPRODUZIERBAR aus den echten Dateien folgen
// (kein Hand-Hash, keine Attrappe). Da die CoCo-trie-dataset-Dateien direkt im ce-Repo committet sind
// (KEIN Submodul), laeuft dieser Test deterministisch in CI trotz GIT_SUBMODULE_STRATEGY=none.
//
// Nicht geprueft: xml + sosd_books_200M — deren Dateien liegen NICHT lokal vor (honest-0, dokumentierte
// Luecke in den jeweiligen .test_data.xml); eine Akte gegen unvollstaendige/fehlende Dateien waere eine
// Fabrikation und ist bewusst unterlassen.
#include <comdare/measurement/dataset_loader/dataset_akte.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace {

namespace dl = comdare::measurement::dataset_loader;

struct KanonCase {
    std::string   id;
    std::string   file_name; // relativ zu COMDARE_CE_DATASET_DIR
    std::uint64_t checksum;
    std::uint64_t line_count;
};

[[nodiscard]] std::filesystem::path dataset_dir() { return std::filesystem::path{COMDARE_CE_DATASET_DIR}; }

// Erwartungswerte = compute_dataset_akte gegen die committeten Dateien (identisch zu den XML-Akten).
TEST(Kanon25DatasetAkte, ChecksumsAndLineCountsMatchCommittedFiles) {
    std::vector<KanonCase> const cases{
        {"url", "it-2004.urls_no_suffixes_small", 0x888786d54c6fd026ULL, 5000},
        {"protein", "proteins.distinct_no_suffixes_small", 0xd7b59914049ee21aULL, 14411},
        {"tpcds-id", "customer_id_column.txt.sorted_no_suffixes_small", 0x1081e3892da8ced6ULL, 629661},
        {"trec-terms", "trec-text.terms_no_suffixes_small", 0xccee239019e815a1ULL, 855665},
    };

    for (auto const& c : cases) {
        auto const path = dataset_dir() / c.file_name;
        ASSERT_TRUE(std::filesystem::exists(path)) << "committed dataset file missing: " << path.string();

        auto const akte = dl::compute_dataset_akte(c.id, path, "coco-trie:no_suffixes_small");
        EXPECT_EQ(akte.checksum, c.checksum) << "checksum mismatch for dataset '" << c.id << "'";
        EXPECT_EQ(akte.line_count, c.line_count) << "line_count mismatch for dataset '" << c.id << "'";
        EXPECT_EQ(akte.preprocessing, "coco-trie:no_suffixes_small");
        EXPECT_EQ(akte.id, c.id);
    }
}

// Die Akte-Serialisierung bleibt das dokumentierte key=value-Manifest (DATASETS_SCHEMA.md).
TEST(Kanon25DatasetAkte, SerializationIsStableManifest) {
    auto const path = dataset_dir() / "it-2004.urls_no_suffixes_small";
    ASSERT_TRUE(std::filesystem::exists(path));
    auto const serialized =
        dl::serialize_dataset_akte(dl::compute_dataset_akte("url", path, "coco-trie:no_suffixes_small"));
    EXPECT_NE(serialized.find("id=url\n"), std::string::npos);
    EXPECT_NE(serialized.find("checksum=0x888786d54c6fd026\n"), std::string::npos);
    EXPECT_NE(serialized.find("line_count=5000\n"), std::string::npos);
    EXPECT_NE(serialized.find("preprocessing=coco-trie:no_suffixes_small\n"), std::string::npos);
}

} // namespace
