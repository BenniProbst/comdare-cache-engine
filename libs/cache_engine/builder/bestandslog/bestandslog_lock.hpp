#pragma once
// bestandslog_lock.hpp -- G3 / #46b Lagerhaltung, Scheibe B2 (Ledger §62-B, §66 Lager-Gate).
//
// Die KOORDINATIONS-Schicht des Bestandslogs auf dem Objekt-Store (minio): (1) eine Transport-Naht
// BestandTransport{fetch,store,remove,stat} (Injektions-Muster wie CachePushFn -> real an
// ArtifactCache, Fake = In-Memory-Map im Test), (2) ein Owner-Token-Verify-Lock mit ttl-Bruch und
// (3) der deterministische Record-UNION-Merge zweier Dokument-Versionen.
//
// EHRLICHE GRENZE (Ledger §2 des Designs): mc/S3 bieten mit den Bordmitteln des Hauses KEIN echtes
// CAS (kein bedingtes PUT ueber mc). Der Lock ist deshalb NICHT korrektheitstragend fuer das
// Datenueberleben -- er ist nur kurze Schreib-Exklusivitaet. Ein Rest-Race-Fenster bleibt. Das
// Design macht Kollisionen UNSCHAEDLICH statt unmoeglich:
//   (a) Reservierungs-Records sind per-Owner eindeutig (owner_uuid/seq); der Schreiber ersetzt NIE
//       blind, sondern merged deterministisch (fetch->merge->store, doc_revision monoton).
//   (b) Artefakt-Pushes sind idempotent (gleiche SHA512-Keys -> identische Bytes) -> doppelt
//       reservierte Slices kosten nur doppelte Arbeit, nie Datenverlust.
//   (c) Ein Lock aelter als ttl darf von JEDER Maschine gebrochen werden -> eine tote Maschine
//       sperrt nie dauerhaft.
// Der Zweit-Verify (nach dem store nochmals fetch + Token-Vergleich) verkleinert das Fenster,
// schliesst es aber ehrlich nicht -- die Harmlosigkeit oben traegt die Korrektheit.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), nur stdlib + bestandslog_document.hpp.
// Die Zeit ist ueber NowFn injizierbar -> Tests skripten ttl/Interleavings deterministisch.

#include "bestandslog_document.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

// ─────────────────────────────────────────────────────────────────────────────
// Transport-Naht -- vier Verben auf dem Objekt-Store. std::function-Injektion (Muster CachePushFn,
// artifact_transport-Schicht): real gebunden an ArtifactCache/mc, im Test eine In-Memory-Map. Das
// ist I/O (mc-Shellout), KEIN Hot-Path -> std::function ist zulaessig und hausueblich.
// ─────────────────────────────────────────────────────────────────────────────
struct ObjectStat {
    std::uint64_t size          = 0;
    std::int64_t  mtime_epoch_s = 0;
};

struct BestandTransport {
    // fetch: Objekt-Inhalt lesen (nullopt = existiert nicht).
    std::function<std::optional<std::string>(std::string const& key)> fetch;
    // store: Objekt schreiben/ueberschreiben (true = ok).
    std::function<bool(std::string const& key, std::string const& content)> store;
    // remove: Objekt loeschen (idempotent: fehlend == ok; true = ok).
    std::function<bool(std::string const& key)> remove;
    // stat: Metadaten (nullopt = fehlt). Fuer spaetere Index-/Listing-Scheiben; der Lock nutzt es nicht.
    std::function<std::optional<ObjectStat>(std::string const& key)> stat;
};

// Zeitquelle (Epoch-Sekunden). Injizierbar -> deterministische ttl-/Interleaving-Tests.
using NowFn = std::function<std::int64_t()>;

[[nodiscard]] inline std::int64_t system_now_epoch_s() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// ─────────────────────────────────────────────────────────────────────────────
// Lock-Objekt: {owner_uuid, host, pid, ts, ttl}. Kleines Nebenobjekt <doc>.lock. Serialisierung als
// kompakte, deterministische key=value-Zeile (Owner/Host sind kontrollierte Tokens ohne ';'/'=').
// ─────────────────────────────────────────────────────────────────────────────
struct LockOwner {
    std::string owner_uuid; // per-Maschine-Prozess eindeutig (uuid)
    std::string host;       // z.B. prod1
    long        pid = 0;
};

struct LockRecord {
    std::string  owner_uuid;
    std::string  host;
    long         pid        = 0;
    std::int64_t ts_epoch_s = 0; // Erwerbs-Zeit
    int          ttl_s      = 30;

    friend bool operator==(LockRecord const&, LockRecord const&) = default;
};

