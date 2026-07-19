#pragma once
// Storage #51 (2026-07-18) — artifact_transport/artifact_cache: die schmale, SYNCHRONE Transport-Klasse der
// Zwei-Cache-Storage-Naht (BAUPLAN-TWO-CACHE-STORAGE §3 / VERORTUNGS-BRIEF §1). Zwei getrennte Speicher-Ebenen:
//
//   B  CEB/Tier-Binary-Bau-Artefakte -> minio-Objekt-Store (mc-Shellout). Objekt-Key
//      <build_version>/<stem>/perm.dll(+.version); Vollstaendigkeits-Marke: perm.dll ZUERST, perm.dll.version
//      ZULETZT (halb-gepusht = kein Sidecar = kein Pull). Der Key KOPPELT an dieselbe build_version-Signatur, die
//      dll_is_current lokal prueft -> stale ABI (5->6) wird NIE reused.
//   C  Messergebnisse -> der bestehende write-only `measure-drop`-Pfad per HTTPS-PUT (curl-Shellout): PUT an
//      <COMDARE_MEASUREMENT_DROP_URL>/<YYYYMMDD-HHMMSS>/<datei>. Write-only + additiv: Duplikat -> 409 (NIE
//      ueberschreiben, still additiv-OK), unter einem EINEN datierten Lauf-Baum. Parallel zum bestehenden git-
//      persist:measurements (Doppel-Persistenz, ersetzt es NICHT). KEIN POSIX-Mount (User-Entscheid A 2026-07-18).
//
// DOKTRIN (byte-neutral/Anti-Phantom): from_env() liest AUSSCHLIESSLICH env; ist weder B noch C konfiguriert, ist
// die Instanz INERT (minio_enabled()==false && drop_enabled()==false) -> die injizierten CachePushFn/MeasurementSinkFn
// sind No-Op -> golden/CI byte-identisch. Credentials werden NIE hier gelesen (mc zieht sie aus MC_HOST_<alias>; der
// measure-drop-Token COMDARE_NFS_DROP_TOKEN geht NUR ueber eine 0600-curl-Config-Datei, NIE in argv/ps oder ins Log)
// und NIE geloggt. KEIN Python; C++23; posix_spawn ohne /bin/sh (Muster build_orchestrator.hpp:460).
// Fehler (Push-/Drop-IO) -> InfraErrorClass::ArtefaktIo geloggt (Log NEBEN der Datei) + std::cerr, lokale Kopie bleibt,
// der Aufrufer MISST WEITER (nie Abbruch, nie "measured=0"). SYNCHRON/blockierend — der Aufrufer haengt sie in den
// 1-Thread-Mess-Loop; async/detached ist VERBOTEN (I/O-Contention = Messfehler). Header-only.

#include <cache_engine/measurement/axis_error.hpp> // InfraErrorClass::ArtefaktIo + infra_error_label (Fehler-Log)

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream> // std::cerr (klassifizierte Infra-Fehlerzeile) — include-what-you-use, nicht transitiv verlassen
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;
#endif

namespace comdare::cache_engine::builder::artifact_transport {

// ── Injektions-Naht-Typen (No-Op-Default = leere std::function -> byte-neutral). ──────────────────────────────
// Muster wie CompileFn/AlgoSigFn (build_orchestrator.hpp): der Iterator ruft sie SYNCHRON an der per-Binary-Naht.
// CachePushFn: ein fertig gebautes Tier-Binary-Verzeichnis -> Objekt-Store (Ebene B). Args (bin_dir, build_version);
//   der Client leitet Objekt-Key <build_version>/<stem>/perm.dll(+.version) ab (stem = bin_dir.filename()).
using CachePushFn = std::function<void(std::filesystem::path const& bin_dir, std::string const& build_version)>;
// MeasurementSinkFn: eine Mess-Datei additiv an die write-only measure-drop-Senke legen (Ebene C, HTTPS-PUT). Args
//   (local_file, relative_dest); Ziel-URL = <drop_url>/<lauf_stamp>/<relative_dest>. Leer = No-Op.
using MeasurementSinkFn =
    std::function<void(std::filesystem::path const& local_file, std::string const& relative_dest)>;

// ── Der Transport-Client ──────────────────────────────────────────────────────────────────────────────────────
class ArtifactCache {
public:
    ArtifactCache() = default;

