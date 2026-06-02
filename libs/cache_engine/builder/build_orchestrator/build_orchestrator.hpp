#pragma once
// KF-16 (2026-06-02) — BuildOrchestrator: C++23 multithreaded Bereitstellung der Tier-Binaries VOR den Experimenten.
//
// Architektur (messarchitektur_v5_design §2, bindend): „Profil baut ZUERST den HOST → permutiert die Tier-Binary-
// Config → BAUT ZUERST ALLE DLLs → MISST DANACH (Build vor Mess-Phase)." Doc 26 §0: Build+Messen = Aufgabe der
// Bibliothek (CacheEngineBuilder = das WIE). Dieser Orchestrator realisiert das „baut zuerst alle DLLs" in C++23
// (KEIN Python — ersetzt die Rolle der CMake-Glob-Loop `comdare_build_adhoc_modules` für die Experiment-Baum-Binaries).
//
// Eingabe: der STATISCHE Teilbaum als `StaticBinaryView` (indizierter Iterator, KF-16/experiment_tree). Je Binary
// wird der KF-8-Perm-Source generiert + zu einem perm_<id>.dll kompiliert.
// MULTITHREADING (User 2026-06-02): Default 2 Kerne je parallelem DLL-Build, ALLE CPU-Kerne nutzen →
//   parallel_jobs = max(1, total_cores / cores_per_build). `cores_per_build` ist zugleich der Parallelitäts-Divisor
//   UND der {cores}-Wert im Compile-Kommando (MSVC /MP<cores>).
// Der Compiler-Aufruf ist INJIZIERBAR (`CompileFn`) → Orchestrierung deterministisch testbar (Stub) + realer
// Subprozess-Build (make_system_compile_fn). Header-only, C++23.

#include "../experiment_tree/experiment_tree.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

// ── Konfiguration des Builds ──────────────────────────────────────────────────
struct BuildConfig {
    std::size_t           cores_per_build = 2;  // Default 2 Kerne je parallelem DLL-Build (User 2026-06-02)
    std::size_t           total_cores     = 0;  // 0 → std::thread::hardware_concurrency() (ALLE Kerne)
    std::filesystem::path source_dir;           // perm_<id>.cpp (KF-8-Ausgabe)
    std::filesystem::path output_dir;           // perm_<id>.dll (Build-Ausgabe)

    /// Effektive Gesamtkern-Zahl (0 → Hardware-Concurrency, Fallback 1).
    [[nodiscard]] std::size_t effective_total() const noexcept {
        if (total_cores != 0) return total_cores;
        unsigned const hc = std::thread::hardware_concurrency();
        return hc == 0 ? std::size_t{1} : static_cast<std::size_t>(hc);
    }
    /// Parallele Build-Jobs = ALLE Kerne / Kerne-je-Build (mind. 1).
    [[nodiscard]] std::size_t parallel_jobs() const noexcept {
        std::size_t const cpb = (cores_per_build == 0) ? std::size_t{1} : cores_per_build;
        std::size_t const j   = effective_total() / cpb;
        return j == 0 ? std::size_t{1} : j;
    }
};

// ── Ein Build-Auftrag (eine Tier-Binary) ──────────────────────────────────────
struct BuildJob {
    std::size_t           index = 0;
    std::string           binary_id;
    std::filesystem::path source;
    std::filesystem::path output;
    std::size_t           cores = 2;  // Kerne, die dieser Build nutzen darf ({cores}-Token, z.B. /MP<cores>)
};

// ── Ergebnis je Tier-Binary (indiziert) ───────────────────────────────────────
struct BuildResult {
    std::size_t           index  = 0;
    std::string           binary_id;
    int                   status = -1;  // 0 = Erfolg
    std::filesystem::path output;
    std::string           message;
    [[nodiscard]] bool ok() const noexcept { return status == 0; }
};

struct BuildStats {
    std::size_t total_jobs       = 0;
    std::size_t peak_concurrency = 0;  // beobachtete maximale gleichzeitige Builds (≤ parallel_jobs)
    std::size_t succeeded        = 0;
    std::size_t failed           = 0;
};

using CompileFn   = std::function<int(BuildJob const&)>;            // 0 = Erfolg
using SourceGenFn = std::function<std::string(std::string const&)>; // binary_id → perm-Source (KF-8)

