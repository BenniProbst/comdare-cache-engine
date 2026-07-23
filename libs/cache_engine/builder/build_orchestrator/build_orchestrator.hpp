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
#include "../experiment_tree/progress_heartbeat.hpp" // S1 (§62-B Log-Flush): geflushtes Bau-Fortschritts-Testat
#include <cache_engine/measurement/axis_error.hpp>   // opt-d/d1-carrier: CompilerCompilerErrorClass (A2-Hybrid Teil 1)
#include <cache_engine/measurement/simd_build_gate.hpp>        // Section 40.a-E4: flag-genaues Bau-Gate (Pruef-Dock)
#include <cache_engine/measurement/simd_organ_requirement.hpp> // Section 40.a-E4: per-Binary organ_required-Aggregation

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cctype>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <expected> // d1-carrier (C4/OD-3): std::expected<void, CompilerCompilerErrorClass> als Fehler-Traeger
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

#ifndef _WIN32
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;
#endif

namespace comdare::cache_engine::builder::experiment {

// ── Konfiguration des Builds ──────────────────────────────────────────────────
struct BuildConfig {
    std::size_t           cores_per_build = 4; // KF-16b: Default 4 Threads je parallelem DLL-Build (war 2)
    std::size_t           total_cores     = 0; // 0 → std::thread::hardware_concurrency() (ALLE Kerne)
    std::filesystem::path source_dir;          // perm_<id>.cpp (KF-8-Ausgabe)
    std::filesystem::path output_dir;          // perm_<id>.dll (Build-Ausgabe)
    // KF-16b:
    std::uint64_t ram_per_build_bytes     = 0; // RAM-Budget je Build; 0 = RAM-Gate AUS (nur CPU-Cap)
    std::uint64_t ram_safety_margin_bytes = 0; // Reserve, die immer frei bleiben muss
    std::string   build_version;               // Versions-/Anforderungs-Signatur; leer = nie überspringen
    // (E) 2026-06-04: je Tier-Binary ein eigener Unterordner output_dir/<stem>/ (DLL + Source + .obj + .cl.log
    // + .version landen alle darin). Default false = altes flaches Verhalten (rückwärtskompatibel, opt-in).
    bool per_binary_subdirs = false;
    // W6 (2026-07-19, Ledger §32-F7 "KOMPILATION darf parallel, nur MESSEN ist 1-Thread"): EXPLIZITER Override der
    // parallelen Compile-Worker-Zahl (Bau-Pool). 0 = ungesetzt => die parallel_jobs()-Heuristik (Kerne/cores_per_build)
    // gilt = EXAKT das heutige Verhalten (byte-neutral: kein Aufrufer, der dieses Feld nicht setzt, aendert sich).
    // >0 = harte Worker-Zahl (gekappt auf k = Zahl der Bau-Jobs), UNABHAENGIG von cores_per_build. Grund: eine
    // Tier-Binary ist EINE Translation-Unit -> ein g++-Aufruf nutzt ~1 Kern; die parallel_jobs()-Heuristik reserviert
    // aber cores_per_build Kerne je Build und kollabiert im Mess-Kontext (kleiner Runner) auf ~1 gleichzeitigen Build
    // (~7.5s/DLL sequenziell = Voll-Matrix unfeasible). Der Facade-Rand belegt dies aus Env COMDARE_BUILD_PARALLEL
    // (env_parallelism_value); der achsen-blinde Orchestrator bleibt env-frei/deterministisch-testbar.
    std::size_t build_parallelism = 0;

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
    /// W6: die tatsaechliche Compile-Worker-Zahl fuer k Bau-Jobs. build_parallelism>0 => harter Override (auf k
    /// gekappt, mind. 1); sonst die parallel_jobs()-Heuristik (= heute). IMMER mind. 1 (Fortschritt garantiert),
    /// nie mehr als k (keine Leerlauf-Worker). Byte-neutral bei build_parallelism==0.
    [[nodiscard]] std::size_t effective_build_workers(std::size_t k) const noexcept {
        if (k == 0) return 1;
        std::size_t const want = (build_parallelism != 0) ? build_parallelism : parallel_jobs();
        return std::min(want == 0 ? std::size_t{1} : want, k);
    }
};

/// W6 (Ledger §32-F7): REINER Parser des COMDARE_BUILD_PARALLEL-Roh-Werts (der Facade-Rand ruft std::getenv und
/// reicht das Ergebnis hier herein -> der Orchestrator-Header bleibt env-frei/deterministisch testbar). nullptr /
/// leer / nicht-numerisch / 0 => 0 (= ungesetzt = parallel_jobs()-Heuristik = heutiges byte-neutrales Verhalten).
/// Sonst der geparste Wert (>0 = harte Worker-Zahl). Fuehrende Ziffern werden gelesen (strtoul), Ueberlauf => 0.
[[nodiscard]] inline std::size_t env_parallelism_value(char const* raw) noexcept {
    if (raw == nullptr || *raw == '\0') return 0;
    char*               end = nullptr;
    unsigned long const v   = std::strtoul(raw, &end, 10);
    if (end == raw) return 0; // keine fuehrende Ziffer => ungesetzt-aequivalent
    return static_cast<std::size_t>(v);
}

// ── Ein Build-Auftrag (eine Tier-Binary) ──────────────────────────────────────
struct BuildJob {
    std::size_t           index = 0;
    std::string           binary_id;
    std::filesystem::path source;
    std::filesystem::path output;
    std::size_t           cores = 4; // Threads, die dieser Build nutzen darf ({cores}-Token, z.B. /MP<cores>)
};

// ── Ergebnis je Tier-Binary (indiziert) ───────────────────────────────────────
struct BuildResult {
    std::size_t           index = 0;
    std::string           binary_id;
    int                   status  = -1;    // 0 = Erfolg
    bool                  skipped = false; // KF-16b: bestehende DLL war versions-aktuell → nicht neu gebaut
    std::filesystem::path output;
    std::string           message;
    // d1-carrier (A2-Hybrid Teil 1, C4/OD-3): klassifizierter Bau-Ausgang. Erfolg = has_value (Default);
    // Fehlschlag = std::unexpected(CompilerCompilerErrorClass). Der Builder zieht das geteilte Taxonomie-
    // Vokabular (measurement/axis_error.hpp) hoch — Bau-KONFIG bleibt Wert-runter (opt-d), Fehler-KLASSE Wert-hoch.
    std::expected<void, ::comdare::cache_engine::measurement::BuildError> outcome{};
    // Inkrementeller Tier-Binary-Cache (Bauplan §1+§3): die Organ-Algorithmus-Signatur DIESER Binary (perm.algos-
    // Inhalt), waehrend des Baus aus spec.axes berechnet. Leer, wenn keine AlgoSigFn injiziert ist (rueckwaerts-
    // kompatibel). Der Mess-Resume-Pfad (cache_engine_builder_iterator) haengt sie additiv an den Resume-Stamp ->
    // eine Algo-Aenderung erzwingt die Neu-Messung GENAU der betroffenen Binaries (ehrlich, kein stilles Stale-Resume).
    std::string        algo_sig;
    [[nodiscard]] bool ok() const noexcept { return status == 0; }
};

struct BuildStats {
    std::size_t   total_jobs         = 0;
    std::size_t   peak_concurrency   = 0; // beobachtete max. gleichzeitige Builds (≤ parallel_jobs ∩ RAM-Limit)
    std::size_t   succeeded          = 0;
    std::size_t   failed             = 0;
    std::size_t   skipped            = 0; // KF-16b: versions-aktuelle DLLs (resumiert)
    std::size_t   built              = 0; // tatsächlich (neu) kompiliert
    std::uint64_t min_free_ram_bytes = (std::numeric_limits<std::uint64_t>::max)(); // RAM-Low-Water-Mark
};

using CompileFn   = std::function<int(BuildJob const&)>;            // 0 = Erfolg
using SourceGenFn = std::function<std::string(std::string const&)>; // binary_id → perm-Source (KF-8)
using FreeRamFn   = std::function<std::uint64_t()>;                 // freier physischer RAM in Bytes
// Inkrementeller Tier-Binary-Cache (Bauplan §3): spec.axes (die 17 (achse,wert)-Paare der Binary) → deterministische
// Organ-Algorithmus-Signatur (algo_sig, perm.algos-Inhalt). Analog SourceGenFn injiziert (die Facade baut sie aus der
// compile-time Versions-Tabelle, axis_variant_version_table.hpp::compose_algo_signature). Leer/ungesetzt = Organ-Gate
// AUS -> byte-neutrales Alt-Verhalten (nur perm.dll.version-Skip). Der achsen-blinde Orchestrator bleibt registry-frei.
using AlgoSigFn = std::function<std::string(std::vector<std::pair<std::string, std::string>> const&)>;
// W11 (Ledger §43.c, 2026-07-19): Completion-Hook -- feuert je Binary SOFORT nach der Finalisierung von results[j]
// (aus dem Build-Worker-Thread, in COMPLETION-Reihenfolge, NICHT index-geordnet). Zweck: der BAU-Modus (provision_only)
// haengt hier einen asynchronen Push-Pump ein, der die fertige perm.dll ueberlappend mit dem weiterlaufenden Bau in den
// Objekt-Store schiebt (statt Batch NACH provision_all). Default leer => nie gefeuert => byte-identisch. Muss thread-safe
// sein (mehrere Worker feuern gleichzeitig) -- der Konsument (AsyncPushPump::enqueue) ist intern mutex-serialisiert. Die
// index-geordnete progress_sink-/Determinismus-Naht (W6/§38) bleibt UNBERUEHRT: sie feuert weiterhin sequenziell im
// 1-Thread-Loop NACH provision_all; nur der reihenfolge-UNABHAENGIGE Objekt-Store-Push nutzt diesen Completion-Kanal.
using BinaryDoneFn = std::function<void(BuildResult const&)>;

/// Default-FreeRamFn: „unbegrenzt" → RAM-Gate effektiv aus, wenn keine reale Abfrage injiziert ist.
[[nodiscard]] inline std::uint64_t free_ram_unlimited() noexcept { return (std::numeric_limits<std::uint64_t>::max)(); }

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
    for (char c : s) {
        h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
        h *= 0x100000001b3ULL;
    }
    static constexpr char hexd[] = "0123456789abcdef";
    std::string           out(16, '0');
    for (int i = 15; i >= 0; --i) {
        out[i] = hexd[h & 0xF];
        h >>= 4;
    }
    return out;
}

