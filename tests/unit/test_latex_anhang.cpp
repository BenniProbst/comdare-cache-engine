// SPDX-License-Identifier: Apache-2.0
// Tests fuer latex_anhang (Phase 8)

#include <gtest/gtest.h>
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// KEINE Handabschrift der latex_anhang-Schnittstelle mehr (09.08.2026): hier standen CsvRow
// (16 Felder) und fuenf Signaturen ein ZWEITES Mal. Bei einer STRUKTUR ist die Abschrift die
// teurere Sorte -- weicht ein Feld ab, meldet das kein Binder, sondern es werden stillschweigend
// falsche Zahlen gelesen. Jetzt liest der Test dieselbe Definition wie die Bibliothek.
#include "latex_anhang.hpp"

namespace {

class TempFile {
public:
    explicit TempFile(std::string const& suffix) {
        path_ = ::comdare::test::user_tmp_dir() / ("comdare_latex_test_" + std::to_string(rand()) + suffix);
    }
    ~TempFile() {
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }
    [[nodiscard]] std::filesystem::path const& path() const { return path_; }

private:
    std::filesystem::path path_;
};

[[nodiscard]] std::string read_file(std::filesystem::path const& p) {
    std::ifstream     in{p};
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void write_csv_with_header(std::filesystem::path const& p, std::vector<std::string> const& body_lines) {
    std::ofstream out{p};
    out << "permutation_id,fingerprint,succeeded,workload_used,op_count,total_cycles,"
        << "cache_misses_l1,cache_misses_l2,cache_misses_l3,dtlb_misses,"
        << "coherence_invalidations,energy_micro_joules,"
        << "bytes_allocated,bytes_in_use_peak,external_frag,internal_frag\n";
    for (auto const& l : body_lines) out << l << "\n";
}

} // namespace

TEST(LatexAnhang, SplitsSimpleLine) {
    auto cols = latex_anhang::split_csv_line("a,b,c");
    ASSERT_EQ(cols.size(), 3u);
    EXPECT_EQ(cols[0], "a");
    EXPECT_EQ(cols[1], "b");
    EXPECT_EQ(cols[2], "c");
}

TEST(LatexAnhang, EscapesUnderscoreAndAmpersand) {
    EXPECT_EQ(latex_anhang::escape_latex("ce_lockfree"), "ce\\_lockfree");
    EXPECT_EQ(latex_anhang::escape_latex("a&b"), "a\\&b");
    EXPECT_EQ(latex_anhang::escape_latex("100%"), "100\\%");
}

TEST(LatexAnhang, EscapesBraces) { EXPECT_EQ(latex_anhang::escape_latex("{ok}"), "\\{ok\\}"); }

TEST(LatexAnhang, ParseCsvWithThreeRows) {
    TempFile csv{".csv"};
    write_csv_with_header(csv.path(), {
                                          "ce_a:sa:al:tds,11111,1,ycsb_a,500,12345,100,50,10,5,2,0,0,0,0,0",
                                          "ce_b:sa:al:tds,22222,1,ycsb_b,500,99999,200,100,20,10,4,0,0,0,0,0",
                                          "ce_c:sa:al:tds,33333,0,ycsb_a,0,0,0,0,0,0,0,0,0,0,0,0",
                                      });

    std::vector<latex_anhang::CsvRow> rows;
    EXPECT_EQ(latex_anhang::parse_csv(csv.path().string(), rows), 0);
    ASSERT_EQ(rows.size(), 3u);
    EXPECT_EQ(rows[0].permutation_id, "ce_a:sa:al:tds");
    EXPECT_EQ(rows[0].total_cycles, 12345u);
    EXPECT_TRUE(rows[0].succeeded);
    EXPECT_FALSE(rows[2].succeeded);
}

TEST(LatexAnhang, ParseCsvMissingFileReturnsError) {
    std::vector<latex_anhang::CsvRow> rows;
    EXPECT_NE(latex_anhang::parse_csv("/nonexistent/foo.csv", rows), 0);
}

TEST(LatexAnhang, WriteLatexHasBooktabsStructure) {
    std::vector<latex_anhang::CsvRow> rows;
    rows.push_back({"ce_test:_x", 0xABC, true, "n/a", 100, 200, 30, 20, 10, 5, 0, 0, 0, 0, 0.0, 0.0});

    TempFile tex{".tex"};
    EXPECT_EQ(latex_anhang::write_latex(tex.path().string(), rows, "Test caption", "tab:test"), 0);

    auto content = read_file(tex.path());
    EXPECT_NE(content.find("\\begin{table}"), std::string::npos);
    EXPECT_NE(content.find("\\toprule"), std::string::npos);
    EXPECT_NE(content.find("\\midrule"), std::string::npos);
    EXPECT_NE(content.find("\\bottomrule"), std::string::npos);
    EXPECT_NE(content.find("\\end{table}"), std::string::npos);
    EXPECT_NE(content.find("ce\\_test:\\_x"), std::string::npos) << "Underscores must be escaped in LaTeX output";
    EXPECT_NE(content.find("\\caption{Test caption}"), std::string::npos);
    EXPECT_NE(content.find("\\label{tab:test}"), std::string::npos);
}

TEST(LatexAnhang, WriteLatexFullMetricsHasAllColumns) {
    std::vector<latex_anhang::CsvRow> rows;
    rows.push_back({"ce_a", 0xA, true, "n/a", 100, 200, 30, 20, 10, 7, 4, 0, 0, 0, 0.0, 0.0});

    TempFile tex{".tex"};
    EXPECT_EQ(latex_anhang::write_latex_full_metrics(tex.path().string(), rows, "Full", "tab:full"), 0);

    auto content = read_file(tex.path());
    EXPECT_NE(content.find("dTLB"), std::string::npos);
    EXPECT_NE(content.find("coherence"), std::string::npos);
    // Daten-Werte
    EXPECT_NE(content.find(" & 7 "), std::string::npos) << "dTLB-Wert sollte in der Tabelle erscheinen";
    EXPECT_NE(content.find(" & 4 "), std::string::npos) << "Coherence-Wert sollte in der Tabelle erscheinen";
}

TEST(LatexAnhang, WriteLatexSkipsFailedRows) {
    std::vector<latex_anhang::CsvRow> rows;
    rows.push_back({"ok_row", 0x1, true, "n/a", 100, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0});
    rows.push_back({"fail_row", 0x2, false, "n/a", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0});

    TempFile tex{".tex"};
    EXPECT_EQ(latex_anhang::write_latex(tex.path().string(), rows, "x", "tab:x"), 0);
    auto content = read_file(tex.path());
    EXPECT_NE(content.find("ok\\_row"), std::string::npos);
    EXPECT_EQ(content.find("fail\\_row"), std::string::npos) << "Failed rows should be skipped in summary table";
}

TEST(LatexAnhang, ParseCsvMalformedReturnsError) {
    TempFile csv{".csv"};
    {
        std::ofstream out{csv.path()};
        out << "permutation_id,fingerprint\n"; // Header zu kurz
        out << "ce_a,nicht_eine_zahl,1,500\n"; // Zeile zu kurz
    }
    std::vector<latex_anhang::CsvRow> rows;
    EXPECT_NE(latex_anhang::parse_csv(csv.path().string(), rows), 0);
}
