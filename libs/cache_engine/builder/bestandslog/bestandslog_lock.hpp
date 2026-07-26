#pragma once
// bestandslog_lock.hpp -- G3 / #46b Lagerhaltung, Scheibe B2 (Ledger §62-B, §66 Lager-Gate).
//
// Die KOORDINATIONS-Schicht des Bestandslogs auf dem Objekt-Store (minio): (1) eine Transport-Naht
// BestandTransport{fetch,store,remove,stat} (Injektions-Muster wie CachePushFn -> real an
// ArtifactCache, Fake = In-Memory-Map im Test), (2) ein Owner-Token-Verify-Lock mit ttl-Bruch,
// (3) der deterministische Record-UNION-Merge zweier Dokument-Versionen und (4) der EINE gelockte
// Schreib-Zyklus with_document_lock (Acquire -> Frist-Wache -> Arbeit -> Release).
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
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

// ---------------------------------------------------------------------------
// Transport-Naht -- vier Verben auf dem Objekt-Store. std::function-Injektion (Muster CachePushFn,
// artifact_transport-Schicht): real gebunden an ArtifactCache/mc, im Test eine In-Memory-Map. Das
// ist I/O (mc-Shellout), KEIN Hot-Path -> std::function ist zulaessig und hausueblich.
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Lock-Objekt: {owner_uuid, host, pid, ts, ttl}. Kleines Nebenobjekt <doc>.lock. Serialisierung als
// kompakte, deterministische key=value-Zeile (Owner/Host sind kontrollierte Tokens ohne ';'/'=').
// ---------------------------------------------------------------------------
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
    // ttl-DEFAULT (N7-D3): 90 s. Der Ledger nennt keine Zahl; 30 s waren zu knapp, weil die
    // Schreib-Sektion aus Netz-Verben (fetch + store + Zweit-Verify) UND dem Parse/Merge/Emit des
    // vollen Dokuments besteht -- schon das knapp budgetierte Objekt-Store-Profil liegt im
    // Worst-Fall bei ca. 40 s. Bricht eine zweite Maschine den Lock, WAEHREND der erste noch
    // schreibt, entsteht genau der Zwei-Schreiber-Zustand, den das Lock verhindern soll
    // (Invariante: Sektions-Wall-Clock < ttl). Die ttl bleibt PARAMETER von try_acquire_lock --
    // dieser Default gilt nur fuer frisch gebaute Records.
    int ttl_s = 90;

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
    std::size_t i = 0;
    while (i < s.size()) {
        std::size_t eq = s.find('=', i);
        if (eq == std::string_view::npos) return std::nullopt;
        std::string_view key = s.substr(i, eq - i);
        std::size_t      sc  = s.find(';', eq + 1);
        std::string_view val = (sc == std::string_view::npos) ? s.substr(eq + 1) : s.substr(eq + 1, sc - eq - 1);
        if (key == "owner") {
            r.owner_uuid = std::string{val};
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
    // owner ist Pflicht und muss ein Token TRAGEN (N7-D5): ein fehlendes ODER leeres owner-Feld
    // ergibt KEINEN gueltigen Lock. Vorher genuegte die Anwesenheit des Keys, ein leerer Wert kam
    // durch -- dann verglichen try_acquire_lock ("" != "" ist falsch) und release_lock ("" == "" ist
    // wahr) einen leeren Owner mit sich selbst: fremde frische Locks waren ueberschreibbar und
    // fremde Locks loeschbar. Beide Waechter haengen an dieser einen Zeile.
    if (r.owner_uuid.empty()) return std::nullopt;
    return r;
}

[[nodiscard]] inline bool lock_is_stale(LockRecord const& r, std::int64_t now_s) noexcept {
    return (now_s - r.ts_epoch_s) > static_cast<std::int64_t>(r.ttl_s);
}

// ---------------------------------------------------------------------------
// try_acquire_lock -- EIN Versuch (kein Backoff-Loop; der Aufrufer wiederholt mit Zufalls-Jitter).
// Ablauf: bestehenden Lock lesen -> fremd & frisch => nicht bekommen; fremd & stale => brechen;
// eigenen Token schreiben -> Zweit-Verify (erneut lesen) -> owner==ich ? bekommen : verloren.
// Der Zweit-Verify faengt die MEISTEN Races ab; das Rest-Fenster traegt der Record-Union-Merge.
// ---------------------------------------------------------------------------
[[nodiscard]] inline bool try_acquire_lock(BestandTransport const& t, std::string const& lock_key, LockOwner const& me,
                                           int ttl_s, std::int64_t now_s) {
    // Fail-closed (N7-D5): ohne eigenes Owner-Token ist kein Lock haltbar -- der Zweit-Verify
    // scheiterte ohnehin an parse_lock. Frueher Ausstieg, damit ein solcher Aufrufer WEDER ein
    // wertloses Lock-Objekt hinterlaesst NOCH einen fremden stale-Lock bricht, den er nicht halten
    // kann. Der Host haelt das Bestandslog in diesem Fall ganz aus (Env-Gate).
    if (me.owner_uuid.empty()) return false;
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

// release_lock -- nur den EIGENEN Lock loeschen (nie einen fremden). Idempotent. Ein leerer eigener
// owner_uuid kann hier nichts loeschen: parse_lock liefert fuer ein leeres owner-Feld nullopt, und
// gegen einen fremden, gefuellten Token schlaegt der Vergleich fehl (N7-D5).
inline void release_lock(BestandTransport const& t, std::string const& lock_key, LockOwner const& me) {
    if (auto raw = t.fetch(lock_key)) {
        auto existing = parse_lock(*raw);
        if (existing && existing->owner_uuid == me.owner_uuid) t.remove(lock_key);
    }
}

// ---------------------------------------------------------------------------
// Der gelockte Schreib-Zyklus (§62-B SCHREIB-LOCK-SEMANTIK, N7-D1/D3). Das Lock deckt GENAU EINEN
// Schreibvorgang am Dokument (einen fetch-merge-store-Zyklus, namentlich auch den ETA-Schreibvorgang)
// und endet spaetestens mit der Eintragung der ersten pro-forma-Reservierung.
// ---------------------------------------------------------------------------

// Lock-Schluessel zum Dokument: das kleine Nebenobjekt <doc>.lock (s. Kopf). EINE Ableitung, damit
// Schreiber und Brecher nie ueber verschiedene Schluessel reden.
[[nodiscard]] inline std::string lock_key_for(std::string_view doc_key) {
    std::string k{doc_key};
    k += ".lock";
    return k;
}

// Ergebnis EINES Zyklus -- benanntes Outcome statt bool-Salat, weil der Aufrufer die Faelle
// unterschiedlich behandelt: lock_unavailable/deadline_exceeded sind WIEDERHOLBAR (Jitter-Zyklus),
// work_failed ist ein echter Fehlschlag der Arbeit selbst.
enum class LockOutcome {
    ok,                // Lock bekommen, Arbeit als gelungen gemeldet, Lock freigegeben
    lock_unavailable,  // fremder frischer Lock oder verlorenes Race -- nichts geschrieben
    deadline_exceeded, // Frist-Wache: mehr als ttl/2 verbraucht -- bewusst NICHT geschrieben
    work_failed        // Lock war da, die Arbeit selbst schlug fehl
};

[[nodiscard]] inline std::string_view to_string(LockOutcome o) noexcept {
    switch (o) {
        case LockOutcome::ok: return "ok";
        case LockOutcome::lock_unavailable: return "lock_unavailable";
        case LockOutcome::deadline_exceeded: return "deadline_exceeded";
        case LockOutcome::work_failed: return "work_failed";
    }
    return "lock_unavailable";
}

namespace detail {

// RAII um die kurze Schreib-Exklusivitaet: gibt den EIGENEN Lock auf JEDEM Ausgang frei -- auch wenn
// die Arbeit wirft. release_lock ist idempotent und loescht nie einen fremden Token, deshalb braucht
// dieser Halter kein disarm(). Reine I/O-Naht, kein Hot-Path.
class LockHold {
public:
    LockHold(BestandTransport const& t, std::string lock_key, LockOwner me)
        : t_{&t}, lock_key_{std::move(lock_key)}, me_{std::move(me)} {}

    LockHold(LockHold const&)            = delete;
    LockHold& operator=(LockHold const&) = delete;
    LockHold(LockHold&&)                 = delete;
    LockHold& operator=(LockHold&&)      = delete;

    ~LockHold() {
        try {
            release_lock(*t_, lock_key_, me_);
        } catch (...) {
            // best-effort: die Freigabe darf den Abbau nie zum Absturz bringen; den Rest bricht die ttl.
        }
    }

private:
    BestandTransport const* t_;
    std::string             lock_key_;
    LockOwner               me_;
};

} // namespace detail

// ---------------------------------------------------------------------------
// with_document_lock -- EIN gelockter Schreib-Zyklus: Acquire -> Frist-Wache -> fn -> Release.
//
// EIN Versuch, kein Backoff-Loop: die Wiederholung mit Zufalls-Jitter macht der AUFRUFER um den
// GANZEN Zyklus (so schreibt es der Kopfkommentar von try_acquire_lock vor). Deshalb wird
// lock_unavailable ABSICHTLICH NICHT geloggt -- das ist der normale Kollisionsfall; erst der
// Aufrufer weiss, ob es der letzte Versuch war, und faerbt die Meldung.
//
// FRIST-WACHE (N7-D3): schon der Acquire kostet Netz-Zeit (fetch + store + Zweit-Verify). Sind davon
// mehr als ttl/2 verbraucht, wird NICHT MEHR GESCHRIEBEN, sondern der Lock freigegeben und
// deadline_exceeded gemeldet -- sonst schriebe der Halter unter einem Lock, den eine zweite Maschine
// als stale bricht, waehrend er noch schreibt (genau der Zwei-Schreiber-Zustand, den das Lock
// verhindern soll). Die Invariante dahinter: Sektions-Wall-Clock < ttl.
//
// fn liefert bool = "Arbeit gelungen" (z.B. store_document_merged(...).has_value()) -- ein
// verschluckter Store-Fehler ist an dieser Naht damit nicht mehr moeglich. Die Zeit kommt ueber
// NowFn, also skripten die Tests Frist- und Konfliktfaelle ohne Wall-Clock.
// ---------------------------------------------------------------------------
template <class Fn>
[[nodiscard]] inline LockOutcome with_document_lock(BestandTransport const& t, std::string const& doc_key,
                                                    LockOwner const& me, int ttl_s, NowFn const& now_fn, Fn&& fn) {
    static_assert(std::is_invocable_r_v<bool, Fn&>,
                  "fn muss bool liefern: true == Arbeit gelungen (kein verschluckter Store-Fehler)");

    if (me.owner_uuid.empty()) {
        // Fail-closed (N7-D5): ohne Owner-Token ist das Lock wertlos -> nie schreiben. Das ist ein
        // Konfigurations-Fehler des Hosts, keine Kollision -> genau eine Zeile auf cerr.
        std::cerr << "[bestandslog] FEHLER: leerer owner_uuid -- gelockter Schreibvorgang auf '" << doc_key
                  << "' abgelehnt\n";
        return LockOutcome::lock_unavailable;
    }

    std::string const  lock_key    = lock_key_for(doc_key);
    std::int64_t const acquired_at = now_fn();
    if (!try_acquire_lock(t, lock_key, me, ttl_s, acquired_at)) return LockOutcome::lock_unavailable;

    detail::LockHold const hold{t, lock_key, me}; // Freigabe auf JEDEM Ausgang, auch bei Wurf aus fn

    std::int64_t const vor_arbeit = now_fn();
    if (vor_arbeit - acquired_at > static_cast<std::int64_t>(ttl_s) / 2) {
        std::cerr << "[bestandslog] FEHLER: Lock-Frist auf '" << doc_key << "' zur Haelfte verbraucht ("
                  << (vor_arbeit - acquired_at) << "s von ttl=" << ttl_s
                  << "s) -- NICHT geschrieben, Lock freigegeben\n";
        return LockOutcome::deadline_exceeded;
    }

    if (!fn()) return LockOutcome::work_failed;

    // Nach-Wache: die Arbeit selbst kann die Frist gesprengt haben. Geschrieben IST dann schon (der
    // Union-Merge traegt die Kollision), aber die Budget-Invariante war verletzt -- das muss sichtbar
    // sein, sonst misst niemand, dass die Sektion aus dem ttl gelaufen ist.
    if (std::int64_t const nach_arbeit = now_fn(); nach_arbeit - acquired_at > static_cast<std::int64_t>(ttl_s))
        std::cerr << "[bestandslog] warn: Lock-Sektion auf '" << doc_key << "' dauerte " << (nach_arbeit - acquired_at)
                  << "s > ttl=" << ttl_s << "s (der Schreibvorgang war zu diesem Zeitpunkt erfolgt)\n";
    return LockOutcome::ok;
}

// ---------------------------------------------------------------------------
// Record-Union-Merge -- fetch->merge->store statt blinder Ersetzung. Vereinigt zwei Dokument-
// Versionen deterministisch (Reihenfolge egal): bestand per (key_sha512, combo, opt, simd),
// reservierungen per id. Konflikt-Aufloesung ist MONOTON (Fortschritt geht nie verloren) und
// deterministisch:
//   * bestand:        gleiches Tupel -> spaeteres done_utc gewinnt; Gleichstand -> a (stabil).
//   * reservierungen: gleiche id -> hoehere Fortschritts-Stufe gewinnt (offen<released<done);
//                     Gleichstand -> die mit gefuellter eta_s (kalibriert); sonst a (stabil).
// doc_revision(merged) = max(a,b)+1 (monoton). Ausgabe-Vektoren nach Key sortiert -> byte-stabiler
// Emit unabhaengig von der Eingabe-Reihenfolge.
// ---------------------------------------------------------------------------
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

// Der Identitaets-Schluessel eines Bestands-Eintrags in map-Form: VIEWS auf die vier
// Identitaets-Felder, damit der 128-hex-Key nicht je Lookup kopiert wird. Die Tupel-Ordnung ist
// byte-identisch zu eintrag_identity_less (std::string und std::string_view vergleichen beide
// lexikographisch ueber char_traits) -- same_eintrag_identity/eintrag_identity_less bleiben die eine
// Wahrheit, das hier ist nur ihre Nachschlage-Form.
//
// LEBENSDAUER: die Views zeigen in die QUELL-Dokumente (a bzw. b), die waehrend des ganzen Merge
// unveraendert leben. Nie in den Ausgabe-Vektor zeigen lassen -- dessen push_back verschiebt die
// Elemente und wuerde kurze (SSO-)Strings entwurzeln.
using EintragIdentityView = std::tuple<std::string_view, std::string_view, std::string_view, std::string_view>;

[[nodiscard]] inline EintragIdentityView eintrag_identity_view(BestandEintrag const& e) noexcept {
    return {e.key_sha512, e.zelle.combo, e.zelle.opt, e.zelle.simd};
}

} // namespace detail

[[nodiscard]] inline BestandslogDocument merge_documents(BestandslogDocument const& a, BestandslogDocument const& b) {
    BestandslogDocument out;
    out.genus             = a.genus;
    out.syntax_version    = std::max(a.syntax_version, b.syntax_version);
    out.semantics_version = std::max(a.semantics_version, b.semantics_version);
    out.created_utc       = a.created_utc;
    out.doc_revision      = std::max(a.doc_revision, b.doc_revision) + 1;

    // bestand: Union per IDENTITAETS-TUPEL (key_sha512 + Zell-Koordinaten, §62-NACHTRAG-4). Ueber den
    // key_sha512 ALLEIN wuerden zwei Bauten derselben Permutation unter verschiedener ISA zu EINEM
    // Eintrag verschmelzen -- der zweite verlore seinen Pfad. same_eintrag_identity/eintrag_identity_less
    // sind die eine Definition (bestandslog_document.hpp), gegen die auch der Index schluesselt.
    //
    // Der Union laeuft ueber eine std::map Identitaet -> INDEX in out.bestand (User-Direktive: der
    // Lager-Lookup ist ein map-Lookup ueber den SHA512-Schluessel, nicht ein Linear-Scan). Der
    // vorherige std::find_if je b-Eintrag war O(|a|*|b|) mit 128-hex-Stringvergleichen; lokal
    // gemessen (halb ueberlappende Mengen, -O2): 33 / 138 / 573 ms fuer n=4096 / 8192 / 16384, also
    // saubere Vervierfachung je Verdopplung -> hochgerechnet ca. 2,5 min fuer n=262144. Diese Zeit
    // laeuft INNERHALB der Lock-Sektion und sprengt damit jede ttl. Die map-Form braucht fuer
    // n=262144 gemessene 0,19 s: O((n+m) log n) statt O(n*m).
    //
    // Die Semantik ist unveraendert: emplace laesst das ERSTE Vorkommen einer Identitaet gewinnen,
    // genau wie find_if das erste Vorkommen fand -> auch ein Dokument mit Doppel-Identitaeten (die
    // der Parser nicht ausschliesst) verhaelt sich wie zuvor, inklusive der Anzahl der Eintraege.
    // Neu angehaengte b-Eintraege werden mit ihrem Index registriert, damit ein zweiter b-Eintrag
    // derselben Identitaet in ihn hineinmergt statt ein Duplikat anzulegen.
    out.bestand = a.bestand;
    std::map<detail::EintragIdentityView, std::size_t> bestand_idx;
    for (std::size_t i = 0; i < a.bestand.size(); ++i)
        bestand_idx.emplace(detail::eintrag_identity_view(a.bestand[i]), i);
    for (auto const& be : b.bestand) {
        auto const key = detail::eintrag_identity_view(be);
        auto const it  = bestand_idx.find(key);
        if (it == bestand_idx.end()) {
            out.bestand.push_back(be);
            bestand_idx.emplace(key, out.bestand.size() - 1);
        } else {
            out.bestand[it->second] = detail::pick_eintrag(out.bestand[it->second], be);
        }
    }
    std::sort(out.bestand.begin(), out.bestand.end(), eintrag_identity_less);

    // reservierungen: Union per id -- dieselbe map-Bauform, Schluessel ist die id (Views in a bzw. b).
    out.reservierungen = a.reservierungen;
    std::map<std::string_view, std::size_t> res_idx;
    for (std::size_t i = 0; i < a.reservierungen.size(); ++i)
        res_idx.emplace(std::string_view{a.reservierungen[i].id}, i);
    for (auto const& br : b.reservierungen) {
        std::string_view const key = br.id;
        auto const             it  = res_idx.find(key);
        if (it == res_idx.end()) {
            out.reservierungen.push_back(br);
            res_idx.emplace(key, out.reservierungen.size() - 1);
        } else {
            out.reservierungen[it->second] = detail::pick_reservierung(out.reservierungen[it->second], br);
        }
    }
    std::sort(out.reservierungen.begin(), out.reservierungen.end(),
              [](BatchReservierung const& x, BatchReservierung const& y) { return x.id < y.id; });

    return out;
}

namespace detail {

// "leer" == kein einziges Nicht-Whitespace-Byte (ASCII-explizit, ohne Locale-Abhaengigkeit). Ein
// solches Objekt traegt keinen einzigen Record -> die Erst-Anlage darf es ersetzen. Alles andere ist
// Inhalt, den nur ein GELUNGENER Parse ersetzen darf (s. Voll-Wipe-Wache).
[[nodiscard]] inline bool blank_content(std::string_view s) noexcept {
    for (char c : s)
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\f' && c != '\v') return false;
    return true;
}

} // namespace detail

// ---------------------------------------------------------------------------
// store_document_merged -- der eine sichere Schreib-Weg: remote lesen, mit dem lokalen Stand
// mergen (Union), zurueckschreiben. NIE blinde Ersetzung.
//
// VOLL-WIPE-WACHE (der eine Datenverlust-Pfad des Bestandslogs): ein VORHANDENES, nicht leeres
// Remote-Dokument, das nicht parsbar oder grammatik-fremd ist, wird NIE ueberschrieben. Der
// Objekt-Store schreibt mit einem ueberschreibenden mc cp ohne Versionierung und ohne Backup, und im
// Reservierungs-Pfad ist der lokale Stand ein Dokument mit EINER Reservierung und LEEREM bestand --
// ein einziger solcher Schreibvorgang loeschte also den Lagerbestand eines mehrtaegigen Laufs.
// Fail-closed: eine FEHLER-Zeile auf cerr (Fehler sichtbar, kein stiller Erfolgs-Haken) und nullopt;
// was daraus folgt, entscheidet der Aufrufer. document_syntax_supported wird VOR dem Merge gerufen --
// der Schutz, den die Versions-Stempel versprechen und den vorher nichts ausfuehrte.
//
// FEHLENDES oder LEERES Remote bleibt die ERST-ANLAGE: der lokale Stand wird geschrieben.
// doc_revision steigt in BEIDEN Zweigen monoton -- im Merge-Zweig ueber merge_documents
// (max(a,b)+1), in der Erst-Anlage ueber local+1.
//
// Gibt das TATSAECHLICH geschriebene Dokument zurueck (fuer den Aufrufer: der neue doc_revision-
// Stand) bzw. nullopt bei Store-Fehler ODER bei bewusst stehen gelassenem Remote.
// ---------------------------------------------------------------------------
[[nodiscard]] inline std::optional<BestandslogDocument>
store_document_merged(BestandTransport const& t, std::string const& doc_key, BestandslogDocument const& local) {
    BestandslogDocument to_write;
    auto const          raw = t.fetch(doc_key);
    if (raw && !detail::blank_content(*raw)) {
        auto const remote = parse_bestandslog(*raw);
        if (!remote) {
            std::cerr << "[bestandslog] FEHLER: Remote-Dokument '" << doc_key << "' (" << raw->size()
                      << " Byte) ist nicht parsbar -- Schreibvorgang ABGEBROCHEN, der Bestand bleibt unberuehrt\n";
            return std::nullopt;
        }
        if (!document_syntax_supported(*remote)) {
            std::cerr << "[bestandslog] FEHLER: Remote-Dokument '" << doc_key
                      << "' hat unvertraegliche syntax_version=" << remote->syntax_version << " (lesbar: 1.."
                      << kSyntaxVersion << ") -- Schreibvorgang ABGEBROCHEN, der Bestand bleibt unberuehrt\n";
            return std::nullopt;
        }
        to_write = merge_documents(*remote, local);
    } else {
        to_write              = local;
        to_write.doc_revision = local.doc_revision + 1;
    }
    if (!t.store(doc_key, emit_document(to_write))) return std::nullopt;
    return to_write;
}

} // namespace comdare::cache_engine::builder::bestandslog