    /// Konstruiert die Instanz AUSSCHLIESSLICH aus der Umgebung (byte-neutraler Default: alles leer -> INERT).
    ///   COMDARE_MINIO_ENDPOINT  — der mc-Alias-Name (Ebene B). Credentials NIE hier; mc zieht sie aus MC_HOST_<alias>.
    ///   COMDARE_MINIO_BUCKET    — Ziel-Bucket (getrennt vom buildsystem-cache).
    ///   COMDARE_MINIO_PREFIX    — optionaler Key-Praefix im Bucket.
    ///   COMDARE_MEASUREMENT_DROP_URL — Basis-URL des write-only measure-drop (Ebene C, HTTPS-PUT).
    ///   COMDARE_NFS_DROP_TOKEN  — Bearer-Token fuer den measure-drop (NUR aus env; geht ausschliesslich ueber eine
    ///                             0600-curl-Config, NIE in argv/ps oder ins Log).
    ///   COMDARE_MC_BIN / COMDARE_CURL_BIN — optional: Pfad/Name der mc-/curl-Binaries (Default via PATH).
    [[nodiscard]] static ArtifactCache from_env() {
        ArtifactCache c;
        c.endpoint_ = env_or_empty("COMDARE_MINIO_ENDPOINT");
        c.bucket_   = env_or_empty("COMDARE_MINIO_BUCKET");
        c.prefix_   = env_or_empty("COMDARE_MINIO_PREFIX");
        c.drop_url_ = env_or_empty("COMDARE_MEASUREMENT_DROP_URL");
        c.token_    = env_or_empty("COMDARE_NFS_DROP_TOKEN"); // NIE geloggt, NIE in argv
        if (std::string const mc = env_or_empty("COMDARE_MC_BIN"); !mc.empty()) c.mc_bin_ = mc;
        if (std::string const cu = env_or_empty("COMDARE_CURL_BIN"); !cu.empty()) c.curl_bin_ = cu;
        c.run_stamp_ = make_run_stamp(); // EIN datierter Lauf-Baum (Sink-Besitzer des Timestamps)
        return c;
    }

    /// true, wenn Ebene B (minio-Push) konfiguriert ist (Endpoint UND Bucket gesetzt).
    [[nodiscard]] bool minio_enabled() const noexcept { return !endpoint_.empty() && !bucket_.empty(); }
    /// true, wenn Ebene C (measure-drop-Sink) konfiguriert ist (Drop-URL gesetzt).
    [[nodiscard]] bool drop_enabled() const noexcept { return !drop_url_.empty(); }
    /// true, wenn WEDER B noch C konfiguriert ist -> die Instanz ist INERT (No-Op, byte-neutral).
    [[nodiscard]] bool inert() const noexcept { return !minio_enabled() && !drop_enabled(); }

    [[nodiscard]] std::string const& run_stamp() const noexcept { return run_stamp_; }

