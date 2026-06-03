#pragma once
// KF-16 + KF-16b (2026-06-02) — BuildOrchestrator: C++23 multithreaded Bereitstellung der Tier-Binaries VOR den
// Experimenten, RAM-gewahr + inkrementell-resumierbar.
//
// Architektur (messarchitektur_v5_design §2): „Profil baut ZUERST den HOST → BAUT ZUERST ALLE DLLs → MISST DANACH".
// Doc 26 §0: Build = Aufgabe der Bibliothek (CacheEngineBuilder = WIE). Realisiert „baut zuerst alle DLLs" in C++23
// (kein Python; ersetzt die CMake-Glob-Loop comdare_build_adhoc_modules für die Experiment-Baum-Binaries).
//
// KF-16b (User 2026-06-02): (1) INKREMENTELL/RESUMIERBAR — nur fehlende/veraltete DLLs neu bauen; eine bestehende
// DLL wird ÜBERSPRUNGEN, wenn ihr Versions-Sidecar der build_version genügt (überlebt Absturz). (2) RAM-ADMISSION —
// freien RAM überwachen; weiteren Build (je cores_per_build Threads) nur starten, wenn genug RAM frei (mind. 1 läuft
// immer → Fortschritt); RAM-Low-Water-Mark wird gemessen. (3) Default cores_per_build = 4, KEINE Oversubscription
// über den CPU-Kern-Pool (parallel_jobs × cores_per_build ≤ Kerne; ein einzelner Build nutzt nie mehr als alle Kerne).
//
// Compiler-Aufruf + RAM-Abfrage sind INJIZIERBAR (CompileFn / FreeRamFn) → deterministisch testbar. Header-only, C++23.
// Die reale OS-RAM-Abfrage (GlobalMemoryStatusEx/sysinfo) liegt in system_ram.hpp (hält diesen Header windows.h-frei).

#include "../experiment_tree/experiment_tree.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

// ── Konfiguration des Builds ──────────────────────────────────────────────────
struct BuildConfig {
    std::size_t           cores_per_build = 4;  // KF-16b: Default 4 Threads je parallelem DLL-Build (war 2)
    std::size_t           total_cores     = 0;  // 0 → std::thread::hardware_concurrency() (ALLE Kerne)
    std::filesystem::path source_dir;           // perm_<id>.cpp (KF-8-Ausgabe)
    std::filesystem::path output_dir;           // perm_<id>.dll (Build-Ausgabe)
    // KF-16b:
    std::uint64_t         ram_per_build_bytes    = 0;  // RAM-Budget je Build; 0 = RAM-Gate AUS (nur CPU-Cap)
    std::uint64_t         ram_safety_margin_bytes = 0; // Reserve, die immer frei bleiben muss
    std::string           build_version;               // Versions-/Anforderungs-Signatur; leer = nie überspringen

    /// Effektive Gesamtkern-Zahl (0 → Hardware-Concurrency, Fallback 1).
    [[nodiscard]] std::size_t effective_total() const noexcept {
        if (total_cores != 0) return total_cores;
        unsigned const hc = std::thread::hardware_concurrency();
        return hc == 0 ? std::size_t{1} : static_cast<std::size_t>(hc);
    }
    /// Threads je Build, GEKAPPT auf die Gesamtkerne (verhindert Oversubscription, wenn cores_per_build > Kerne).
    [[nodiscard]] std::size_t effective_cores_per_build() const noexcept {
        std::size_t const cpb = (cores_per_build == 0) ? std::size_t{1} : cores_per_build;
        return std::min(cpb, effective_total());
    }
    /// Parallele Build-Jobs = Kerne / Threads-je-Build (mind. 1). Garantiert jobs × cpb ≤ Kerne (KEINE Oversubscription).
    [[nodiscard]] std::size_t parallel_jobs() const noexcept {
        std::size_t const j = effective_total() / effective_cores_per_build();
        return j == 0 ? std::size_t{1} : j;
    }
};

// ── Ein Build-Auftrag (eine Tier-Binary) ──────────────────────────────────────
struct BuildJob {
    std::size_t           index = 0;
    std::string           binary_id;
    std::filesystem::path source;
    std::filesystem::path output;
    std::size_t           cores = 4;  // Threads, die dieser Build nutzen darf ({cores}-Token, z.B. /MP<cores>)
};

