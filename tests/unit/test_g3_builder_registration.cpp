// test_g3_builder_registration -- G3 / #46b Lagerhaltung, Scheibe I1.
//
// Die Bestandslog-Laufzeit-Zustandsmaschine des Bau-Loops (LagerRunState) mit Fake-Transport:
// Lager-Index laden, per-Binary-Klassifikation (Dedup-Hit / frisch / no_key), Flush + Union-Merge.
// Deterministisch, ohne minio.
//
// Ertuechtigung 26.07. (Lane E): dazu die AUFRUFER-SEITE des gelockten Schreib-Zyklus, die in dieser
// Schicht liegt, weil Iterator und flush() sie teilen -- with_document_lock_retry (Zwei-Schreiber-
// Simulation: der zweite bekommt lock_unavailable, wiederholt EINMAL mit Jitter und gelingt),
// store_reservation_locked (Reservierung nur UNTER gehaltenem Lock im Store), die ECHTE 30min-
// pro-forma-Frist (make_slice_reservation/utc_iso_from_epoch, B11), das provision_only-Gate
// (planer_driven_active, B5) und die RE-QUEUE in flush (B13: fresh_ bleibt bei Store-Fehler
// vorgemerkt, statt mit dem Batch aus dem RAM zu verschwinden).

#include "bestandslog/builder_registration.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace bl = comdare::cache_engine::builder::bestandslog;

namespace {

// In-Memory-Objekt-Store (Fake-Transport). Zwei Skript-Haken machen die Lock-Interleavings
// deterministisch: before_fetch laeuft VOR der Antwort (damit ein Test den fremden Lock genau
// zwischen zwei Versuchen freigeben kann), before_store VOR dem Schreiben (damit ein Test sieht,
// WELCHER Lock in dem Moment gehalten wird). fail_store_key laesst genau einen Schluessel scheitern.
struct FakeStore {
    std::map<std::string, std::string>      objs;
    std::function<void(std::string const&)> before_fetch;
    std::function<void(std::string const&)> before_store;
    std::string                             fail_store_key;

    bl::BestandTransport transport() {
        bl::BestandTransport t;
        t.fetch = [this](std::string const& k) -> std::optional<std::string> {
            if (before_fetch) before_fetch(k);
            auto it = objs.find(k);
            if (it == objs.end()) return std::nullopt;
            return it->second;
        };
        t.store = [this](std::string const& k, std::string const& c) -> bool {
            if (before_store) before_store(k);
            if (!fail_store_key.empty() && k == fail_store_key) return false;
            objs[k] = c;
            return true;
        };
        t.remove = [this](std::string const& k) -> bool {
            objs.erase(k);
            return true;
        };
        t.stat = [this](std::string const& k) -> std::optional<bl::ObjectStat> {
            auto it = objs.find(k);
            if (it == objs.end()) return std::nullopt;
            return bl::ObjectStat{.size = it->second.size(), .mtime_epoch_s = 0};
        };
        return t;
    }
};

constexpr char const* kDocKey  = "bestandslog/binary_bestand.xml";
constexpr char const* kLockKey = "bestandslog/binary_bestand.xml.lock";

// ttl DIESER Tests: eine BELIEBIGE Obergrenze. Keiner der Faelle unten haengt an der Zahl -- die
// ttl-Semantik (ereignis-gebundene Obergrenze fuer den Stale-Bruch) und die Frist-Wache
// (section_budget) gehoeren bestandslog_lock.hpp und sind dort getestet. Hier zaehlt allein, dass ein
// fremder Lock zur Skript-Zeit FRISCH ist; der Produktions-Default kommt aus LockRecord (s. den
// Identitaets-Test unten), nie aus einer Zahl in dieser Datei.
constexpr int kTestTtlS = 90;

// Der Schreiber DIESES Prozesses und ein fremder auf derselben Zelle (Zwei-Schreiber-Simulation:
// prod1 und prod2 schreiben laut Ledger-Symmetrie in DASSELBE Dokument).
bl::LockOwner me() { return bl::LockOwner{"uuid-prod1-42", "prod1", 4242}; }

// Faengt std::cerr fuer die Dauer des Scopes ab -> die Testat-Zeilen sind PRUEFBAR (Anzahl + Text)
// statt Test-Rauschen (Muster test_g3_bestandslog_lock.cpp).
class CerrCapture {
public:
    CerrCapture() : alt_{std::cerr.rdbuf(puffer_.rdbuf())} {}
    CerrCapture(CerrCapture const&)            = delete;
    CerrCapture& operator=(CerrCapture const&) = delete;
    ~CerrCapture() { std::cerr.rdbuf(alt_); }

