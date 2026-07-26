#pragma once
// builder_registration.hpp -- G3 / #46b Lagerhaltung, Scheibe I1 (CEB-Iterator-Verdrahtung).
//
// Die BESTANDSLOG-Laufzeit-Zustandsmaschine fuer den Bau-Loop: (a) den vorbestehenden Lager-Index
// VOR dem Bau laden (Dedup-Basis + Merge-Basis), (b) je fertiger Binary klassifizieren (war schon im
// Lager -> Dedup-Hit; sonst frisch vormerken -- thread-sicher aus den Build-Workern), (c) nach dem
// Bau die frischen Eintraege EINMAL deterministisch ins Binary-Bestandslog mergen (store_document_
// merged, single-threaded). Der KEY je Binary kommt vom Host-injizierten Provider (Option A, Ledger
// I1-Entscheid): der 128-hex-Fingerprint ist im Tier-Binary einkompiliert und wird vom Host aus dem
// (I2/.fingerprint-Sidecar) gelesen -- diese Schicht bleibt stamp-/env-blind.
//
// OPT-IN: der Aufrufer aktiviert das nur, wenn Transport + Doc-Key + Key-Provider gesetzt sind. Ohne
// das laeuft NICHTS (byte-/verhaltensneutral -- die Byte-Wachen bleiben gruen OHNE Update).
//
// Ertuechtigung 26.07. (N7-D1 / Lager-Gate): diese Schicht traegt zusaetzlich die AUFRUFER-SEITE des
// gelockten Schreib-Zyklus, den bestandslog_lock.hpp (B2) anbietet. Vier Naehte, die der Iterator
// (I1b) und flush() TEILEN -- deshalb liegen sie hier und nicht dort:
//   (a) with_document_lock_retry -- EIN gelockter Zyklus + GENAU EINE Wiederholungsrunde mit
//       Zufalls-Jitter (die Bauform, die der Kopfkommentar von try_acquire_lock vorschreibt) und
//       danach die outcome-abhaengige Testat-Zeile. JEDER Bestandslog-Schreibvorgang laeuft hier
//       durch; ungelockte Schreibwege gibt es nicht mehr.
//   (b) utc_iso_from_epoch -- now_utc_iso() fuer einen BELIEBIGEN Zeitpunkt (die 30min-pro-forma-
//       Frist braucht now UND now+30min aus DEMSELBEN now; zwei Aufrufe von now_utc_iso() ergaben
//       eine bereits abgelaufene Frist).
//   (c) make_slice_reservation -- die pro-forma-Reservierung EINES Bau-Slices mit der ECHTEN Frist.
//   (d) planer_driven_active -- das Gate des Planer-getriebenen Baus (Bestandslog AN *und*
//       provision_only): im 1-Thread-MESS-Lauf darf kein Reservierungs-Roundtrip mc-Prozesse
//       spawnen (Ledger: Batch-Typen nie mischen, Mess-Exklusivitaet).
// Sie liegen bewusst NICHT im Iterator: dort sind sie nur ueber den vollen Tier-/dlopen-Include-Satz
// erreichbar, hier deckt sie der schlanke Unit-Test (test_g3_builder_registration) literal ab.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (Section erlaubt), stdlib + die B1-B4-Bestandslog-
// Header. Kein std::variant, statischer Dispatch. observe() ist thread-sicher (Build-Worker-Hook).

#include "bestandslog_document.hpp"
#include "bestandslog_factory.hpp"
#include "bestandslog_index.hpp"
#include "bestandslog_lock.hpp"
#include "reservation_lifecycle.hpp" // pro_forma_deadline_epoch_s/make_pro_forma_reservation (B4, nur lesend genutzt)

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <process.h> // ::_getpid (Muster test_parser_konsolidierung.cpp:16)
#else
#include <unistd.h> // ::getpid (Muster artifact_cache.hpp:662)
#endif