// ── Ergebnis je Tier-Binary (indiziert) ───────────────────────────────────────
struct BuildResult {
    std::size_t           index   = 0;
    std::string           binary_id;
    int                   status  = -1;     // 0 = Erfolg
    bool                  skipped = false;  // KF-16b: bestehende DLL war versions-aktuell → nicht neu gebaut
    std::filesystem::path output;
    std::string           message;
    [[nodiscard]] bool ok() const noexcept { return status == 0; }
};

struct BuildStats {
    std::size_t   total_jobs        = 0;
    std::size_t   peak_concurrency  = 0;   // beobachtete max. gleichzeitige Builds (≤ parallel_jobs ∩ RAM-Limit)
    std::size_t   succeeded         = 0;
    std::size_t   failed            = 0;
    std::size_t   skipped           = 0;   // KF-16b: versions-aktuelle DLLs (resumiert)
    std::size_t   built             = 0;   // tatsächlich (neu) kompiliert
    std::uint64_t min_free_ram_bytes = (std::numeric_limits<std::uint64_t>::max)();  // RAM-Low-Water-Mark
};

using CompileFn   = std::function<int(BuildJob const&)>;            // 0 = Erfolg
using SourceGenFn = std::function<std::string(std::string const&)>; // binary_id → perm-Source (KF-8)
using FreeRamFn   = std::function<std::uint64_t()>;                 // freier physischer RAM in Bytes

/// Default-FreeRamFn: „unbegrenzt" → RAM-Gate effektiv aus, wenn keine reale Abfrage injiziert ist.
[[nodiscard]] inline std::uint64_t free_ram_unlimited() noexcept {
    return (std::numeric_limits<std::uint64_t>::max)();
}

/// Bezeichner-Sanitisierung identisch zu KF-8 (perm_<sanitized>.cpp/.dll).
[[nodiscard]] inline std::string orch_sanitize(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out += (std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
    return out;
}

/// FNV-1a-Hash über den (vollen) binary_id — stabiler, kurzer Eindeutigkeits-Suffix (hex).
[[nodiscard]] inline std::string orch_fnv1a_hex(std::string_view s) noexcept {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    for (char c : s) { h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c)); h *= 0x100000001b3ULL; }
    static constexpr char hexd[] = "0123456789abcdef";
    std::string out(16, '0');
    for (int i = 15; i >= 0; --i) { out[i] = hexd[h & 0xF]; h >>= 4; }
    return out;
}

/// L-LAZY-E2E (2026-06-03): der DATEI-STEM `perm_<…>` einer Tier-Binary — LÄNGEN-GEKAPPT gegen Windows-MAX_PATH (260).
/// Der volle, sanitisierte 19-Achsen-binary_id ist ~520+ Zeichen → `source_dir/perm_<id>.cpp` sprengt MAX_PATH →
/// `std::ofstream::open` schlägt still fehl (Befund 2026-06-03: built=0, src-Dir leer). Lösung: ist der sanitisierte
/// Pfad lang, wird er auf ein lesbares Präfix + `_<index>_<fnv1a-hex>` gekürzt (stabil + kollisionsfrei je Pfad).
/// Kurze IDs (z.B. Tests/handbenannte Tiere) bleiben unverändert (rückwärtskompatibel). `kStemMax` lässt Raum für
/// das source_dir-Präfix + ".dll.version"-Suffix unter 260.
inline constexpr std::size_t kStemMax = 120;
[[nodiscard]] inline std::string orch_make_stem(std::string_view binary_id, std::size_t index) {
    std::string san = orch_sanitize(binary_id);
    if (san.size() <= kStemMax) return san;                       // kurz genug → unverändert (rückwärtskompatibel)
    std::string suffix = "_" + std::to_string(index) + "_" + orch_fnv1a_hex(binary_id);
    std::size_t const keep = (kStemMax > suffix.size()) ? (kStemMax - suffix.size()) : 0;
    return san.substr(0, keep) + suffix;                          // Präfix (lesbar) + index + Hash (eindeutig)
}