    [[nodiscard]] std::string text() const { return puffer_.str(); }
    [[nodiscard]] std::size_t zeilen() const {
        auto const s = puffer_.str();
        return static_cast<std::size_t>(std::count(s.begin(), s.end(), '\n'));
    }

private:
    std::ostringstream puffer_;
    std::streambuf*    alt_;
};

// Skript-Uhr als NowFn: fester Wert (der letzte bleibt stehen) -> ttl-/Frist-Faelle ohne Wall-Clock.
bl::NowFn feste_uhr(std::int64_t ts) {
    return [ts]() { return ts; };
}

// Den Owner-Token des aktuell gehaltenen Locks lesen ("" = kein Lock).
std::string lock_owner_in(FakeStore const& s) {
    auto it = s.objs.find(kLockKey);
    if (it == s.objs.end()) return "";
    auto const r = bl::parse_lock(it->second);
    return r ? r->owner_uuid : std::string{"<unparsable>"};
}

// Ein gueltiger 128-hex-Key (aus einer bekannten Ziffer aufgebaut).
std::string hexkey(char c) { return std::string(128, c); }

// Die Zell-Koordinaten des "Laufs" (§62-NACHTRAG-4) -- je Lauf konstant, vom Host mitgereicht.
bl::ZellKoordinaten const kZelle{.combo = "default", .opt = "O2", .simd = "avx2"};
bl::ZellKoordinaten const kZelleAndereIsa{.combo = "default", .opt = "O2", .simd = "avx512"};

} // namespace

TEST(G3BuilderRegistration, NowUtcIsoFormat) {
    std::string const s = bl::now_utc_iso();
    ASSERT_EQ(s.size(), 20u); // YYYY-MM-DDTHH:MM:SSZ
    EXPECT_EQ(s[4], '-');
    EXPECT_EQ(s[10], 'T');
    EXPECT_EQ(s.back(), 'Z');
}

TEST(G3BuilderRegistration, LoadEmptyLager) {
    FakeStore         s;
    auto              t = s.transport();
    bl::LagerRunState st;
    st.load(t, kDocKey); // nichts im Store -> leerer Index
    EXPECT_EQ(st.lager_size(), 0u);
    EXPECT_EQ(st.lager_hits(), 0u);
    EXPECT_EQ(st.pending_fresh(), 0u);
}

TEST(G3BuilderRegistration, ObserveNoKey) {
    FakeStore         s;
    auto              t = s.transport();
    bl::LagerRunState st;
    st.load(t, kDocKey);
    EXPECT_EQ(st.observe("", kZelle, "pfad", 10, "", "utc"), bl::DedupOutcome::no_key);
    EXPECT_EQ(st.observe("nothex", kZelle, "pfad", 10, "", "utc"), bl::DedupOutcome::no_key); // falsche Laenge
    EXPECT_EQ(st.pending_fresh(), 0u);
}

TEST(G3BuilderRegistration, ObserveFreshThenFlush) {
    FakeStore         s;
    auto              t = s.transport();
    bl::LagerRunState st;
    st.load(t, kDocKey);

    EXPECT_EQ(st.observe(hexkey('a'), kZelle, "tier/0.dll", 428032, "[d,e,f]", "2026-07-23T12:00:00Z"),
              bl::DedupOutcome::fresh_register);
    EXPECT_EQ(st.observe(hexkey('b'), kZelle, "tier/1.dll", 12345, "[a,b,c]", "2026-07-23T12:01:00Z"),
              bl::DedupOutcome::fresh_register);
    EXPECT_EQ(st.pending_fresh(), 2u);

    auto n = st.flush(t, kDocKey, "2026-07-23T12:02:00Z", me());
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, 2u);
    EXPECT_EQ(st.pending_fresh(), 0u);

    // Der Store traegt jetzt ein Binary-Bestandslog mit beiden Eintraegen.
    auto doc = bl::parse_bestandslog(s.objs[kDocKey]);
    ASSERT_TRUE(doc.has_value());
    EXPECT_EQ(doc->genus, bl::Genus::binary);
    EXPECT_EQ(doc->bestand.size(), 2u);

    // Zweiter Flush ohne neue Beobachtung -> nichts zu tun.
    auto n2 = st.flush(t, kDocKey, "2026-07-23T12:03:00Z", me());
    ASSERT_TRUE(n2.has_value());
    EXPECT_EQ(*n2, 0u);
}