/// L-LAZY-E2E (2026-06-03): der DATEI-STEM `perm_<…>` einer Tier-Binary — LÄNGEN-GEKAPPT gegen Windows-MAX_PATH (260).
/// Der volle, sanitisierte 17-Achsen-binary_id ist ~520+ Zeichen → `source_dir/perm_<id>.cpp` sprengt MAX_PATH →
/// `std::ofstream::open` schlägt still fehl (Befund 2026-06-03: built=0, src-Dir leer). Lösung: ist der sanitisierte
/// Pfad lang, wird er auf ein lesbares Präfix + `_<index>_<fnv1a-hex>` gekürzt (stabil + kollisionsfrei je Pfad).
/// Kurze IDs (z.B. Tests/handbenannte Tiere) bleiben unverändert (rückwärtskompatibel). `kStemMax` lässt Raum für
/// das source_dir-Präfix + ".dll.version"-Suffix unter 260.
inline constexpr std::size_t     kStemMax = 120;
[[nodiscard]] inline std::string orch_make_stem(std::string_view binary_id, std::size_t index) {
    std::string san = orch_sanitize(binary_id);
    if (san.size() <= kStemMax) return san; // kurz genug → unverändert (rückwärtskompatibel)
    std::string       suffix = "_" + std::to_string(index) + "_" + orch_fnv1a_hex(binary_id);
    std::size_t const keep   = (kStemMax > suffix.size()) ? (kStemMax - suffix.size()) : 0;
    return san.substr(0, keep) + suffix; // Präfix (lesbar) + index + Hash (eindeutig)
}

