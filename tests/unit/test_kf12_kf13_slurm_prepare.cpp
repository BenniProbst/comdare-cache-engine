// test_kf12_kf13_slurm_prepare — KF-12 + KF-13 (2026-06-02) — VORBEREITET, NICHT AUSGEFÜHRT
// Belegt: der C++23-Generator emittiert korrekte SLURM/ZIH-Skript-TEXTE (architektonische Laufzeit-Ausnahmen +
// Array/Singularity/Webhook). KEINE Submission — nur die generierte Skript-Struktur wird geprüft (Cluster-gated).
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> test_kf12_kf13_slurm_prepare.cpp

#include "builder/experiment_tree/slurm_launcher.hpp"

#include <iostream>
#include <string>

namespace ex = comdare::cache_engine::builder::experiment;

static int g_fail = 0;
void check_true(char const* what, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n"; if (!c) ++g_fail; }
static bool has(std::string const& h, std::string const& n) { return h.find(n) != std::string::npos; }

int main() {
    std::cout << "KF-12/13: SLURM/ZIH-Skript-Generator (VORBEREITET, NICHT submittet):\n";

    ex::ZihJobConfig z;
    z.array_size      = 10;                 // KF-13: 10 Array-Tasks (ein Binary je Task)
    z.webhook_url     = "http://10.0.60.1:8080/result";
    ex::ArchRuntimeConfig a;
    a.cpu_affinity         = "0-3";
    a.hw_prefetcher_msr_hex = "0x0";        // KF-12: HW-Prefetcher via MSR 0x1A4
    a.governor             = "performance";
    a.smt                  = 0;             // SMT aus
    a.aslr                 = 0;             // ASLR aus (reproduzierbar)
    a.numa_node            = "0";

    std::string const sb = ex::generate_sbatch_script(z, a);

    // KF-13: SLURM-Array + ZIH-Account + Singularity + Webhook
    check_true("ZIH-Account p_llm_compile", has(sb, "#SBATCH --account=p_llm_compile"));
    check_true("SLURM-Array 0-9 (10 Tasks)", has(sb, "#SBATCH --array=0-9"));
    check_true("partition barnard", has(sb, "#SBATCH --partition=barnard"));
    check_true("Singularity-Exec", has(sb, "singularity exec comdare-ce.sif"));
    check_true("Array-Task-Selektion (SLURM_ARRAY_TASK_ID)", has(sb, "SLURM_ARRAY_TASK_ID"));
    check_true("Webhook-curl an VLAN-60", has(sb, "curl -fsS -X POST") && has(sb, "10.0.60.1:8080/result"));

    // KF-12: architektonische Laufzeit-Ausnahmen
    check_true("Governor performance (cpupower)", has(sb, "cpupower frequency-set -g performance"));
    check_true("HW-Prefetcher MSR 0x1a4 (wrmsr)", has(sb, "wrmsr -a 0x1a4 0x0"));
    check_true("SMT aus (/sys/devices/system/cpu/smt)", has(sb, "/sys/devices/system/cpu/smt/control"));
    check_true("NUMA-Bindung (numactl)", has(sb, "numactl --cpunodebind=0 --membind=0"));
    check_true("CPU-Affinity (taskset)", has(sb, "taskset -c 0-3"));
    check_true("ASLR aus (setarch -R)", has(sb, "setarch -R"));

    // Auslassung: leere Felder erzeugen KEINE Direktive (kein versehentliches Setzen).
    ex::ArchRuntimeConfig empty;
    std::string const setup_empty = ex::generate_arch_setup(empty);
    check_true("leere Arch-Config → kein wrmsr/cpupower", !has(setup_empty, "wrmsr") && !has(setup_empty, "cpupower"));
    check_true("leere Arch-Config → kein setarch/taskset im Prefix", ex::generate_exec_prefix(empty).empty());

    // GATING-Hinweis (Selbst-Dokumentation, keine Ausführung)
    std::cout << "  [i]  Skripte VORBEREITET — Submission (sbatch/ssh ZIH) ist GATE-MAXIMAL (User-Freigabe nötig)\n";

    std::cout << "\n==== KF-12/13 SLURM/ZIH-Skript-Generator (vorbereitet): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