TEST(G3BuilderRegistration, DedupHitAgainstPreloadedLager) {
    FakeStore s;
    auto      t = s.transport();

    // Vorbestehendes Lager mit Key 'a' seed'en.
    bl::BestandslogDocument seed;
    seed.genus = bl::Genus::binary;
    bl::BestandEintrag e;
    e.key_sha512 = hexkey('a');
    e.zelle      = kZelle;
    e.pfad       = "tier/0.dll";
    seed.bestand.push_back(e);
    s.objs[kDocKey] = bl::emit_document(seed);

    bl::LagerRunState st;
    st.load(t, kDocKey);
    EXPECT_EQ(st.lager_size(), 1u);

    // 'a' ist schon im Lager -> Hit (kein Neu-Eintrag); 'b' ist neu -> fresh.
    EXPECT_EQ(st.observe(hexkey('a'), kZelle, "tier/0.dll", 428032, "", "utc"), bl::DedupOutcome::lager_hit);
    EXPECT_EQ(st.observe(hexkey('b'), kZelle, "tier/1.dll", 999, "", "utc"), bl::DedupOutcome::fresh_register);
    EXPECT_EQ(st.lager_hits(), 1u);
    EXPECT_EQ(st.pending_fresh(), 1u);

    auto n = st.flush(t, kDocKey, "utc2", me());
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, 1u); // nur 'b' frisch

    // Der Merge vereint das vorbestehende 'a' + das neue 'b'.
    auto doc = bl::parse_bestandslog(s.objs[kDocKey]);
    ASSERT_TRUE(doc.has_value());
    EXPECT_EQ(doc->bestand.size(), 2u);
}

// ---------------------------------------------------------------------------
// Folge-A / §62-NACHTRAG-4: dieselbe Binary unter EINER anderen Zelle ist KEIN Dedup-Treffer.
// Ueber den Fingerprint allein waere der avx512-Bau als "schon im Lager" durchgefallen.
// ---------------------------------------------------------------------------
TEST(G3BuilderRegistration, DedupIsPerCellNotPerFingerprint) {
    FakeStore s;
    auto      t = s.transport();

    bl::BestandslogDocument seed;
    seed.genus = bl::Genus::binary;
    bl::BestandEintrag e;
    e.key_sha512 = hexkey('a');
    e.zelle      = kZelle; // avx2 liegt im Lager
    e.pfad       = "tier/avx2/0.dll";
    seed.bestand.push_back(e);
    s.objs[kDocKey] = bl::emit_document(seed);

    bl::LagerRunState st;
    st.load(t, kDocKey);
    EXPECT_EQ(st.lager_size(), 1u);

    // Gleicher Fingerprint, gleiche Zelle -> Treffer.
    EXPECT_EQ(st.observe(hexkey('a'), kZelle, "tier/avx2/0.dll", 1, "", "utc"), bl::DedupOutcome::lager_hit);
    // Gleicher Fingerprint, ANDERE ISA -> frisch (das ist der Kern von NACHTRAG-4).
    EXPECT_EQ(st.observe(hexkey('a'), kZelleAndereIsa, "tier/avx512/0.dll", 1, "", "utc"),
              bl::DedupOutcome::fresh_register);
    EXPECT_EQ(st.lager_hits(), 1u);
    EXPECT_EQ(st.pending_fresh(), 1u);

    auto const n = st.flush(t, kDocKey, "utc2", me());
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, 1u);

    // Nach dem Merge stehen ZWEI Eintraege mit demselben key_sha512 und verschiedener simd im Dokument.
    auto const doc = bl::parse_bestandslog(s.objs[kDocKey]);
    ASSERT_TRUE(doc.has_value());
    ASSERT_EQ(doc->bestand.size(), 2u);
    EXPECT_EQ(doc->bestand[0].key_sha512, doc->bestand[1].key_sha512);
    EXPECT_NE(doc->bestand[0].zelle.simd, doc->bestand[1].zelle.simd);
}