// ── KF-16b: Versions-Sidecar (Inkrement/Resume) ──────────────────────────────
[[nodiscard]] inline std::filesystem::path version_sidecar_path(std::filesystem::path const& output) {
    return std::filesystem::path{output.string() + ".version"};
}
// Inkrementeller Tier-Binary-Cache (Bauplan §1): das ZWEITE, additive Sidecar `<output>.algos` traegt die Organ-
// Provenienz (algo_sig) — bewusst SEPARAT von `.version` (System-Provenienz: ext/cxx/opt/target/ceb), damit Organ-
// und System-Achsen NIE vermischt werden. perm.dll.version bleibt byte-genau unveraendert.
[[nodiscard]] inline std::filesystem::path algo_sidecar_path(std::filesystem::path const& output) {
    return std::filesystem::path{output.string() + ".algos"};
}
/// true, wenn die DLL existiert, ihr `.version`-Sidecar exakt der geforderten System-Version entspricht UND (nur wenn
/// eine algo_sig gefordert ist) ihr `.algos`-Sidecar exakt der erwarteten Organ-Signatur entspricht (→ überspringen).
/// algo_sig LEER = Organ-Gate AUS (rueckwaerts-kompatibel: reiner Versions-Skip, byte-identisch zum Alt-Verhalten).
/// Beide Prueflinge sind reine String-Gleichheit -> additiv, risikoarm; ein fehlendes `.algos` bei gesetzter Erwartung
/// erzwingt Neubau (Alt-Binary vor der Cache-Einfuehrung -> einmal frisch, dann Sidecar vorhanden).
[[nodiscard]] inline bool dll_is_current(std::filesystem::path const& output, std::string const& version,
                                         std::string const& algo_sig = std::string{}) {
    if (version.empty()) return false; // ohne Versions-Anforderung nie überspringen
    std::error_code ec;
    if (!std::filesystem::exists(output, ec)) return false; // DLL fehlt → bauen
    {
        std::ifstream f{version_sidecar_path(output), std::ios::binary};
        if (!f) return false;
        std::string content((std::istreambuf_iterator<char>(f)), {});
        if (content != version) return false; // System-Provenienz (ext/cxx/opt/target/ceb) veraltet → bauen
    }
    if (!algo_sig.empty()) { // Organ-Gate aktiv: die einkompilierten Algorithmus-Versionen muessen ebenfalls passen
        std::ifstream af{algo_sidecar_path(output), std::ios::binary};
        if (!af) return false; // kein Organ-Sidecar → als veraltet behandeln (Neubau schreibt es)
        std::string acontent((std::istreambuf_iterator<char>(af)), {});
        if (acontent != algo_sig) return false; // eine Variante im Tupel wurde gebumpt → GENAU diese Binary neu
    }
    return true;
}
inline void write_version_sidecar(std::filesystem::path const& output, std::string const& version) {
    if (version.empty()) return;
    std::ofstream f{version_sidecar_path(output), std::ios::binary | std::ios::trunc};
    if (f) f << version;
}
/// Schreibt das Organ-Provenienz-Sidecar (`.algos`). Leer = no-op (kein AlgoSigFn injiziert -> byte-neutral). Nur bei
/// erfolgreichem Bau (r.status==0) aufgerufen -> ein Fehlbau hinterlaesst KEIN falsches Organ-Sidecar.
inline void write_algos_sidecar(std::filesystem::path const& output, std::string const& algo_sig) {
    if (algo_sig.empty()) return;
    std::ofstream f{algo_sidecar_path(output), std::ios::binary | std::ios::trunc};
    if (f) f << algo_sig;
}

