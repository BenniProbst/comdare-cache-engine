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

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // W12-B (S1/§43): COMDARE_ANATOMY_ABI_MAJOR +
                                                           // kCebContractCodegenMinor fuer das +ceb-Key-Segment
                                                           // (Single-Source wie system_axes_version_suffix)
#include <cache_engine/measurement/axis_error.hpp> // InfraErrorClass::ArtefaktIo + infra_error_label (Fehler-Log)

#include <cctype>
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
// W11 (Ledger §43.c): PartialMarkerFn -- nach je N gepushten DLLs im BAU-Modus feuert der async Push-Pump einen
//   TEIL-Marker in den Objekt-Store, damit ein Runner, der einen abgebrochenen Chunk-Job wieder aufnimmt, die
//   bereits gepushten DLLs dieses Chunks pullen kann (Cluster-Resume). Args (build_version, part_index); der Client
//   leitet den Key <build_version>/_gn_chunk_markers/<range>.part<k>.done ab (range im Client-Closure gekapselt).
//   Leer = No-Op (byte-neutral). Reihenfolge-unabhaengig (Objekt-Store), daher aus dem Push-Thread erlaubt.
using PartialMarkerFn = std::function<void(std::string const& build_version, std::size_t part_index)>;

// ── Der Transport-Client ──────────────────────────────────────────────────────────────────────────────────────
class ArtifactCache {
public:
    ArtifactCache() = default;