// ---------------------------------------------------------------------------
// Folge-A (T2): der lesende lager_contains-Zugriff -- ueber den LagerKey und ueber (hex, Zelle).
// Ungueltiges Hex ergibt einen KONSERVATIVEN Miss (false), nie einen falschen Treffer.
// ---------------------------------------------------------------------------
TEST(G3BuilderRegistration, LagerContainsAccessor) {
    FakeStore s;
    auto      t = s.transport();

    bl::BestandslogDocument seed;
    seed.genus = bl::Genus::binary;
    bl::BestandEintrag e;
    e.key_sha512 = hexkey('a');
    e.zelle      = kZelle;
    seed.bestand.push_back(e);
    s.objs[kDocKey] = bl::emit_document(seed);

    bl::LagerRunState st;
    st.load(t, kDocKey);

    auto const k = bl::lager_key_from_hex(hexkey('a'), kZelle);
    ASSERT_TRUE(k.has_value());
    EXPECT_TRUE(st.lager_contains(*k));
    EXPECT_TRUE(st.lager_contains(hexkey('a'), kZelle));

    EXPECT_FALSE(st.lager_contains(hexkey('a'), kZelleAndereIsa)); // andere Zelle
    EXPECT_FALSE(st.lager_contains(hexkey('b'), kZelle));          // anderer Fingerprint
    EXPECT_FALSE(st.lager_contains("", kZelle));                   // kein Hex -> konservativer Miss
    EXPECT_FALSE(st.lager_contains("kein-hex", kZelle));           // ungueltiges Hex -> konservativer Miss

    // Der Accessor ist lesend: er veraendert weder hits_ noch die Vormerk-Liste.
    EXPECT_EQ(st.lager_hits(), 0u);
    EXPECT_EQ(st.pending_fresh(), 0u);
}

TEST(G3BuilderRegistration, StoreFailurePropagates) {
    // Transport mit fehlschlagendem store -> flush liefert nullopt. Mit dem gelockten Zyklus
    // scheitert schon der Lock-Erwerb (auch das Lock-Objekt ist ein store) -> nichts geschrieben,
    // und der Batch bleibt vorgemerkt (Re-Queue, B13).
    bl::BestandTransport t;
    t.fetch = [](std::string const&) -> std::optional<std::string> { return std::nullopt; };
    t.store = [](std::string const&, std::string const&) -> bool { return false; };
    bl::LagerRunState st;
    st.load(t, kDocKey);
    EXPECT_EQ(st.observe(hexkey('c'), kZelle, "p", 1, "", "u"), bl::DedupOutcome::fresh_register);
    CerrCapture const cap;
    auto              n = st.flush(t, kDocKey, "u", me());
    EXPECT_FALSE(n.has_value());
    EXPECT_EQ(st.pending_fresh(), 1u) << cap.text();
}

// ===========================================================================================
// Ertuechtigung 26.07. (Lane E) -- Iterator-Gates + Lock-Verdrahtung.
// ===========================================================================================

// ---------------------------------------------------------------------------
// B11 -- die ECHTE 30min-pro-forma-Frist. Vorher entstanden reserviert_utc und pro_forma_bis_utc aus
// ZWEI now_utc_iso()-Aufrufen: die Frist war eine Sekunde nach ihrer Eintragung abgelaufen, der
// Takeover-Schutz damit wirkungslos und die Log-Zeile eines mehrtaegigen Laufs eine Unwahrheit.
// ---------------------------------------------------------------------------
TEST(G3BuilderRegistration, UtcIsoFromEpochIsLiteral) {
    EXPECT_EQ(bl::utc_iso_from_epoch(0), "1970-01-01T00:00:00Z");
    EXPECT_EQ(bl::utc_iso_from_epoch(1800), "1970-01-01T00:30:00Z");
    EXPECT_EQ(bl::utc_iso_from_epoch(1785062711), "2026-07-26T10:45:11Z");
    // now_utc_iso ist genau die Epoch-Form auf der System-Uhr (Format unveraendert, s.o.).
    EXPECT_EQ(bl::now_utc_iso().size(), 20u);
}