// G5 (W9.5): dedizierter Rueckgabe-Code des Compile-Wrappers fuer "Compiler-Binary nicht gefunden"
// (posix_spawnp ENOENT). Der Orchestrator klassifiziert ihn als ToolchainFehlt (die geforderte Toolchain
// fehlt -- D1-Domaene, ein klassifizierbarer Bau-Config-Zustand) statt als generischen InfraErrorClass::
// ProzessStart. 126 = POSIX-Konvention "command found but not executable"; g++/clang geben ihn nie selbst
// zurueck -> innerhalb dieses Wrappers eindeutig "Compiler-Binary fehlt". Andere spawn-Fehler bleiben 127/Infra.
inline constexpr int kExitToolchainMissing = 126;

// ── Der Orchestrator ──────────────────────────────────────────────────────────
class BuildOrchestrator {
public:
    BuildOrchestrator(BuildConfig cfg, CompileFn compile, SourceGenFn gen, FreeRamFn free_ram = free_ram_unlimited,
                      AlgoSigFn algo_sig = {})
        : cfg_{std::move(cfg)}, compile_{std::move(compile)}, gen_{std::move(gen)}, free_ram_{std::move(free_ram)},
          algo_sig_{std::move(algo_sig)} {}

    /// W11: den Completion-Hook setzen (BAU-Modus async Push-Feed). VOR provision_all aufrufen; leer = kein Hook
    /// (byte-neutral). Feuert je Binary aus dem Worker-Thread nach results[j]-Finalisierung (Completion-Reihenfolge).
    void set_on_binary_done(BinaryDoneFn fn) { on_binary_done_ = std::move(fn); }

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
    std::vector<BuildResult> provision_all(StaticBinaryView const& view, std::span<const std::size_t> selection,
                                           BuildStats* stats = nullptr) {
        return provision_core(
            view, selection.size(), [selection](std::size_t j) noexcept { return selection[j]; }, stats);
    }