    /// Ebene B: schiebt die Bau-Artefakte EINES Tier-Binary-Ordners in den Objekt-Store. Vollstaendigkeits-Marke:
    /// perm.dll ZUERST, perm.dll.version ZULETZT (schlaegt perm.dll fehl, wird das Sidecar NICHT gepusht -> Pull
    /// findet keine Marke -> kein falscher Reuse). Objekt-Key = <build_version>/<stem>/perm.dll(+.version), stem =
    /// bin_dir.filename(). Fehler -> ArtefaktIo geloggt + lokale Kopie bleibt; MESSEN WEITER (kein throw).
    void push_tier_binary(std::filesystem::path const& bin_dir, std::string const& build_version) const {
        if (!minio_enabled()) return; // No-Op (byte-neutral)
        std::string const           stem     = bin_dir.filename().string();
        std::string const           key_base = build_version + "/" + stem;
        std::filesystem::path const dll      = bin_dir / "perm.dll";
        std::filesystem::path const sidecar  = bin_dir / "perm.dll.version";
        std::filesystem::path const log      = bin_dir / "perm.dll.push.log";

        std::error_code ec;
        if (!std::filesystem::exists(dll, ec)) {
            log_artefakt_io(log, "perm.dll fehlt lokal, nichts zu pushen: " + dll.string());
            return;
        }
        // (1) perm.dll ZUERST — die eigentliche Nutzlast.
        if (!mc_cp(dll, key_base + "/perm.dll", log)) {
            log_artefakt_io(log, "Push perm.dll fehlgeschlagen (lokale Kopie bleibt): " + key_base + "/perm.dll");
            return; // Sidecar bewusst NICHT pushen -> halb-gepusht = kein Pull
        }
        // (2) perm.dll.version ZULETZT — die Vollstaendigkeits-/Versions-Marke.
        if (std::filesystem::exists(sidecar, ec)) {
            if (!mc_cp(sidecar, key_base + "/perm.dll.version", log))
                log_artefakt_io(log,
                                "Push perm.dll.version fehlgeschlagen (perm.dll ist oben, Marke fehlt -> kein Pull): " +
                                    key_base + "/perm.dll.version");
        }
    }

    /// Ebene C: legt eine Mess-Datei additiv per HTTPS-PUT im write-only measure-drop ab. Ziel-URL =
    /// <drop_url>/<run_stamp>/<relative_dest>. STRIKT additiv (write-only-Doktrin): ein Duplikat quittiert der
    /// Drop mit 409 -> still additiv-OK (NIE ueberschrieben). Transport-/Server-Fehler -> ArtefaktIo geloggt +
    /// lokale Kopie bleibt; MESSEN WEITER (kein throw). Parallel zum git-persist:measurements (Doppel-Persistenz).
    void sink_measurement(std::filesystem::path const& local_file, std::string const& relative_dest) const {
        if (!drop_enabled()) return; // No-Op (byte-neutral)
        std::error_code ec;
        if (!std::filesystem::exists(local_file, ec)) return; // nichts abzulegen

        std::string url = drop_url_;
        while (!url.empty() && url.back() == '/') url.pop_back(); // genau EIN Trenner
        url += "/" + run_stamp_ + "/" + relative_dest;
        std::filesystem::path const log = local_file.string() + ".drop.log";

        if (!curl_put(url, local_file, log))
            log_artefakt_io(log, "measure-drop PUT fehlgeschlagen (lokale Kopie bleibt): " + url);
    }

private:
    // ── curl-Shellout (Ebene C): SYNCHRON HTTPS-PUT an den write-only measure-drop. Der Bearer-Token geht ueber eine
    //    0600-curl-Config (-K), NIE in argv (ps-Leak-Schutz) und NIE ins Log. 409 = additiv-Duplikat -> OK (kein
    //    Fehler, kein Retry). 2xx = abgelegt. Sonst/Transport-Fehler -> Retry, dann false (Aufrufer loggt ArtefaktIo). ──
    [[nodiscard]] bool curl_put(std::string const& url, std::filesystem::path const& local_file,
                                std::filesystem::path const& log) const {
        std::filesystem::path const cfg = log.string() + ".curlcfg"; // 0600, traegt den Token -> sofort geloescht
        if (!write_curl_config(cfg, url, local_file)) {
            log_artefakt_io(log, "curl-Config nicht schreibbar (0600): " + cfg.string());
            return false;
        }
        std::filesystem::path const out = log.string() + ".curlout"; // nur HTTP_CODE=<n> (kein Token, kein Body)
        std::error_code             ec;
        bool                        ok = false;
        for (std::size_t attempt = 1; attempt <= tries_; ++attempt) {
            int const  rc   = run_argv({curl_bin_, "-K", cfg.string()}, out);
            long const code = parse_http_code(out);
            if (rc == 0 && ((code >= 200 && code < 300) || code == 409)) {
                ok = true; // 2xx = abgelegt; 409 = additiv-Duplikat (write-only-Doktrin) -> beides OK, kein Retry
                break;
            }
            if (rc == 0 && code >= 400 && code < 500) {
                // Terminaler Client-Fehler (401/403/404/…, NICHT 409): ein Retry heilt ihn nicht (Config/Token/Pfad)
                // -> sofort abbrechen statt tries_x zu warten. Der Aufrufer loggt ArtefaktIo, lokale Kopie bleibt.
                log_artefakt_io(log,
                                "measure-drop PUT terminaler HTTP " + std::to_string(code) + " (kein Retry): " + url);
                break;
            }
            if (attempt < tries_) sleep_seconds(sleep_s_); // Transport-Fehler (rc!=0) / 5xx / HTTP_CODE=0 -> erneut
        }
        std::filesystem::remove(out, ec);
        std::filesystem::remove(cfg, ec); // Token-Config sofort weg (auch bei Fehlschlag)
        return ok;
    }