TEST(G3BuilderRegistration, SliceReservationCarriesRealThirtyMinuteDeadline) {
    std::int64_t const now = 1785062711; // 2026-07-26T10:45:11Z
    auto const         r   = bl::make_slice_reservation("uuid-prod1-42", 7, "prod1", 24, 28672, 4096, now);

    EXPECT_EQ(r.id, "uuid-prod1-42/7"); // per-Maschine eindeutig -> der Union-Merge vermischt nie
    EXPECT_EQ(r.typ, bl::BatchTyp::tier);
    EXPECT_EQ(r.status, bl::BatchStatus::offen);
    EXPECT_TRUE(r.eta_s.empty()); // pro forma == noch nicht kalibriert
    EXPECT_EQ(r.maschine, "prod1");
    EXPECT_EQ(r.threads, 24u);
    EXPECT_EQ(r.slice_begin, 28672u);
    EXPECT_EQ(r.slice_count, 4096u);

    // Der Kern: die Frist liegt 30 Minuten NACH der Reservierung -- literal, nicht "spaeter".
    EXPECT_EQ(r.reserviert_utc, "2026-07-26T10:45:11Z");
    EXPECT_EQ(r.pro_forma_bis_utc, "2026-07-26T11:15:11Z");
    EXPECT_EQ(r.pro_forma_bis_utc, bl::utc_iso_from_epoch(now + 30 * 60));
    EXPECT_NE(r.reserviert_utc, r.pro_forma_bis_utc); // die Regression von vorher

    // Und dieselbe Frist in Epoch gelesen: 1 s davor ist sie NICHT abgelaufen, 1 s danach schon.
    std::int64_t const frist = bl::pro_forma_deadline_epoch_s(now);
    EXPECT_EQ(frist - now, 30 * 60);
    EXPECT_FALSE(bl::is_pro_forma_expired(frist, frist - 1));
    EXPECT_TRUE(bl::is_pro_forma_expired(frist, frist + 1));
}

// ---------------------------------------------------------------------------
// B5 -- das provision_only-Gate des Planer-getriebenen Baus. Bestandslog AN allein genuegt NICHT:
// im 1-Thread-MESS-Lauf duerfen die Reservierungen keine mc-Shellouts in die Messung legen.
// ---------------------------------------------------------------------------
TEST(G3BuilderRegistration, PlanerDrivenNeedsBestandslogAndProvisionOnly) {
    EXPECT_TRUE(bl::planer_driven_active(true, true));   // Bau-Lauf mit Bestandslog -> Planer treibt
    EXPECT_FALSE(bl::planer_driven_active(true, false)); // MESS-Lauf -> NIE (der eigentliche Fix)
    EXPECT_FALSE(bl::planer_driven_active(false, true)); // Bestandslog aus -> byte-identisch zum Ist
    EXPECT_FALSE(bl::planer_driven_active(false, false));
}