    [[nodiscard]] BuildConfig const& config() const noexcept { return cfg_; }

private:
    /// Gemeinsamer Bau-Kern (KF-16b). Baut K Binaries; `view_index(j)` mappt den selektions-relativen Index j
    /// (0..K-1) auf den View-Index i. results ist O(K). Der „alle"-Pfad (K=view.size(), view_index=identity)
    /// materialisiert KEINEN Index-Vektor; der Selektions-Pfad hält nur die K gewählten Indizes (Aufrufer-seitig).
    template <class IndexMap>
    std::vector<BuildResult> provision_core(StaticBinaryView const& view, std::size_t k, IndexMap view_index,
                                            BuildStats* stats) {
        if (k == 0 || view.empty()) {
            if (stats) *stats = BuildStats{};
            return {};
        }

        std::error_code ec;
        std::filesystem::create_directories(cfg_.source_dir, ec);
        std::filesystem::create_directories(cfg_.output_dir, ec);

        std::vector<BuildResult> results(k); // O(K) — NICHT view.size() (L-73)
        std::atomic<std::size_t> next{0};

        std::mutex              mtx; // schützt active/peak/min_free + CV-Prädikat
        std::condition_variable cv;
        std::size_t             active   = 0;
        std::size_t             peak     = 0;
        std::uint64_t           min_free = (std::numeric_limits<std::uint64_t>::max)();

        // W6 (Ledger §32-F7): die Compile-Worker-Zahl kommt aus effective_build_workers (harter COMDARE_BUILD_PARALLEL-
        // Override, sonst die parallel_jobs()-Heuristik = heute). Die RAM-Admission (unten) throttelt die tatsaechliche
        // Gleichzeitigkeit weiter (bei gesetztem ram_per_build_bytes) -> ein hoher Override bleibt OOM-sicher. Die
        // results[j] werden POSITIONS-TREU befuellt (j = next.fetch_add) -> die Reihenfolge ist worker-zahl-INVARIANT:
        // der Iterator sammelt sie in j-Ordnung ein und feuert die Sinks streng sequenziell (Determinismus-Gate).
        std::size_t const n_workers = cfg_.effective_build_workers(k);
        std::size_t const cores     = cfg_.effective_cores_per_build();
        // RAM-Baseline EINMAL zu Beginn messen (= jetzt frei verfügbar). Die Admission reserviert dagegen
        // (active+1)×Budget — deterministisch + OOM-sicher, unabhängig vom Ramp-Lag des OS-free_ram.
        std::uint64_t const baseline_free = (cfg_.ram_per_build_bytes != 0) ? free_ram_() : 0;

        // S1 (§62-B Log-Flush, Befund 6h-stumm): geflushtes Bau-Fortschritts-Testat je fertiger Binary (zeit-gated,
        // thread-sicher). Rein auf std::cerr -> golden/CSV-NEUTRAL (kein Mess-Datum, kein binary_id-Byte).
        // #27 (2026-07-23): ZUSAETZLICH zaehl-gated alle n_workers Builds (= K = effective_build_workers = COMDARE_BUILD_
        // PARALLEL, lane_build_parallelism beide Lanes 24) -> der Job-Log zeigt "alle K Builds" den Slice-Fortschritt
        // (X/<slice>), auch wenn K Builds schneller als 30s fertig sind. Kombiniert mit dem 30s-Zeit-Gate: was zuerst kommt.
        ProgressHeartbeat build_hb{"tier-build", k, std::cerr, std::chrono::seconds{30}, n_workers};

        // W11: EINE Finalisierungs-Naht je Binary -> results[j] setzen + (falls gesetzt) den Completion-Hook feuern.
        // Feuert aus dem Worker-Thread in COMPLETION-Reihenfolge; der Hook-Konsument ist thread-safe. Leer = byte-neutral.
        auto finalize = [&](std::size_t slot, BuildResult&& res) {
            results[slot] = std::move(res);
            build_hb.tick(); // S1: je fertiger Binary ein (zeit-gated geflushtes) Fortschritts-Testat
            if (on_binary_done_) on_binary_done_(results[slot]);
        };

        auto worker = [&] {
            for (;;) {
                std::size_t const j = next.fetch_add(1); // selektions-relativ
                if (j >= k) return;
                std::size_t const i = view_index(j); // → View-Index

                BuildResult r;
                // Defensiv: ungültiger Selektions-Index → Fehler-Result statt OOB-Dekodierung.
                if (i >= view.size()) {
                    r.index   = i;
                    r.status  = -3;
                    r.message = "selection index out of range";
                    finalize(j, std::move(r));
                    continue;
                }

                BinarySpec const  spec = view[i]; // by value (operator[] dekodiert on-demand)
                std::string const id   = orch_make_stem(spec.binary_id, spec.index); // MAX_PATH-sicher (gekappt+Hash)

                BuildJob job;
                job.index     = spec.index;
                job.binary_id = spec.binary_id;
                // (E): per-Binary-Ordner output_dir/<stem>/ (DLL+Source+.obj+.cl.log+.version teilen ihn) ODER
                // altes flaches Layout (Source in source_dir, DLL in output_dir). Der Stem `id` ist MAX_PATH-sicher.
                // KRITISCH (MAX_PATH): im Unterordner darf der Datei-Name den (langen) Stem NICHT wiederholen —
                // `<stem>/perm_<stem>.dll` würde den Stem DOPPELT in den Pfad legen → >260 Zeichen (ofstream-open
                // schlägt still fehl, .cl.log fehlt; Befund 2026-06-04). Der Ordnername `<stem>` disambiguiert
                // bereits → die Dateien heißen schlicht `perm.cpp`/`perm.dll` (kurz, MAX_PATH-sicher).
                if (cfg_.per_binary_subdirs) {
                    std::error_code             dec;
                    std::filesystem::path const bin_dir = cfg_.output_dir / id;
                    std::filesystem::create_directories(bin_dir, dec);
                    job.source = bin_dir / "perm.cpp";
                    job.output = bin_dir / "perm.dll";
                } else {
                    job.source = cfg_.source_dir / ("perm_" + id + ".cpp");
                    job.output = cfg_.output_dir / ("perm_" + id + ".dll");
                }
                job.cores = cores;

                // Inkrementeller Tier-Binary-Cache (Bauplan §3): die Organ-Algorithmus-Signatur DIESER Binary aus
                // spec.axes (den 17 (achse,wert)-Paaren). Leer, wenn keine AlgoSigFn injiziert ist (Organ-Gate aus).
                std::string const algos = algo_sig_ ? algo_sig_(spec.axes) : std::string{};

                r.index     = spec.index;
                r.binary_id = spec.binary_id;
                r.output    = job.output;
                r.algo_sig  = algos;

                // Section 40.a-E4: flag-genaues Bau-Gate (Pruef-Dock) an der CEB-Bau-Delegation. Aus der Organ-
                // Signatur (spec.axes) wird die per-Binary-Anforderung aggregiert; solange kein Organ required-
                // Flags deklariert (heutiger Stand ALLER Organe), ist sie LEER -> Zulassung trivial -> KEINE
                // Wirkung (byte-/golden-neutral). Aktiviert: Section 37 "Organ <= Maschinen-Signatur" wird HIER
                // durchgesetzt (Verletzung -> D1 HardwareErweiterungFehlt, Log + weiter; kein Compile) -- der
                // per-Perm-Flag-Kanal (Fassade) bleibt unberuehrt, keine Doppelung.
                if (auto const gate_req = ::comdare::cache_engine::measurement::aggregate_required_for_axes(spec.axes);
                    !gate_req.empty()) {
                    if (auto const gate_err = ::comdare::cache_engine::measurement::admit_organ_on_machine(
                            gate_req, ::comdare::cache_engine::measurement::active_machine_signature())) {
                        r.status  = -4; // Gate-Ablehnung: Organ verlangt ein von der Maschine nicht freigegebenes Flag
                        r.message = std::string{"simd-gate: "} +
                                    std::string{::comdare::cache_engine::measurement::error_class_label(*gate_err)};
                        r.outcome = std::unexpected(::comdare::cache_engine::measurement::BuildError{*gate_err});
                        finalize(j, std::move(r));
                        continue; // Log + weiter (baut/misst die uebrigen Binaries)
                    }
                }

                // (A) INKREMENTELL: bestehende, versions- UND organ-aktuelle DLL überspringen (Resume nach Absturz).
                if (dll_is_current(job.output, cfg_.build_version, algos)) {
                    r.status  = 0;
                    r.skipped = true;
                    r.message = "übersprungen (Version aktuell)";
                    finalize(j, std::move(r));
                    continue;
                }

                // (B) RAM-ADMISSION: warten, bis genug freier RAM (mind. 1 Build läuft immer → Fortschritt).
                {
                    std::unique_lock<std::mutex> lk(mtx);
                    cv.wait(lk, [&] {
                        if (cfg_.ram_per_build_bytes == 0) return true; // RAM-Gate aus
                        if (active == 0) return true;                   // Fortschritt garantieren (mind. 1)
                        // Reservierung: die RAM aller gleichzeitigen Builds (inkl. des neuen) darf das
                        // Start-Baseline nicht übersteigen → deterministischer Cap floor(baseline/Budget), kein OOM.
                        if ((active + 1) * cfg_.ram_per_build_bytes + cfg_.ram_safety_margin_bytes > baseline_free)
                            return false;
                        // Dynamische Verschärfung: aktueller freier RAM muss für EINEN weiteren Build reichen.
                        return free_ram_() >= cfg_.ram_per_build_bytes + cfg_.ram_safety_margin_bytes;
                    });
                    ++active;
                    peak                   = std::max(peak, active);
                    std::uint64_t const fr = free_ram_();
                    if (fr < min_free) min_free = fr;
                }

                // (C) Source generieren (KF-8) + kompilieren — OHNE Lock (echt parallel).
                {
                    std::ofstream f{job.source, std::ios::binary | std::ios::trunc};
                    if (!f) {
                        r.status  = -2;
                        r.message = "Quelle nicht schreibbar";
                    } else {
                        f << gen_(spec.binary_id);
                    }
                }
                if (r.status != -2) {
                    r.status  = compile_(job);
                    r.message = (r.status == 0) ? "ok" : ("compile-exit " + std::to_string(r.status));
                    // d1-carrier + INC-29.2: den rohen Exit-Code in die richtige Fehler-DOMAENE uebersetzen
                    // (Erfolg = has_value). 127=spawn/argv (Prozess-Start), 125=rsp-IO, <0=Signal/Abbruch =>
                    // INFRA (kein Compiler-Urteil, NIE als D1 fehletikettieren, Sweep-Fix); sonst nonzero =
                    // vom Compiler abgelehnte Achsen-Kombination (D1). Der Iterator liest r.outcome +
                    // error_domain() fuer die richtige Log-Zeile ([Infra-Fehler:…] vs [Compiler-Compiler-Fehler:…]).
                    namespace cm = ::comdare::cache_engine::measurement;
                    if (r.status == 0)
                        r.outcome = {};
                    else if (r.status == kExitToolchainMissing)
                        // G5 (W9.5): Compiler-Binary nicht auffindbar (ENOENT an der spawn-Naht) -> die geforderte
                        // Toolchain fehlt. Eigene D1-Klasse ToolchainFehlt (NICHT der generische Infra-ProzessStart,
                        // NICHT CompileKombination = kein Compiler-Urteil ueber die Achsen-Kombination). Der Sweep
                        // misst die uebrigen Permutationen weiter (honest-weiter, kein Abbruch).
                        r.outcome = std::unexpected(cm::BuildError{cm::CompilerCompilerErrorClass::ToolchainFehlt});
                    else if (r.status == 127)
                        r.outcome = std::unexpected(cm::BuildError{cm::InfraErrorClass::ProzessStart});
                    else if (r.status == 125)
                        r.outcome = std::unexpected(cm::BuildError{cm::InfraErrorClass::ArtefaktIo});
                    else if (r.status < 0 || r.status >= 128)
                        // NACH-Prüfung-Fix: Signal-Abbruch. decode_process_status liefert 128+WTERMSIG POSITIV
                        // (137=SIGKILL/OOM-Killer im RAM-Druck-Parallelbau, 139=SIGSEGV, 134=SIGABRT) — das ist
                        // ein Prozess-Abbruch (INFRA), NIE ein Compiler-Urteil. (r.status<0 deckt zusätzlich die
                        // Orchestrator-Sentinels.) Vorher fielen 128+sig fälschlich in den D1-else (Sweep-Rüge).
                        r.outcome = std::unexpected(cm::BuildError{cm::InfraErrorClass::ProzessAbbruch});
                    else
                        r.outcome = std::unexpected(cm::BuildError{cm::CompilerCompilerErrorClass::CompileKombination});
                    if (r.status == 0) {
                        write_version_sidecar(job.output, cfg_.build_version); // System-Provenienz-Resume-Marke
                        write_algos_sidecar(job.output, algos); // Organ-Provenienz (Bauplan §1); leer=no-op
                    }
                }
                finalize(j, std::move(r));

                {
                    std::lock_guard<std::mutex> lk(mtx);
                    --active;
                }
                cv.notify_all();
            }
        };

        { // Thread-Pool; jthread-Destruktor joint am Blockende
            std::vector<std::jthread> pool;
            pool.reserve(n_workers);
            for (std::size_t w = 0; w < n_workers; ++w) pool.emplace_back(worker);
        }
        build_hb.done(); // S1: Bau-Phase abgeschlossen (nach dem jthread-Join) -- genau eine geflushte Abschluss-Zeile

        if (stats) {
            stats->total_jobs         = results.size();
            stats->peak_concurrency   = peak;
            stats->min_free_ram_bytes = min_free;
            stats->succeeded = stats->failed = stats->skipped = stats->built = 0;
            for (auto const& r : results) {
                (r.ok() ? stats->succeeded : stats->failed)++;
                if (r.skipped)
                    ++stats->skipped;
                else if (r.ok())
                    ++stats->built;
            }
        }
        return results;
    }

