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
#include "../bvset_teilmenge.hpp"  // Task #59: bvset_ist_teilmenge (registry-frei, kennt nur die Grammatik)
#include "fingerprint_sidecar.hpp" // G4b-1/AUF-A2: fingerprint_sidecar_path (hierher extrahiert, Single Source)
#include <cache_engine/measurement/axis_error.hpp> // opt-d/d1-carrier: CompilerCompilerErrorClass (A2-Hybrid Teil 1)
#include <cache_engine/measurement/algo_stempel_zulassung.hpp> // S-7: Achsen-Algo-Hardware-Stempel-Zulassung (KON9-05)
#include <cache_engine/measurement/simd_build_gate.hpp>        // Section 40.a-E4: flag-genaues Bau-Gate (Pruef-Dock)
#include <cache_engine/measurement/simd_organ_requirement.hpp> // Section 40.a-E4: per-Binary organ_required-Aggregation

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cctype>
#include <chrono> // A5/F5 Baupunkt (1): per-Binary-Compile-Dauer (steady_clock um den Compiler-Aufruf)
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
#include <optional> // A5/F5 Baupunkt (1): dauer_s ist optional -- "nicht gemessen" ist keine 0
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
    std::string   build_version{};             // Versions-/Anforderungs-Signatur; leer = nie ueberspringen
    // G2-3 (Lager-Gate A7): Zell-/ISA-konstante Build-Varianten-Signatur (build_variant_sidecar.hpp::
    // compose_variant_signature aus dem BuildVariantDefinitionV1-POD). Leer = Variant-Gate AUS (byte-neutral,
    // reiner Versions-/Organ-Skip wie bisher); die CEB/Facade fuellt sie in der spaeteren Integrations-Scheibe.
    // {} wie bei allen optionalen Feldern dieses Structs (= 4, = 0, = false): GCCs
    // -Wmissing-field-initializers meldet nur Felder OHNE Default-Member-Initialisierer --
    // die zwei Strings waren die einzigen ohne, obwohl "leer = Gate AUS" ihr dokumentierter
    // Default ist. Semantisch identisch (Aggregat-Auslassung wertinitialisiert ohnehin).
    std::string build_variant_sig{};
    // Task #59 (Additiv-Vertrag GLIED [6]) -- ZWEI GETRENNTE FELDER, obwohl heute derselbe String hineingeht.
    // Sie beantworten verschiedene Fragen und duerfen deshalb nicht zu einem verschmelzen:
    //   current_bvset_glied  = das SOLL des Skip-Gates ("was fuehrt der Treiber HEUTE?").
    //   sidecar_bvset_glied  = was beim Neubau als Zeile 2 neben die Binary geschrieben wird ("was traegt
    //                          GENAU DIESE Binary?").
    // Heute sind beide die Treiber-Signatur, weil [6] run-konstant ist. Wuerde [6] je per Permutation
    // gebildet, waere das SOLL weiter run-konstant und die Aufzeichnung per Binary -- ein gemeinsames Feld
    // muesste dann getrennt werden, und zwar an einer Stelle, an der beide Bedeutungen schon vermischt sind.
    // Leer = das jeweilige Verhalten von vorher (kein Teilmengen-Pfad / v1-Sidecar), byte-neutral.
    std::string current_bvset_glied{};
    std::string sidecar_bvset_glied{};
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
    std::string algo_sig;
    // T2-A/K2-NB (Codex-Scope-K2, Haertung (d) "EINMAL lesen"): der 128-hex-Fingerprint, den provision_core
    // fuer DIESE Binary EINMAL beim Provider geholt hat -- derselbe Wert, den (A) als Skip-Erwartung
    // verglichen und den der Erfolgszweig als `.fingerprint`-Sidecar geschrieben hat.
    //
    // WARUM ER MITREIST (und der Mess-Pfad den Provider nicht ein zweites Mal fragt): das DLL-Gate und der
    // Mess-Resume-Stamp riefen cfg.bestand_fingerprint_fn GETRENNT auf. Dieselbe std::function garantiert
    // keinen identischen Rueckgabewert -- ein zustandsabhaengiger Provider koennte mit X pruefen und mit Y
    // stempeln, und der Stamp bezeugte dann eine Identitaet, die das Gate nie gesehen hat. Muster identisch
    // zu algo_sig darueber: waehrend des Baus EINMAL erhoben, im Ergebnis getragen, vom Mess-Pfad
    // konsumiert. Leer, wenn kein Provider injiziert ist (byte-neutral) ODER wenn der Job vor dem
    // Provider-Aufruf ausgeschieden ist (Gate-Ablehnung) -- dann ist r.ok() false und der Mess-Pfad
    // erreicht den Stamp ohnehin nicht.
    std::string fingerprint;
    // A5/F5 Baupunkt (1) -- PER-BINARY-TIMING. Die Wanduhr-Dauer GENAU des externen Compiler-Aufrufs
    // (compile_(job)), wie die F5-Spez sie verlangt ("Messung um den externen Compiler-Aufruf").
    //
    // WARUM optional UND NICHT double: ein Job, der gar nicht kompiliert hat -- Sidecar-Skip
    // (dll_is_current), Gate-Ablehnung, unschreibbare Quelle -- hat KEINE Dauer. Eine 0 waere hier eine
    // erfundene Zahl, und sie wuerde als "unendlich schneller Compile" in jeden Mittelwert und jeden
    // Median einwandern. nullopt heisst "nicht gemessen"; der Kalibrier-Konsument kann den Filter damit
    // nicht vergessen (EtaKalibrierung::beobachte bekommt den Wert gar nicht erst zu sehen).
    //
    // WAS BEWUSST NICHT DRINSTECKT: die RAM-Admission-Wartezeit (Block (B)). Sie ist Warteschlangen-Zeit,
    // keine Arbeit. Sie mitzumessen wuerde die Parallelitaet DOPPELT buchen -- die ETA-Formel
    // Sum(t_i)/N_threads modelliert die Serialisierung bereits selbst.
    std::optional<double> dauer_s;
    [[nodiscard]] bool    ok() const noexcept { return status == 0; }
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
// I2 (Lager-Gate Integration): binary_id -> 128-hex K7b-Fingerprint (SHA-512 der vier gerenderten Stempel-Zeilen,
// identisch zu dem, den die DLL via comdare_anatomy_version_lines()->sha512_line traegt). Die Facade komponiert ihn
// aus DENSELBEN Zeilen wie der Emitter (compose_organ_stamp_line + system/measurement/merge) -> drift-frei. Leer/
// ungesetzt = kein .fingerprint-Sidecar (byte-neutral). Der achsen-blinde Orchestrator bleibt load-/registry-frei.
using FingerprintFn = std::function<std::string(std::string const&)>;
// Task #59 (Additiv-Vertrag GLIED [6]): (binary_id, AUFGEZEICHNETES bvset) -> 128-hex. Der ZWEITE Provider ist
// noetig, weil FingerprintFn per Konstruktion immer mit dem HEUTIGEN bvset rechnet -- er kann die Frage "welchen
// Hash haette diese Binary mit der damals gefuehrten Enable-Menge?" gar nicht stellen. Genau die beantwortet die
// Bindungs-Pruefung von dll_is_current. KEINE ZWEIT-ABLEITUNG: die Facade baut ihn aus DERSELBEN
// lazy_adhoc_fingerprint_for-Komposition wie den Bestands-Provider, nur mit dem bvset als Parameter statt als
// vorab aufgeloester Konstante. Leer/ungesetzt = Teilmengen-Pfad AUS (byte-neutral).
using BvsetFingerprintFn = std::function<std::string(std::string const&, std::string const&)>;
// S-7 (KON9-05, 2026-08-13): (achse, wert) -> algo_version-Literal. Der Provider traegt die Versions-Welt in den
// achsen-blinden Orchestrator, OHNE dass der die Registries kennt (die Facade baut ihn EINMAL je Lauf aus
// build_axis_variant_version_table; die Versionen reisen NUR ueber diese Funktion -- kein Registry-Include hier).
// PROVIDER-KONTRAKT:
//   * LEERER String == "kein bekannter Versions-Stand fuer dieses Paar" -- im Gate NEUTRAL uebersprungen. Das
//     deckt Achsen ohne Versions-Traegerschaft (Sub-Achsen-Ebenen "cacheline.*"/"node_width.*"/"alloc_hw.*",
//     profile_to_tree.hpp:104-125, "tier") UND profil-eigene Werte ausserhalb der Enabled-Registry (gemessener
//     Bestandsfall: planner_thesis_min permutiert search_algo=bplus; der .algos-Pfad stempelt dort den Sentinel
//     als reine PROVENIENZ und baut trotzdem, compose_algo_signature :269/:291). Exakt die Behandlung, die
//     required_of unbekannten Klassen und compose_algo_signature Nicht-Slots gibt: eine Version, die niemand
//     deklariert hat, FORDERT nichts.
//   * Ein NICHT-LEERES Literal ist eine BEHAUPTUNG des Providers und wird geurteilt: unparsbare/Sentinel-Formen
//     ("0.0.0") parsen auf den K-5-Sentinel und fallen im Gate FAIL-CLOSED (HardwareErweiterungFehlt, geerbt
//     aus flag_menge_in_signatur:103). Die Drift-Wache "jede registrierte Variante traegt eine parsbare
//     Version" lebt NICHT hier, sondern compile-hart am Tabellen-Bau (W::algo_version-Zugriff +
//     assert_version_grammar + guard_all_registered_organ_versions).
// Leer/ungesetzt = Bruecke AUS (byte-neutral, exakt das Verhalten von vorher) -- gleiches opt-in-Muster wie
// set_bvset_fingerprint_provider.
using AlgoVersionFn = std::function<std::string(std::string const&, std::string const&)>;
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
// G2-3 (Lager-Gate A7): das DRITTE, additive Sidecar `<output>.variant` traegt die Build-Varianten-Signatur (page_type/
// simd_extension/general_hardware = Zell-/ISA-Ebene, build_variant_sidecar.hpp::compose_variant_signature) -- bewusst
// SEPARAT von `.version` (System-Provenienz) und `.algos` (Organ-Provenienz). Trennungs-Doktrin :221-223 fortgefuehrt;
// perm.dll.version bleibt byte-genau unveraendert (rueckwaerts-kompatibel).
[[nodiscard]] inline std::filesystem::path variant_sidecar_path(std::filesystem::path const& output) {
    return std::filesystem::path{output.string() + ".variant"};
}
/// A2-SHA512-ONLY-SKIP-GATE (F7, KONSOLIDIERT:72) -- GEEICHT am 2026-08-05 auf Basis 24e07219 (GATE 5, der EINE
/// Anker-Vollzug, GENAU EINMAL). DER EINE VERGLEICH: die Binary ist genau dann aktuell, wenn ihr `.fingerprint`-
/// Sidecar byte-gleich dem erwarteten CT-Fingerprint ihrer Bau-Identitaet ist. Owner-Wort: "Skip-Pruefung NUR gegen
/// die komplex geplante SHA512-Validierung -- sie deckt ALLE Stempel allein ('das war der Sinn des SHA512')."
///
/// FAIL-CLOSED in allen vier Zweifelsfaellen (Uebergangsregel F7: "kein .fingerprint => nicht aktuell => Neubau";
/// KEIN Grandfathering, KEINE Alt-Key-Uebersetzung):
///   1. expected_fingerprint leer  -- ohne CT-Erwartung wird NIE uebersprungen (kein Provider = ehrlicher Neubau),
///   2. DLL fehlt                  -- ein Sidecar ohne Binary skippt nie (Phantom-Schutz, vgl. prunable_artifacts),
///   3. Sidecar fehlt/leer         -- Alt-/Fremd-Bestand ohne Anker,
///   4. Sidecar nicht 128-hex      -- Formverstoss (dieselbe Wache wie bestand_key_of; read_fingerprint_sidecar).
///
/// EINE SCHLUESSEL-WELT (der eigentliche Zweck der Eichung): expected kommt aus DERSELBEN Quelle, die auch
/// write_fingerprint_sidecar speist (FingerprintFn, provision_core berechnet ihn EINMAL je Job) und der Leser ist
/// DERSELBE wie der des Lager-Binders (read_fingerprint_sidecar). Damit gilt im CODE, nicht per Disziplin:
/// Skip-Gate == .fingerprint-Inhalt == minio-Objekt-Key == Bestandslog key_sha512 == Baum-Blatt-Identitaet.
///
/// L14 (deklarierte, NICHT stille Luecke): geeicht wurde MIT LEEREM Overlay-Glied -- das Overlay-Glied
/// (COMDARE_OVERLAY_SOURCE_HASH) traegt heute nur Separator+Format. Der SHA512 deckt
/// damit reine Quell-Code-Aenderungen (ABNAHME-3/4-Voll-Soll) NOCH NICHT; geheilt wird das im Overlay-Fenster
/// (Phase 6), und zwar layout-bruch-frei. Der geeichte Referenz-Vektor ist kFrozenFingerprintV1
/// (test_g3_sha512_index.cpp, identisch in test_w10_system_cell_values.cpp und test_m_w12_stamp_bausteine.cpp).
///
/// [NACHGEFUEHRT 2026-08-05, O-2/C-2 -- DER GLIED-SATZ, GEGEN DEN DIESES GATE VERGLEICHT:] das Preimage traegt
/// seit fingerprint_format=3 ACHT Glieder (abi::anatomy_fingerprint_glieder ist unveraendert die EINE Quelle):
///   [0] Format-Kennung  [1] Organ-Zeile  [2] System-Zeile  [3] Mess-Tooling-Zeile  [4] Sub-Achsen-Werteset
///   [5] TOOLCHAIN-Glied (Compiler-Haupt-Achse inkl. Flags, opt_level, atomic128, ext/bt/gate/ceb -- heilt C1)
///   [6] BVSET-Glied (Enabled-Mengen-Signatur der Build-Achsen -- heilt C6)
///   [7] Overlay-Source-Hash (weiter LEER, s. L14 oben; ans Ende gewandert)
/// Der Overlay-Absatz oben bleibt unveraendert gueltig -- nur seine Positions-Angabe ("das 6. Glied") war an
/// die Format-2-Ordnung gebunden. Die per-Perm-BEFUELLUNG der beiden neuen Glieder ist die Folge-Scheibe C-3;
/// bis dahin sind sie leer und dieses Gate verhaelt sich unveraendert, ausser dass der Format-Bump den
/// gesamten Alt-Bestand EINMAL fail-closed neu bauen laesst (F7-Uebergangsregel, kein Grandfathering).
/// [NACHGEFUEHRT 2026-08-06, T2-B/T2-C (C-4-Rest) -- DER SATZ "bis dahin sind sie leer" IST HISTORIK. Beide
/// Glieder tragen jetzt Werte, und zwar mit zwei Zusagen, die dieses Gate direkt betreffen:
///   [5] wird PER PERMUTATION gebildet (opt inkl. der aufgeloesten Flags, ext, gate, atomic128, dazu die am
///       Tier-Treiber ERHOBENE Realversion, T2-C) und gleichzeitig in den Bau-Kanal und den CEB-Laufzeit-
///       Zwilling gereicht -- aus EINEM Aufruf, damit beide Seiten nicht driften koennen. Zwei
///       Permutationen derselben Zelle mit anderem opt/ext haben ab hier verschiedene Fingerprints; der
///       Fall "O3 skippt auf die O2-DLL" aus dem Kopf dieses Absatzes ist damit geschlossen.
///   [6] ist RUN-KONSTANT (Enabled-Menge des Treibers, keine Perm-Eigenschaft) und war mit NB/CX-4 bereits
///       vollstaendig.
/// EINE NEUE, HIER RELEVANTE REGEL kommt mit T2-C dazu: ist die REALVERSION des Tier-Treibers nicht
/// erhebbar, wird KEIN Fingerprint-Provider gestellt -- dann ist expected_fingerprint leer, und dieses Gate
/// gibt per Punkt (1) seiner eigenen Regel IMMER false zurueck. Eine unbestimmte Identitaet traegt keinen
/// Skip; der Lauf baut ehrlich neu, statt zu raten.]
///
/// HISTORIK (Stand bis zur Eichung, NICHT geloescht -- Doku-Doktrin): bis 2026-08-05 lautete die Regel "true, wenn
/// die DLL existiert, ihr `.version`-Sidecar exakt der geforderten System-Version entspricht UND (nur wenn eine
/// algo_sig gefordert ist) ihr `.algos`-Sidecar ... UND (nur wenn eine variant_sig gefordert ist) ihr `.variant`-
/// Sidecar ... entspricht; algo_sig/variant_sig LEER = das jeweilige Gate AUS". Diese Dreifach-String-Gleichheit
/// ist mit F7 ("NUR") ERSETZT, nicht ergaenzt: .version/.algos/.variant werden weiter GESCHRIEBEN (Provenienz-
/// Legende bzw. Transport-Vollstaendigkeits-Marke von push_tier_binary), entscheiden aber ueber KEINEN Skip mehr.
///
/// W10-C5 -- ENDE DER UEBERGANGSREGEL "Skip nur bei gleicher OS-Familie" (Vollzugs-Vermerk). Diese Regel war
/// NIE Code: sie lebte deklarativ im Bauplan und war implizit nur ueber die Datei-Plattform-Suffixe der Loader
/// und die faktisch linux-only-Flotte gedeckt. Seit W10-C4 traegt jeder DEFINIERT gebaute Tier-Stempel die
/// Bau-ZELLE (OS-Familie + ISA + simd) IM Fingerprint -- linux-, macos- und aarch64-Baue derselben Permutation
/// haben beweisbar verschiedene SHA512. Fuer NEUBAUTEN ist die Regel damit gegenstandslos; ein Code-Rueckbau
/// entfaellt, weil nie Code existierte. Fuer den STALEN Bestand deckt der W10-M2-Marker: prae-W10-Binaries
/// passieren dieses Gate nicht mehr (Einzel-Pfad '+ceb=7.1' vs '+ceb=7.2'; Perm-Pfad 'kein +ceb' vs '+ceb=7.2'
/// nach der C4-Verdrahtung) -- das ist der Grund, warum der Contract-Minor im selben Commit gewandert ist.
/// E-24-C8-NACHZUG (Werte-Stand, damit dieser Absatz nicht als LEBENDE Aussage falsch dasteht): die vier
/// Zahlen oben sind der W10-Stand und bleiben als solcher stehen. Seit E-24 C8 lautet der lebende Wert
/// '+ceb=8.0' (Major 7->8 mit Minor-RESET 2->0) -- der stale Bestand faellt damit ERST RECHT aus dem Gate,
/// und zwar an beiden Pfaden. Der verbindliche Wert steht nirgends hier, sondern in
/// anatomy_module_abi_v1_decl.hpp (COMDARE_ANATOMY_ABI_MAJOR + kCebContractCodegenMinor).
/// K1-CROSS-CHECK -- VOLLZUGS-VERMERK (A2-Eichung 2026-08-05): der K1-Cross-Check (OS-Familie aus dem
/// .version-Sidecar) war NIE eigener Code -- er WAR die .version-Gleichheit selbst und ist mit ihr ersatzlos
/// entfallen. Er war ausdruecklich nur "ZWEITE Verteidigungslinie ..., bis das A2-SHA512-only-Gate geeicht ist";
/// diese Bedingung ist mit dieser Zeile eingetreten. Sein Gegenstand (Define-Verkabelungsfehler) liegt seit
/// W10-C4 im Fingerprint-Glied [2] selbst: die Bau-ZELLE (OS-Familie + ISA + simd) steht IM Preimage, ein
/// verkabelungsfalscher Bau hat damit einen anderen SHA512 und faellt an genau diesem einen Vergleich durch.
///
/// [NACHGEFUEHRT 2026-08-11, Task #59 -- DER SATZ "DER EINE VERGLEICH" IST HISTORIK.] Der Absatz oben
/// beschreibt weiter richtig, WAS verglichen wird (der Fingerprint, nicht .version/.algos/.variant), aber
/// seine Relation war fuer das Glied [6] die falsche. Gleichheit ist eine symmetrische Relation; der
/// Vertrag, den sie durchsetzen soll, ist es nicht:
///
///   ERWEITERUNG   der Treiber fuehrt heute MEHR Achsen-Varianten als beim Bau der vorhandenen Binary.
///                 Die Binary bleibt gueltig -- sie wurde aus einer TEILMENGE der heutigen Faehigkeiten
///                 gebaut. Unter Gleichheit fiel sie durch: die gesamte Flotte wurde neu gebaut, weil
///                 IRGENDWO eine Variante hinzukam, die die meisten Binaries nie beruehrt haetten.
///   EINSCHRAENKUNG der Treiber fuehrt WENIGER. Die Binary ist NICHT mehr gueltig.
///
/// Beide Faelle waren ununterscheidbar. Das ist der Defekt, den diese Ueberladung heilt, und zwar an der
/// Relation -- nicht an der Glied-Natur ([6] bleibt RUN-KONSTANT) und nicht an der Glied-Ordnung
/// (kAnatomyFingerprintGliedCount bleibt 9, das bvset bleibt im SHA-512-Preimage).
///
/// DIE ZWEI ZUSICHERUNGEN, DIE ZUSAMMEN GELTEN MUESSEN -- eine allein waere ein Loch:
///   (a) TEILMENGE: die aufgezeichnete Menge steckt in der heutigen (bvset_teilmenge.hpp).
///   (b) BINDUNG:   der aufgezeichnete Hash ist NACHWEISLICH ueber genau diese aufgezeichnete Menge
///                  gerechnet -- geprueft per Neuberechnung mit dem aufgezeichneten bvset. Ohne (b) waere
///                  Zeile 2 des Sidecars eine unbeglaubigte Behauptung: wer sie um ein Element erweitert,
///                  bekaeme jede beliebige Binary als "Teilmenge" durchgewunken, waehrend die Binary
///                  selbst unveraendert die alte Menge traegt. (a) prueft die Richtung, (b) prueft, dass
///                  ueberhaupt die richtige Menge geprueft wird.
///
/// ZIRKULARITAETS-VERBOT, PRAEZISIERT (driver_build_variant_signature.hpp:16-17): das Sidecar-bvset wird als
/// IST gelesen -- als Aussage darueber, was die vorhandene Binary traegt -- und per (b) kryptographisch an
/// ihren Hash gebunden. Es wird NIEMALS zum SOLL: die Erwartungsseite kommt unveraendert aus der aktuellen
/// Treiber-Konfiguration, und die uebrigen acht Glieder werden ueberhaupt nicht aus dem Sidecar gelesen.
///
/// WARUM DER TEILMENGEN-PFAD OPT-IN IST (und nicht der Default): ohne gesetzten Kontext verhaelt sich diese
/// Funktion BYTE-GENAU wie vorher -- reine Hash-Gleichheit. Jeder Bestands-Aufrufer und jeder Test, der den
/// Kontext nicht setzt, aendert damit sein Verhalten um kein Bit. Das ist die Bedingung dafuer, dass die
/// EINMAL-Invalidierung unten nicht versehentlich jeden Pfad trifft, der sie gar nicht braucht.
struct SkipBvsetKontext {
    /// Das SOLL: die bvset-Signatur des HEUTIGEN Treibers. Leer = Teilmengen-Pfad AUS.
    std::string current_glied{};
    /// Die Bindungs-Pruefung: (aufgezeichnetes bvset) -> 128-hex fuer GENAU die binary_id dieses Jobs.
    /// Der Aufrufer bindet die binary_id in die Closure -- diese Funktion kennt keine binary_id und soll
    /// keine kennen. Leer = Teilmengen-Pfad AUS.
    std::function<std::string(std::string const&)> recompute{};