// ── KF-16b: Versions-Sidecar (Inkrement/Resume) ──────────────────────────────
[[nodiscard]] inline std::filesystem::path version_sidecar_path(std::filesystem::path const& output) {
    return std::filesystem::path{output.string() + ".version"};
}
/// true, wenn die DLL existiert UND ihr Sidecar exakt der geforderten Version entspricht (→ überspringen).
[[nodiscard]] inline bool dll_is_current(std::filesystem::path const& output, std::string const& version) {
    if (version.empty()) return false;                       // ohne Versions-Anforderung nie überspringen
    std::error_code ec;
    if (!std::filesystem::exists(output, ec)) return false;  // DLL fehlt → bauen
    std::ifstream f{version_sidecar_path(output), std::ios::binary};
    if (!f) return false;
    std::string content((std::istreambuf_iterator<char>(f)), {});
    return content == version;
}
inline void write_version_sidecar(std::filesystem::path const& output, std::string const& version) {
    if (version.empty()) return;
    std::ofstream f{version_sidecar_path(output), std::ios::binary | std::ios::trunc};
    if (f) f << version;
}

// ── Der Orchestrator ──────────────────────────────────────────────────────────
class BuildOrchestrator {
public:
    BuildOrchestrator(BuildConfig cfg, CompileFn compile, SourceGenFn gen, FreeRamFn free_ram = free_ram_unlimited)
        : cfg_{std::move(cfg)}, compile_{std::move(compile)}, gen_{std::move(gen)}, free_ram_{std::move(free_ram)} {}

    /// Stellt ALLE Tier-Binaries des statischen Teilbaums bereit (rückwärtskompatibel): je Binary Source (KF-8)
    /// + DLL kompilieren — INKREMENTELL, RAM-gewahr, MULTITHREADED. ⚠️ results-Vektor O(view.size()): nur für
    /// HANDHABBARE Views (Pilot/Test); für riesige Inventare provision_all(view, selection, stats) (D2/L-73).
    /// Der „alle"-Pfad nutzt eine Identity-Index-Map → erzeugt KEINEN ∏-großen Index-Vektor.
    std::vector<BuildResult> provision_all(StaticBinaryView const& view, BuildStats* stats = nullptr) {
        return provision_core(view, view.size(), [](std::size_t j) noexcept { return j; }, stats);
    }

    /// D2 / L-73: baut NUR die selektierten View-Indizes (BuildSelection.indices). results-Vektor O(K=selection.size()),
    /// NICHT O(∏); `next.fetch_add` läuft bis K; je Worker `view[selection[j]]`. OOM-sicher bei riesiger View
    /// (Doc 26 §2). Aufruf: `orch.provision_all(view, sel.indices, &stats)` (BuildSelection.indices → span).
    std::vector<BuildResult> provision_all(StaticBinaryView const& view,
                                           std::span<const std::size_t> selection, BuildStats* stats = nullptr) {
        return provision_core(view, selection.size(),
                              [selection](std::size_t j) noexcept { return selection[j]; }, stats);
    }