    BuildConfig  cfg_;
    CompileFn    compile_;
    SourceGenFn  gen_;
    FreeRamFn    free_ram_;
    AlgoSigFn    algo_sig_;       // Bauplan §3: spec.axes → algo_sig; leer = Organ-Gate aus (byte-neutral)
    BinaryDoneFn on_binary_done_; // W11: per-Binary Completion-Hook (BAU-Modus async Push-Feed); leer = byte-neutral
};

namespace detail {

[[nodiscard]] inline int decode_process_status(int status) noexcept {
#ifdef _WIN32
    return status;
#else
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return status;
#endif
}

[[nodiscard]] inline bool shell_safe_token(std::string_view s) noexcept {
    if (s.empty()) return false;
    for (char const c : s) {
        unsigned char const uc = static_cast<unsigned char>(c);
        // '+' fuer Compiler-Namen wie g++-16; ':' und '\\' bleiben bewusst draussen (Injection-Schutz;
        // der _WIN32-Zweig ist damit de facto auf den MSVC-Pfad make_system_compile_fn verwiesen).
        if (std::isalnum(uc) || c == '_' || c == '.' || c == '/' || c == '-' || c == '@' || c == '+') continue;
        return false;
    }
    return true;
}

[[nodiscard]] inline int run_argv_redirected(std::vector<std::string> const& argv, std::filesystem::path const& log) {
    if (argv.empty()) return 127;
#ifdef _WIN32
    // Der E4-g++-Pfad ist POSIX. Diese Fallback-Route bleibt shell-basiert, aber
    // nur fuer strikt validierte Tokens aktiv.
    for (auto const& arg : argv) {
        if (!shell_safe_token(arg)) {
            std::ofstream lf{log, std::ios::app};
            lf << "ungueltiges Shell-Token: " << arg << "\n";
            return 127;
        }
    }
    std::string cmd;
    for (auto const& arg : argv) {
        if (!cmd.empty()) cmd += ' ';
        cmd += '"' + arg + '"';
    }
    cmd += " > \"" + log.string() + "\" 2>&1";
    return decode_process_status(std::system(cmd.c_str()));
#else
    std::string const log_s = log.string();

    posix_spawn_file_actions_t actions;
    int                        rc = posix_spawn_file_actions_init(&actions);
    if (rc != 0) return 127;

    rc = posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, log_s.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (rc == 0) rc = posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO);
    if (rc != 0) {
        posix_spawn_file_actions_destroy(&actions);
        std::ofstream lf{log, std::ios::app};
        lf << "posix_spawn_file_actions fehlgeschlagen: " << std::strerror(rc) << "\n";
        return 127;
    }

