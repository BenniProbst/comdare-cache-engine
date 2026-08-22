#pragma once
// messwert_registrierung.hpp -- G-E3 (A1-Lager-Rest-Welle): der PRODUKTIVE Schreiber des ZWEITEN
// Bestandslog-Genus (measurement).
//
// IST-BEFUND, den diese Datei schliesst (OE-E-Dossier G-E3): die Factory existiert seit B3
// (bestandslog_factory.hpp:60 MesswertKeyPolicy -- Digest ueber die voll-permutativen Mess-Zeilen +
// Hardware-Identitaet, Zelle als getrennte Klammer), aber es gab KEINEN Schreiber, der ein
// measurement-Dokument fortschreibt. Das Lager kannte damit faktisch nur EIN Genus.
//
// BAUFORM = das bewaehrte observe/flush-Muster des LagerRunState (builder_registration.hpp), aber
// auf Genus::measurement gestempelt:
//   load()    -- den vorbestehenden Messwert-Bestand laden (Dedup-Basis),
//   observe() -- je GEMESSENER ZELLE einen BestandEintrag vormerken (thread-sicher, denn der
//                Debug-Mess-Pool kann mehrere Zellen ueberlappen lassen),
//   flush()   -- EINMAL gelockt ins measurement-Dokument mergen (Union, doc_revision monoton).
// D-05 (KONSOLIDIERT:24): das Bestandslog wird beim BAU **und** beim MESSEN fortgeschrieben, JE
// REALM -- deshalb ein EIGENES Dokument mit eigenem doc_key, nicht ein zweiter Abschnitt im
// Binary-Dokument. Die Genera bleiben getrennt (Section 62-B Factory-Pattern).
//
// DER SCHLUESSEL: die Mess-Identitaet ist NICHT der Binary-Fingerprint. Sie entsteht ueber
// MesswertKeyPolicy aus den Mess-Zeilen + Hardware-Identitaet; messwert_key_hex() ist die EINE
// Ableitung dafuer. Eine Fusion mit dem Binary-Key waere Section 66-N3-widrig -- die vermessene
// Binary wird ueber ihren eigenen Fingerprint REFERENZIERT (Feld stempel/versions), nicht in den
// Schluessel gemischt.
//
// G-E6-ANSCHLUSS: der Eintrag traegt das optionale versions-Tag (syntax_version 4). Die
// Dedup-Identitaet bleibt (key_sha512, zelle) -- das Tag macht die ohnehin im Preimage steckende
// Version nur LESBAR fuer den versions-selektiven Nachbau (G-A1/G-A6).
//
// DOKTRIN: header-only C++23, ASCII-Kommentare, stdlib + die B1-B4-Bestandslog-Header. Kein
// std::variant, statischer Dispatch. observe() ist thread-sicher.

