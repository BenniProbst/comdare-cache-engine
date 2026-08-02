#pragma once
// Storage #51 (2026-07-18) — artifact_transport/artifact_cache: die schmale, SYNCHRONE Transport-Klasse der
// Zwei-Cache-Storage-Naht (BAUPLAN-TWO-CACHE-STORAGE §3 / VERORTUNGS-BRIEF §1). Zwei getrennte Speicher-Ebenen:
//
//   B  CEB/Tier-Binary-Bau-Artefakte -> minio-Objekt-Store (mc-Shellout). Objekt-Key
//      cache_key_prefix(build_version)/<stem>/perm.dll(+ kOptionalTierSidecars +.version); Vollstaendigkeits-Marke:
//      perm.dll ZUERST, perm.dll.version ZULETZT (halb-gepusht = keine Marke = kein Pull). Der Key KOPPELT an dieselbe
//      build_version-Signatur, die dll_is_current lokal prueft -> stale ABI (5->6) wird NIE reused.
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

#include <array> // #13 (B25): kOptionalTierSidecars -- die EINE Push-/Pull-Reihenfolge der optionalen Sidecars
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
#include <iostream>  // std::cerr (klassifizierte Infra-Fehlerzeile) -- include-what-you-use, nicht transitiv verlassen
#include <optional>  // G5 (P-B): prune_verdict-Remote-Werte (std::optional)
#include <stdexcept> // TP1FK1-B2: ArtefaktPushFehler (sichtbarer Transport-Fehler des Tier-Push)
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

// G5 (P-B, Ledger Section 65/66): reine Prune-Entscheidung (Fake-testbar, keine mc-Abhaengigkeit).
// Lokal loeschen NUR, wenn der Remote-Bestand die lokale Binary BEWEISBAR spiegelt: remote .version
// existiert UND ist byte-gleich zur lokalen .version (Provenienz) UND die remote perm.dll-Groesse ist
// gleich der lokalen. JEDER Zweifel (fehlend / mismatch / unbekannt / keine lokale .version) => KEIN
// Loeschen (behalten). So kann ein Netz-/stat-Fehler NIE ein Artefakt vernichten.
struct PruneVerdict {
    bool        prune = false;
    std::string reason;
};
[[nodiscard]] inline PruneVerdict prune_verdict(std::string const&                   local_version,
                                                std::optional<std::string> const&    remote_version,
                                                std::uintmax_t                       local_size,
                                                std::optional<std::uintmax_t> const& remote_size) {
    if (local_version.empty()) return {false, "no_local_version"};
    if (!remote_version) return {false, "remote_version_missing"};
    if (*remote_version != local_version) return {false, "version_mismatch"};
    if (!remote_size) return {false, "remote_size_unknown"};
    if (*remote_size != local_size) return {false, "size_mismatch"};
    return {true, "verified"};
}

// #13 (B25/L-d): die OPTIONALEN Mittel-Sidecars EINES Tier-Binary-Ordners, in HARTER Push-Reihenfolge. Die EINE
// Liste, aus der push_tier_binary, pull_tier_binary UND prunable_artifacts lesen -- keine der drei Richtungen kann
// damit von den anderen abdriften (der Defekt, der `.fingerprint` ueberhaupt erst aus dem Push fallen liess).
// perm.dll (Nutzlast, ZUERST) und perm.dll.version (Vollstaendigkeits-Marke, ZULETZT) stehen bewusst NICHT darin:
// ihre Position IST das Protokoll und darf nicht in einer Schleife verschwinden.
//   perm.dll.algos       Organ-Provenienz (build_orchestrator.hpp:234 algo_sidecar_path) -- Organ-Gate dll_is_current
//   perm.dll.fingerprint 128-hex K7b-Lager-Anker (fingerprint_sidecar.hpp:34) -- ohne ihn liefert bestand_key_of
//                        nullopt (DedupOutcome::no_key) und die hydrierte Binary ist fuer das Lager UNSICHTBAR
//   perm.dll.variant     Build-Varianten-Signatur (build_orchestrator.hpp:242) -- Variant-Gate von dll_is_current
// Die drei Suffixe stehen hier als Literale, wie schon "perm.dll"/"perm.dll.version" in dieser Datei: der Transport
// haengt bewusst NICHT am build_orchestrator (die Abhaengigkeit zeigt nur in die andere Richtung).
inline constexpr std::array<char const*, 3> kOptionalTierSidecars{"perm.dll.algos", "perm.dll.fingerprint",
                                                                  "perm.dll.variant"};

// G5 (P-B): die EINZIGEN Dateien, die geprunt werden duerfen -- die Tier-Binary, ihre Vollstaendigkeits-Marke und
// ihre optionalen Sidecars. NIEMALS result.csv / measure_out / Logs (Messdaten-nie-loeschen-Doktrin, HART --
// prune.log ueberlebt, weil es hier NICHT aufgelistet ist). Single-Source der Prune-Menge (Test +
// verify_remote_then_prune).
//
// #13-Nachtrag: die Menge zieht die Sidecars aus kOptionalTierSidecars und ist damit per Konstruktion
// DECKUNGSGLEICH mit dem Push-Satz. Vorher endeten .fingerprint/.variant nach einem Prune als WAISEN neben einer
// geloeschten perm.dll -- und ein `.fingerprint` ohne Binary ist eine Phantom-Quelle fuer bestand_key_of (das Lager
// haette eine Binary gemeldet, die lokal nicht mehr existiert). Ein Objekt, das nie erzeugt wurde, ist hier
// unschaedlich: verify_remote_then_prune loescht ueber std::filesystem::remove, das ein fehlendes Ziel ignoriert.
// EHRLICHE Grenze: hat ein AELTERER Push das .fingerprint noch nicht mitgeschoben, nimmt der Prune es lokal
// dennoch mit -- eine spaetere Hydrierung liefert dann keinen Lager-Anker. Das ist genau der Alt-Zustand vor #13
// (Lager-Unsichtbarkeit, nicht-fatal, dll_is_current traegt sich weiter ueber .version) und bewusst dem
// Phantom-Eintrag vorgezogen.
[[nodiscard]] inline std::vector<std::filesystem::path> prunable_artifacts(std::filesystem::path const& bin_dir) {
    std::vector<std::filesystem::path> out;
    out.reserve(2 + kOptionalTierSidecars.size());
    out.push_back(bin_dir / "perm.dll");
    out.push_back(bin_dir / "perm.dll.version");
    for (char const* leaf : kOptionalTierSidecars) out.push_back(bin_dir / leaf);
    return out;
}

// G5 (P-B): Zustand einer Prune-Pruefung je Stem. verified = Remote wurde geprueft (pruned+kept);
// skipped = inert (kein minio) ODER keine lokale Binary (nichts zu tun).
enum class PruneState { pruned, kept, skipped };
struct PruneOutcome {
    PruneState  state = PruneState::skipped;
    std::string reason;
};

// Folge-A (T1, Ledger §62-NACHTRAG-4): Metadaten EINES Objekts unter EINEM Key -- der Rueckgabe-POD von
// ArtifactCache::object_stat. LOKAL zu dieser Schicht: er kennt weder das Bestandslog noch dessen
// Transport-Naht (die Uebersetzung in bestandslog::ObjectStat macht der eine Binder,
// bestandslog/artifact_cache_transport.hpp) -> die Abhaengigkeit zeigt NUR in diese Richtung.
//
// mtime_epoch_s ist EHRLICH begrenzt: die mc-stat-Naht dieses Clients parst ausschliesslich das
// size-Feld (parse_json_size) -- der Wert bleibt deshalb 0 == "nicht ermittelt" und darf NIE als
// echter Zeitstempel gelesen werden. Er steht hier, weil die Transport-Naht ihn im Vertrag fuehrt;
// ein Konsument, der ihn braucht, muss zuerst den lastModified-Parser nachziehen (dann faellt dieser
// Kommentar). Kein Phantom-Wert: 0 ist als "unbekannt" definiert, nicht als "1970".
struct ObjectMeta {
    std::uint64_t size          = 0;
    std::int64_t  mtime_epoch_s = 0; // 0 == unbekannt (diese Naht ermittelt keine mtime)
};