namespace comdare::cache_engine::builder::bestandslog {

// UTC-ISO-8601-Zeitstempel EINES Epoch-Zeitpunkts (z.B. 2026-07-23T13:45:11Z). Nur fuer die
// Bestandslog-Metadaten (done_utc/created_utc/reserviert_utc/pro_forma_bis_utc); wird ausschliesslich
// im aktiven (opt-in) Pfad geschrieben -> golden-neutral.
//
// Die EPOCH-Form ist die tragende (B11): eine Frist entsteht als now + Dauer in Epoch-Sekunden und
// wird EINMAL formatiert. Wer zweimal now_utc_iso() ruft, schreibt zwei fast gleiche Zeitpunkte --
// genau so entstand die Frist, die eine Sekunde nach ihrer Eintragung abgelaufen war.
[[nodiscard]] inline std::string utc_iso_from_epoch(std::int64_t epoch_s) {
    std::time_t const t = static_cast<std::time_t>(epoch_s);
    std::tm           tmv{};
#if defined(_WIN32)
    gmtime_s(&tmv, &t);
#else
    gmtime_r(&t, &tmv);
#endif
    char buf[32] = {};
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tmv);
    return std::string{buf};
}

[[nodiscard]] inline std::string now_utc_iso() { return utc_iso_from_epoch(system_now_epoch_s()); }

// ---------------------------------------------------------------------------
// Aufrufer-Seite des gelockten Schreib-Zyklus (N7-D1).
// ---------------------------------------------------------------------------

// Prozess-Id fuer das Lock-Objekt. REIN DIAGNOSTISCH: der Lock vergleicht ausschliesslich owner_uuid
// (bestandslog_lock.hpp) -- die pid steht im Objekt, damit eine haengende Maschine im Log auffindbar
// bleibt. Plattform-Weiche wie im Haus ueblich.
[[nodiscard]] inline long current_pid() noexcept {
#if defined(_WIN32)
    return static_cast<long>(::_getpid());
#else
    return static_cast<long>(::getpid());
#endif
}

// Der Lock-Owner DIESES Prozesses aus den Host-Feldern. Ein leerer owner_uuid bleibt leer -- die
// Fail-closed-Wache in with_document_lock/parse_lock (N7-D5) muss ihn sehen und ablehnen; diese Naht
// erfindet keinen Token.
[[nodiscard]] inline LockOwner make_lock_owner(std::string owner_uuid, std::string maschine) {
    return LockOwner{std::move(owner_uuid), std::move(maschine), current_pid()};
}

// Die ttl-Zahl hat EINE Heimat: den LockRecord-Default (bestandslog_lock.hpp -- die EREIGNIS-gebundene
// 30min-Obergrenze fuer den Stale-Bruch). Diese Naht liest sie dort ab, statt sie zu wiederholen; wer
// die Zahl aendert, aendert sie GENAU DORT.
[[nodiscard]] inline int default_lock_ttl_s() { return LockRecord{}.ttl_s; }

// Gate des PLANER-GETRIEBENEN Baus (B5): der Slice-Bau mit Reservierungs-Lifecycle laeuft NUR im
// Bereitstellungs-Lauf. Im MESS-Lauf (1 Thread, Mess-Exklusivitaet) ergaeben die Reservierungen je
// Fenster zwei Dokument-Roundtrips mit je mehreren mc-Spawns MITTEN in der Messung -- der Ledger
// verbietet das Mischen der Batch-Typen. Bestandslog AN allein genuegt also nicht.
[[nodiscard]] inline bool planer_driven_active(bool bestandslog_active, bool provision_only) noexcept {
    return bestandslog_active && provision_only;
}

namespace detail {

// Wartezeit der EINEN Wiederholungsrunde: 50-500 ms, deterministisch aus (owner_uuid, doc_key).
// KEIN std::random_device: gebraucht wird nur, dass zwei kollidierende MASCHINEN verschieden lange
// warten (sonst kollidieren sie erneut im Gleichschritt) -- und genau das leistet der Seed, weil
// jede Maschine einen eigenen owner_uuid traegt. Entropie im Bau-Pfad waere Aufwand ohne Gewinn.
[[nodiscard]] inline std::chrono::milliseconds retry_jitter(std::string_view owner_uuid, std::string_view doc_key) {
    std::size_t const seed = std::hash<std::string_view>{}(owner_uuid) ^ (std::hash<std::string_view>{}(doc_key) << 1);
    std::mt19937      rng{static_cast<std::uint32_t>(seed)};
    return std::chrono::milliseconds{std::uniform_int_distribution<int>{50, 500}(rng)};
}

} // namespace detail