    /// Beide Haelften noetig: eine Teilmenge ohne Bindung waere faelschbar, eine Bindung ohne Soll-Menge
    /// haette nichts, wogegen sie prueft.
    [[nodiscard]] bool aktiv() const noexcept { return !current_glied.empty() && static_cast<bool>(recompute); }
};

[[nodiscard]] inline bool dll_is_current(std::filesystem::path const& output, std::string const& expected_fingerprint,
                                         SkipBvsetKontext const& bvset_ctx) {
    if (expected_fingerprint.empty()) return false; // (1) ohne CT-Erwartung nie ueberspringen (fail-closed)
    std::error_code ec;
    if (!std::filesystem::exists(output, ec) || ec) return false; // (2) DLL fehlt -> bauen
    auto const vorhanden = read_fingerprint_sidecar(output);      // (3)+(4) fehlt/leer/nicht-128-hex -> nullopt
    if (!vorhanden) return false;

    // (5) TEILMENGEN-PFAD AUS -> exakt der Vergleich von vorher. KEIN v1-Nachteil, kein neues Verhalten.
    if (!bvset_ctx.aktiv()) return *vorhanden == expected_fingerprint;

    // (6) v1-SIDECAR AUF DEM SCHARFEN PFAD -> false. DIE EINMAL-INVALIDIERUNG, ausdruecklich und nicht
    //     nebenbei: ein Sidecar ohne bvset-Zeile kann seine Menge nicht ausweisen, also kann fuer es weder
    //     (a) noch (b) entschieden werden. Es fail-closed durchzuwinken hiesse, den Vertrag genau fuer den
    //     Bestand auszusetzen, fuer den er gebaut wurde. Die Invalidierung ist EINMALIG UND SELBSTHEILEND:
    //     der ausgeloeste Neubau schreibt v2, jeder Folgelauf skippt wieder. Sie steht VOR dem
    //     Gleichheits-Schnellpfad -- sonst bliebe ein v1-Bestand mit zufaellig passendem Hash unbemerkt
    //     liegen und faende erst Monate spaeter, beim ersten echten Erweiterungs-Fall, seine Grenze.
    //     TERMINIERUNG: dieser eine Flotten-Neubau gehoert bewusst gesetzt, nicht in einen laufenden
    //     Mess-Betrieb hinein (286er-Mehrtagesexperiment).
    auto const recorded_bvset = read_bvset_glied_sidecar(output);
    if (!recorded_bvset) return false;

    // (7) SCHNELLPFAD: gleicher Hash heisst gleiche Identitaet in ALLEN neun Gliedern -- das schliesst den
    //     Fall recorded == current ein und braucht keine Neuberechnung.
    if (*vorhanden == expected_fingerprint) return true;

    // (8) RICHTUNG: nur ERWEITERUNG passiert hier. Einschraenkung (recorded traegt etwas, das current nicht
    //     mehr fuehrt) faellt durch -- das ist der Fall, fuer den der Neubau richtig ist.
    if (!bvset_ist_teilmenge(*recorded_bvset, bvset_ctx.current_glied)) return false;

    // (9) BINDUNG: der aufgezeichnete Hash muss sich aus dem aufgezeichneten bvset REPRODUZIEREN lassen.
    //     Schlaegt das fehl, weicht die Binary in mindestens einem der uebrigen acht Glieder ab (andere
    //     Toolchain, andere Zelle, anderes Mess-Tooling) ODER Zeile 2 wurde manipuliert. Beides ist ein
    //     Neubau-Grund, und beide sind hier bewusst nicht unterscheidbar: der Skip haengt am Nachweis,
    //     nicht an der Diagnose.
    return bvset_ctx.recompute(*recorded_bvset) == *vorhanden;
}