// -- TP1FK1-B2 (Codex-Befund): der SICHTBARE Transport-Fehler des Tier-Push ------------------------------------
// BEFUND: push_tier_binary loggte einen fehlgeschlagenen mc-Push und kehrte zurueck. Fuer JEDEN Aufrufer war das
// von einem Erfolg nicht zu unterscheiden -- die CachePushFn ist void. Zwei Folgen, beide real:
//   * AsyncPushPump::failed_dirs() (TP1-N2/B-1) sieht nur GEWORFENE Pushes. Mit einem log+return-Transport war die
//     Liste per Konstruktion IMMER leer -> der B-1-Ausschluss (Bestandslog-Eintraege ohne Store-Objekt verwerfen)
//     lief ins Leere und das geteilte Dokument meldete Binaries, die der Store nie erhielt. Der Bau-Filter des
//     Folgelaufs uebersprang dann eine nirgends existierende Binary -- der stille Verlustpfad.
//   * der synchrone Mess-Pfad-Push konnte gar nicht erst merken, dass er nichts abgelegt hat.
// FIX: ein fehlgeschlagener Objekt-Push WIRFT. Die ArtefaktIo-Log-Zeile NEBEN der Datei bleibt unveraendert
// (Klassifikation + Pfad + mc-Ausgabe); die Exception traegt zusaetzlich Pfad, Objekt-Key und den letzten
// mc-Exit-Code, damit der Faenger nicht raten muss. WER FAENGT, ENTSCHEIDET: der Pump verbucht failed_dirs und
// baut weiter, der Mess-Pfad loggt klassifiziert und MISST WEITER -- kein Pfad bricht einen Lauf ab.
class ArtefaktPushFehler : public std::runtime_error {
public:
    ArtefaktPushFehler(std::string const& was, std::filesystem::path lokal, std::string object_key, int exit_code)
        : std::runtime_error{was + " -- lokal='" + lokal.string() + "' objekt_key='" + object_key +
                             "' letzter_mc_exit=" + std::to_string(exit_code)},
          lokal_{std::move(lokal)}, object_key_{std::move(object_key)}, exit_code_{exit_code} {}

    [[nodiscard]] std::filesystem::path const& lokal() const noexcept { return lokal_; }
    [[nodiscard]] std::string const&           object_key() const noexcept { return object_key_; }
    /// Exit des LETZTEN mc-Versuchs. 0 bedeutet "mc cp meldete Erfolg, aber der Groessen-Verify war negativ".
    [[nodiscard]] int exit_code() const noexcept { return exit_code_; }

private:
    std::filesystem::path lokal_;
    std::string           object_key_;
    int                   exit_code_;
};

// ── Injektions-Naht-Typen (No-Op-Default = leere std::function -> byte-neutral). ──────────────────────────────
// Muster wie CompileFn/AlgoSigFn (build_orchestrator.hpp): der Iterator ruft sie SYNCHRON an der per-Binary-Naht.
// CachePushFn: ein fertig gebautes Tier-Binary-Verzeichnis -> Objekt-Store (Ebene B). Args (bin_dir, build_version);
//   der Client leitet Objekt-Key cache_key_prefix(build_version)/<stem>/perm.dll(+.version) ab (stem = bin_dir.filename()).
//   TP1FK1-B2: die Naht DARF WERFEN (ArtefaktPushFehler beim realen Client) -- ein Transport-Fehler ist kein
//   Rueckgabewert dieser void-Signatur und war deshalb bisher unsichtbar. Jeder Aufrufer faengt (Pump: failed_dirs;
//   Mess-Pfad: klassifiziertes Log + weitermessen); ein Lauf-Abriss ist an KEINER Stelle erlaubt.
using CachePushFn = std::function<void(std::filesystem::path const& bin_dir, std::string const& build_version)>;
// MeasurementSinkFn: eine Mess-Datei additiv an die write-only measure-drop-Senke legen (Ebene C, HTTPS-PUT). Args
//   (local_file, relative_dest); Ziel-URL = <drop_url>/<lauf_stamp>/<relative_dest>. Leer = No-Op.
using MeasurementSinkFn =
    std::function<void(std::filesystem::path const& local_file, std::string const& relative_dest)>;