/// Bezeichner-Sanitisierung identisch zu KF-8 (perm_<sanitized>.cpp/.dll).
[[nodiscard]] inline std::string orch_sanitize(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out += (std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
    return out;
}

// ── Der Orchestrator ──────────────────────────────────────────────────────────
class BuildOrchestrator {
public:
    BuildOrchestrator(BuildConfig cfg, CompileFn compile, SourceGenFn gen)
        : cfg_{std::move(cfg)}, compile_{std::move(compile)}, gen_{std::move(gen)} {}

    /// Stellt ALLE Tier-Binaries des statischen Teilbaums bereit: je Binary Source generieren (KF-8) + DLL
    /// kompilieren — MULTITHREADED (parallel_jobs Worker, je cores_per_build Kerne). Liefert je Binary ein
    /// indiziertes Ergebnis (results[i] gehört zu view[i]). Befüllt optional BuildStats (Peak-Parallelität).
    std::vector<BuildResult> provision_all(StaticBinaryView const& view, BuildStats* stats = nullptr) {
        if (view.empty()) { if (stats) *stats = BuildStats{}; return {}; }

        std::error_code ec;
        std::filesystem::create_directories(cfg_.source_dir, ec);
        std::filesystem::create_directories(cfg_.output_dir, ec);

        std::vector<BuildResult> results(view.size());
        std::atomic<std::size_t> next{0};
        std::atomic<std::size_t> active{0};
        std::atomic<std::size_t> peak{0};

        std::size_t const n_workers = std::min(cfg_.parallel_jobs(), view.size());

        auto worker = [&] {
            for (;;) {
                std::size_t const i = next.fetch_add(1);
                if (i >= view.size()) return;

                std::size_t const cur = active.fetch_add(1) + 1;  // Eintritt: Parallelität +1
                std::size_t pk = peak.load();
                while (cur > pk && !peak.compare_exchange_weak(pk, cur)) { /* CAS-max */ }

                BinarySpec const& spec = view[i];
                std::string const id   = orch_sanitize(spec.binary_id);

                BuildJob job;
                job.index     = spec.index;
                job.binary_id = spec.binary_id;
                job.source    = cfg_.source_dir / ("perm_" + id + ".cpp");
                job.output    = cfg_.output_dir / ("perm_" + id + ".dll");
                job.cores     = (cfg_.cores_per_build == 0) ? std::size_t{1} : cfg_.cores_per_build;

                BuildResult r;
                r.index     = spec.index;
                r.binary_id = spec.binary_id;
                r.output    = job.output;

                // 1) KF-8-Source generieren + schreiben
                {
                    std::ofstream f{job.source, std::ios::binary | std::ios::trunc};
                    if (!f) { r.status = -2; r.message = "Quelle nicht schreibbar"; }
                    else    { f << gen_(spec.binary_id); }
                }
                // 2) DLL kompilieren (injizierte CompileFn)
                if (r.status != -2) {
                    r.status  = compile_(job);
                    r.message = (r.status == 0) ? "ok" : ("compile-exit " + std::to_string(r.status));
                }

                results[i] = std::move(r);
                active.fetch_sub(1);  // Austritt: Parallelität -1
            }
        };

        {  // Thread-Pool; jthread-Destruktor joint am Blockende → danach sind alle Builds fertig
            std::vector<std::jthread> pool;
            pool.reserve(n_workers);
            for (std::size_t w = 0; w < n_workers; ++w) pool.emplace_back(worker);
        }

        if (stats) {
            stats->total_jobs       = results.size();
            stats->peak_concurrency = peak.load();
            stats->succeeded = stats->failed = 0;
            for (auto const& r : results) (r.ok() ? stats->succeeded : stats->failed)++;
        }
        return results;
    }

    [[nodiscard]] BuildConfig const& config() const noexcept { return cfg_; }

private:
    BuildConfig cfg_;
    CompileFn   compile_;
    SourceGenFn gen_;
};

/// Default-CompileFn: realer MSVC-Subprozess, baut perm_<id>.cpp → perm_<id>.dll (SHARED). {cores} → /MP<cores>.
/// Host-Werkzeug (ruft den Compiler via std::system; cl muss im PATH/Env sein, z.B. vcvars64). Ausgabe unterdrückt.
[[nodiscard]] inline CompileFn make_system_compile_fn(std::vector<std::string> include_dirs = {},
                                                      std::string std_flag = "/std:c++latest") {
    return [include_dirs = std::move(include_dirs), std_flag = std::move(std_flag)](BuildJob const& job) -> int {
        std::string cmd = "cl /nologo " + std_flag + " /EHsc /LD /MP" + std::to_string(job.cores);
        for (auto const& inc : include_dirs) cmd += " /I\"" + inc + "\"";
        cmd += " \"" + job.source.string() + "\"";
        cmd += " /Fe:\"" + job.output.string() + "\"";
        cmd += " /Fo:\"" + job.output.string() + ".obj\"";
        cmd += " > nul 2>&1";
        return std::system(cmd.c_str());
    };
}

}  // namespace comdare::cache_engine::builder::experiment