    /// Konstruiert die Instanz AUSSCHLIESSLICH aus der Umgebung (byte-neutraler Default: alles leer -> INERT).
    ///   COMDARE_MINIO_ENDPOINT  — der mc-Alias-Name (Ebene B). Credentials NIE hier; mc zieht sie aus MC_HOST_<alias>.
    ///   COMDARE_MINIO_BUCKET    — Ziel-Bucket (getrennt vom buildsystem-cache).
    ///   COMDARE_MINIO_PREFIX    — optionaler Key-Praefix im Bucket.
    ///   COMDARE_MEASUREMENT_DROP_URL — Basis-URL des write-only measure-drop (Ebene C, HTTPS-PUT).
    ///   COMDARE_NFS_DROP_TOKEN  — Basic-Auth-Passwort fuer den measure-drop (nginx htpasswd, HANDOFF3: user
    ///                             'measure' + Token; NUR aus env; geht ausschliesslich ueber eine 0600-curl-Config,
    ///                             NIE in argv/ps oder ins Log).
    ///   COMDARE_NFS_DROP_USER   — optionaler Basic-Auth-User des measure-drop (Default: "measure").
    ///   COMDARE_MC_BIN / COMDARE_CURL_BIN — optional: Pfad/Name der mc-/curl-Binaries (Default via PATH).
    [[nodiscard]] static ArtifactCache from_env() {
        ArtifactCache c;
        c.endpoint_ = env_or_empty("COMDARE_MINIO_ENDPOINT");
        c.bucket_   = env_or_empty("COMDARE_MINIO_BUCKET");
        c.prefix_   = env_or_empty("COMDARE_MINIO_PREFIX");
        c.drop_url_ = env_or_empty("COMDARE_MEASUREMENT_DROP_URL");
        c.token_    = env_or_empty("COMDARE_NFS_DROP_TOKEN"); // NIE geloggt, NIE in argv
        if (std::string const du = env_or_empty("COMDARE_NFS_DROP_USER"); !du.empty()) c.drop_user_ = du;
        if (std::string const mc = env_or_empty("COMDARE_MC_BIN"); !mc.empty()) c.mc_bin_ = mc;
        if (std::string const cu = env_or_empty("COMDARE_CURL_BIN"); !cu.empty()) c.curl_bin_ = cu;
        // W12-B (S1): die vom Planer gewaehlte Mess-Tooling-Combo reist ueber COMDARE_MEASUREMENT_COMBO (leer/[all]-
        // Default => Director exportiert sie NICHT, experiment_plan_director.hpp:779) und stempelt die DLL-Bytes REAL
        // (lazy_adhoc_source_gen.hpp:227-238) OHNE Wirkung auf die Perm-build_version -> als +mtool-Key-Segment
        // sanitisiert hinterlegt (unten cache_key_prefix), damit zwei Combos nicht auf denselben Objekt-Key kollidieren.
        c.measurement_combo_ = sanitize_key_segment(env_or_empty("COMDARE_MEASUREMENT_COMBO"));
        // S5-Rest (Blackhole-Wache): curl-Timeouts + Retry-Budget aus env uebersteuerbar (Muster der Env-Gates oben).
        // Defaults konservativ (connect 10s, max-time 120s JE VERSUCH) -> bei ERREICHBAREM Storage feuern sie NIE
        // (keine Verhaltensaenderung); ein Netz-Blackhole/TLS-Stall terminiert nun BOUNDED (tries_ x max_time) statt
        // unbegrenzt zu haengen und die resource_group ceb-measurement-exclusive endlos zu halten.
        c.connect_timeout_s_ = env_size_or("COMDARE_ARTEFAKT_CONNECT_TIMEOUT_S", c.connect_timeout_s_);
        c.max_time_s_        = env_size_or("COMDARE_ARTEFAKT_MAX_TIME_S", c.max_time_s_);
        c.tries_             = env_size_or("COMDARE_ARTEFAKT_TRIES", c.tries_);
        if (c.tries_ == 0) c.tries_ = 1; // nie 0 -> sonst wird nie versucht (stiller Drop)
        c.sleep_s_   = env_size_or("COMDARE_ARTEFAKT_RETRY_SLEEP_S", c.sleep_s_);
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

    /// W11/W12 (§43): die EINE Stelle, an der aus der build_version der Objekt-Store-Key-PRAEFIX wird. SINGLE-SOURCE:
    /// push_tier_binary (DLL-Key) UND push_chunk_partial_marker (Teil-Marker-Key) ziehen den Praefix ausschliesslich
    /// von hier -> KEIN Key-Drift. Die .version/.algos-Sidecar-Semantik bleibt UNBERUEHRT (nur der Key-Praefix).
    ///
    /// S1 (W12-B, §43): der Praefix = die per-Perm build_version (traegt bereits +cxx=/+opt=/+ext=/[+bt=Debug],
    /// profile_run_entry.hpp:652-656) PLUS die drei Segmente, die die Perm-build_version NICHT traegt, aber die
    /// DLL-Bytes mitbestimmen:
    ///   +ceb=<major>.<minor>  CEB-Contract-Version (System/Framework-Ebene): ein ABI-Bump invalidiert ALLE Binaries.
    ///                         Single-Source-identisch zu system_axes_version_suffix (profile_run_facade.cpp:345); der
    ///                         Perm-Pfad-build_version fehlt dieses Segment -> hier eingefaltet (kein Doppel-+ceb, da
    ///                         push_tier_binary/push_chunk_partial_marker NUR die Perm-build_version einspeisen).
    ///   +mtool=<combo>        sanitisierte COMDARE_MEASUREMENT_COMBO: die je-Combo-Bauten stempeln die DLL REAL ohne
    ///                         Wirkung auf die Perm-build_version -> ohne dieses Segment kollidierten zwei Combos auf
    ///                         denselben Key. Leer (Default/[all]) = Default-DLL.
    ///   +mrg=none             Reserve-Segment fuer den spaeteren K6a-Merge-Stempel (#37): JETZT reserviert, damit die
    ///                         PRT-Merge-Einfuehrung den dann befuellten Bucket NICHT ein zweites Mal invalidiert.
    [[nodiscard]] std::string cache_key_prefix(std::string const& build_version) const {
        return build_version + "+ceb=" + std::to_string(COMDARE_ANATOMY_ABI_MAJOR) + "." +
               std::to_string(::comdare::cache_engine::abi::kCebContractCodegenMinor) + "+mtool=" + measurement_combo_ +
               "+mrg=none";
    }

    /// Ebene B: schiebt die Bau-Artefakte EINES Tier-Binary-Ordners in den Objekt-Store. HARTE Reihenfolge perm.dll
    /// ZUERST, perm.dll.algos MITTE (Organ-Provenienz, nur wenn lokal vorhanden), perm.dll.version ZULETZT — die
    /// Vollstaendigkeits-Marke bleibt das LETZTE Objekt: schlaegt perm.dll ODER perm.dll.algos fehl, wird die
    /// .version-Marke NICHT gepusht -> Pull findet keine vollstaendige Marke -> kein falscher Reuse. Objekt-Key =
    /// <key_prefix>/<stem>/perm.dll(+.algos,+.version), stem = bin_dir.filename(). perm.dll.algos ist das Organ-Gate
    /// von dll_is_current (build_orchestrator.hpp:242-246): ohne es ist jeder Remote-Treffer wirkungslos (AlgoSigFn
    /// produktiv IMMER gesetzt); Organ-Gate AUS (keine AlgoSigFn) => .algos-Datei fehlt lokal => sauberer 2-Objekt-
    /// Push (byte-neutral zum Alt-Verhalten). Fehler -> ArtefaktIo geloggt + lokale Kopie bleibt; MESSEN WEITER.
    void push_tier_binary(std::filesystem::path const& bin_dir, std::string const& build_version) const {
        if (!minio_enabled()) return; // No-Op (byte-neutral)
        std::string const stem     = bin_dir.filename().string();
        std::string const key_base = cache_key_prefix(build_version) + "/" + stem; // W11/W12: Single-Source-Praefix
        std::filesystem::path const dll     = bin_dir / "perm.dll";
        std::filesystem::path const algos   = bin_dir / "perm.dll.algos"; // Organ-Provenienz-Sidecar (fehlt = Gate aus)
        std::filesystem::path const sidecar = bin_dir / "perm.dll.version";
        std::filesystem::path const log     = bin_dir / "perm.dll.push.log";

        std::error_code ec;
        if (!std::filesystem::exists(dll, ec)) {
            log_artefakt_io(log, "perm.dll fehlt lokal, nichts zu pushen: " + dll.string());
            return;
        }
        // (1) perm.dll ZUERST — die eigentliche Nutzlast.
        if (!mc_cp(dll, key_base + "/perm.dll", log)) {
            log_artefakt_io(log, "Push perm.dll fehlgeschlagen (lokale Kopie bleibt): " + key_base + "/perm.dll");
            return; // Sidecars bewusst NICHT pushen -> halb-gepusht = kein Pull
        }
        // (2) perm.dll.algos MITTE — die Organ-Provenienz (nur wenn lokal vorhanden; Organ-Gate aus => uebersprungen).
        //     Schlaegt der Push fehl, wird die .version-Marke bewusst NICHT gesetzt: unvollstaendiger Push = kein Pull.
        if (std::filesystem::exists(algos, ec)) {
            if (!mc_cp(algos, key_base + "/perm.dll.algos", log)) {
                log_artefakt_io(log, "Push perm.dll.algos fehlgeschlagen (perm.dll ist oben, Marke bleibt weg -> kein "
                                     "Pull): " +
                                         key_base + "/perm.dll.algos");
                return; // Version-Marke NICHT setzen -> unvollstaendig = kein falscher Reuse
            }
        }
        // (3) perm.dll.version ZULETZT — die Vollstaendigkeits-/Versions-Marke (letztes Objekt).
        if (std::filesystem::exists(sidecar, ec)) {
            if (!mc_cp(sidecar, key_base + "/perm.dll.version", log))
                log_artefakt_io(log,
                                "Push perm.dll.version fehlgeschlagen (perm.dll ist oben, Marke fehlt -> kein Pull): " +
                                    key_base + "/perm.dll.version");
        }
    }

    /// W11 (Ledger §43.c): Ebene B TEIL-Marker. Legt einen kleinen Marker <build_version>/_gn_chunk_markers/
    /// <range>.part<k>.done in den Objekt-Store (range: ':' -> '-', wie die YAML-Whole-Chunk-Marke). Signalisiert:
    /// die ersten k*N DLLs dieses Chunks sind gepusht -> ein resumierender Runner darf den PREFIX pullen und lokal
    /// via dll_is_current uebernehmen (nur die fehlenden werden neu gebaut). No-Op ohne Ebene B. Fehler -> ArtefaktIo
    /// geloggt, Bau LAEUFT WEITER (kein throw). Inhalt = part/utc-Metadaten (nur informativ; die Existenz zaehlt).
    void push_chunk_partial_marker(std::string const& build_version, std::string const& range,
                                   std::size_t part_index) const {
        if (!minio_enabled()) return; // No-Op (byte-neutral)
        std::string range_key = range;
        for (char& c : range_key)
            if (c == ':') c = '-'; // deckungsgleich mit der YAML-Marke ${GN_RANGE//:/-}
        std::string const object_key = cache_key_prefix(build_version) + "/_gn_chunk_markers/" + range_key + ".part" +
                                       std::to_string(part_index) + ".done"; // W11/W12: Single-Source-Praefix

        // Kleinen lokalen Marker schreiben (temp), pushen, entfernen. Der Inhalt ist rein informativ.
        std::error_code             ec;
        std::filesystem::path const tmp =
            std::filesystem::temp_directory_path(ec) /
            ("comdare_gn_part_" + range_key + "_" + std::to_string(part_index) + "_" + run_stamp_ + ".done");
        std::filesystem::path const log = tmp.string() + ".push.log";
        {
            std::ofstream f{tmp, std::ios::binary | std::ios::trunc};
            if (!f) {
                log_artefakt_io(log, "Teil-Marker lokal nicht schreibbar: " + tmp.string());
                return;
            }
            f << "part=" << part_index << " range=" << range << " build_version=" << build_version << "\n";
        }
        if (!mc_cp(tmp, object_key, log))
            log_artefakt_io(log, "Teil-Marker-Push fehlgeschlagen (Bau laeuft weiter): " + object_key);
        std::filesystem::remove(tmp, ec);
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
            // --connect-timeout/--max-time (S5-Rest): kappen Verbindungsaufbau + Gesamtdauer JE VERSUCH -> ein
            // haengender curl (Netz-Blackhole/TLS-Stall) blockiert nicht mehr unbegrenzt (bounded ueber tries_).
            int const  rc = run_argv({curl_bin_, "--connect-timeout", std::to_string(connect_timeout_s_), "--max-time",
                                      std::to_string(max_time_s_), "-K", cfg.string()},
                                     out);
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
        // measure-drop authentifiziert per HTTP Basic Auth (nginx htpasswd, HANDOFF3), NICHT per Bearer:
        // 11401-Beleg 19.07.: Bearer -> 401, Basic 'measure:<token>' -> 201. curl-Config-Direktive 'user'.
        if (!token_.empty()) body += "user = \"" + drop_user_ + ":" + token_ + "\"\n";
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

    /// Liest eine nicht-negative Ganzzahl aus env (leer/unparsebar -> fallback). Fuer die curl-Timeout-/Retry-Gates
    /// (COMDARE_ARTEFAKT_*): eine ungueltige Vorgabe faellt HONEST auf den konservativen Default zurueck (nie throw).
    [[nodiscard]] static std::size_t env_size_or(char const* name, std::size_t fallback) {
        std::string const v = env_or_empty(name);
        if (v.empty()) return fallback;
        std::size_t value = 0;
        for (char const ch : v) {
            if (ch < '0' || ch > '9') return fallback; // unparsebar -> Default
            value = value * 10 + static_cast<std::size_t>(ch - '0');
        }
        return value;
    }

    /// Sanitisiert ein Objekt-Key-Segment IDENTISCH zu orch_make_stem/orch_sanitize (build_orchestrator.hpp:178):
    /// jedes Nicht-alnum-Zeichen wird '_'. Haelt den Key path-/shell-sicher (z.B. '[a,b,c]' -> '_a_b_c_', kein '/').
    [[nodiscard]] static std::string sanitize_key_segment(std::string_view s) {
        std::string out;
        out.reserve(s.size());
        for (char const c : s) out += (std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
        return out;
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

    std::string endpoint_;                // COMDARE_MINIO_ENDPOINT (mc-Alias-Name; leer = Ebene B aus)
    std::string bucket_;                  // COMDARE_MINIO_BUCKET
    std::string prefix_;                  // COMDARE_MINIO_PREFIX (optional)
    std::string drop_url_;                // COMDARE_MEASUREMENT_DROP_URL (measure-drop Basis-URL; leer = Ebene C aus)
    std::string token_;                   // COMDARE_NFS_DROP_TOKEN (NIE geloggt, NIE in argv — nur 0600-curl-Config)
    std::string drop_user_{"measure"};    // COMDARE_NFS_DROP_USER (Basic-Auth-User; HANDOFF3-Default 'measure')
    std::string mc_bin_   = "mc";         // COMDARE_MC_BIN override (Default via PATH)
    std::string curl_bin_ = "curl";       // COMDARE_CURL_BIN override (Default via PATH)
    std::string measurement_combo_;       // COMDARE_MEASUREMENT_COMBO sanitisiert (+mtool-Segment des Objekt-Key)
    std::string run_stamp_;               // datierter Lauf-Baum (Sink-Besitzer des Timestamps)
    std::size_t tries_             = 12;  // Retry-Zahl (Vorbild copy_results_to_nas.sh: 12; COMDARE_ARTEFAKT_TRIES)
    std::size_t sleep_s_           = 5;   // Pause zwischen Versuchen (s; COMDARE_ARTEFAKT_RETRY_SLEEP_S)
    std::size_t connect_timeout_s_ = 10;  // curl --connect-timeout je Versuch (COMDARE_ARTEFAKT_CONNECT_TIMEOUT_S)
    std::size_t max_time_s_        = 120; // curl --max-time je Versuch (COMDARE_ARTEFAKT_MAX_TIME_S)
};

} // namespace comdare::cache_engine::builder::artifact_transport