// W11 (Ledger §43.c): PartialMarkerFn -- nach je N gepushten DLLs im BAU-Modus feuert der async Push-Pump einen
//   TEIL-Marker in den Objekt-Store, damit ein Runner, der einen abgebrochenen Chunk-Job wieder aufnimmt, die
//   bereits gepushten DLLs dieses Chunks pullen kann (Cluster-Resume). Args (build_version, part_index); der Client
//   leitet den Key cache_key_prefix(build_version)/_gn_chunk_markers/<range>.part<k>.done ab (range im Client-Closure gekapselt).
//   Leer = No-Op (byte-neutral). Reihenfolge-unabhaengig (Objekt-Store), daher aus dem Push-Thread erlaubt.
using PartialMarkerFn = std::function<void(std::string const& build_version, std::size_t part_index)>;
// S2 (#46a Pull): CachePullFn -- die BATCH-Warm-Cache-Hydrierung VOR dem Bau. Der Iterator ruft sie EINMAL in Phase A
//   (VOR provision_all), sie zieht den ganzen Objekt-Store-Praefix EINER Perm rekursiv nach dest_root (=output_dir), so
//   dass dll_is_current die gepullten DLLs als versions-aktuell erkennt und den Rebuild ueberspringt. Args (dest_root,
//   build_version). Leer = No-Op (byte-neutral). Der Host belegt sie via ArtifactCache::pull_tier_prefix.
using CachePullFn = std::function<void(std::filesystem::path const& dest_root, std::string const& build_version)>;

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
        c.sleep_s_ = env_size_or("COMDARE_ARTEFAKT_RETRY_SLEEP_S", c.sleep_s_);
        // S2 (#46a Pull): EIGENES, knappes Budget fuer die Warm-Cache-Hydrierung -- ein MISS/Netz-Blackhole darf den
        // Bau-Start NICHT lange blockieren (im Gegensatz zum Push, der 12x zaehen darf). Default 2 Versuche, 1s Pause;
        // env-uebersteuerbar. (Der harte per-Aufruf-Wall-Clock-Cap kommt UNTEN als timeout-Wrapper; das knappe
        // try-Budget bounded zusaetzlich die Retry-ZAHL.)
        c.pull_tries_ = env_size_or("COMDARE_ARTEFAKT_PULL_TRIES", c.pull_tries_);
        if (c.pull_tries_ == 0) c.pull_tries_ = 1; // nie 0
        c.pull_sleep_s_ = env_size_or("COMDARE_ARTEFAKT_PULL_RETRY_SLEEP_S", c.pull_sleep_s_);
        // Cache-Resthygiene (2026-07-21): harter Wall-Clock-Cap fuer die mc-Shellouts (Push UND Pull) gegen Netz-
        // Blackhole/TLS-Stall (mc hat keinen eigenen per-Aufruf-Timeout). `timeout <s>`-Prefix, WENN das timeout-Binary
        // vorhanden ist (sonst unveraendert). Default AN -- Blackhole-Schutz IST der Zweck: Push 120s, Pull 20s (kurz,
        // damit ein MISS den Bau-Start nicht blockiert); env-uebersteuerbar. Nur bei Ebene B relevant -> nur dann proben.
        if (std::string const tb = env_or_empty("COMDARE_TIMEOUT_BIN"); !tb.empty()) c.timeout_bin_ = tb;
        c.mc_push_timeout_s_ = env_size_or("COMDARE_MC_TIMEOUT_S", c.mc_push_timeout_s_);
        c.mc_pull_timeout_s_ = env_size_or("COMDARE_MC_PULL_TIMEOUT_S", c.mc_pull_timeout_s_);
        if (!c.endpoint_.empty() && !c.bucket_.empty()) // Ebene B: einmalige Verfuegbarkeits-Probe (sonst kein Prefix)
            c.timeout_available_ = probe_timeout(c.timeout_bin_);
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
    /// Lane F R3 (O-8 Schritt 10): `build_version` kommt ab hier VOLLSTAENDIG aus der EINEN Suffix-Quelle
    /// (profile_facade/system_version_suffix.hpp) -- diese Funktion baut den System-Achsen-Teil also nicht
    /// mehr nach, sie konsumiert ihn. Was sie ANHAENGT, sind ausschliesslich CACHE-scoped Segmente
    /// (+ceb/+mtool/+mrg): sie beschreiben den Objekt-Bucket, nicht die System-Achsen-Belegung, und
    /// gehoeren deshalb bewusst NICHT in kSuffixSegmentOrder.
    [[nodiscard]] std::string cache_key_prefix(std::string const& build_version) const {
        return build_version + "+ceb=" + std::to_string(COMDARE_ANATOMY_ABI_MAJOR) + "." +
               std::to_string(::comdare::cache_engine::abi::kCebContractCodegenMinor) + "+mtool=" + measurement_combo_ +
               "+mrg=none";
    }

    /// Ebene B: schiebt die Bau-Artefakte EINES Tier-Binary-Ordners in den Objekt-Store. HARTE Reihenfolge perm.dll
    /// ZUERST, dann die optionalen Sidecars in der Reihenfolge von kOptionalTierSidecars (.algos, .fingerprint,
    /// .variant), perm.dll.version ZULETZT -- die Vollstaendigkeits-Marke bleibt das LETZTE Objekt: schlaegt perm.dll
    /// ODER einer der vorhandenen Sidecars fehl, wird die .version-Marke NICHT gepusht -> Pull findet keine
    /// vollstaendige Marke -> kein falscher Reuse. Objekt-Key = <key_prefix>/<stem>/perm.dll(+.algos,+.fingerprint,
    /// +.variant,+.version), stem = bin_dir.filename(). Ein Sidecar, das LOKAL FEHLT, wird still uebersprungen (kein
    /// Fehler): jedes der drei haengt an einem eigenen opt-in (AlgoSigFn / bestand_fingerprint_fn / build_variant_sig),
    /// und "nicht erzeugt" ist der legitime Default -> Gate aus => weniger Objekte, byte-neutral zum Alt-Verhalten.
    /// Ein Sidecar, das lokal DA ist und dessen Push FEHLSCHLAEGT, ist dagegen ein unvollstaendiger Satz und haelt die
    /// Marke zurueck (Praezedenz .algos): der Remote-Satz ist per Konstruktion entweder der lokale Satz oder gar nichts.
    ///
    /// #13 (B25/L-d): .fingerprint und .variant kamen NACH -- ohne sie trug eine hydrierte Binary nie einen
    /// Lager-Anker (bestand_key_of -> nullopt -> DedupOutcome::no_key: das Lager konnte genau das nicht
    /// inventarisieren, was es selbst hydriert hat) und nie ihre Varianten-Provenienz (dll_is_current haette bei
    /// gesetzter Variant-Erwartung neu gebaut). Fehler -> ArtefaktIo geloggt + lokale Kopie bleibt; MESSEN WEITER.
    ///
    /// TP1FK1-B2 (Codex-Befund): JEDER der drei Fehl-Ausgaenge WIRFT jetzt ArtefaktPushFehler (mit lokalem Pfad,
    /// Objekt-Key und letztem mc-Exit) -- zusaetzlich zur unveraenderten ArtefaktIo-Log-Zeile neben der Datei. Vorher
    /// war log+return fuer die void-CachePushFn von einem Erfolg ununterscheidbar: AsyncPushPump::failed_dirs() blieb
    /// per Konstruktion leer, der B-1-Bestandslog-Ausschluss lief ins Leere, und das geteilte Dokument meldete
    /// Binaries, die der Store nie erhielt. "MESSEN WEITER" bleibt gueltig -- es ist jetzt eine Entscheidung des
    /// FAENGERS (Pump/Mess-Pfad) statt einer stillen Eigenschaft des Transports. Der Fall "perm.dll fehlt lokal" ist
    /// KEIN Transport-Fehler und wirft weiterhin nicht (es gab nichts zu pushen).
    void push_tier_binary(std::filesystem::path const& bin_dir, std::string const& build_version) const {
        if (!minio_enabled()) return; // No-Op (byte-neutral)
        std::string const stem     = bin_dir.filename().string();
        std::string const key_base = cache_key_prefix(build_version) + "/" + stem; // W11/W12: Single-Source-Praefix
        std::filesystem::path const dll     = bin_dir / "perm.dll";
        std::filesystem::path const sidecar = bin_dir / "perm.dll.version";
        std::filesystem::path const log     = bin_dir / "perm.dll.push.log";

        std::error_code ec;
        if (!std::filesystem::exists(dll, ec)) {
            // KEIN Transport-Fehler: es gibt schlicht nichts zu pushen (kein Wurf -- der Aufrufer haette
            // nichts zu entscheiden). Die Log-Zeile bleibt, damit der Fall nicht stumm ist.
            log_artefakt_io(log, "perm.dll fehlt lokal, nichts zu pushen: " + dll.string());
            return;
        }
        // TP1FK1-B2 (Codex-Befund CX-W2/CX-W3): die perm.dll IST da, aber die lokale Vollstaendigkeits-
        // Marke perm.dll.version FEHLT. Frueher fiel Schritt (3) dann still aus (kein else) und
        // push_tier_binary kehrte normal zurueck -- fuer die void-CachePushFn ununterscheidbar von einem
        // vollstaendigen Push. Der Pull verlangt aber genau diese Remote-Marke (s.u. (0)) und liefert
        // sonst MISS: ein Bestandslog-Anspruch auf einen Satz, den kein Pull findet -- dieselbe B2-Klasse,
        // nur ueber die ABWESENHEIT der Marke statt ueber den Fehlschlag. Deshalb hier ein SICHTBARER
        // Fehler VOR jedem Upload (keine halbe Nutzlast in den Store). Das ist NICHT der non-werfende Fall
        // "perm.dll fehlt lokal" (da gab es nichts zu pushen) -- hier gaebe es etwas zu pushen, aber ohne
        // Marke waere es ein Phantom. write_version_sidecar (build_orchestrator) schreibt die Marke nach
        // JEDEM erfolgreichen Bau; fehlt sie hier, ist der lokale Satz selbst schon unvollstaendig.
        if (!std::filesystem::exists(sidecar, ec)) {
            log_artefakt_io(log, "perm.dll.version fehlt lokal (Vollstaendigkeits-Marke) -- der Satz kann nie "
                                 "vollstaendig gepusht werden, kein Pull faende ihn: " +
                                     sidecar.string());
            throw ArtefaktPushFehler{"perm.dll.version fehlt lokal (Vollstaendigkeits-Marke, ohne die kein Pull den "
                                     "Satz findet -- der Store bekaeme eine Phantom-Binary)",
                                     sidecar, key_base + "/perm.dll.version", -1};
        }
        int exit_code = -1;
        // (1) perm.dll ZUERST — die eigentliche Nutzlast.
        if (!mc_cp(dll, key_base + "/perm.dll", log, &exit_code)) {
            log_artefakt_io(log, "Push perm.dll fehlgeschlagen (lokale Kopie bleibt): " + key_base + "/perm.dll");
            // TP1FK1-B2: Sidecars bewusst NICHT pushen (halb-gepusht = kein Pull) UND werfen -- ohne den Wurf
            // waere dieser Ausgang fuer die void-CachePushFn von einem Erfolg nicht zu unterscheiden.
            throw ArtefaktPushFehler{"Push perm.dll fehlgeschlagen (lokale Kopie bleibt, kein Objekt im Store)", dll,
                                     key_base + "/perm.dll", exit_code};
        }
        // (2) die OPTIONALEN Sidecars MITTE, in Listen-Reihenfolge (.algos -> .fingerprint -> .variant). Fehlt die
        //     lokale Datei, wird sie uebersprungen; schlaegt ihr Push fehl, bleibt die .version-Marke bewusst weg.
        for (char const* leaf : kOptionalTierSidecars) {
            std::filesystem::path const local = bin_dir / leaf;
            if (!std::filesystem::exists(local, ec)) continue; // Gate aus / nicht erzeugt -> nichts zu spiegeln
            if (!mc_cp(local, key_base + "/" + leaf, log, &exit_code)) {
                log_artefakt_io(log, std::string{"Push "} + leaf +
                                         " fehlgeschlagen (perm.dll ist oben, Marke bleibt weg -> kein Pull): " +
                                         key_base + "/" + leaf);
                // TP1FK1-B2: der Satz ist unvollstaendig -- Version-Marke NICHT setzen (kein falscher Reuse) und
                // den Fehler sichtbar machen. Ein Bestandslog-Eintrag auf einen halben Satz waere ein Phantom.
                throw ArtefaktPushFehler{
                    std::string{"Push "} + leaf +
                        " fehlgeschlagen (Satz unvollstaendig, Vollstaendigkeits-Marke bleibt weg)",
                    local, key_base + "/" + leaf, exit_code};
            }
        }
        // (3) perm.dll.version ZULETZT — die Vollstaendigkeits-/Versions-Marke (letztes Objekt).
        if (std::filesystem::exists(sidecar, ec)) {
            if (!mc_cp(sidecar, key_base + "/perm.dll.version", log, &exit_code)) {
                log_artefakt_io(log,
                                "Push perm.dll.version fehlgeschlagen (perm.dll ist oben, Marke fehlt -> kein Pull): " +
                                    key_base + "/perm.dll.version");
                // TP1FK1-B2: OHNE die Marke findet kein Pull den Satz -- fuer den Store ist diese Binary nicht
                // vorhanden. Genau deshalb muss der Aufrufer es erfahren (Bestandslog-Ausschluss).
                throw ArtefaktPushFehler{
                    "Push perm.dll.version fehlgeschlagen (ohne Vollstaendigkeits-Marke ist der Satz fuer den Pull "
                    "nicht vorhanden)",
                    sidecar, key_base + "/perm.dll.version", exit_code};
            }
        }
    }

    /// S2 (#46a) Ebene B PULL (Spiegel zu push_tier_binary): holt den Objekt-Satz EINES Tier-Binary-Ordners aus dem
    /// Objekt-Store nach bin_dir. Uebernahme NUR, wenn die REMOTE-Vollstaendigkeits-Marke perm.dll.version existiert
    /// (invertierte ZULETZT-Pruefung: halb-gepusht ohne Marke => MISS => lokal bauen). HARTE Reihenfolge lokal: die
    /// lokale .version UND alle kOptionalTierSidecars ZUERST WEG (write-ZULETZT-Disziplin -> ein Teil-Pull hinterlaesst
    /// NIE eine irrefuehrende Marke, und ein STEHENGEBLIEBENES Sidecar eines fruheren Baus kann dll_is_current nie eine
    /// Provenienz vortaeuschen, die der Remote-Satz nicht deckt), dann perm.dll, dann die optionalen Sidecars in
    /// Listen-Reihenfolge (je nur, wenn remote vorhanden), dann perm.dll.version ZULETZT. Eigenes knappes
    /// Timeout-Budget (pull_tries_, nicht die Push-Defaults). Fehler -> ArtefaktIo geloggt, kein throw, Rueckgabe
    /// false => der Aufrufer BAUT (dll_is_current arbitriert danach ohnehin lokal). true = vollstaendiger Satz hydriert.
    ///
    /// KOSTEN, ehrlich: je optionalem Sidecar kostet die Existenz-Probe EIN zusaetzliches `mc stat` (drei statt eins).
    /// Das ist der per-Binary-Weg; die BATCH-Hydrierung des Voll-Baus laeuft ueber pull_tier_prefix (EIN mc mirror,
    /// keine Zusatz-Spawns).
    [[nodiscard]] bool pull_tier_binary(std::filesystem::path const& bin_dir, std::string const& build_version) const {
        if (!minio_enabled()) return false; // inert (byte-neutral)
        std::string const           stem     = bin_dir.filename().string();
        std::string const           key_base = cache_key_prefix(build_version) + "/" + stem; // Single-Source-Praefix
        std::filesystem::path const dll      = bin_dir / "perm.dll";
        std::filesystem::path const version  = bin_dir / "perm.dll.version";
        std::filesystem::path const log      = bin_dir / "perm.dll.pull.log";

        // (0) REMOTE-Vollstaendigkeits-Marke pruefen: ohne remote perm.dll.version war der Push halb/nie -> MISS.
        if (!mc_remote_exists(key_base + "/perm.dll.version")) return false; // MISS -> bauen (bin_dir NICHT angelegt)

        std::error_code ec;
        std::filesystem::create_directories(bin_dir, ec);
        std::filesystem::remove(version, ec); // Marke ZUERST weg -> ein abgebrochener Pull ergibt garantiert Neubau
        for (char const* leaf : kOptionalTierSidecars)
            std::filesystem::remove(bin_dir / leaf, ec); // lokal exakt der remote Satz nach dem Pull

        // (1) perm.dll ZUERST.
        if (!mc_pull(key_base + "/perm.dll", dll, log)) {
            log_artefakt_io(log, "Pull perm.dll fehlgeschlagen -> lokal bauen: " + key_base + "/perm.dll");
            return false;
        }
        // (2) die optionalen Sidecars MITTE, in Listen-Reihenfolge -- je nur wenn remote vorhanden (Gate aus beim Push
        //     => remote fehlt => uebersprungen, kein Fehler). Ein remote VORHANDENES, aber nicht holbares Sidecar ist
        //     dagegen ein Fehlschlag: die Marke bleibt weg, dll_is_current baut neu.
        for (char const* leaf : kOptionalTierSidecars) {
            std::string const key = key_base + "/" + leaf;
            if (!mc_remote_exists(key)) continue;
            if (!mc_pull(key, bin_dir / leaf, log)) {
                log_artefakt_io(log, std::string{"Pull "} + leaf + " fehlgeschlagen -> lokal bauen: " + key);
                return false; // .version bleibt weg -> dll_is_current baut neu
            }
        }
        // (3) perm.dll.version ZULETZT — die lokale Vollstaendigkeits-Marke.
        if (!mc_pull(key_base + "/perm.dll.version", version, log)) {
            log_artefakt_io(log,
                            "Pull perm.dll.version fehlgeschlagen -> lokal bauen: " + key_base + "/perm.dll.version");
            return false;
        }
        return true;
    }

    /// S2 (#46a) BATCH-Warm-Cache-Hydrierung (der Iterator-PULL-HOOK, VOR provision_all): zieht den GANZEN Objekt-Store-
    /// Praefix EINER Perm rekursiv nach dest_root -> dest_root/<stem>/perm.dll + JEDES daneben liegende Sidecar. Diese
    /// Naht ist SUFFIX-BLIND (`mc mirror` ueber den Praefix, keine Objekt-Liste) und traegt die #13-Sidecars .fingerprint
    /// /.variant deshalb OHNE Code-Aenderung mit -- anders als pull_tier_binary, das je Suffix einzeln probt. EIN
    /// mc-Prozess (nicht
    /// x|Binaries| Spawns; vgl. Dossier Option (a) vs (b)). Der _gn_chunk_markers-Namensraum wird ausgespart (Marker sind
    /// keine Tier-Binaries; orch_make_stem erzeugt diesen Stem nie). Korrektheit entscheidet danach AUSSCHLIESSLICH lokal
    /// dll_is_current (ein Teil-Pull/False-Pull => der betroffene Stem hat kein/kein passendes .version/.algos => Neubau).
    /// Leerer/fehlender Praefix (erste Welle) => mc-Fehler => false => alles lokal bauen. Kein throw; No-Op wenn inert.
    bool pull_tier_prefix(std::string const& build_version, std::filesystem::path const& dest_root) const {
        if (!minio_enabled()) return false; // inert (byte-neutral)
        std::error_code ec;
        std::filesystem::create_directories(dest_root, ec);
        std::string src = mc_target(cache_key_prefix(build_version)); // <alias>/<bucket>/[<prefix>/]<KEY>
        if (src.empty() || src.back() != '/') src += '/';             // mc mirror erwartet SRC-Verzeichnis
        std::string dst = dest_root.string();
        if (dst.empty() || dst.back() != '/') dst += '/';
        std::filesystem::path const log = dest_root / "perm.pull.recursive.log";
        for (std::size_t attempt = 1; attempt <= pull_tries_; ++attempt) {
            // G5 (P-B, Design-Vorbedingung Hydration): `mc mirror` STATT `mc cp --recursive` -- INKREMENTELL
            // (kopiert nur neue/geaenderte Objekte) und damit retry-RESUMIERBAR: ein abgebrochener Pull setzt beim
            // naechsten Versuch fort statt alles neu zu ziehen. `--exclude` spart den Marker-Namensraum aus (best-
            // effort; Korrektheit haengt NICHT daran -- Marker tragen kein perm.dll, dll_is_current ignoriert sie,
            // kein realer Stem == _gn_chunk_markers). mc_pull_timeout_s_ = COMDARE_MC_PULL_TIMEOUT_S (Wall-Clock-Cap).
            int const rc = run_argv(
                mc_argv({mc_bin_, "mirror", "--exclude", "_gn_chunk_markers/*", src, dst}, mc_pull_timeout_s_), log);
            if (rc == 0) return true;
            if (attempt < pull_tries_) sleep_seconds(pull_sleep_s_);
        }
        return false; // MISS (leerer Praefix / Netzfehler) -> lokal bauen; dll_is_current bleibt der Arbiter
    }

    /// FIX A (2026-07-26, Regression zu G4a f3a6e68d): der Remote-Praefix eines Prune-Laufs kommt aus der LOKALEN
    /// `perm.dll.version` NEBEN dem Artefakt -- nicht aus einem uebergebenen/env-geratenen Wert. Der Parameter
    /// `build_version_fallback` greift nur noch, wenn die Sidecar-Datei fehlt oder leer ist.
    ///
    /// WARUM: push_tier_binary legt die Objekte unter cache_key_prefix(<PER-PERM build_version>)/<stem> ab -- die
    /// Facade haengt je Permutation "+cxx=...+opt=...+ext=..." an (profile_run_entry.hpp), und GENAU dieser
    /// suffigierte String steht auch in der lokalen `.version` (write_version_sidecar schreibt cfg_.build_version).
    /// Der Prune-Aufrufer kannte den Suffix nicht und reichte den nackten Basis-Wert ("m3v2") herein -> der Praefix
    /// zeigte ins Leere -> mc_remote_exists immer false -> JEDER Stem endete auf "behalten", pruned=0, DAUERHAFT und
    /// unauffaellig (nicht-fatal). Die Sidecar-Datei ist die einzige Quelle, die per Konstruktion mit dem Push
    /// uebereinstimmt: derselbe String, der die Binary gestempelt hat. Damit ist der Fehler strukturell nicht
    /// wiederholbar -- ein neues build_version-Segment wandert automatisch in beide Praefixe.
    ///
    /// Der Inhalt wird BEWUSST NICHT normalisiert (kein Trim): write_version_sidecar schreibt den String roh und ohne
    /// Zeilenende, push_tier_binary nutzt denselben String. Jede Normalisierung hier waere eine zweite Wahrheit, die
    /// vom Push abweichen koennte. Derselbe rohe Inhalt dient unten zusaetzlich dem Byte-Gleichheits-Vergleich gegen
    /// die Remote-Sidecar -- Praefix-Quelle und Provenienz-Beweis stammen also aus EINER Lesung.
    [[nodiscard]] std::string prune_key_base(std::filesystem::path const& bin_dir,
                                             std::string const&           build_version_fallback) const {
        std::string const stem           = bin_dir.filename().string();
        std::string const local_version  = read_text_file(bin_dir / "perm.dll.version");
        std::string const prefix_version = local_version.empty() ? build_version_fallback : local_version;
        return cache_key_prefix(prefix_version) + "/" + stem;
    }

    /// G5 (P-B, Ledger Section 65/66): PRUNE lokal->0 NACH verifiziertem Remote-Bestand. Loescht die lokale Tier-
    /// Binary + ihre Marke + ihre Sidecars (prunable_artifacts, deckungsgleich mit dem Push-Satz) NUR, wenn der
    /// Objekt-Store sie BEWEISBAR spiegelt: remote perm.dll.version existiert UND ist byte-gleich zur lokalen
    /// .version (Provenienz) UND die remote perm.dll-Groesse == lokale. JEDER Fehler-/Zweifel-Pfad => KEIN Loeschen
    /// (behalten). NIEMALS Messdaten (NUR prunable_artifacts; result.csv/measure_out/prune.log bleiben, weil sie
    /// dort nicht aufgelistet sind). Ein [PRUNE]-Testat je Stem an bin_dir/
    /// perm.prune.log (append, bleibt). No-Op (skipped) wenn inert (kein minio) oder keine lokale Binary.
    /// Der Praefix stammt aus prune_key_base (s. FIX A dort); `build_version` ist nur noch der Fallback.
    [[nodiscard]] PruneOutcome verify_remote_then_prune(std::filesystem::path const& bin_dir,
                                                        std::string const&           build_version) const {
        if (!minio_enabled())
            return {PruneState::skipped, "inert"}; // opt-in: kein minio => nie loeschen (byte-neutral)
        std::string const           stem    = bin_dir.filename().string();
        std::filesystem::path const dll     = bin_dir / "perm.dll";
        std::filesystem::path const version = bin_dir / "perm.dll.version";
        std::filesystem::path const log     = bin_dir / "perm.prune.log";

        std::error_code ec;
        if (!std::filesystem::exists(dll, ec) || !std::filesystem::exists(version, ec)) {
            log_prune_testat(log, stem, false, "no_local_artifact");
            return {PruneState::skipped, "no_local_artifact"};
        }
        std::string const    local_version = read_text_file(version);
        std::uintmax_t const local_size    = std::filesystem::file_size(dll, ec);
        if (ec) {
            log_prune_testat(log, stem, false, "local_size_unreadable");
            return {PruneState::skipped, "local_size_unreadable"};
        }
        // FIX A: Praefix erst JETZT bilden -- er haengt an der eben gelesenen lokalen .version, nicht am Parameter.
        std::string const key_base = prune_key_base(bin_dir, build_version);

        // Remote-Provenienz: remote perm.dll.version existiert? -> nach tmp holen -> Inhalt lesen (nullopt sonst).
        std::optional<std::string> remote_version;
        if (mc_remote_exists(key_base + "/perm.dll.version")) {
            std::filesystem::path const tmp = version.string() + ".remote";
            if (mc_pull(key_base + "/perm.dll.version", tmp, log)) remote_version = read_text_file(tmp);
            std::filesystem::remove(tmp, ec);
        }
        // Remote-Groesse der perm.dll (STRIKT: nullopt bei jedem stat-/Parse-Fehler -> konservativ behalten).
        std::optional<std::uintmax_t> const remote_size = mc_remote_size(key_base + "/perm.dll", log);

        PruneVerdict const v = prune_verdict(local_version, remote_version, local_size, remote_size);
        if (v.prune) {
            for (auto const& p : prunable_artifacts(bin_dir)) std::filesystem::remove(p, ec);
            log_prune_testat(log, stem, true, v.reason);
            return {PruneState::pruned, v.reason};
        }
        log_prune_testat(log, stem, false, v.reason);
        return {PruneState::kept, v.reason};
    }

    /// W11 (Ledger §43.c): Ebene B TEIL-Marker. Legt einen kleinen Marker cache_key_prefix(build_version)/_gn_chunk_markers/
    /// <range>.part<k>.done in den Objekt-Store (range: ':' -> '-', wie die YAML-Whole-Chunk-Marke). Signalisiert:
    /// die ersten k*N DLLs dieses Chunks sind gepusht -> ein resumierender Runner darf den PREFIX pullen und lokal
    /// via dll_is_current uebernehmen (nur die fehlenden werden neu gebaut). No-Op ohne Ebene B. Fehler -> ArtefaktIo
    /// geloggt, Bau LAEUFT WEITER (kein throw). Inhalt = part/utc-Metadaten (nur informativ; die Existenz zaehlt).
    ///
    /// TP1FK1-B2, BEWUSSTE ABGRENZUNG: der Teil-Marker bleibt NICHT-werfend. Er behauptet nichts ueber einen
    /// abgelegten Satz -- sein Fehlen IST das ehrliche Signal (der resumierende Runner pullt dann eben nicht und
    /// baut). Nur der Objekt-Push selbst (push_tier_binary) traegt eine Aussage, deren Fehlschlag unsichtbar
    /// bleiben WUERDE und deshalb geworfen werden MUSS.
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

    // ── Folge-A (T1, §62-NACHTRAG-4): der OBJEKT-PER-KEY-Weg ────────────────────────────────────────────────────
    //
    // push_/pull_tier_binary bewegen den DREI-Objekt-Satz EINES Tier-Binary-Ordners (mit der .version-
    // Vollstaendigkeitsmarke als Protokoll). Das Bestandslog braucht etwas anderes: EIN Objekt unter EINEM Key,
    // Inhalt im Speicher (kleine XML-/key=value-Nutzlasten: das Bestandslog-Dokument und sein Lock-Nebenobjekt).
    // Diese vier Verben sind genau das -- ADDITIV ueber DERSELBEN privaten mc-Schicht (mc_cp/mc_pull/
    // mc_remote_exists/mc_remote_size + das neue idempotente mc_rm), also mit denselben Retries, denselben
    // Wall-Clock-Caps und demselben Ziel-/Praefix-Aufbau. KEIN zweiter Transport-Weg, kein zweites Retry-Regime.
    //
    // Weil mc dateibasiert arbeitet, geht jeder Verb ueber eine kurzlebige TEMP-Datei; sie wird immer entfernt,
    // das Fehler-Log NUR im Fehlerfall behalten (sichtbarer Fehler, vgl. Messdaten-/Log-Doktrin).
    //
    // INERT-NEUTRAL: ohne Ebene B (minio_enabled()==false) tut KEIN Verb etwas und meldet ehrlich "nichts
    // geschehen" (nullopt bzw. false) -- nie einen falschen Erfolg. Ein Aufrufer, der auf einer inerten Instanz
    // arbeitet, sieht ein leeres Lager und registriert nichts (byte-neutral wie der Rest dieser Klasse).

    /// N7-D2 (Lock-Sektions-Budget): eine KOPIE dieser Instanz mit knapp budgetierten mc-Nahten -- identische
    /// Konfiguration (Endpoint/Bucket/Praefix, Key-Montage, run_stamp, Ebene C), NUR die vier Budget-Zahlen ersetzt.
    /// WARUM: der Bestandslog-Schreibvorgang laeuft unter einem Dokument-Lock mit endlicher ttl, und die Netz-Retries
    /// liegen IN dieser Sektion. Mit den Push-Defaults (tries_=12 x cap 120s je cp + 120s je stat-Verify) ergibt das
    /// eine Worst-Sektion von ~50 min und sprengt jede ttl -> eine zweite Maschine bricht den Lock, waehrend der erste
    /// Schreiber noch schreibt (zwei Schreiber am Dokument, genau der verbotene Zustand). Der Host bindet deshalb NUR
    /// den Bestandslog-Transport (bestandslog/artifact_cache_transport.hpp) an diese Zweit-Instanz; der Haupt-Cache
    /// (Tier-Push/Pull) behaelt seine bewusst zaehen Defaults. KEIN Runtime-Switch und KEIN zweiter Transport-Weg:
    /// dieselbe Klasse, dieselbe private mc-Schicht, dasselbe Retry-Regime -- nur andere Zahlen.
    ///
    /// tries==0 wird auf 1 geklemmt, identisch zur Doktrin in from_env (:167/:174): ein 0-Budget wuerde die
    /// Retry-Schleifen nie betreten und JEDEN Push still verwerfen. timeout_s==0 bleibt erhalten und bedeutet -- wie
    /// COMDARE_MC_TIMEOUT_S=0 -- bewusst "kein Wall-Clock-Cap".
    ///
    /// NEBENWIRKUNG, hier benannt statt verschwiegen: tries_ ist auch das curl-Retry-Budget der Ebene C (curl_put).
    /// Die budgetierte Kopie ist per Auftrag ausschliesslich an die Objekt-per-Key-Verben gebunden, die kein curl
    /// anfassen -> praktisch wirkungslos. sleep_s_/pull_sleep_s_ bleiben unberuehrt; bei tries==1 wird ohnehin nie
    /// geschlafen (die Pause liegt ZWISCHEN Versuchen).
    [[nodiscard]] ArtifactCache with_object_budget(std::size_t tries, std::size_t timeout_s) const {
        ArtifactCache c      = *this;
        c.tries_             = (tries == 0 ? 1 : tries); // nie 0 -> sonst nie ein Versuch (stiller Drop)
        c.pull_tries_        = c.tries_;
        c.mc_push_timeout_s_ = timeout_s;
        c.mc_pull_timeout_s_ = timeout_s;
        return c;
    }

    /// Liest EIN Objekt als String. nullopt = nicht vorhanden, Pull-Fehler ODER inert -- alle drei bedeuten fuer
    /// den Aufrufer dasselbe: "dieser Inhalt steht nicht zur Verfuegung" (der Bestandslog-Leser faellt dann auf
    /// den leeren Stand zurueck). Ein leeres Objekt liefert einen leeren String; der Parser lehnt ihn ohnehin ab.
    [[nodiscard]] std::optional<std::string> object_fetch(std::string const& object_key) const {
        if (!minio_enabled()) return std::nullopt; // inert (byte-neutral)
        if (!mc_remote_exists(object_key)) return std::nullopt;
        std::filesystem::path const tmp = object_scratch_path(object_key, ".fetch");
        std::filesystem::path const log = tmp.string() + ".pull.log";
        std::error_code             ec;
        std::filesystem::remove(tmp, ec); // Rest eines abgebrochenen Vorlaufs -> nie einen alten Inhalt lesen
        if (!mc_pull(object_key, tmp, log)) {
            log_artefakt_io(log, "object_fetch fehlgeschlagen: " + object_key);
            std::filesystem::remove(tmp, ec);
            return std::nullopt;
        }
        std::string content = read_text_file(tmp);
        std::filesystem::remove(tmp, ec);
        std::filesystem::remove(log, ec); // Erfolg -> kein Log-Rest im TEMP
        return content;
    }

    /// Schreibt EIN Objekt (ueberschreibend). true = beweisbar oben (mc cp exit 0 + Groessen-Verify von mc_cp).
    /// false = inert, TEMP nicht schreibbar oder Push fehlgeschlagen -> der Aufrufer behandelt es als Store-Fehler.
    [[nodiscard]] bool object_store(std::string const& object_key, std::string const& content) const {
        if (!minio_enabled()) return false; // inert (byte-neutral)
        std::filesystem::path const tmp = object_scratch_path(object_key, ".store");
        std::filesystem::path const log = tmp.string() + ".push.log";
        std::error_code             ec;
        {
            std::ofstream f{tmp, std::ios::binary | std::ios::trunc};
            if (!f) {
                log_artefakt_io(log, "object_store: TEMP-Datei nicht schreibbar: " + tmp.string());
                return false;
            }
            f.write(content.data(), static_cast<std::streamsize>(content.size()));
            f.flush();
            if (!f) { // Schreib-/Flush-Fehler -> NICHT pushen (ein halber Inhalt waere schlimmer als keiner)
                log_artefakt_io(log, "object_store: TEMP-Datei unvollstaendig geschrieben: " + tmp.string());
                std::filesystem::remove(tmp, ec);
                return false;
            }
        }
        bool const ok = mc_cp(tmp, object_key, log);
        if (!ok) log_artefakt_io(log, "object_store fehlgeschlagen (Objekt NICHT geschrieben): " + object_key);
        std::filesystem::remove(tmp, ec);
        if (ok) std::filesystem::remove(log, ec);
        return ok;
    }

    /// Loescht EIN Objekt. IDEMPOTENT: ein bereits fehlendes Objekt gilt als geloescht (true). false = inert oder
    /// `mc rm` fehlgeschlagen. Die Idempotenz traegt den Lock-Bruch (release/stale-break duerfen doppelt laufen).
    [[nodiscard]] bool object_remove(std::string const& object_key) const {
        if (!minio_enabled()) return false; // inert (byte-neutral)
        return mc_rm(object_key);
    }

    /// Metadaten EINES Objekts. nullopt = fehlt, Groesse nicht beweisbar ODER inert. STRIKT wie mc_remote_size:
    /// eine unbekannte Groesse ergibt KEIN Halb-Ergebnis mit size==0 (das waere von einem leeren Objekt nicht zu
    /// unterscheiden). Zur mtime siehe ObjectMeta: diese Naht ermittelt sie nicht (0 == unbekannt).
    [[nodiscard]] std::optional<ObjectMeta> object_stat(std::string const& object_key) const {
        if (!minio_enabled()) return std::nullopt; // inert (byte-neutral)
        if (!mc_remote_exists(object_key)) return std::nullopt;
        auto const sz = mc_remote_size(object_key, object_scratch_path(object_key, ".stat"));
        if (!sz) return std::nullopt;
        return ObjectMeta{static_cast<std::uint64_t>(*sz), 0};
    }

    /// Cache-Resthygiene (2026-07-21): umschliesst ein mc-argv mit einem `timeout <s>`-Wall-Clock-Cap, WENN das
    /// timeout-Binary verfuegbar ist und timeout_s>0 (sonst UNVERAENDERT zurueck). Aufbau: `timeout -k 5 <s> mc ...`
    /// (GNU coreutils: SIGTERM nach <s>, SIGKILL 5s Grace spaeter). REIN/STATISCH -> literal testbar (Kommando-Aufbau).
    [[nodiscard]] static std::vector<std::string> build_mc_argv(std::vector<std::string> argv,
                                                                std::string const& timeout_bin, bool timeout_available,
                                                                std::size_t timeout_s) {
        if (!timeout_available || timeout_s == 0 || argv.empty()) return argv; // kein Wrapper -> unveraendert
        std::vector<std::string> out;
        out.reserve(argv.size() + 4);
        out.push_back(timeout_bin);
        out.push_back("-k");
        out.push_back("5"); // Grace: SIGKILL 5s nach SIGTERM (haengendes mc erzwungen beenden)
        out.push_back(std::to_string(timeout_s));
        for (auto& a : argv) out.push_back(std::move(a));
        return out;
    }