#include "bestandslog_document.hpp"
#include "bestandslog_factory.hpp"
#include "bestandslog_index.hpp"
#include "bestandslog_lock.hpp"
#include "builder_registration.hpp" // with_document_lock_retry / default_lock_ttl_s / now_utc_iso (EINE Naht)

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::bestandslog {

/// Die EINE Ableitung des Messwert-Schluessels: sha512 ueber die '\n'-getrennten Komponenten
/// (voll-permutative Mess-Zeilen + Hardware-Identitaet), als 128-hex. Sie geht ueber die
/// MesswertKeyPolicy der B3-Factory -- diese Datei rechnet NICHTS selbst.
[[nodiscard]] inline std::string messwert_key_hex(std::span<std::string_view const> komponenten) {
    return to_hex(Bestand<MesswertKeyPolicy>::key_of(komponenten));
}

/// Ausgang der per-Zelle-Klassifikation (Spiegel von DedupOutcome, eigener Typ, damit ein Aufrufer
/// die zwei Genera nicht versehentlich vermischt).
enum class MesswertOutcome {
    lager_hit,      // schon im Messwert-Lager -> KEINE Neu-Registrierung (die Zelle war gemessen)
    fresh_register, // neu -> vorgemerkt
    no_key          // kein gueltiger 128-hex-Schluessel -> nichts getan
};

[[nodiscard]] constexpr std::string_view to_string(MesswertOutcome o) noexcept {
    switch (o) {
        case MesswertOutcome::lager_hit: return "lager_hit";
        case MesswertOutcome::fresh_register: return "fresh_register";
        case MesswertOutcome::no_key: return "no_key";
    }
    return "no_key";
}

// ---------------------------------------------------------------------------
// MesswertRunState -- der Bestandslog-Zustand EINES Mess-Laufs (Genus::measurement).
// ---------------------------------------------------------------------------
class MesswertRunState {
public:
    /// Vor dem Messen: den vorbestehenden Messwert-Bestand laden. Fehlt/unlesbar -> leeres Lager
    /// (alles gilt als frisch). Idempotent.
    void load(BestandTransport const& t, std::string const& doc_key) {
        auto bestand = make_messwert_bestand();
        if (t.fetch) {
            if (auto raw = t.fetch(doc_key)) {
                if (auto doc = parse_bestandslog(*raw)) bestand.load_from_document(*doc);
            }
        }
        std::lock_guard<std::mutex> lk(mtx_);
        lager_ = bestand.index();
    }

    /// Je GEMESSENER ZELLE (MULTITHREADED-tauglich -- der Debug-Mess-Pool laesst Zellen ueberlappen).
    /// key_hex ist der Messwert-Schluessel (messwert_key_hex), NICHT der Binary-Fingerprint.
    /// pfad ist die Verortung im Messdaten-Realm-Baum (S2-Grammatik); stempel referenziert die
    /// vermessene Binary; versions ist der optionale G-E6-Tag.
    MesswertOutcome observe(std::string key_hex, ZellKoordinaten zelle, std::string pfad, std::uint64_t bytes,
                            std::string stempel, std::string done_utc, std::string versions = {}) {
        if (key_hex.empty()) return MesswertOutcome::no_key;
        auto const key = lager_key_from_hex(key_hex, zelle);
        if (!key) return MesswertOutcome::no_key;
        std::lock_guard<std::mutex> lk(mtx_);
        if (lager_.find(*key) != lager_.end()) {
            ++hits_;
            return MesswertOutcome::lager_hit;
        }
        BestandEintrag e;
        e.key_sha512 = std::move(key_hex);
        e.zelle      = std::move(zelle);
        e.pfad       = std::move(pfad);
        e.bytes      = bytes;
        e.stempel    = std::move(stempel);
        e.done_utc   = std::move(done_utc);
        e.versions   = std::move(versions);
        fresh_.push_back(std::move(e));
        return MesswertOutcome::fresh_register;
    }

    /// Lesender Zugriff fuer eine Mess-Presence-Naht (war diese Zelle schon gemessen?).
    ///
    /// C-14-SCHWESTER (#97/E-12, KON3-06-Klasse; T-6 "beide Genera", Audit-Fund S97-F1): eine
    /// LEERE Lauf-Zelle begruendet NIE einen "schon gemessen"-Befund. Ein kollabierter
    /// Leer-Zellen-Bestand (je Kennung EIN Eintrag mit leerer Zelle) traefe sonst via
    /// lager_key_from_hex jede leer gefragte Zelle (leer-vs-leer matcht) -- dieselbe Luecke,
    /// die C-14 auf der Binary-Seite an der Presence-Naht schliesst (lager_presence.hpp,
    /// make_lager_presence). PLATZIERUNGS-ABWAEGUNG: dort sitzt die Wache BEWUSST an der Naht
    /// und NICHT in LagerRunState::lager_contains (der Koeder-Test ASSERTet das rohe Tupel);
    /// HIER ist lager_contains laut diesem Docstring AUSSCHLIESSLICH die (kuenftige)
    /// Mess-Presence-Naht -- die Wache sitzt deshalb direkt an ihr. Der rohe Index-Weg dieses
    /// Genus bleibt observe() (dedupliziert weiter ueber das unbewachte Tupel; Schwester-Karte
    /// S97-F1, Traeger wie F-106). Beweis: test_c14_messwert_presence_wache (Koeder ROT vor
    /// dieser Wache, 2026-08-22).
    [[nodiscard]] bool lager_contains(std::string_view key_hex, ZellKoordinaten const& zelle) const {
        if (zelle.empty()) return false; // C-14-Schwester: leere Lauf-Zelle ist NIE ein Beleg (s. oben)
        auto const k = lager_key_from_hex(key_hex, zelle);
        if (!k) return false; // KONSERVATIVER Miss: lieber einmal zu viel messen als eine Luecke melden
        std::lock_guard<std::mutex> lk(mtx_);
        return lager_.find(*k) != lager_.end();
    }

    /// Nach dem Messen (SINGLE-THREADED): die frisch vorgemerkten Eintraege in EINEM gelockten
    /// store_document_merged ins MEASUREMENT-Bestandslog schreiben. Genus::measurement ist hier fest
    /// -- ein Schreiber, der sein Genus vom Aufrufer bekaeme, koennte die zwei Realms vermischen.
    ///
    /// RE-QUEUE wie beim Binary-Genus (B13): fresh_ wird ERST NACH gelungenem Store geleert, und nur
    /// der geschriebene PRAEFIX -- nebenlaeufig hinzugekommene Eintraege bleiben erhalten.
    [[nodiscard]] std::optional<std::size_t> flush(BestandTransport const& t, std::string const& doc_key,
                                                   std::string const& created_utc, LockOwner const& me,
                                                   int   ttl_s  = default_lock_ttl_s(),
                                                   NowFn now_fn = NowFn{&system_now_epoch_s}) {
        std::vector<BestandEintrag> batch;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            batch = fresh_;
        }
        if (batch.empty()) return std::size_t{0};
        std::size_t const   n = batch.size();
        BestandslogDocument local;
        local.genus        = Genus::measurement;
        local.created_utc  = created_utc;
        local.bestand      = std::move(batch);
        auto const outcome = with_document_lock_retry(t, doc_key, me, ttl_s, now_fn, "Messwert-Registrierung", [&]() {
            return store_document_merged(t, doc_key, local).has_value();
        });
        if (outcome != LockOutcome::ok) {
            std::cerr << "[bestandslog] warn: " << n
                      << " frische Messwert-Eintraege bleiben vorgemerkt (Re-Queue beim naechsten flush)\n";
            return std::nullopt;
        }
        {
            std::lock_guard<std::mutex> lk(mtx_);
            fresh_.erase(fresh_.begin(), fresh_.begin() + static_cast<std::ptrdiff_t>(n));
        }
        return n;
    }

    /// TP1-N2-Analogon fuer den Mess-Pfad: vorgemerkte Eintraege verwerfen, deren pfad unter einem
    /// der Praefixe liegt (z.B. weil der Rueckschrieb der Zelle nachweislich scheiterte). Vergleich
    /// MIT Trenner -- ein nacktes starts_with liesse "...=1" auch "...=16" verwerfen (TP1-F1).
    [[nodiscard]] std::size_t discard_fresh_with_pfad_prefix(std::vector<std::string> const& prefixes) {
        if (prefixes.empty()) return 0;
        std::lock_guard<std::mutex> lk(mtx_);
        std::size_t const           vorher = fresh_.size();
        std::erase_if(fresh_, [&prefixes](BestandEintrag const& e) {
            for (auto const& p : prefixes)
                if (!p.empty() && e.pfad.starts_with(p + "/")) return true;
            return false;
        });
        return vorher - fresh_.size();
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
    LagerIndex                  lager_;
    std::vector<BestandEintrag> fresh_;
    std::size_t                 hits_ = 0;
};

} // namespace comdare::cache_engine::builder::bestandslog