[[nodiscard]] inline std::string serialize_lock(LockRecord const& r) {
    std::string out;
    out += "owner=";
    out += r.owner_uuid;
    out += ";host=";
    out += r.host;
    out += ";pid=";
    out += std::to_string(r.pid);
    out += ";ts=";
    out += std::to_string(r.ts_epoch_s);
    out += ";ttl=";
    out += std::to_string(r.ttl_s);
    return out;
}

[[nodiscard]] inline std::optional<LockRecord> parse_lock(std::string_view s) {
    LockRecord  r;
    bool        have_owner = false;
    std::size_t i          = 0;
    while (i < s.size()) {
        std::size_t eq = s.find('=', i);
        if (eq == std::string_view::npos) return std::nullopt;
        std::string_view key = s.substr(i, eq - i);
        std::size_t      sc  = s.find(';', eq + 1);
        std::string_view val = (sc == std::string_view::npos) ? s.substr(eq + 1) : s.substr(eq + 1, sc - eq - 1);
        if (key == "owner") {
            r.owner_uuid = std::string{val};
            have_owner   = true;
        } else if (key == "host") {
            r.host = std::string{val};
        } else if (key == "pid") {
            long v = 0;
            std::from_chars(val.data(), val.data() + val.size(), v);
            r.pid = v;
        } else if (key == "ts") {
            std::int64_t v = 0;
            std::from_chars(val.data(), val.data() + val.size(), v);
            r.ts_epoch_s = v;
        } else if (key == "ttl") {
            int v = 0;
            std::from_chars(val.data(), val.data() + val.size(), v);
            r.ttl_s = v;
        }
        if (sc == std::string_view::npos) break;
        i = sc + 1;
    }
    if (!have_owner) return std::nullopt; // owner ist Pflicht -> nie stille Fehlfaerbung
    return r;
}

[[nodiscard]] inline bool lock_is_stale(LockRecord const& r, std::int64_t now_s) noexcept {
    return (now_s - r.ts_epoch_s) > static_cast<std::int64_t>(r.ttl_s);
}

// ─────────────────────────────────────────────────────────────────────────────
// try_acquire_lock -- EIN Versuch (kein Backoff-Loop; der Aufrufer wiederholt mit Zufalls-Jitter).
// Ablauf: bestehenden Lock lesen -> fremd & frisch => nicht bekommen; fremd & stale => brechen;
// eigenen Token schreiben -> Zweit-Verify (erneut lesen) -> owner==ich ? bekommen : verloren.
// Der Zweit-Verify faengt die MEISTEN Races ab; das Rest-Fenster traegt der Record-Union-Merge.
// ─────────────────────────────────────────────────────────────────────────────
[[nodiscard]] inline bool try_acquire_lock(BestandTransport const& t, std::string const& lock_key, LockOwner const& me,
                                           int ttl_s, std::int64_t now_s) {
    if (auto raw = t.fetch(lock_key)) {
        if (auto existing = parse_lock(*raw)) {
            if (existing->owner_uuid != me.owner_uuid && !lock_is_stale(*existing, now_s))
                return false; // fremd und frisch -> nicht bekommen
            if (existing->owner_uuid != me.owner_uuid && lock_is_stale(*existing, now_s))
                t.remove(lock_key); // stale -> brechen (jede Maschine darf das)
        }
        // unparsebares Lock-Objekt: als brechbar behandeln (wird ueberschrieben).
    }
    LockRecord mine{me.owner_uuid, me.host, me.pid, now_s, ttl_s};
    if (!t.store(lock_key, serialize_lock(mine))) return false;
    // Zweit-Verify (settle): sieht der Store jetzt MEINEN Token?
    auto after = t.fetch(lock_key);
    if (!after) return false;
    auto verified = parse_lock(*after);
    return verified && verified->owner_uuid == me.owner_uuid;
}