// ---------------------------------------------------------------------------
// with_document_lock_retry -- der EINE Weg, auf dem das Bestandslog schreibt.
//
// with_document_lock macht bewusst EINEN Versuch und schweigt beim normalen Kollisionsfall; die
// Wiederholung gehoert um den GANZEN Zyklus und damit hierher. GENAU EINE Wiederholungsrunde: der
// Zweit-Versuch deckt die kurze fremde Schreib-Sektion ab, ein Backoff-Loop wuerde im 2-Tage-Lauf
// nur Bau-Zeit gegen Buchhaltung tauschen.
//
// Der BAU BRICHT NIE AN EINEM LOCK AB (das Bestandslog ist Buchhaltung, kein Bau-Gate): jeder Ausgang
// ausser ok ergibt genau EINE Testat-Zeile auf cerr und ein normales return. `was` benennt die
// Schreibstelle, damit die Zeile im Log eines mehrtaegigen Laufs zuordenbar ist.
//
// section_budget_s reist unveraendert durch (Default = der des gelockten Zyklus): die drei heutigen
// Schreibstellen sind der millisekunden-kurze Fall, und der ETA-Fall (section_budget_s = ttl_s, s.
// with_document_lock) soll die Wiederholungsrunde ebenso nutzen koennen -- ein Wrapper, der eine
// Stellschraube seines Wrappees verschluckt, waere ein Behelfsweg.
// ---------------------------------------------------------------------------
template <class Fn>
[[nodiscard]] inline LockOutcome with_document_lock_retry(BestandTransport const& t, std::string const& doc_key,
                                                          LockOwner const& me, int ttl_s, NowFn const& now_fn,
                                                          std::string_view was, Fn&& fn,
                                                          int section_budget_s = kSectionBudgetSeconds) {
    static_assert(std::is_invocable_r_v<bool, Fn&>, "fn muss bool liefern: true == Arbeit gelungen");
    Fn&         work = fn; // ZWEI Aufrufe moeglich -> nie weiterleiten/verschieben
    LockOutcome o    = with_document_lock(t, doc_key, me, ttl_s, now_fn, work, section_budget_s);
    if (o == LockOutcome::lock_unavailable || o == LockOutcome::deadline_exceeded) {
        std::this_thread::sleep_for(detail::retry_jitter(me.owner_uuid, doc_key));
        o = with_document_lock(t, doc_key, me, ttl_s, now_fn, work, section_budget_s);
    }
    switch (o) {
        case LockOutcome::ok: break; // gruener Zyklus schweigt (der Aufrufer meldet die Summe)
        case LockOutcome::lock_unavailable:
            std::cerr
                << "[bestandslog] warn: " << was << " auf '" << doc_key
                << "' NICHT eingetragen -- Schreib-Lock in zwei Versuchen fremd gehalten; der Bau laeuft weiter\n";
            break;
        case LockOutcome::deadline_exceeded:
            std::cerr << "[bestandslog] warn: " << was << " auf '" << doc_key
                      << "' NICHT eingetragen -- Lock-Frist in zwei Versuchen verbraucht; der Bau laeuft weiter\n";
            break;
        case LockOutcome::work_failed:
            std::cerr << "[bestandslog] FEHLER: " << was << " auf '" << doc_key
                      << "' nicht persistiert -- der Schreibvorgang selbst schlug fehl (Store/Merge-Wache oben); der "
                         "Bau laeuft weiter\n";
            break;
    }
    return o;
}

// ---------------------------------------------------------------------------
// Reservierungs-Schreibstelle (I1b): pro-forma / Kalibrierung+Done / Release. IMMER gelockt.
// ---------------------------------------------------------------------------

// Die pro-forma-Reservierung EINES Bau-Slices mit der ECHTEN 30min-Frist (B11): reserviert_utc und
// pro_forma_bis_utc entstehen aus DEMSELBEN now_epoch_s, die Frist ueber pro_forma_deadline_epoch_s
// (kProFormaMinutes, reservation_lifecycle.hpp). Die id ist owner_uuid/slice_seq -- per Maschine
// eindeutig, damit der Record-Union-Merge zwei Maschinen nie vermischt.
[[nodiscard]] inline BatchReservierung make_slice_reservation(std::string const& owner_uuid, std::size_t slice_seq,
                                                              std::string maschine, unsigned threads,
                                                              std::uint64_t slice_begin, std::uint64_t slice_count,
                                                              std::int64_t now_epoch_s) {
    return make_pro_forma_reservation(owner_uuid + "/" + std::to_string(slice_seq), BatchTyp::tier, std::move(maschine),
                                      threads, slice_begin, slice_count, utc_iso_from_epoch(now_epoch_s),
                                      utc_iso_from_epoch(pro_forma_deadline_epoch_s(now_epoch_s)));
}