    /// Schreibt die curl-Config atomar mit Modus 0600 (open(O_CREAT|O_TRUNC,0600)) — der Token liegt so NIE in argv
    /// (kein ps-Leak auf dem geteilten Runner) und nur eigentuemer-lesbar + transient auf Platte (danach entfernt).
    [[nodiscard]] bool write_curl_config(std::filesystem::path const& cfg, std::string const& url,
                                         std::filesystem::path const& local_file) const {
#ifdef _WIN32
        (void)cfg;
        (void)url;
        (void)local_file;
        return false; // Storage-Weg ist Cluster-Linux
#else
        int const fd = ::open(cfg.string().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fd < 0) return false;
        std::string body;
        body += "request = \"PUT\"\n";
        body += "upload-file = \"" + local_file.string() + "\"\n";
        body += "url = \"" + url + "\"\n";
        if (!token_.empty()) body += "header = \"Authorization: Bearer " + token_ + "\"\n";
        body += "silent\nshow-error\n";
        body += "output = \"/dev/null\"\n";                 // Response-Body verwerfen
        body += "write-out = \"HTTP_CODE=%{http_code}\"\n"; // Status -> stdout (== out-Datei)
        std::size_t off = 0;
        bool        okw = true;
        while (off < body.size()) {
            ssize_t const w = ::write(fd, body.data() + off, body.size() - off);
            if (w < 0) {
                if (errno == EINTR) continue;
                okw = false;
                break;
            }
            off += static_cast<std::size_t>(w);
        }
        ::close(fd);
        return okw;
#endif
    }

    /// Parst `HTTP_CODE=<n>` aus der curl-write-out-Ausgabe (0 = nicht gefunden -> als Fehler behandelt).
    [[nodiscard]] static long parse_http_code(std::filesystem::path const& out) {
        std::ifstream              f{out, std::ios::binary};
        std::string                content((std::istreambuf_iterator<char>(f)), {});
        constexpr std::string_view key = "HTTP_CODE=";
        std::size_t                p   = content.find(key);
        if (p == std::string::npos) return 0;
        p += key.size();
        long code  = 0;
        bool digit = false;
        while (p < content.size() && content[p] >= '0' && content[p] <= '9') {
            code  = code * 10 + (content[p] - '0');
            digit = true;
            ++p;
        }
        return digit ? code : 0;
    }
    // ── mc-Shellout (Ebene B): SYNCHRON `mc cp` mit Retry + `mc stat`-Groessen-Verify. Kein /bin/sh, kein Python. ──
    [[nodiscard]] bool mc_cp(std::filesystem::path const& local, std::string const& object_key,
                             std::filesystem::path const& log) const {
        std::string const    target = mc_target(object_key);
        std::error_code      ec;
        std::uintmax_t const local_size = std::filesystem::file_size(local, ec);

        for (std::size_t attempt = 1; attempt <= tries_; ++attempt) {
            // `mc cp --quiet` = atomarer Put (mc verifiziert den Upload intern via ETag). stdout/stderr -> Log.
            int const rc = run_argv({mc_bin_, "cp", "--quiet", local.string(), target}, log);
            if (rc == 0 && mc_size_verified(target, local_size, log)) return true;
            if (attempt < tries_) sleep_seconds(sleep_s_);
        }
        return false;
    }

