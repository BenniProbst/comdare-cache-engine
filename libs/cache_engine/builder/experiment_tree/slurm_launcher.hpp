#pragma once
// KF-12 + KF-13 (2026-06-02) — VORBEREITET, NICHT AUSGEFÜHRT (Cluster-gated).
//
// C++23-Generator für die SLURM/ZIH-Experiment-Skripte. Er EMITTIERT Skript-TEXT (Host-Werkzeug) — submittet
// NICHTS. Die Ausführung erfordert Cluster-Zugang + privilegierte SLURM-VM (ZIH); Submission ist GATE-MAXIMAL
// (mit User abzusprechen, CLAUDE.md „Kritische Manöver"). Kein Python (C++23 erzeugt bash = Cluster-Interface).
//
// KF-12 — architektonische LAUFZEIT-AUSNAHMEN (was IN-PROCESS via Algorithm_Resource_Control NICHT geht, KF-7):
//   CPU-Affinity (taskset/numactl) · HW-Prefetcher MSR 0x1A4 (wrmsr, CAP_SYS_RAWIO) · Governor (cpupower) ·
//   SMT (/sys/devices/system/cpu) · ASLR (setarch -R) · NUMA-Bindung (numactl). Diese sind die „StaticAxis/
//   architektonisch"-Größen, die ein privilegierter Pre-Exec-Block setzt, bevor die Tier-Binary läuft.
// KF-13 — ZIH-Skalierung: SLURM-Array (ein Job je Binary/Setting) · Singularity-Exec (mitgebrachter Container) ·
//   Result-Webhook (curl an VLAN-60 Build-Result-Webhook, CLAUDE.md). Projekt p_llm_compile.
//
// C++23, header-only.

#include <string>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

// ── KF-12: architektonische Laufzeit-Ausnahmen (privilegierter Pre-Exec-Block) ──
struct ArchRuntimeConfig {
    std::string  cpu_affinity   = "";          // z.B. "0-3" (leer = nicht setzen)
    std::string  hw_prefetcher_msr_hex = "";   // MSR 0x1A4 Wert hex, z.B. "0x0" (alle Prefetcher an); leer = unverändert
    std::string  governor       = "";          // "performance"/"powersave" (leer = nicht setzen)
    int          smt            = -1;          // 1=an, 0=aus, -1=nicht setzen
    int          aslr           = -1;          // 1=an, 0=aus(setarch -R), -1=nicht setzen
    std::string  numa_node      = "";          // z.B. "0" (leer = nicht binden)
};

// ── KF-13: ZIH-Job-Konfiguration (SLURM-Array + Singularity + Webhook) ──
struct ZihJobConfig {
    std::string  account        = "p_llm_compile";   // ZIH-Projektkennung (CLAUDE.md)
    std::string  partition      = "barnard";          // CPU-Cluster; "capella" für GPU
    std::string  time_limit     = "01:00:00";
    std::size_t  array_size     = 1;                  // KF-13: ein Array-Task je Binary/Setting
    std::string  singularity_image = "comdare-ce.sif";
    std::string  binary_dir     = "perms";            // Verzeichnis der perm_<id>.dll/.bin (KF-8/16)
    std::string  webhook_url    = "";                 // VLAN-60 Build-Result-Webhook (leer = kein curl)
    std::string  job_name       = "comdare-ce-experiment";
};

/// KF-12: erzeugt den privilegierten Pre-Exec-Setup-Block (bash) für die architektonischen Laufzeit-Ausnahmen.
/// Jede Zeile ist gated (läuft nur in einer SLURM-Admin-VM); nicht gesetzte Felder werden ausgelassen.
[[nodiscard]] inline std::string generate_arch_setup(ArchRuntimeConfig const& a) {
    std::string s = "# KF-12 architektonische Laufzeit-Ausnahmen (privilegiert, SLURM-VM) — NICHT lokal ausführen\n";
    if (!a.governor.empty())            s += "cpupower frequency-set -g " + a.governor + "\n";
    if (a.smt == 0)                     s += "echo off | sudo tee /sys/devices/system/cpu/smt/control\n";
    else if (a.smt == 1)                s += "echo on  | sudo tee /sys/devices/system/cpu/smt/control\n";
    if (!a.hw_prefetcher_msr_hex.empty()) s += "wrmsr -a 0x1a4 " + a.hw_prefetcher_msr_hex + "   # HW-Prefetcher (CAP_SYS_RAWIO)\n";
    return s;
}

/// KF-12: der Lauf-Präfix (taskset/numactl/setarch), der die Tier-Binary-Ausführung umklammert.
[[nodiscard]] inline std::string generate_exec_prefix(ArchRuntimeConfig const& a) {
    std::string p;
    if (!a.numa_node.empty())     p += "numactl --cpunodebind=" + a.numa_node + " --membind=" + a.numa_node + " ";
    if (!a.cpu_affinity.empty())  p += "taskset -c " + a.cpu_affinity + " ";
    if (a.aslr == 0)              p += "setarch -R ";   // ASLR aus für reproduzierbare Adressen
    return p;
}

/// KF-12 + KF-13: erzeugt das vollständige sbatch-Skript (Array + Singularity + Webhook + Arch-Setup).
/// VORBEREITET — submittet NICHTS. `sbatch <datei>` erst nach User-Freigabe auf einer Login-Node.
[[nodiscard]] inline std::string generate_sbatch_script(ZihJobConfig const& z, ArchRuntimeConfig const& a) {
    std::string s = "#!/bin/bash\n";
    s += "#SBATCH --job-name=" + z.job_name + "\n";
    s += "#SBATCH --account=" + z.account + "\n";
    s += "#SBATCH --partition=" + z.partition + "\n";
    s += "#SBATCH --time=" + z.time_limit + "\n";
    s += "#SBATCH --array=0-" + std::to_string(z.array_size == 0 ? 0 : z.array_size - 1) + "\n";  // KF-13
    s += "#SBATCH --output=ce_%A_%a.out\n\n";
    s += "set -euo pipefail\n";
    s += "TASK=${SLURM_ARRAY_TASK_ID}\n\n";
    s += generate_arch_setup(a);                                   // KF-12
    s += "\n# KF-13: ein perm-Binary je Array-Task über Singularity ausführen\n";
    s += "BIN=$(ls " + z.binary_dir + "/perm_*.bin | sed -n \"$((TASK+1))p\")\n";
    s += generate_exec_prefix(a) +                                 // KF-12 Lauf-Präfix
         "singularity exec " + z.singularity_image + " \"$BIN\" > result_${TASK}.json\n";
    if (!z.webhook_url.empty())                                    // KF-13 Webhook
        s += "curl -fsS -X POST --data-binary @result_${TASK}.json " + z.webhook_url + " || true\n";
    return s;
}

}  // namespace comdare::cache_engine::builder::experiment
