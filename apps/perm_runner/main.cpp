// apps/perm_runner — der lokale Cluster-Mess-Runner je Binary (L-CLUSTER gate-frei). Lädt EINE perm-DLL via
// AnatomyModuleLoader, treibt den Mess-Workload über IObservableTier (run_observable_perm), emittiert EINE
// result_ingest-Zeile (binary_id + 13 ';'-Felder) nach stdout. Auf dem Cluster fährt eine SLURM-Task genau EINEN
// perm_runner mit EINER DLL (kein main() in der DLL → der Runner ist das Executable, das die DLL lädt). Die echte
// Cluster-Submission (sbatch/apptainer) bleibt GATE-MAXIMAL — dieses CLI ist LOKAL gegen jede perm-DLL verifizierbar.
//
// Aufruf:  perm_runner <perm.dll> [binary_id] [n_ops]   (Default binary_id = DLL-Pfad, n_ops = 1000).

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp>
#include <builder/experiment_tree/perm_runner.hpp>
#include <anatomy/anatomy_base.hpp>
#include <anatomy/observable_tier.hpp>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    namespace loader = comdare::cache_engine::builder::anatomy_loader;
    namespace ex     = comdare::cache_engine::builder::experiment;
    namespace ana    = comdare::cache_engine::anatomy;

    if (argc < 2) { std::cerr << "usage: perm_runner <perm.dll> [binary_id] [n_ops]\n"; return 2; }
    std::string const   dll       = argv[1];
    std::string const   binary_id = (argc >= 3) ? argv[2] : dll;
    std::uint64_t const n_ops     = (argc >= 4) ? std::strtoull(argv[3], nullptr, 10) : 1000u;

    loader::AnatomyModuleHandle handle;
    int const st = loader::AnatomyModuleLoader::load(dll.c_str(), handle);
    if (st != loader::status_ok) {
        std::cerr << "perm_runner: load fehlgeschlagen (" << loader::status_name(st) << "): " << dll << "\n";
        return 1;
    }
    ana::IAnatomyBase* base = handle.anatomy();
    auto* tier = (base != nullptr) ? dynamic_cast<ana::IObservableTier*>(base) : nullptr;
    if (tier == nullptr) {
        std::cerr << "perm_runner: keine IObservableTier-Mess-Ebene in der DLL (kein Mess-Build?): " << dll << "\n";
        return 1;
    }
    // Mess-Lauf + EINE result_ingest-Zeile nach stdout (der Cluster-Webhook/Ingest liest exakt dieses Format).
    std::cout << ex::run_observable_perm(*tier, binary_id, n_ops) << "\n";
    return 0;
}