/// Bestands-Ueberladung: ohne Kontext ist der Teilmengen-Pfad AUS und dies ist byte-genau der Vergleich, den
/// die A2-Eichung 2026-08-05 gesetzt hat. Sie bleibt, damit kein Aufrufer eine leere Struktur mitschleppen
/// muss, um das Verhalten von gestern zu bekommen.
[[nodiscard]] inline bool dll_is_current(std::filesystem::path const& output, std::string const& expected_fingerprint) {
    return dll_is_current(output, expected_fingerprint, SkipBvsetKontext{});
}
// TP1FK1-B2 (Codex-Befund CX-W3): der Schreibfehler wird NICHT mehr verschluckt. Frueher hiess
// 'if (f) f << version;' -- misslang das Anlegen oder Schreiben, entstand STILL keine .version, und
// der spaetere Push haette einen Satz OHNE Vollstaendigkeits-Marke abgelegt (kein Pull findet ihn).
// Die .version ist die EINE Marke, deren Abwesenheit push_tier_binary (artifact_cache.hpp) jetzt
// scharf faengt; die Abwesenheit selbst darf aber nicht stumm entstehen. Deshalb hier eine
// klassifizierte Zeile (ArtefaktIo), wenn das Schreiben scheitert -- byte-neutral fuer den Erfolgspfad.
inline void write_version_sidecar(std::filesystem::path const& output, std::string const& version) {
    if (version.empty()) return;
    auto const    p = version_sidecar_path(output);
    std::ofstream f{p, std::ios::binary | std::ios::trunc};
    bool          geschrieben = false;
    if (f) {
        f << version;
        f.flush();
        geschrieben = f.good();
    }
    if (!geschrieben)
        std::cerr << "[" << measurement::infra_error_label(measurement::InfraErrorClass::ArtefaktIo)
                  << "] perm.dll.version NICHT geschrieben: " << p.string()
                  << " -- der Satz bliebe ohne Vollstaendigkeits-Marke (kein Pull faende ihn)\n"
                  << std::flush;
}
/// Schreibt das Organ-Provenienz-Sidecar (`.algos`). Leer = no-op (kein AlgoSigFn injiziert -> byte-neutral). Nur bei
/// erfolgreichem Bau (r.status==0) aufgerufen -> ein Fehlbau hinterlaesst KEIN falsches Organ-Sidecar.
inline void write_algos_sidecar(std::filesystem::path const& output, std::string const& algo_sig) {
    if (algo_sig.empty()) return;
    std::ofstream f{algo_sidecar_path(output), std::ios::binary | std::ios::trunc};
    if (f) f << algo_sig;
}
/// Schreibt das Build-Varianten-Sidecar (`.variant`, G2-3/A7). Leer = no-op (keine Variant-Signatur injiziert ->
/// byte-neutral). Nur bei erfolgreichem Bau (r.status==0) aufgerufen -> ein Fehlbau hinterlaesst KEIN falsches Sidecar.
inline void write_variant_sidecar(std::filesystem::path const& output, std::string const& variant_sig) {
    if (variant_sig.empty()) return;
    std::ofstream f{variant_sidecar_path(output), std::ios::binary | std::ios::trunc};
    if (f) f << variant_sig;
}
// I2 (HISTORIK, Stand bis 2026-08-05): das VIERTE Sidecar `<output>.fingerprint` -- der 128-hex K7b-Fingerprint der
// Binary (Lager-Index-Schluessel, bestand_key_of liest es). Bewusst SEPARAT von .version/.algos/.variant (das ist der
// kompakte Provenienz-Anker, kein Skip-Kriterium). perm.dll.* bleiben byte-genau unveraendert.
// A2-EICHUNG (GATE 5, F7, 2026-08-05) -- LEBENDE WAHRHEIT: die DATEI-Trennung oben gilt weiter, die Klammer "kein
// Skip-Kriterium" ist ueberholt. `.fingerprint` IST seit der Eichung das Skip-Kriterium von dll_is_current (und
// bleibt zugleich der Lager-Index-Schluessel) -- deshalb schreibt provision_core hier denselben Wert, den (A) als
// Skip-Erwartung gelesen hat.
// G4b-1/AUF-A2 (2026-07-26): fingerprint_sidecar_path steht seit dieser Scheibe in fingerprint_sidecar.hpp (oben
// inkludiert) -- unveraendert, gleicher Namespace. Grund: der Lager-Binder (bestandslog/fingerprint_key_source.hpp)
// muss denselben Pfad bilden, ohne diesen schweren Header zu ziehen. Es gibt weiter nur EINE Wahrheit zum Suffix.
/// Schreibt das Fingerprint-Sidecar (`.fingerprint`, I2). Leer = no-op (keine FingerprintFn injiziert -> byte-neutral).
/// Nur bei erfolgreichem Bau (r.status==0) aufgerufen -> ein Fehlbau hinterlaesst KEIN falsches Sidecar.
///
/// F3/C5 (A2-Eichungs-Nachreview, 2026-08-05) -- FAIL-LOUD im TP1FK1-B2-Muster (Vorbild write_version_sidecar
/// oben). Bis hierher hiess es 'if (f) f << fingerprint;': misslang das Anlegen oder das Schreiben, entstand
/// STILL kein `.fingerprint`. Das ist seit der A2-Eichung schwerer als beim `.version`-Sidecar, denn dieses
/// Sidecar IST das Skip-Kriterium: seine stille Abwesenheit kostet keinen falschen Skip (dll_is_current ist
/// fail-closed), aber sie kostet BEI JEDEM FOLGELAUF einen vollen Neubau derselben Binary -- eine Regression,
/// die sich als "der Cache greift nie" zeigt und deren Ursache ohne diese Zeile nirgends steht. Byte-neutral
/// fuer den Erfolgspfad (dieselbe klassifizierte ArtefaktIo-Zeile wie beim `.version`-Zwilling).
///
/// [NACHGEFUEHRT 2026-08-11, Task #59 -- SIDECAR-FORMAT v2] Der Schreiber legt jetzt ZWEI Zeilen an, sobald
/// `bvset_glied` gefuellt ist: Zeile 1 der Hash (unveraendert), Zeile 2 der Klartext des Preimage-Glieds [6].
/// Ist `bvset_glied` leer, entsteht byte-genau die v1-Form von vorher -- kein Aufrufer, der das Argument nicht
/// setzt, aendert ein einziges Byte auf der Platte. Das ist die Bedingung dafuer, dass diese Scheibe additiv
/// bleibt: die neue Faehigkeit kostet nichts, wo sie nicht bestellt ist.
inline void write_fingerprint_sidecar(std::filesystem::path const& output, std::string const& fingerprint,
                                      std::string const& bvset_glied = {}) {
    if (fingerprint.empty()) return;
    auto const    p = fingerprint_sidecar_path(output);
    std::ofstream f{p, std::ios::binary | std::ios::trunc};
    bool          geschrieben = false;
    if (f) {
        f << fingerprint;
        if (!bvset_glied.empty()) f << '\n' << bvset_glied;
        f.flush();
        geschrieben = f.good();
    }
    if (!geschrieben)
        std::cerr << "[" << measurement::infra_error_label(measurement::InfraErrorClass::ArtefaktIo)
                  << "] perm.dll.fingerprint NICHT geschrieben: " << p.string()
                  << " -- der Lager-Anker fehlt, jeder Folgelauf baut diese Binary erneut\n"
                  << std::flush;
}

