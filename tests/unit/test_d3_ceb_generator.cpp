// D3 (L-77) — ceb_generator: zwei klar getrennte Pfade literal belegt.
// (1) generate_perm_source = DIAGNOSE-SHELL (Pfad-#defines + perm_run-Stub, KEIN ADHOC-Makro).
// (2) generate_all_real<Engine> = REALER BR-4-Anatomie-Emitter (COMDARE_DEFINE_ANATOMY_MODULE_ADHOC + Umbrella).
// Build: cl /std:c++latest /EHsc test_d3_ceb_generator.cpp /I libs/cache_engine  (ein Include-Root, kein Boost).

#include "builder/experiment_tree/ceb_generator.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace comdare::cache_engine::builder::experiment;

// Fake-Komposition mit den 17 von adhoc_macro_args erwarteten Member-Typen (type_name-fähig; Werte egal).
struct FakeComp {
    using search_algo = int; using cache_traversal = int; using mapping = int;
    using path_compression = int; using node_type = int; using memory_layout = int;
    using allocator = int; using prefetch = int; using concurrency = int;
    using serialization = int; using telemetry = int; using value_handle = int;
    using isa = int; using index_organization = int; using io_dispatch = int;
    using migration_policy = int; using filter = int;
};
struct FakeEngine {
    template <class F> static void for_each_composition_type(F&& f) { f.template operator()<FakeComp>(); }
};

static int g_fail = 0;
static void check(bool cond, std::string const& msg) {
    std::cout << (cond ? "  [OK]  " : "  [FAIL] ") << msg << "\n";
    if (!cond) ++g_fail;
}
static bool has(std::string const& hay, std::string const& needle) { return hay.find(needle) != std::string::npos; }
static std::string read_file(std::filesystem::path const& p) {
    std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str();
}

int main() {
    std::cout << "==== D3 (1) Diagnose-Shell (KEINE reale Anatomie) ====\n";
    std::string shell = generate_perm_source("search_algo=Array256/allocator=mimalloc");
    check(has(shell, "COMDARE_PERM_") && has(shell, "_run("), "Diagnose-Shell: Pfad-#defines + perm_<id>_run-Stub");
    check(!has(shell, "COMDARE_DEFINE_ANATOMY_MODULE_ADHOC"), "Diagnose-Shell: KEIN ADHOC-Makro (keine reale Anatomie)");
    check(has(shell, "0.0"), "perm_run misst nichts (Default 0.0)");

    std::cout << "\n==== D3 (2) generate_all_real<Engine> = REALER ADHOC-Anatomie-Pfad (BR-4) ====\n";
    auto dir = std::filesystem::temp_directory_path() / "comdare_d3_real";
    std::error_code ec; std::filesystem::remove_all(dir, ec);
    auto files = generate_all_real<FakeEngine>(dir);
    check(files.size() == 1, "generate_all_real emittiert 1 Modul (1 FakeComp)");
    std::string real = files.empty() ? std::string{} : read_file(files[0]);
    check(has(real, "COMDARE_DEFINE_ANATOMY_MODULE_ADHOC"), "reales Modul: COMDARE_DEFINE_ANATOMY_MODULE_ADHOC (reale Anatomie)");
    check(has(real, "all_axes_umbrella.hpp"), "reales Modul: #include all_axes_umbrella.hpp");
    check(std::filesystem::exists(dir / "anatomy_manifest.txt"), "anatomy_manifest.txt geschrieben (idx->Datei)");

    std::cout << "\n==== D3 Kontrast: die zwei Pfade sind disjunkt ====\n";
    check(!has(shell, "all_axes_umbrella") && !has(real, "_run("),
          "String-Diagnose-Pfad != reale-Anatomie-Pfad (Output disjunkt)");

    std::cout << "\n==== " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
