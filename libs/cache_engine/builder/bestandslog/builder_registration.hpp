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
// DOKTRIN: header-only C++23, ASCII-Kommentare (Section erlaubt), stdlib + die B1-B3-Bestandslog-
// Header. Kein std::variant, statischer Dispatch. observe() ist thread-sicher (Build-Worker-Hook).

#include "bestandslog_document.hpp"
#include "bestandslog_factory.hpp"
#include "bestandslog_index.hpp"
#include "bestandslog_lock.hpp"

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

// UTC-ISO-8601-Zeitstempel (z.B. 2026-07-23T13:45:11Z). Nur fuer die Bestandslog-Metadaten (done_utc/
// created_utc); wird ausschliesslich im aktiven (opt-in) Pfad geschrieben -> golden-neutral.
[[nodiscard]] inline std::string now_utc_iso() {
    std::time_t const t = std::time(nullptr);
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
    // Sha512Index). Fehlt/unlesbar -> leerer Index (alles gilt als frisch). Idempotent.
    void load(BestandTransport const& t, std::string const& doc_key) {
        auto bestand = make_binary_bestand();
        if (t.fetch) {
            if (auto raw = t.fetch(doc_key)) {
                if (auto doc = parse_bestandslog(*raw)) bestand.load_from_document(*doc);
            }
        }
        lager_ = bestand.index();
    }

    // Aus dem Completion-Hook (MULTITHREADED): eine fertige Binary klassifizieren. key_hex leer ->
    // no_key. Im Lager -> lager_hit (dedup, kein Neu-Eintrag). Sonst -> fresh_register (vorgemerkt).
    DedupOutcome observe(std::string key_hex, std::string pfad, std::uint64_t bytes, std::string stempel,
                         std::string done_utc) {
        if (key_hex.empty()) return DedupOutcome::no_key;
        auto const key = key_from_hex(key_hex);
        if (!key) return DedupOutcome::no_key;
        std::lock_guard<std::mutex> lk(mtx_);
        if (lager_.find(*key) != lager_.end()) {
            ++hits_;
            return DedupOutcome::lager_hit;
        }
        BestandEintrag e;
        e.key_sha512 = std::move(key_hex);
        e.pfad       = std::move(pfad);
        e.bytes      = bytes;
        e.stempel    = std::move(stempel);
        e.done_utc   = std::move(done_utc);
        fresh_.push_back(std::move(e));
        return DedupOutcome::fresh_register;
    }

    // Nach dem Bau (SINGLE-THREADED): die frisch vorgemerkten Eintraege in EINEM store_document_merged
    // ins Binary-Bestandslog schreiben (Union-Merge, doc_revision monoton). Gibt die Zahl frisch
    // registrierter Eintraege zurueck (0 = nichts zu tun) bzw. nullopt bei Store-Fehler.
    [[nodiscard]] std::optional<std::size_t> flush(BestandTransport const& t, std::string const& doc_key,
                                                   std::string const& created_utc) {
        std::vector<BestandEintrag> batch;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            batch.swap(fresh_);
        }
        if (batch.empty()) return std::size_t{0};
        BestandslogDocument local;
        local.genus       = Genus::binary;
        local.created_utc = created_utc;
        local.bestand     = std::move(batch);
        auto written      = store_document_merged(t, doc_key, local);
        if (!written) return std::nullopt;
        return local.bestand.size();
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
    Sha512Index                 lager_; // vorbestehender Lager-Bestand (Dedup-Basis)
    std::vector<BestandEintrag> fresh_; // frisch gebaute, noch nicht persistierte Eintraege
    std::size_t                 hits_ = 0;
};

} // namespace comdare::cache_engine::builder::bestandslog