/// F3/C5 -- STALE-SIDECAR-RAEUMUNG. Entfernt ein Sidecar, dessen NEUER Wert leer ist.
///
/// DIE LUECKE, DIE SIE SCHLIESST: alle vier Writer oben sind bei leerem Wert ein no-op (bewusst byte-neutral --
/// "kein Provider injiziert" darf keine leere Marke erzeugen). Beim NEUBAU einer Binary bedeutet dasselbe
/// no-op aber etwas ganz anderes: das Sidecar des VORGAENGER-Baus bleibt liegen und beschreibt ab sofort eine
/// Binary, die es nie gesehen hat. Fuer `.fingerprint` ist das der scharfe Fall (C5): ein Lauf OHNE
/// FingerprintFn baut fail-closed neu (expected leer -> nie Skip) und laesst den ALTEN 128-hex daneben liegen;
/// ein spaeterer Lauf MIT Provider vergleicht dann gegen eine Marke, die eine fremde Bau-Identitaet bezeugt --
/// und skippt im Treffer-Fall eine Binary, die er nie gebaut hat. Fuer `.version`/`.algos`/`.variant` ist die
/// Folge milder (Provenienz-/Transport-Marken statt Skip-Kriterium), aber gleicher Art: eine Legende, die auf
/// den Vorgaenger zeigt, ist schlechter als keine Legende.
///
/// KONSEQUENT UEBER ALLE VIER: der Fall "Wert nicht leer" braucht keine Raeumung -- der Writer oeffnet mit
/// std::ios::trunc und ueberschreibt vollstaendig. Geraeumt wird also genau dort, wo sonst ein no-op ein
/// Alt-Byte konservieren wuerde. Fail-loud im selben Muster: ein nicht entfernbares Sidecar ist ein
/// ArtefaktIo-Zustand, kein stiller Zustand.
inline void prune_stale_sidecar(std::filesystem::path const& p) {
    std::error_code ec;
    if (!std::filesystem::exists(p, ec) || ec) return; // nichts da (oder nicht befragbar) -> nichts zu raeumen
    if (std::filesystem::remove(p, ec) && !ec) return;
    std::cerr << "[" << measurement::infra_error_label(measurement::InfraErrorClass::ArtefaktIo)
              << "] stale Sidecar NICHT entfernt: " << p.string()
              << " -- es beschreibt ab jetzt den VORGAENGER-Bau dieser Binary\n"
              << std::flush;
}