private:
    /// Instanz-Naht: umschliesst ein mc-argv mit dem Wall-Clock-Cap dieser Instanz (timeout_bin_/timeout_available_).
    [[nodiscard]] std::vector<std::string> mc_argv(std::vector<std::string> argv, std::size_t timeout_s) const {
        return build_mc_argv(std::move(argv), timeout_bin_, timeout_available_, timeout_s);
    }

    /// Einmalige Verfuegbarkeits-Probe des timeout-Binaries: `timeout --version` -> rc 0 vorhanden, 127 (ENOENT) fehlt.
    /// Nur so wird der Wrapper aktiviert -> fehlt `timeout`, bleiben die mc-Kommandos UNVERAENDERT (kein 127-Bruch).
    [[nodiscard]] static bool probe_timeout(std::string const& timeout_bin) {
#ifdef _WIN32
        (void)timeout_bin;
        return false; // Storage-Weg ist Cluster-Linux; kein GNU timeout auf Windows
#else
        std::error_code             ec;
        std::filesystem::path const probe =
            std::filesystem::temp_directory_path(ec) /
            ("comdare_timeout_probe_" + std::to_string(static_cast<long>(::getpid())) + ".out");
        int const rc = run_argv({timeout_bin, "--version"}, probe);
        std::filesystem::remove(probe, ec);
        return rc == 0;
#endif
    }

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
    //
    // TP1FK1-B2: `letzter_exit` (optional) nimmt den Exit-Code des LETZTEN mc-cp-Versuchs auf -- ohne ihn koennte
    // die Fehlermeldung des werfenden Aufrufers den Grund nur behaupten. -1 = nie gestartet (tries_ ist per
    // from_env/with_object_budget auf >=1 geklemmt, der Wert ist also die ehrliche Reserve). 0 bei false bedeutet
    // "mc cp meldete Erfolg, aber der Groessen-Verify war negativ" -- eine andere Lage als ein cp-Fehler.
    [[nodiscard]] bool mc_cp(std::filesystem::path const& local, std::string const& object_key,
                             std::filesystem::path const& log, int* letzter_exit = nullptr) const {
        std::string const    target = mc_target(object_key);
        std::error_code      ec;
        std::uintmax_t const local_size = std::filesystem::file_size(local, ec);
        if (letzter_exit != nullptr) *letzter_exit = -1;

        for (std::size_t attempt = 1; attempt <= tries_; ++attempt) {
            // `mc cp --quiet` = atomarer Put (mc verifiziert den Upload intern via ETag). stdout/stderr -> Log.
            int const rc =
                run_argv(mc_argv({mc_bin_, "cp", "--quiet", local.string(), target}, mc_push_timeout_s_), log);
            if (letzter_exit != nullptr) *letzter_exit = rc;
            if (rc == 0 && mc_size_verified(target, local_size, log)) return true;
            if (attempt < tries_) sleep_seconds(sleep_s_);
        }
        return false;
    }

    // ── S2 (#46a) mc-Shellout PULL-Seite: remote-Existenz + `mc cp` remote->lokal, eigenes knappes pull_tries_-Budget. ──
    /// `mc stat --json <object_key>` -> Existenz (rc==0). Fuer die invertierte ZULETZT-Pruefung (remote .version da?).
    /// Die kurzlebige stat-Redirect-Datei (nur mc-stdout; kein Credential) liegt im TEMP-Verzeichnis (garantiert
    /// existent -- NICHT im noch-nicht-angelegten Ziel-bin_dir). EIN Versuch (stat ist billig; ein Hang wuerde von
    /// Retries nicht geheilt -- das knappe cp-Budget bounded den Gesamt-Pull).
    [[nodiscard]] bool mc_remote_exists(std::string const& object_key) const {
        std::error_code             ec;
        std::filesystem::path const scratch =
            std::filesystem::temp_directory_path(ec) /
            ("comdare_mc_stat_" + run_stamp_ + "_" + std::to_string(std::hash<std::string>{}(object_key)) + ".json");
        int const rc =
            run_argv(mc_argv({mc_bin_, "stat", "--json", mc_target(object_key)}, mc_pull_timeout_s_), scratch);
        std::filesystem::remove(scratch, ec);
        return rc == 0;
    }

    /// `mc cp --quiet <remote> <local>` mit pull_tries_-Budget (nicht die Push-Defaults). Erfolg = rc==0 UND lokale
    /// Datei existiert (mc cp verifiziert die Integritaet intern via ETag, wie push-seitig). stdout/stderr -> Log.
    [[nodiscard]] bool mc_pull(std::string const& object_key, std::filesystem::path const& local,
                               std::filesystem::path const& log) const {
        std::string const target = mc_target(object_key);
        std::error_code   ec;
        for (std::size_t attempt = 1; attempt <= pull_tries_; ++attempt) {
            int const rc =
                run_argv(mc_argv({mc_bin_, "cp", "--quiet", target, local.string()}, mc_pull_timeout_s_), log);
            if (rc == 0 && std::filesystem::exists(local, ec)) return true;
            if (attempt < pull_tries_) sleep_seconds(pull_sleep_s_);
        }
        return false;
    }

    /// `mc stat --json <target>` -> Groesse parsen und gegen die lokale Groesse pruefen. Ein Parse-Fehler wird NICHT
    /// als Push-Fehler gewertet (mc cp exit 0 hat die Integritaet bereits geprueft) -> best-effort-Zusatz-Verify.
    [[nodiscard]] bool mc_size_verified(std::string const& target, std::uintmax_t local_size,
                                        std::filesystem::path const& log) const {
        std::filesystem::path const stat_out = log.string() + ".stat";
        int const       rc = run_argv(mc_argv({mc_bin_, "stat", "--json", target}, mc_push_timeout_s_), stat_out);
        std::error_code ec;
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

    /// G5 (P-B): remote Objekt-Groesse via `mc stat --json` -> nullopt bei stat-/Parse-Fehler. STRIKT (kein
    /// best-effort-true wie mc_size_verified): fuer den Prune muss die Groesse BEWEISBAR gleich sein; ein
    /// unbekannter Wert = KEIN Loeschen (prune_verdict behaelt bei remote_size==nullopt konservativ).
    [[nodiscard]] std::optional<std::uintmax_t> mc_remote_size(std::string const&           object_key,
                                                               std::filesystem::path const& log) const {
        std::filesystem::path const stat_out = log.string() + ".prune.stat";
        int const                   rc =
            run_argv(mc_argv({mc_bin_, "stat", "--json", mc_target(object_key)}, mc_pull_timeout_s_), stat_out);
        std::error_code ec;
        if (rc != 0) {
            std::filesystem::remove(stat_out, ec);
            return std::nullopt;
        }
        std::ifstream f{stat_out, std::ios::binary};
        std::string   content((std::istreambuf_iterator<char>(f)), {});
        f.close();
        std::filesystem::remove(stat_out, ec);
        std::uintmax_t sz = 0;
        if (!parse_json_size(content, sz)) return std::nullopt;
        return sz;
    }

    /// Folge-A (T1): `mc rm <target>` -- der EINE Loesch-Verb dieser Schicht, IDEMPOTENT: ein bereits fehlendes
    /// Objekt gilt als geloescht (true), ohne mc ueberhaupt zu rufen. EIN Versuch (rm ist billig; ein Hang wuerde
    /// von Retries nicht geheilt -- der Wall-Clock-Cap bounded ihn, wie bei mc_remote_exists). Die kurzlebige
    /// Redirect-Datei traegt nur mc-stdout (kein Credential) und liegt im TEMP-Verzeichnis.
    [[nodiscard]] bool mc_rm(std::string const& object_key) const {
        if (!mc_remote_exists(object_key)) return true; // idempotent: fehlend == geloescht
        std::filesystem::path const scratch = object_scratch_path(object_key, ".rm.out");
        int const                   rc =
            run_argv(mc_argv({mc_bin_, "rm", "--quiet", mc_target(object_key)}, mc_pull_timeout_s_), scratch);
        std::error_code ec;
        std::filesystem::remove(scratch, ec);
        return rc == 0;
    }

    /// Folge-A (T1): kurzlebiger TEMP-Pfad je (Objekt-Key, Verb). Das TEMP-Verzeichnis ist garantiert vorhanden
    /// (anders als ein noch nicht angelegtes Ziel-bin_dir; Muster mc_remote_exists). run_stamp_ + Key-Hash halten
    /// zwei parallele Laeufe/Keys auseinander; das Suffix nennt den Verb, damit ein zurueckgebliebenes Log
    /// zuordenbar bleibt.
    [[nodiscard]] std::filesystem::path object_scratch_path(std::string const& object_key, char const* tag) const {
        std::error_code ec;
        return std::filesystem::temp_directory_path(ec) /
               ("comdare_obj_" + run_stamp_ + "_" + std::to_string(std::hash<std::string>{}(object_key)) + tag);
    }

    /// G5 (P-B): Datei-Inhalt als String (leer bei Fehler). Fuer den byte-genauen .version-Provenienz-Vergleich.
    [[nodiscard]] static std::string read_text_file(std::filesystem::path const& p) {
        std::ifstream f{p, std::ios::binary};
        if (!f) return {};
        return std::string((std::istreambuf_iterator<char>(f)), {});
    }

    /// G5 (P-B): ein [PRUNE]-Testat je Stem an bin_dir/perm.prune.log ANHAENGEN (append; das Log ueberlebt den
    /// Prune, weil es NICHT in prunable_artifacts steht). Format: [PRUNE] stem=<s> action=<pruned|behalten> reason=<r>.
    static void log_prune_testat(std::filesystem::path const& log, std::string const& stem, bool pruned,
                                 std::string const& reason) {
        std::ofstream f{log, std::ios::binary | std::ios::app};
        if (!f) return;
        f << "[PRUNE] stem=" << stem << " action=" << (pruned ? "pruned" : "behalten") << " reason=" << reason << "\n";
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
    std::size_t pull_tries_        = 2;   // S2: Pull-Retry-Budget (knapp; COMDARE_ARTEFAKT_PULL_TRIES)
    std::size_t pull_sleep_s_      = 1;   // S2: Pause zwischen Pull-Versuchen (s; COMDARE_ARTEFAKT_PULL_RETRY_SLEEP_S)
    std::string timeout_bin_       = "timeout"; // COMDARE_TIMEOUT_BIN (Wall-Clock-Cap-Wrapper der mc-Shellouts)
    bool        timeout_available_ = false; // einmalig probiert (probe_timeout); false => KEIN Prefix (unveraendert)
    std::size_t mc_push_timeout_s_ = 120;   // mc-Push cp/stat Wall-Clock-Cap in s (COMDARE_MC_TIMEOUT_S; 0 = kein Cap)
    std::size_t mc_pull_timeout_s_ =
        20; // mc-Pull cp/stat Wall-Clock-Cap in s (COMDARE_MC_PULL_TIMEOUT_S; 0 = kein Cap)
};

} // namespace comdare::cache_engine::builder::artifact_transport