    std::vector<char*> c_argv;
    c_argv.reserve(argv.size() + 1);
    for (auto const& arg : argv) c_argv.push_back(const_cast<char*>(arg.c_str()));
    c_argv.push_back(nullptr);

    pid_t pid = 0;
    rc        = posix_spawnp(&pid, argv.front().c_str(), &actions, nullptr, c_argv.data(), ::environ);
    posix_spawn_file_actions_destroy(&actions);
    if (rc != 0) {
        std::ofstream lf{log, std::ios::app};
        lf << "posix_spawnp(" << argv.front() << ") fehlgeschlagen: " << std::strerror(rc) << "\n";
        // G5 (W9.5): ENOENT = das Compiler-Binary existiert nicht -> die Toolchain fehlt (eigener Code, den der
        // Orchestrator als ToolchainFehlt klassifiziert). Andere spawn-Fehler (EACCES/ENOMEM/...) bleiben
        // generischer Prozess-Start (127, Infra-Domaene, NIE als Compiler-Urteil fehletikettiert).
        return (rc == ENOENT) ? kExitToolchainMissing : 127;
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) continue;
        std::ofstream lf{log, std::ios::app};
        lf << "waitpid(" << pid << ") fehlgeschlagen: " << std::strerror(errno) << "\n";
        return 127;
    }
    return decode_process_status(status);
#endif
}

} // namespace detail

/// Scheibe 2b (Ledger 61/62, §62-G (4)) -- Build-Typ-Debug-Flags, TOOLCHAIN-abstrahiert (NICHT an einer
/// Linux-Stelle hartkodiert). Fuer die gesamte g++/clang-Toolchain-Familie der Projekt-Doktrin gilt
/// universal "-O0 -g" (kein Optimierer + volle Debug-Info):
///   - g++-16 auf den 8 Docker-Distro-Baendern (Linux-Plattform-Doktrin),
///   - MSYS2/MinGW-g++ auf Windows 11 + Windows Server,
///   - clang bzw. g++ auf macOS x86_64 UND ARM64.
/// Der Debug-Zweck ist "DASS es funktioniert" + schnellerer DLL-Bau fuer den Verdrahtungs-Check
/// (§61-MODI), NICHT Mess-Korrektheit. Die PERM-Identitaet [d,e,f] bleibt im Stempel O3 (Perm-Kennung);
/// die tatsaechliche Compile-Einstellung traegt der (i)-+bt=Debug-Stempel (Bruecke #49/K7b: immer-
/// expliziter Compile-Stempel je Binary). Heute EIN Pfad, aber benannte Naht: ein kuenftiger MSVC-Pfad
/// (cl: /Od /Zi) dockt ueber die Compiler-System-Achse am make_system_compile_fn-Spiegel an.
[[nodiscard]] inline std::string debug_flags_for_toolchain() { return "-O0 -g"; }

