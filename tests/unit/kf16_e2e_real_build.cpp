// kf16_e2e_real_build — KF-16 (2026-06-02) REALER E2E-Build
// Treibt den BuildOrchestrator mit der ECHTEN make_system_compile_fn (cl /LD) über KF-8-Quellen → reale perm_<id>.dll.
// Muss in einer vcvars64-Umgebung laufen (cl im PATH/Env). Build: cl /std:c++latest /EHsc /I<libs/cache_engine> ...

#include "builder/build_orchestrator/build_orchestrator.hpp"
#include "builder/experiment_tree/ceb_generator.hpp"
#include "builder/experiment_tree/experiment_tree.hpp"

#include <filesystem>
#include <iostream>
#include <memory>

namespace ex = comdare::cache_engine::builder::experiment;

int main() {
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build({
        ex::AxisLevel{"traversal", {"ART"}, true, ""},
        ex::AxisLevel{"node", {"N4", "N16"}, true, ""}, // 2 Binaries (kurze reale Compile-Zeit)
        ex::AxisLevel{"node.cl_line", {"64"}, true, ""},
    });
    ex::StaticBinaryView view = tree.static_binary_view();
    std::cout << "Statischer Teilbaum: " << view.size() << " Tier-Binaries bereitzustellen\n";

    std::filesystem::path const base = std::filesystem::temp_directory_path() / "comdare_kf16_e2e";
    std::error_code             ec;
    std::filesystem::remove_all(base, ec);

    ex::BuildConfig cfg;
    cfg.cores_per_build = 2; // Default: 2 Kerne je Build
    cfg.total_cores     = 0; // alle CPU-Kerne (Hardware-Concurrency)
    cfg.source_dir      = base / "src";
    cfg.output_dir      = base / "dll";

    ex::BuildOrchestrator orch{cfg, ex::make_system_compile_fn(), &ex::generate_perm_source};
    ex::BuildStats        stats;
    auto                  results = orch.provision_all(view, &stats);

    std::cout << "parallel_jobs=" << cfg.parallel_jobs() << " peak=" << stats.peak_concurrency
              << " ok=" << stats.succeeded << " fail=" << stats.failed << "\n";

    std::size_t dll = 0;
    for (auto const& r : results) {
        bool exists = std::filesystem::exists(r.output);
        std::cout << "  [" << r.index << "] " << (r.ok() && exists ? "DLL " : "FEHLT ") << r.output.filename().string()
                  << "  (" << r.message << ")\n";
        if (r.ok() && exists) ++dll;
    }

    bool pass = (dll == view.size()) && (stats.failed == 0);
    std::cout << "\n==== KF-16 E2E realer Build: " << dll << "/" << view.size() << " DLLs  → "
              << (pass ? "OK" : "FEHLER") << " ====\n";
    return pass ? 0 : 1;
}