    [[nodiscard]] BuildConfig const& config() const noexcept { return cfg_; }

private:
    /// Gemeinsamer Bau-Kern (KF-16b). Baut K Binaries; `view_index(j)` mappt den selektions-relativen Index j
    /// (0..K-1) auf den View-Index i. results ist O(K). Der „alle"-Pfad (K=view.size(), view_index=identity)
    /// materialisiert KEINEN Index-Vektor; der Selektions-Pfad hält nur die K gewählten Indizes (Aufrufer-seitig).
    template <class IndexMap>
    std::vector<BuildResult> provision_core(StaticBinaryView const& view, std::size_t k,
                                            IndexMap view_index, BuildStats* stats) {
        if (k == 0 || view.empty()) { if (stats) *stats = BuildStats{}; return {}; }

        std::error_code ec;
        std::filesystem::create_directories(cfg_.source_dir, ec);
        std::filesystem::create_directories(cfg_.output_dir, ec);

        std::vector<BuildResult> results(k);             // O(K) — NICHT view.size() (L-73)
        std::atomic<std::size_t> next{0};

        std::mutex              mtx;             // schützt active/peak/min_free + CV-Prädikat
        std::condition_variable cv;
        std::size_t             active = 0;
        std::size_t             peak   = 0;
        std::uint64_t           min_free = (std::numeric_limits<std::uint64_t>::max)();

        std::size_t const n_workers = std::min(cfg_.parallel_jobs(), k);
        std::size_t const cores     = cfg_.effective_cores_per_build();
        // RAM-Baseline EINMAL zu Beginn messen (= jetzt frei verfügbar). Die Admission reserviert dagegen
        // (active+1)×Budget — deterministisch + OOM-sicher, unabhängig vom Ramp-Lag des OS-free_ram.
        std::uint64_t const baseline_free = (cfg_.ram_per_build_bytes != 0) ? free_ram_() : 0;

        auto worker = [&] {
            for (;;) {
                std::size_t const j = next.fetch_add(1);   // selektions-relativ
                if (j >= k) return;
                std::size_t const i = view_index(j);       // → View-Index

                BuildResult r;
                // Defensiv: ungültiger Selektions-Index → Fehler-Result statt OOB-Dekodierung.
                if (i >= view.size()) {
                    r.index = i; r.status = -3; r.message = "selection index out of range";
                    results[j] = std::move(r);
                    continue;
                }

                BinarySpec const spec = view[i];           // by value (operator[] dekodiert on-demand)
                std::string const id  = orch_make_stem(spec.binary_id, spec.index);  // MAX_PATH-sicher (gekappt+Hash)

                BuildJob job;
                job.index     = spec.index;
                job.binary_id = spec.binary_id;
                job.source    = cfg_.source_dir / ("perm_" + id + ".cpp");
                job.output    = cfg_.output_dir / ("perm_" + id + ".dll");
                job.cores     = cores;

                r.index = spec.index; r.binary_id = spec.binary_id; r.output = job.output;

                // (A) INKREMENTELL: bestehende, versions-aktuelle DLL überspringen (Resume nach Absturz).
                if (dll_is_current(job.output, cfg_.build_version)) {
                    r.status = 0; r.skipped = true; r.message = "übersprungen (Version aktuell)";
                    results[j] = std::move(r);
                    continue;
                }

                // (B) RAM-ADMISSION: warten, bis genug freier RAM (mind. 1 Build läuft immer → Fortschritt).
                {
                    std::unique_lock<std::mutex> lk(mtx);
                    cv.wait(lk, [&] {
                        if (cfg_.ram_per_build_bytes == 0) return true;     // RAM-Gate aus
                        if (active == 0) return true;                       // Fortschritt garantieren (mind. 1)
                        // Reservierung: die RAM aller gleichzeitigen Builds (inkl. des neuen) darf das
                        // Start-Baseline nicht übersteigen → deterministischer Cap floor(baseline/Budget), kein OOM.
                        if ((active + 1) * cfg_.ram_per_build_bytes + cfg_.ram_safety_margin_bytes > baseline_free)
                            return false;
                        // Dynamische Verschärfung: aktueller freier RAM muss für EINEN weiteren Build reichen.
                        return free_ram_() >= cfg_.ram_per_build_bytes + cfg_.ram_safety_margin_bytes;
                    });
                    ++active;
                    peak = std::max(peak, active);
                    std::uint64_t const fr = free_ram_();
                    if (fr < min_free) min_free = fr;
                }

                // (C) Source generieren (KF-8) + kompilieren — OHNE Lock (echt parallel).
                {
                    std::ofstream f{job.source, std::ios::binary | std::ios::trunc};
                    if (!f) { r.status = -2; r.message = "Quelle nicht schreibbar"; }
                    else    { f << gen_(spec.binary_id); }
                }
                if (r.status != -2) {
                    r.status  = compile_(job);
                    r.message = (r.status == 0) ? "ok" : ("compile-exit " + std::to_string(r.status));
                    if (r.status == 0) write_version_sidecar(job.output, cfg_.build_version);  // Resume-Marke
                }
                results[j] = std::move(r);

                { std::lock_guard<std::mutex> lk(mtx); --active; }
                cv.notify_all();
            }
        };

        {  // Thread-Pool; jthread-Destruktor joint am Blockende
            std::vector<std::jthread> pool;
            pool.reserve(n_workers);
            for (std::size_t w = 0; w < n_workers; ++w) pool.emplace_back(worker);
        }

        if (stats) {
            stats->total_jobs        = results.size();
            stats->peak_concurrency  = peak;
            stats->min_free_ram_bytes = min_free;
            stats->succeeded = stats->failed = stats->skipped = stats->built = 0;
            for (auto const& r : results) {
                (r.ok() ? stats->succeeded : stats->failed)++;
                if (r.skipped) ++stats->skipped; else if (r.ok()) ++stats->built;
            }
        }
        return results;
    }

    BuildConfig cfg_;
    CompileFn   compile_;
    SourceGenFn gen_;
    FreeRamFn   free_ram_;
};

/// Default-CompileFn: realer MSVC-Subprozess, baut perm_<id>.cpp → perm_<id>.dll (SHARED). {cores} → /MP<cores>.
/// Host-Werkzeug (ruft cl via std::system; cl muss im PATH/Env sein, z.B. vcvars64). Ausgabe unterdrückt.
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