// EINE Reservierung gelockt ins Dokument mergen (Voll-Dokument-RMW, s. N7-D4 Schnitt A). true == der
// Record steht im Store; false == er steht NICHT drin und die Testat-Zeile ist geschrieben.
[[nodiscard]] inline bool store_reservation_locked(BestandTransport const& t, std::string const& doc_key,
                                                   LockOwner const& me, int ttl_s, NowFn const& now_fn,
                                                   BatchReservierung const& r, std::string_view was) {
    BestandslogDocument doc;
    doc.genus       = Genus::binary;
    doc.created_utc = utc_iso_from_epoch(now_fn());
    doc.reservierungen.push_back(r);
    return with_document_lock_retry(t, doc_key, me, ttl_s, now_fn, was, [&]() {
               return store_document_merged(t, doc_key, doc).has_value();
           }) == LockOutcome::ok;
}

// Ausgang der per-Binary-Dedup-Klassifikation.
enum class DedupOutcome {
    lager_hit,      // Key war schon im vorbestehenden Lager -> KEINE Neu-Registrierung (dedup)
    fresh_register, // neu -> zur Registrierung vorgemerkt
    no_key          // Key-Provider lieferte nichts (z.B. Sidecar fehlt) -> nichts getan
};

// ---------------------------------------------------------------------------
// LagerRunState -- der Bestandslog-Zustand EINES Bau-Laufs. load() vor dem Bau, observe() je fertiger
// Binary (thread-sicher, aus den Build-Workern), flush() nach dem Bau (single-threaded).
// ---------------------------------------------------------------------------
class LagerRunState {
public:
    // Vor dem Bau: den vorbestehenden Lager-Bestand aus dem Store laden (fetch doc -> parse ->
    // LagerIndex). Fehlt/unlesbar -> leerer Index (alles gilt als frisch). Idempotent. Der Index
    // traegt ALLE Zellen des Lagers; welche Zelle DIESER Lauf baut, sagt erst die Abfrage.
    void load(BestandTransport const& t, std::string const& doc_key) {
        auto bestand = make_binary_bestand();
        if (t.fetch) {
            if (auto raw = t.fetch(doc_key)) {
                if (auto doc = parse_bestandslog(*raw)) bestand.load_from_document(*doc);
            }
        }
        lager_ = bestand.index();
    }

    // Aus dem Completion-Hook (MULTITHREADED): eine fertige Binary klassifizieren. key_hex leer oder
    // kein gueltiges 128-hex -> no_key. Im Lager (unter DEM Tupel aus Fingerprint UND Zelle) ->
    // lager_hit (dedup, kein Neu-Eintrag). Sonst -> fresh_register (vorgemerkt).
    //
    // zelle sind die drei Zell-Koordinaten DIESES Bau-Laufs (§62-NACHTRAG-4): der Host reicht sie mit,
    // weil der Fingerprint die per-Zelle-ISA nicht traegt. Leere Zelle = "nicht gemeldet" -> das
    // Verhalten entspricht dem Ein-Feld-Dedup von zuvor (default-neutral).
    DedupOutcome observe(std::string key_hex, ZellKoordinaten zelle, std::string pfad, std::uint64_t bytes,
                         std::string stempel, std::string done_utc) {
        if (key_hex.empty()) return DedupOutcome::no_key;
        auto const key = lager_key_from_hex(key_hex, zelle);
        if (!key) return DedupOutcome::no_key;
        std::lock_guard<std::mutex> lk(mtx_);
        if (lager_.find(*key) != lager_.end()) {
            ++hits_;
            return DedupOutcome::lager_hit;
        }
        BestandEintrag e;
        e.key_sha512 = std::move(key_hex);
        e.zelle      = std::move(zelle);
        e.pfad       = std::move(pfad);
        e.bytes      = bytes;
        e.stempel    = std::move(stempel);
        e.done_utc   = std::move(done_utc);
        fresh_.push_back(std::move(e));
        return DedupOutcome::fresh_register;
    }