    /// `mc stat --json <target>` -> Groesse parsen und gegen die lokale Groesse pruefen. Ein Parse-Fehler wird NICHT
    /// als Push-Fehler gewertet (mc cp exit 0 hat die Integritaet bereits geprueft) -> best-effort-Zusatz-Verify.
    [[nodiscard]] bool mc_size_verified(std::string const& target, std::uintmax_t local_size,
                                        std::filesystem::path const& log) const {
        std::filesystem::path const stat_out = log.string() + ".stat";
        int const                   rc       = run_argv({mc_bin_, "stat", "--json", target}, stat_out);
        std::error_code             ec;
        if (rc != 0) {
            std::filesystem::remove(stat_out, ec);
            return true; // Verify nicht durchfuehrbar -> cp-exit-0 gilt (gutes Push nicht an einem stat-Quirk kippen)
        }
        std::ifstream f{stat_out, std::ios::binary};
        std::string   content((std::istreambuf_iterator<char>(f)), {});
        f.close();
        std::filesystem::remove(stat_out, ec);
        std::uintmax_t remote_size = 0;
        if (!parse_json_size(content, remote_size)) return true; // Groesse nicht auffindbar -> best-effort-OK
        return remote_size == local_size;
    }

    /// Extrahiert das erste "size":<zahl>-Feld aus einem mc-stat-JSON (ohne JSON-Lib; robust gegen Whitespace).
    [[nodiscard]] static bool parse_json_size(std::string_view json, std::uintmax_t& out) {
        constexpr std::string_view key = "\"size\"";
        std::size_t                p   = json.find(key);
        if (p == std::string_view::npos) return false;
        p += key.size();
        while (p < json.size() && (json[p] == ' ' || json[p] == ':' || json[p] == '\t')) ++p;
        std::uintmax_t v     = 0;
        bool           digit = false;
        while (p < json.size() && json[p] >= '0' && json[p] <= '9') {
            v     = v * 10 + static_cast<std::uintmax_t>(json[p] - '0');
            digit = true;
            ++p;
        }
        if (!digit) return false;
        out = v;
        return true;
    }

    /// mc-Ziel-String: <alias>/<bucket>/[<prefix>/]<object_key>.
    [[nodiscard]] std::string mc_target(std::string const& object_key) const {
        std::string t = endpoint_ + "/" + bucket_ + "/";
        if (!prefix_.empty()) {
            t += prefix_;
            if (t.back() != '/') t += '/';
        }
        t += object_key;
        return t;
    }

    // ── Prozess-Start ohne /bin/sh (Muster build_orchestrator.hpp:460): posix_spawnp(argv), stdout+stderr -> redirect. ──
    [[nodiscard]] static int run_argv(std::vector<std::string> const& argv, std::filesystem::path const& redirect) {
        if (argv.empty()) return 127;
#ifdef _WIN32
        // Der Storage-Weg ist Cluster-Linux (mc/curl = POSIX). Auf Windows degradiert dieser Pfad sichtbar (Log +
        // Fehler-Exit), statt einen unsicheren std::system-String zu bauen -> die lokale Kopie bleibt, MESSEN WEITER.
        std::ofstream lf{redirect, std::ios::app};
        lf << "artifact_transport: mc/curl-Shellout auf _WIN32 nicht unterstuetzt (Storage-Weg ist Cluster-Linux)\n";
        return 127;
#else
        std::string const          redir_s = redirect.string();
        posix_spawn_file_actions_t actions;
        if (posix_spawn_file_actions_init(&actions) != 0) return 127;
        int rc = posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, redir_s.c_str(),
                                                  O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (rc == 0) rc = posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO);
        if (rc != 0) {
            posix_spawn_file_actions_destroy(&actions);
            return 127;
        }
        std::vector<char*> c_argv;
        c_argv.reserve(argv.size() + 1);
        for (auto const& a : argv) c_argv.push_back(const_cast<char*>(a.c_str()));
        c_argv.push_back(nullptr);