// ---------------------------------------------------------------------------
// N7-D1 -- with_document_lock_retry: EIN Zyklus + GENAU EINE Wiederholungsrunde mit Jitter.
// Zwei-Schreiber-Simulation: prod2 haelt den Lock, prod1 bekommt lock_unavailable, wiederholt und
// gelingt, nachdem prod2 freigegeben hat. Die Arbeit laeuft dabei GENAU EINMAL.
// ---------------------------------------------------------------------------
TEST(G3BuilderRegistration, LockRetryWinsAfterForeignWriterReleases) {
    FakeStore s;
    auto      t = s.transport();
    // Fremder, FRISCHER Lock (prod2 schreibt gerade) -> Versuch 1 bekommt ihn nicht.
    s.objs[kLockKey] = bl::serialize_lock(bl::LockRecord{"uuid-prod2-9", "prod2", 99, 1000, kTestTtlS});

    int lock_lesungen = 0;
    s.before_fetch    = [&s, &lock_lesungen](std::string const& k) {
        if (k != kLockKey) return;
        // prod2 gibt seinen Lock genau zum ZWEITEN Lese-Versuch frei (der Wiederholungsrunde).
        if (++lock_lesungen == 2) s.objs.erase(kLockKey);
    };

    int               laeufe = 0;
    CerrCapture const cap;
    auto const o = bl::with_document_lock_retry(t, kDocKey, me(), kTestTtlS, feste_uhr(1000), "Testat-Arbeit", [&]() {
        ++laeufe;
        s.objs[kDocKey] = "geschrieben";
        return true;
    });

    EXPECT_EQ(o, bl::LockOutcome::ok);
    EXPECT_EQ(laeufe, 1);                      // im verlorenen Versuch lief NICHTS
    EXPECT_EQ(s.objs[kDocKey], "geschrieben"); // die Wiederholung hat geschrieben
    EXPECT_EQ(lock_owner_in(s), "");           // eigener Lock wieder freigegeben
    EXPECT_EQ(cap.zeilen(), 0u) << cap.text(); // gelungener Zyklus schweigt
}

TEST(G3BuilderRegistration, LockRetryGivesUpAfterOneRoundAndBuildContinues) {
    FakeStore s;
    auto      t      = s.transport();
    s.objs[kLockKey] = bl::serialize_lock(bl::LockRecord{"uuid-prod2-9", "prod2", 99, 1000, kTestTtlS});

    int               laeufe = 0;
    CerrCapture const cap;
    auto const        o =
        bl::with_document_lock_retry(t, kDocKey, me(), kTestTtlS, feste_uhr(1000), "pro-forma-Reservierung", [&]() {
            ++laeufe;
            return true;
        });
    auto const text = cap.text();

    EXPECT_EQ(o, bl::LockOutcome::lock_unavailable);
    EXPECT_EQ(laeufe, 0);                        // NICHTS geschrieben
    EXPECT_EQ(s.objs.count(kDocKey), 0u);        // das Dokument bleibt unberuehrt
    EXPECT_EQ(lock_owner_in(s), "uuid-prod2-9"); // fremder Lock unberuehrt (nie gebrochen)
    EXPECT_EQ(cap.zeilen(), 1u) << text;         // GENAU eine Testat-Zeile
    EXPECT_NE(text.find("[bestandslog] warn"), std::string::npos) << text;
    EXPECT_NE(text.find("pro-forma-Reservierung"), std::string::npos) << text;
    EXPECT_NE(text.find("der Bau laeuft weiter"), std::string::npos) << text;
}

TEST(G3BuilderRegistration, LockRetryReportsWorkFailureAsFehler) {
    FakeStore s;
    auto      t      = s.transport();
    s.fail_store_key = kDocKey; // der Lock gelingt, der Dokument-Schreibvorgang scheitert

    CerrCapture const cap;
    auto const o = bl::with_document_lock_retry(t, kDocKey, me(), kTestTtlS, feste_uhr(1000), "Bestands-Registrierung",
                                                [&]() { return t.store(kDocKey, "egal"); });
    auto const text = cap.text();

    EXPECT_EQ(o, bl::LockOutcome::work_failed);
    EXPECT_EQ(lock_owner_in(s), ""); // Lock auch im Fehlschlag freigegeben (RAII)
    EXPECT_EQ(cap.zeilen(), 1u) << text;
    EXPECT_NE(text.find("[bestandslog] FEHLER"), std::string::npos) << text;
    EXPECT_NE(text.find("Bestands-Registrierung"), std::string::npos) << text;
}