    // Folge-A (T2): LESENDER Lager-Zugriff fuer die Presence-Naht (lager_presence.hpp). Thread-sicher --
    // liest lager_ unter demselben mtx_ wie observe(), damit ein Presence-Scan aus dem Planer-Thread und
    // die Klassifikation aus den Build-Workern nebeneinander laufen duerfen. lager_ selbst wird nach
    // load() nicht mehr veraendert; der Mutex schuetzt gegen ein NEBENLAEUFIGES load() und haelt die
    // Invariante "jeder Zugriff auf lager_ unter mtx_" ohne Ausnahme.
    [[nodiscard]] bool lager_contains(LagerKey const& k) const {
        std::lock_guard<std::mutex> lk(mtx_);
        return lager_.find(k) != lager_.end();
    }

    // Bequemlichkeits-Ueberladung fuer die Laufzeit-Naht: 128-hex + Zelle. Ungueltiges Hex -> false
    // (KONSERVATIVER Miss: lieber einmal zu viel bauen als eine fehlende Binary als vorhanden melden).
    [[nodiscard]] bool lager_contains(std::string_view key_hex, ZellKoordinaten const& zelle) const {
        auto const k = lager_key_from_hex(key_hex, zelle);
        if (!k) return false;
        return lager_contains(*k);
    }

    // Nach dem Bau (SINGLE-THREADED): die frisch vorgemerkten Eintraege in EINEM gelockten
    // store_document_merged ins Binary-Bestandslog schreiben (Union-Merge, doc_revision monoton). Gibt
    // die Zahl geschriebener Eintraege zurueck (0 = nichts zu tun) bzw. nullopt, wenn NICHTS
    // geschrieben wurde (Store/Merge-Fehler ODER Lock nicht bekommen).
    //
    // RE-QUEUE (B13): fresh_ wird ERST NACH dem gelungenen Store geleert. Vorher swappte diese
    // Methode die Vormerk-Liste VOR dem Schreiben leer -- bei Store-Fehler war der Batch damit auch
    // aus dem RAM verschwunden, ohne jede Wiederholungsmoeglichkeit. Jetzt bleibt er vorgemerkt und
    // der naechste flush versucht es erneut. Deshalb eine KOPIE statt swap: die Liste muss bis zum
    // Erfolg unangetastet bleiben. Geleert wird genau der geschriebene PRAEFIX (observe() haengt nur
    // hinten an, also bleiben nebenlaeufig hinzugekommene Eintraege erhalten).
    //
    // me/ttl_s/now_fn sind der gelockte Schreib-Zyklus (N7-D1): ohne Owner-Token schreibt diese
    // Methode nichts (fail-closed in with_document_lock).
    [[nodiscard]] std::optional<std::size_t> flush(BestandTransport const& t, std::string const& doc_key,
                                                   std::string const& created_utc, LockOwner const& me,
                                                   int   ttl_s  = default_lock_ttl_s(),
                                                   NowFn now_fn = NowFn{&system_now_epoch_s}) {
        std::vector<BestandEintrag> batch;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            batch = fresh_; // KOPIE -- fresh_ bleibt bis zum gelungenen Store die Wahrheit
        }
        if (batch.empty()) return std::size_t{0};
        std::size_t const   n = batch.size();
        BestandslogDocument local;
        local.genus        = Genus::binary;
        local.created_utc  = created_utc;
        local.bestand      = std::move(batch);
        auto const outcome = with_document_lock_retry(t, doc_key, me, ttl_s, now_fn, "Bestands-Registrierung", [&]() {
            return store_document_merged(t, doc_key, local).has_value();
        });
        if (outcome != LockOutcome::ok) {
            std::cerr << "[bestandslog] warn: " << n
                      << " frische Eintraege bleiben vorgemerkt (Re-Queue beim naechsten flush)\n";
            return std::nullopt;
        }
        {
            std::lock_guard<std::mutex> lk(mtx_);
            fresh_.erase(fresh_.begin(), fresh_.begin() + static_cast<std::ptrdiff_t>(n));
        }
        return n;
    }

    [[nodiscard]] std::size_t lager_hits() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return hits_;
    }
    [[nodiscard]] std::size_t lager_size() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return lager_.size();
    }
    [[nodiscard]] std::size_t pending_fresh() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return fresh_.size();
    }

private:
    mutable std::mutex          mtx_;
    LagerIndex                  lager_; // vorbestehender Lager-Bestand (Dedup-Basis, alle Zellen)
    std::vector<BestandEintrag> fresh_; // frisch gebaute, noch nicht persistierte Eintraege
    std::size_t                 hits_ = 0;
};

} // namespace comdare::cache_engine::builder::bestandslog