/// Default-CompileFn: realer MSVC-Subprozess, baut perm_<id>.cpp → perm_<id>.dll (SHARED). {cores} → /MP<cores>.
/// Host-Werkzeug (ruft cl via std::system; cl muss im PATH/Env sein, z.B. vcvars64). Ausgabe unterdrückt.
[[nodiscard]] inline CompileFn make_system_compile_fn(std::vector<std::string> include_dirs = {},
                                                      std::string              std_flag     = "/std:c++latest",
                                                      std::string              opt_flag     = "/O2") {
    // Bau-INC-2c.opt-c: MSVC-Spiegel des POSIX-opt_flag-Kanals. cl defaultet SONST still auf /Od
    // (kein -O2-Aequivalent) -> die opt_level-Unter-Achse (msvc_opt_flag(): /Od,/O1,/O2) waere unter
    // cl ein toter Accessor. Default "/O2" = Symmetrie zum g++-Default -O2. POSIX-first (Cluster=Linux):
    // kein aktiver cl-Aufrufer, rein additive Symmetrie; die Facade-Verdrahtung folgt am Windows-Track.
    return [include_dirs = std::move(include_dirs), std_flag = std::move(std_flag),
            opt_flag = std::move(opt_flag)](BuildJob const& job) -> int {
        std::string cmd = "cl /nologo " + std_flag + " " + opt_flag + " /EHsc /LD /MP" + std::to_string(job.cores);
        for (auto const& inc : include_dirs) cmd += " /I\"" + inc + "\"";
        cmd += " \"" + job.source.string() + "\"";
        cmd += " /Fe:\"" + job.output.string() + "\"";
        cmd += " /Fo:\"" + job.output.string() + ".obj\"";
        cmd += " > nul 2>&1";
        return std::system(cmd.c_str());
    };
}

/// POSIX-CompileFn: realer g++-Subprozess, baut perm_<id>.cpp -> perm_<id>.so (SHARED).
/// Nutzt @rsp und posix_spawnp(argv), also keinen /bin/sh-String; der wait-status wird
/// auf den tatsaechlichen Prozess-Exitcode dekodiert.
[[nodiscard]] inline CompileFn make_gpp_compile_fn(std::vector<std::string> include_dirs = {},
                                                   std::vector<std::string> defines = {}, std::string cxx = "g++-16",
                                                   std::vector<std::string> link_libs = {},
                                                   std::string opt_flag = "-O2", bool emit_fno_gnu_unique = true) {
    // Bau-INC-2c.opt-b: opt_flag = der volle Optimierungs-Flag-String (Konvention aus opt-a:
    // OptO*Option::gcc_opt_flag() liefert "-O2"/"-O3"/"-Ofast"). Der Signatur-Default "-O2" ist ein
    // TRANSITIONALER, achsen-blinder Builder-Fallback fuer Direkt-Aufrufer, die (noch) nichts setzen —
    // NICHT der CEB-Default und KEIN Pin. Der bewegliche CEB-Default ist O3 (Ruling 2026-07-18, Option B,
    // DefaultOptLevelOption=OptO3Option); die Facade (profile_run_facade active_opt_level) sourct ihn und
    // reicht ihn hier als opt_flag runter. Ein harter O3-Signatur-Default hier waere selbst ein neuer Pin —
    // daher bleibt der transitional-ueberschreibbare "-O2" stehen (der Facade-Wert gewinnt immer).
    return [include_dirs = std::move(include_dirs), defines = std::move(defines), cxx = std::move(cxx),
            link_libs = std::move(link_libs), opt_flag = std::move(opt_flag),
            emit_fno_gnu_unique](BuildJob const& job) -> int {
        std::filesystem::path const rsp = job.output.string() + ".rsp";
        {
            std::ofstream rf{rsp};
            if (!rf) return 125;
            rf << "-std=c++23\n";
            rf << opt_flag << "\n"; // opt-b: war hart "-O2"; Default-opt_flag=="-O2" => byte-identisch
            rf << "-fPIC\n";
            rf << "-shared\n";
            // Compiler-Dialekt-Gate (opt-d, A2-Hybrid Teil 2): -fno-gnu-unique ist GNU-only — clang bricht mit
            // "unknown argument". Die WISSENS-QUELLE ist die Compiler-System-Achse (CompilerSystemAxis::
            // supports_fno_gnu_unique(): Gcc=true, Clang=false); der achsen-blinde Builder empfaengt sie als
            // vom Facade/Planer gesteuerten bool-WERT (Muster (2), keine String-Sniff-Heuristik mehr hier).
            if (emit_fno_gnu_unique) rf << "-fno-gnu-unique\n";
            rf << "-fdiagnostics-color=never\n";
            for (auto const& d : defines) rf << d << "\n";
            for (auto const& i : include_dirs) rf << "-I\"" << i << "\"\n";
            rf << "\"" << job.source.string() << "\"\n";
            // Archive MUESSEN nach der Quelle stehen: ld scannt statische Archive left-to-right und
            // zieht Member nur fuer bereits offene undefined-Referenzen (sonst bleiben mi_* ungeloest).
            for (auto const& l : link_libs) rf << "\"" << l << "\"\n";
            rf << "-o \"" << job.output.string() << "\"\n";
        }

        std::filesystem::path const log = job.output.string() + ".cxx.log";
        return detail::run_argv_redirected({cxx, "@" + rsp.string()}, log);
    };
}

} // namespace comdare::cache_engine::builder::experiment