        pid_t pid = 0;
        rc        = posix_spawnp(&pid, argv.front().c_str(), &actions, nullptr, c_argv.data(), ::environ);
        posix_spawn_file_actions_destroy(&actions);
        if (rc != 0) return 127;

        int status = 0;
        while (waitpid(pid, &status, 0) < 0) {
            if (errno == EINTR) continue;
            return 127;
        }
        if (WIFEXITED(status)) return WEXITSTATUS(status);
        if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
        return status;
#endif
    }

    static void sleep_seconds(std::size_t s) {
        if (s != 0) std::this_thread::sleep_for(std::chrono::seconds(s));
    }

    /// Schreibt eine klassifizierte Infra-Fehlerzeile NEBEN die Datei (Log neben CSV) + auf std::cerr. Kein throw,
    /// kein Credential-Leak (nur Pfade/Groessen). Praefix identisch zum Iterator-Stil ([Infra-Fehler: artefakt_io]).
    static void log_artefakt_io(std::filesystem::path const& log, std::string const& msg) {
        namespace cm                 = ::comdare::cache_engine::measurement;
        std::string_view const label = cm::infra_error_label(cm::InfraErrorClass::ArtefaktIo);
        std::string const      line  = "[Infra-Fehler: " + std::string{label} + "] " + msg;
        std::ofstream          lf{log, std::ios::app};
        if (lf) lf << line << "\n";
        std::cerr << line << "\n";
    }

    [[nodiscard]] static std::string env_or_empty(char const* name) {
        if (char const* e = std::getenv(name); e != nullptr && *e != '\0') return std::string{e};
        return {};
    }

    /// YYYYMMDD-HHMMSS (lokale Zeit) — der datierte Lauf-Baum-Ordner der measure-drop-Senke.
    [[nodiscard]] static std::string make_run_stamp() {
        std::time_t const now = std::time(nullptr);
        std::tm           tm{};
#if defined(_WIN32)
        localtime_s(&tm, &now);
#else
        localtime_r(&now, &tm);
#endif
        char buf[24] = {};
        std::strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", &tm);
        return std::string{buf};
    }

    std::string endpoint_;          // COMDARE_MINIO_ENDPOINT (mc-Alias-Name; leer = Ebene B aus)
    std::string bucket_;            // COMDARE_MINIO_BUCKET
    std::string prefix_;            // COMDARE_MINIO_PREFIX (optional)
    std::string drop_url_;          // COMDARE_MEASUREMENT_DROP_URL (measure-drop Basis-URL; leer = Ebene C aus)
    std::string token_;             // COMDARE_NFS_DROP_TOKEN (NIE geloggt, NIE in argv — nur 0600-curl-Config)
    std::string mc_bin_   = "mc";   // COMDARE_MC_BIN override (Default via PATH)
    std::string curl_bin_ = "curl"; // COMDARE_CURL_BIN override (Default via PATH)
    std::string run_stamp_;         // datierter Lauf-Baum (Sink-Besitzer des Timestamps)
    std::size_t tries_   = 12;      // Retry-Zahl (Vorbild copy_results_to_nas.sh: 12)
    std::size_t sleep_s_ = 5;       // Pause zwischen Versuchen (s)
};

} // namespace comdare::cache_engine::builder::artifact_transport