/// Raeumt alle vier Sidecars, deren neuer Wert leer ist (s. prune_stale_sidecar). Aufruf-Ort ist der
/// Erfolgszweig des NEUBAUS in provision_core, unmittelbar VOR den vier Writern -- Raeumen und Schreiben
/// stehen damit an EINER Stelle und koennen nicht auseinanderlaufen.
inline void prune_stale_sidecars(std::filesystem::path const& output, std::string const& version,
                                 std::string const& algo_sig, std::string const& variant_sig,
                                 std::string const& fingerprint) {
    if (version.empty()) prune_stale_sidecar(version_sidecar_path(output));
    if (algo_sig.empty()) prune_stale_sidecar(algo_sidecar_path(output));
    if (variant_sig.empty()) prune_stale_sidecar(variant_sidecar_path(output));
    if (fingerprint.empty()) prune_stale_sidecar(fingerprint_sidecar_path(output));
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

    /// A5/F5: einen WEITEREN Completion-Hook ANHAENGEN, statt den bestehenden zu verdraengen.
    ///
    /// WARUM ES DIESE ZWEITE FORM GIBT: set_on_binary_done ueberschreibt. Der Iterator setzt dort bereits
    /// den kombinierten Push-/Bestandslog-Hook (cache_engine_builder_iterator.hpp); ein zweiter set_-Aufruf
    /// -- etwa fuer die ETA-Kalibrierung -- haette den async Push-Pump STILL abgeschaltet. Ein stiller
    /// Verlust ist genau die Fehlerklasse, die hier nicht entstehen darf, deshalb steht die additive Form
    /// neben der setzenden. Reihenfolge: der zuerst registrierte Hook laeuft zuerst.
    void add_on_binary_done(BinaryDoneFn fn) {
        if (!fn) return;
        if (!on_binary_done_) {
            on_binary_done_ = std::move(fn);
            return;
        }
        on_binary_done_ = [erst = std::move(on_binary_done_), zweit = std::move(fn)](BuildResult const& b) {
            erst(b);
            zweit(b);
        };
    }

    /// I2: den Fingerprint-Provider setzen (Lager-Index-Anker je Binary). VOR provision_all aufrufen; leer = kein
    /// .fingerprint-Sidecar (byte-neutral). Opt-in wie set_on_binary_done -> die ctor-Signatur bleibt unveraendert.
    void set_fingerprint_provider(FingerprintFn fn) { fingerprint_ = std::move(fn); }

    /// Task #59: der RECOMPUTE-Provider (binary_id, aufgezeichnetes bvset) -> 128-hex. Er beantwortet die eine
    /// Frage, die der normale Fingerprint-Provider NICHT beantworten kann: "welchen Hash haette diese Binary,
    /// wenn man sie mit der aufgezeichneten Enable-Menge statt der heutigen rechnet?" Genau daran haengt die
    /// Bindungs-Pruefung (9) in dll_is_current.
    ///
    /// NICHT GESETZT = Teilmengen-Pfad AUS, auch wenn cfg.current_bvset_glied gefuellt ist. Das ist Absicht:
    /// eine Teilmengen-Pruefung ohne Bindung waere schwaecher als die Gleichheit, die sie ersetzt -- lieber
    /// gar nicht scharf als scheinbar scharf. VOR provision_all aufrufen, gleiches opt-in-Muster wie
    /// set_fingerprint_provider.
    void set_bvset_fingerprint_provider(BvsetFingerprintFn fn) { bvset_fingerprint_ = std::move(fn); }

    /// S-7 (KON9-05): den Achsen-Algo-Versions-Provider setzen -- die ZULASSUNGS-BRUECKE
    /// "Achsen-Version fordert Flags <= Maschinen-Signatur gibt frei" (Urteil:
    /// measurement/algo_stempel_zulassung.hpp ueber flag_menge_in_signatur, S-3a).
    ///
    /// NICHT GESETZT = Bruecke AUS (byte-neutral, exakt das Verhalten von vorher) -- dasselbe
    /// opt-in-Muster wie set_bvset_fingerprint_provider. VOR provision_all aufrufen. Der
    /// Provider-Kontrakt (leer = kein Versions-Traeger, Sentinel = Tabellen-Drift) steht an der
    /// AlgoVersionFn-Deklaration.
    void set_algo_version_provider(AlgoVersionFn fn) { algo_version_ = std::move(fn); }

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
        // E-04-P1 (Trace-Budget): die Kadenz ist ab jetzt env-deckelbar (COMDARE_HEARTBEAT_EVERY) --
        // ungesetzt => n_workers => byte-identisch zum Ist. Nur das Voll-Bau-Profil hebt sie spaeter.
        ProgressHeartbeat build_hb{"tier-build", k, std::cerr, std::chrono::seconds{30}, heartbeat_every_n(n_workers)};

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

                // S-7 (KON9-05): der EIGENE Konjunktions-Block der STEMPEL-Zulassung, NACH dem
                // required-Check -- zwei getrennte Fragen: oben "welche Flags DEKLARIERT das Organ als
                // required" (heute leer, C-3a-Tripwire), hier "welche Flags FORDERT die Achsen-VERSION
                // X.Y.Z" (S-3b Compile-Seite, KON16-02). Urteil je (achse, wert) ueber den EINEN Richter
                // flag_menge_in_signatur (algo_stempel_zulassung.hpp). Provider nicht gesetzt = Bruecke
                // AUS (byte-neutral). Leeres Literal = "kein bekannter Versions-Stand" => NEUTRAL
                // uebersprungen (Sub-Achsen-Ebenen, "tier", profil-eigene Nicht-Registry-Werte wie
                // search_algo=bplus -- Kontrakt an der AlgoVersionFn-Deklaration); ein nicht-leeres
                // Sentinel-/Fehl-Literal ist eine BEHAUPTUNG, parst auf den Sentinel und faellt
                // fail-closed. Heute lehnt der Block nachweislich NIE ab (alle 123 Bestands-Literale
                // sind nackte c-Formen, CT-Batterie in algo_stempel_zulassung.hpp) -- byte-/golden-
                // neutral by construction.
                if (algo_version_) {
                    std::optional<::comdare::cache_engine::measurement::CompilerCompilerErrorClass> stempel_err;
                    for (auto const& [achse, wert] : spec.axes) {
                        std::string const literal = algo_version_(achse, wert);
                        if (literal.empty()) continue; // kein Versions-Traeger: neutral (s. Kontrakt)
                        stempel_err = ::comdare::cache_engine::measurement::stempel_zulassung_je_achse(
                            ::comdare::cache_engine::measurement::parse_algo_semver(literal),
                            ::comdare::cache_engine::measurement::active_machine_signature());
                        if (stempel_err) break;
                    }
                    if (stempel_err) {
                        r.status  = -4; // Stempel-Ablehnung: die Achsen-Version fordert nicht freigegebene Hardware
                        r.message = std::string{"simd-gate(stempel): "} +
                                    std::string{::comdare::cache_engine::measurement::error_class_label(*stempel_err)};
                        r.outcome = std::unexpected(::comdare::cache_engine::measurement::BuildError{*stempel_err});
                        finalize(j, std::move(r));
                        continue; // gleiche D1-Behandlung wie oben: Log + weiter, kein Abbruch
                    }
                }

                // A2-EICHUNG (GATE 5, F7): der erwartete CT-Fingerprint DIESER Bau-Identitaet -- EINMAL je Job
                // berechnet und danach ZWEIMAL benutzt: als Skip-Erwartung in (A) und als Sidecar-Inhalt nach
                // erfolgreichem Bau (unten). Genau diese Wiederverwendung macht die EINE Schluessel-Welt zur
                // CODE-Konstruktion statt zur Disziplin: was das Gate vergleicht, ist byte-gleich dem, was als
                // Lager-Index-Anker neben der Binary landet (== minio-Key == Bestandslog key_sha512).
                // Leer, wenn kein Fingerprint-Provider injiziert ist -> (A) ist dann fail-closed (skippt NIE).
                std::string const expected_fp = fingerprint_ ? fingerprint_(spec.binary_id) : std::string{};
                // T2-A/K2-NB: DERSELBE Wert reist im Ergebnis weiter -- der Mess-Resume-Stamp speist sich
                // daraus, statt den Provider ein zweites Mal zu fragen (s. BuildResult::fingerprint).
                // Damit ist "EINMAL je Job berechnet" nicht mehr nur fuer Gate und Sidecar wahr, sondern
                // fuer ALLE drei Konsumenten dieser Zahl.
                r.fingerprint = expected_fp;

                // (A) INKREMENTELL: bestehende, fingerprint-aktuelle DLL ueberspringen (Resume nach Absturz).
                // DER EINE VERGLEICH (F7 "NUR"): `.fingerprint`-Sidecar == expected_fp. .version/.algos/.variant
                // werden weiter geschrieben (Provenienz-/Transport-Marken), entscheiden hier aber nichts mehr.
                // Task #59: der Teilmengen-Kontext wird HIER gebaut, weil nur hier die binary_id bekannt ist.
                // Die Bindungs-Pruefung muss an GENAU DIESE Binary gebunden sein -- ein Provider, der die id
                // nicht mitfuehrt, koennte den Hash einer anderen Binary reproduzieren und damit einen Skip
                // begruenden, der nichts ueber die vorliegende Datei aussagt.
                SkipBvsetKontext skip_ctx;
                skip_ctx.current_glied = cfg_.current_bvset_glied;
                if (bvset_fingerprint_)
                    skip_ctx.recompute = [this, id = spec.binary_id](std::string const& bvset) {
                        return bvset_fingerprint_(id, bvset);
                    };
                if (dll_is_current(job.output, expected_fp, skip_ctx)) {
                    r.status  = 0;
                    r.skipped = true;
                    r.message = "uebersprungen (Fingerprint aktuell)";
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
                    // A5/F5 Baupunkt (1): die Uhr steht GENAU um den externen Compiler-Aufruf -- nicht um
                    // die Source-Generierung darueber und nicht um die RAM-Admission davor (s. BuildResult::
                    // dauer_s). Sie laeuft auch fuer einen FEHLGESCHLAGENEN Compile: ein Fehlschlag hat
                    // Bau-Zeit gekostet, und sie zu verschweigen wuerde die ETA zu kurz machen.
                    auto const t_compile0 = std::chrono::steady_clock::now();
                    r.status              = compile_(job);
                    r.dauer_s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t_compile0).count();
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
                        // F3/C5: DIESE Binary ist soeben NEU entstanden -- jedes Sidecar, das der folgende
                        // Writer als no-op behandeln wuerde (leerer Wert), traegt sonst weiter die Legende des
                        // VORGAENGER-Baus. Raeumen VOR dem Schreiben, damit beide Wege an einer Stelle stehen.
                        prune_stale_sidecars(job.output, cfg_.build_version, algos, cfg_.build_variant_sig,
                                             expected_fp);
                        write_version_sidecar(job.output, cfg_.build_version); // System-Provenienz-Resume-Marke
                        write_algos_sidecar(job.output, algos); // Organ-Provenienz (Bauplan §1); leer=no-op
                        write_variant_sidecar(job.output,
                                              cfg_.build_variant_sig); // Build-Variante (G2-3/A7); leer=no-op
                        // I2 Lager-Anker; leer=no-op. A2-EICHUNG: DERSELBE Wert, den (A) als Skip-Erwartung
                        // gelesen hat -- nicht neu berechnet. Damit ist "was das Gate erwartet" und "was neben
                        // der Binary liegt" EINE Quelle, und der naechste Lauf skippt genau diese Binary.
                        // Task #59: Zeile 2 = die bvset-Menge, mit der GENAU DIESE Binary gebaut wurde. Der
                        // Wert kommt aus cfg_ und wird NICHT hier abgeleitet -- die Facade reicht denselben
                        // String, den sie auch in den Fingerprint-Maker gegeben hat, damit Zeile 2 und das
                        // Preimage-Glied [6] nicht auseinanderlaufen koennen. Leer = v1-Form wie bisher.
                        write_fingerprint_sidecar(job.output, expected_fp, cfg_.sidecar_bvset_glied);
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

    BuildConfig   cfg_;
    CompileFn     compile_;
    SourceGenFn   gen_;
    FreeRamFn     free_ram_;
    AlgoSigFn     algo_sig_;       // Bauplan §3: spec.axes → algo_sig; leer = Organ-Gate aus (byte-neutral)
    BinaryDoneFn  on_binary_done_; // W11: per-Binary Completion-Hook (BAU-Modus async Push-Feed); leer = byte-neutral
    FingerprintFn fingerprint_;    // I2: binary_id -> 128-hex K7b-Fingerprint (.fingerprint-Sidecar); leer=byte-neutral
    BvsetFingerprintFn bvset_fingerprint_; // Task #59: (binary_id, bvset) -> 128-hex; leer = Teilmengen-Pfad AUS
    AlgoVersionFn      algo_version_;      // S-7: (achse, wert) -> algo_version-Literal; leer = Bruecke AUS
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