// ---------------------------------------------------------------------------
// N7-D1 (a)/(b) -- die Reservierungs-Schreibstelle: EIN Record, gelockt, Rueckgabe auswertbar.
// ---------------------------------------------------------------------------
TEST(G3BuilderRegistration, StoreReservationRunsUnderHeldLock) {
    FakeStore s;
    auto      t = s.transport();

    std::string owner_beim_schreiben = "<kein Lock>";
    s.before_store                   = [&s, &owner_beim_schreiben](std::string const& k) {
        if (k == kDocKey) owner_beim_schreiben = lock_owner_in(s);
    };

    auto const        r = bl::make_slice_reservation(me().owner_uuid, 0, "prod1", 24, 0, 4096, 1785062711);
    CerrCapture const cap;
    EXPECT_TRUE(
        bl::store_reservation_locked(t, kDocKey, me(), kTestTtlS, feste_uhr(1000), r, "pro-forma-Reservierung"));
    EXPECT_EQ(cap.zeilen(), 0u) << cap.text();

    // Der Beweis: waehrend des Dokument-Schreibvorgangs hielt GENAU DIESER Prozess den Lock.
    EXPECT_EQ(owner_beim_schreiben, me().owner_uuid);
    EXPECT_EQ(lock_owner_in(s), ""); // danach freigegeben
    auto const doc = bl::parse_bestandslog(s.objs[kDocKey]);
    ASSERT_TRUE(doc.has_value());
    ASSERT_EQ(doc->reservierungen.size(), 1u);
    EXPECT_EQ(doc->reservierungen[0].id, "uuid-prod1-42/0");
    EXPECT_EQ(doc->reservierungen[0].pro_forma_bis_utc, "2026-07-26T11:15:11Z");
}

TEST(G3BuilderRegistration, StoreReservationYieldsToForeignLockWithoutWriting) {
    FakeStore s;
    auto      t      = s.transport();
    s.objs[kLockKey] = bl::serialize_lock(bl::LockRecord{"uuid-prod2-9", "prod2", 99, 1000, kTestTtlS});

    auto const        r = bl::make_slice_reservation(me().owner_uuid, 3, "prod1", 24, 12288, 4096, 1785062711);
    CerrCapture const cap;
    EXPECT_FALSE(
        bl::store_reservation_locked(t, kDocKey, me(), kTestTtlS, feste_uhr(1000), r, "pro-forma-Reservierung"));
    EXPECT_EQ(s.objs.count(kDocKey), 0u);        // kein Eintrag
    EXPECT_EQ(lock_owner_in(s), "uuid-prod2-9"); // fremder Lock unberuehrt
    EXPECT_EQ(cap.zeilen(), 1u) << cap.text();   // aber sichtbar
}

// ---------------------------------------------------------------------------
// N7-D1 (c) + B13 -- flush ist gelockt UND haelt den Batch bei Fehlschlag zurueck.
// ---------------------------------------------------------------------------
TEST(G3BuilderRegistration, FlushRunsUnderHeldLock) {
    FakeStore s;
    auto      t = s.transport();

    std::string owner_beim_schreiben = "<kein Lock>";
    s.before_store                   = [&s, &owner_beim_schreiben](std::string const& k) {
        if (k == kDocKey) owner_beim_schreiben = lock_owner_in(s);
    };

    bl::LagerRunState st;
    st.load(t, kDocKey);
    ASSERT_EQ(st.observe(hexkey('a'), kZelle, "tier/0.dll", 4096, "", "2026-07-26T10:00:00Z"),
              bl::DedupOutcome::fresh_register);

    CerrCapture const cap;
    auto const        n = st.flush(t, kDocKey, "2026-07-26T10:45:11Z", me(), kTestTtlS, feste_uhr(1000));
    ASSERT_TRUE(n.has_value()) << cap.text();
    EXPECT_EQ(*n, 1u);
    EXPECT_EQ(owner_beim_schreiben, me().owner_uuid);
    EXPECT_EQ(lock_owner_in(s), "");
    EXPECT_EQ(cap.zeilen(), 0u) << cap.text();
}

