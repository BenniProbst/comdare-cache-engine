// test_kf8_ceb_generator — KF-8 (2026-06-02)
// CebGenerator: erzeugt je Binary-Pfad (Experiment-B+-Baum) ein perm_<id>.cpp (C++23, beliebige Achsentiefe).
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> test_kf8_ceb_generator.cpp

#include "builder/experiment_tree/ceb_generator.hpp"
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen
#include "builder/experiment_tree/experiment_tree.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace ex = comdare::cache_engine::builder::experiment;

static int g_fail = 0;
void       check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
static bool contains(std::string const& h, std::string const& n) { return h.find(n) != std::string::npos; }

int main() {
    // generate_perm_source: Struktur prüfen
    std::string const bid = "traversal=ART/node=N4/node.cl_line=64";
    std::string       src = ex::generate_perm_source(bid);
    check_true("axis-#define traversal", contains(src, "#define COMDARE_PERM_TRAVERSAL_IS_ART 1"));
    check_true("axis-#define node", contains(src, "#define COMDARE_PERM_NODE_IS_N4 1"));
    check_true("axis-#define node.cl_line (sanitisiert)", contains(src, "#define COMDARE_PERM_NODE_CL_LINE_IS_64 1"));
    check_true("COMDARE_PERM_PATH = Binary-Pfad", contains(src, "#define COMDARE_PERM_PATH \"" + bid + "\""));
    check_true("Descriptor-Entry comdare_perm_descriptor", contains(src, "comdare_perm_descriptor"));
    check_true("run-Symbol", contains(src, "_run(unsigned long n_ops, double* out_micros_per_op)"));

    // Baum (2 Binaries) -> generate_all
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build({
        ex::AxisLevel{"traversal", {"ART"}, true, ""},
        ex::AxisLevel{"node", {"N4", "N16"}, true, ""},
        ex::AxisLevel{"node.cl_line", {"64"}, true, ""},
        ex::AxisLevel{"concurrency", {"1", "2"}, false, "thread_count"}, // dynamisch -> keine neuen Binaries
    });
    check_eq("binary_count (1x2x1)", tree.binary_count(), std::size_t{2});

    std::filesystem::path const dir = ::comdare::test::user_tmp_dir() / "comdare_kf8_gen";
    std::error_code             ec;
    std::filesystem::remove_all(dir, ec);
    std::size_t n = ex::generate_all(tree, dir);
    check_eq("generate_all: erzeugte Sources", n, std::size_t{2});
    std::size_t cpp_files = 0;
    for (auto const& e : std::filesystem::directory_iterator(dir))
        if (e.path().extension() == ".cpp") ++cpp_files;
    check_eq("perm_*.cpp Dateien auf Disk", cpp_files, std::size_t{2});
    check_true("Manifest vorhanden", std::filesystem::exists(dir / "perm_manifest.txt"));

    // Ein generiertes Source zur externen Compile-Prüfung wegschreiben
    std::filesystem::path const sample = ::comdare::test::user_tmp_dir() / "kf8_sample.cpp";
    {
        std::ofstream f{sample};
        f << src;
    }
    check_true("Sample-Source geschrieben (fuer cl-Compile-Check)", std::filesystem::exists(sample));

    std::cout << "\n==== KF-8 CebGenerator: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