// release_lock -- nur den EIGENEN Lock loeschen (nie einen fremden). Idempotent.
inline void release_lock(BestandTransport const& t, std::string const& lock_key, LockOwner const& me) {
    if (auto raw = t.fetch(lock_key)) {
        auto existing = parse_lock(*raw);
        if (existing && existing->owner_uuid == me.owner_uuid) t.remove(lock_key);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Record-Union-Merge -- fetch->merge->store statt blinder Ersetzung. Vereinigt zwei Dokument-
// Versionen deterministisch (Reihenfolge egal): bestand per key_sha512, reservierungen per id.
// Konflikt-Aufloesung ist MONOTON (Fortschritt geht nie verloren) und deterministisch:
//   * bestand:        gleicher Key -> spaeteres done_utc gewinnt; Gleichstand -> a (stabil).
//   * reservierungen: gleiche id -> hoehere Fortschritts-Stufe gewinnt (offen<released<done);
//                     Gleichstand -> die mit gefuellter eta_s (kalibriert); sonst a (stabil).
// doc_revision(merged) = max(a,b)+1 (monoton). Ausgabe-Vektoren nach Key sortiert -> byte-stabiler
// Emit unabhaengig von der Eingabe-Reihenfolge.
// ─────────────────────────────────────────────────────────────────────────────
namespace detail {

[[nodiscard]] inline int status_rank(BatchStatus s) noexcept {
    switch (s) {
        case BatchStatus::offen: return 0;
        case BatchStatus::released: return 1;
        case BatchStatus::done: return 2;
    }
    return 0;
}

// Waehlt deterministisch den "gewinnenden" Reservierungs-Record bei id-Konflikt.
[[nodiscard]] inline BatchReservierung const& pick_reservierung(BatchReservierung const& a,
                                                                BatchReservierung const& b) noexcept {
    int ra = status_rank(a.status), rb = status_rank(b.status);
    if (rb > ra) return b;
    if (ra > rb) return a;
    // gleicher Rang: die mit gefuellter eta_s bevorzugen (spaeter kalibriert)
    if (a.eta_s.empty() && !b.eta_s.empty()) return b;
    return a; // sonst stabil a
}

[[nodiscard]] inline BestandEintrag const& pick_eintrag(BestandEintrag const& a, BestandEintrag const& b) noexcept {
    if (b.done_utc > a.done_utc) return b; // lexikographisch == chronologisch fuer ISO-8601
    return a;
}

} // namespace detail

[[nodiscard]] inline BestandslogDocument merge_documents(BestandslogDocument const& a, BestandslogDocument const& b) {
    BestandslogDocument out;
    out.genus             = a.genus;
    out.syntax_version    = std::max(a.syntax_version, b.syntax_version);
    out.semantics_version = std::max(a.semantics_version, b.semantics_version);
    out.created_utc       = a.created_utc;
    out.doc_revision      = std::max(a.doc_revision, b.doc_revision) + 1;

    // bestand: Union per key_sha512.
    out.bestand = a.bestand;
    for (auto const& be : b.bestand) {
        auto it = std::find_if(out.bestand.begin(), out.bestand.end(),
                               [&](BestandEintrag const& x) { return x.key_sha512 == be.key_sha512; });
        if (it == out.bestand.end())
            out.bestand.push_back(be);
        else
            *it = detail::pick_eintrag(*it, be);
    }
    std::sort(out.bestand.begin(), out.bestand.end(),
              [](BestandEintrag const& x, BestandEintrag const& y) { return x.key_sha512 < y.key_sha512; });

    // reservierungen: Union per id.
    out.reservierungen = a.reservierungen;
    for (auto const& br : b.reservierungen) {
        auto it = std::find_if(out.reservierungen.begin(), out.reservierungen.end(),
                               [&](BatchReservierung const& x) { return x.id == br.id; });
        if (it == out.reservierungen.end())
            out.reservierungen.push_back(br);
        else
            *it = detail::pick_reservierung(*it, br);
    }
    std::sort(out.reservierungen.begin(), out.reservierungen.end(),
              [](BatchReservierung const& x, BatchReservierung const& y) { return x.id < y.id; });

    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// store_document_merged -- der eine sichere Schreib-Weg: remote lesen, mit dem lokalen Stand
// mergen (Union), zurueckschreiben. NIE blinde Ersetzung. Fehlt das Remote-Objekt, wird der lokale
// Stand mit bumped doc_revision geschrieben. Gibt das TATSAECHLICH geschriebene Dokument zurueck
// (fuer den Aufrufer: der neue doc_revision-Stand) bzw. nullopt bei Store-Fehler.
// ─────────────────────────────────────────────────────────────────────────────
[[nodiscard]] inline std::optional<BestandslogDocument>
store_document_merged(BestandTransport const& t, std::string const& doc_key, BestandslogDocument const& local) {
    BestandslogDocument to_write;
    if (auto raw = t.fetch(doc_key)) {
        if (auto remote = parse_bestandslog(*raw))
            to_write = merge_documents(*remote, local);
        else
            to_write = local; // remote unlesbar -> lokalen Stand setzen (Versionierung schuetzt das Alte)
    } else {
        to_write              = local;
        to_write.doc_revision = local.doc_revision + 1;
    }
    if (!t.store(doc_key, emit_document(to_write))) return std::nullopt;
    return to_write;
}

} // namespace comdare::cache_engine::builder::bestandslog