TEST(G3BuilderRegistration, FlushKeepsBatchQueuedWhenStoreFailsThenWritesItLater) {
    FakeStore s;
    auto      t = s.transport();
    // Unlesbares Remote -> die Voll-Wipe-Wache in store_document_merged schreibt NICHT (nullopt).
    s.objs[kDocKey]   = "das ist kein Bestandslog";
    auto const vorher = s.objs[kDocKey];

    bl::LagerRunState st;
    st.load(t, kDocKey);
    ASSERT_EQ(st.observe(hexkey('a'), kZelle, "tier/0.dll", 4096, "", "2026-07-26T10:00:00Z"),
              bl::DedupOutcome::fresh_register);
    ASSERT_EQ(st.observe(hexkey('b'), kZelle, "tier/1.dll", 8192, "", "2026-07-26T10:01:00Z"),
              bl::DedupOutcome::fresh_register);

    {
        CerrCapture const cap;
        auto const        n    = st.flush(t, kDocKey, "2026-07-26T10:45:11Z", me(), kTestTtlS, feste_uhr(1000));
        auto const        text = cap.text();
        EXPECT_FALSE(n.has_value());
        EXPECT_EQ(s.objs[kDocKey], vorher); // Bestand steht unberuehrt
        EXPECT_EQ(lock_owner_in(s), "");    // Lock freigegeben
        EXPECT_NE(text.find("[bestandslog] FEHLER"), std::string::npos) << text;
        EXPECT_NE(text.find("Re-Queue"), std::string::npos) << text;
        // DER KERN: die zwei Eintraege sind NICHT verloren (vorher swappte flush sie vor dem Store leer).
        EXPECT_EQ(st.pending_fresh(), 2u);
    }

    // Naechster flush auf ein wieder lesbares Dokument -> derselbe Batch geht durch.
    s.objs.erase(kDocKey);
    CerrCapture const cap;
    auto const        n2 = st.flush(t, kDocKey, "2026-07-26T10:50:00Z", me(), kTestTtlS, feste_uhr(2000));
    ASSERT_TRUE(n2.has_value()) << cap.text();
    EXPECT_EQ(*n2, 2u);
    EXPECT_EQ(st.pending_fresh(), 0u);
    auto const doc = bl::parse_bestandslog(s.objs[kDocKey]);
    ASSERT_TRUE(doc.has_value());
    EXPECT_EQ(doc->bestand.size(), 2u);
}

TEST(G3BuilderRegistration, FlushWithoutOwnerTokenWritesNothing) {
    // Fail-closed (N7-D5): ohne Owner-Token ist das Lock wertlos -> nichts schreiben, alles vormerken.
    FakeStore s;
    auto      t = s.transport();

    bl::LagerRunState st;
    st.load(t, kDocKey);
    ASSERT_EQ(st.observe(hexkey('a'), kZelle, "tier/0.dll", 4096, "", "u"), bl::DedupOutcome::fresh_register);

    CerrCapture const cap;
    auto const        n = st.flush(t, kDocKey, "u", bl::LockOwner{}, kTestTtlS, feste_uhr(1000));
    EXPECT_FALSE(n.has_value());
    EXPECT_EQ(st.pending_fresh(), 1u);
    EXPECT_EQ(s.objs.count(kDocKey), 0u);  // kein Dokument
    EXPECT_EQ(s.objs.count(kLockKey), 0u); // und kein wertloses Lock-Objekt hinterlassen
    EXPECT_NE(cap.text().find("FEHLER"), std::string::npos) << cap.text();
}

// Der Owner traegt die diagnostische pid dieses Prozesses (der Lock vergleicht nur owner_uuid).
TEST(G3BuilderRegistration, MakeLockOwnerCarriesPidAndKeepsEmptyTokenEmpty) {
    auto const o = bl::make_lock_owner("uuid-x", "prod1");
    EXPECT_EQ(o.owner_uuid, "uuid-x");
    EXPECT_EQ(o.host, "prod1");
    EXPECT_EQ(o.pid, bl::current_pid());
    EXPECT_GT(o.pid, 0);
    EXPECT_TRUE(bl::make_lock_owner("", "prod1").owner_uuid.empty()); // erfindet kein Token
    // Die ttl-ZAHL gehoert bestandslog_lock.hpp (Ereignis-gebundene Obergrenze); diese Naht darf sie
    // nur ABLESEN. Geprueft wird deshalb die Identitaet, nicht die Zahl -- sonst waere hier eine
    // zweite Heimat, die bei jeder Owner-Praezisierung nachgezogen werden muesste.
    EXPECT_EQ(bl::default_lock_ttl_s(), bl::LockRecord{}.ttl_s);
    EXPECT_GT(bl::default_lock_ttl_s(), 0);
}
