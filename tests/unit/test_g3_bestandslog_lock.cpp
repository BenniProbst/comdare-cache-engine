// test_g3_bestandslog_lock -- G3 / #46b Lagerhaltung, Scheibe B2.
//
// Deckt die Koordinations-Schicht ab: Transport-Naht (Fake = In-Memory-Map), Owner-Token-Verify-Lock
// mit ttl-Bruch, skriptbare Race-Interleavings (verlorenes Race sichtbar), Record-UNION-Merge
// (Doppel-Halt harmlos, Konflikt monoton) und "Lock endet mit erster pro-forma". Ohne minio.
//
// Ertuechtigung 26.07. (Lager-Gate): die VOLL-WIPE-WACHE in store_document_merged (unlesbares oder
// grammatik-fremdes Remote wird nie ueberschrieben), die OWNER-PFLICHT in parse_lock (ein leerer
// owner_uuid klaut keinen und loescht keinen Lock), die AEQUIVALENZ des map-Merge gegen die alte
// Linear-Scan-Bauform (Referenz-Implementierung unten, Zufallsmengen) und der gelockte Schreib-
// Zyklus with_document_lock (Normal-, Frist-, Konflikt-, Fehlschlag- und Wurf-Fall).

#include "bestandslog/bestandslog_lock.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace bl = comdare::cache_engine::builder::bestandslog;

namespace {

// In-Memory-Objekt-Store mit einem EINMALIGEN after_store-Hook -> deterministische Interleavings
// (der Test injiziert einen konkurrierenden Schreiber genau zwischen store und Zweit-Verify).
struct FakeStore {
    std::map<std::string, std::string> objs;
    std::function<void()>              after_store_hook;

    bl::BestandTransport transport() {
        bl::BestandTransport t;
        t.fetch = [this](std::string const& k) -> std::optional<std::string> {
            auto it = objs.find(k);
            if (it == objs.end()) return std::nullopt;
            return it->second;
        };
        t.store = [this](std::string const& k, std::string const& c) -> bool {
            objs[k] = c;
            if (after_store_hook) {
                auto h           = after_store_hook;
                after_store_hook = {}; // Ein-Schuss: keine Rekursion
                h();
            }
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

constexpr char const* kLockKey = "bestandslog/binary_bestand.xml.lock";
constexpr char const* kDocKey  = "bestandslog/binary_bestand.xml";

bl::LockOwner owner_a() { return bl::LockOwner{"uuid-A", "prod1", 111}; }
bl::LockOwner owner_b() { return bl::LockOwner{"uuid-B", "prod2", 222}; }

// Liest den aktuellen Lock-Owner aus dem Store (oder "" wenn kein Lock).
std::string lock_owner_in(FakeStore& s) {
    auto it = s.objs.find(kLockKey);
    if (it == s.objs.end()) return "";
    auto r = bl::parse_lock(it->second);
    return r ? r->owner_uuid : std::string{"<unparsable>"};
}

// Kleine Bauhelfer (Member-Zuweisung statt Luecken-Designated-Init) -- die Merge-Tests interessieren
// sich nur fuer id/status bzw. key/bytes/done_utc.
bl::BatchReservierung mk_res(std::string id, bl::BatchStatus status, std::string maschine = "") {
    bl::BatchReservierung r;
    r.id       = std::move(id);
    r.maschine = std::move(maschine);
    r.status   = status;
    return r;
}
bl::BestandEintrag mk_eintrag(std::string key, std::uint64_t bytes, std::string done_utc) {
    bl::BestandEintrag e;
    e.key_sha512 = std::move(key);
    e.bytes      = bytes;
    e.done_utc   = std::move(done_utc);
    return e;
}

// Faengt std::cerr fuer die Dauer des Scopes ab -> die FEHLER-Zeilen der Wachen sind PRUEFBAR (Anzahl
// und Text) statt Test-Rauschen. Der urspruengliche Puffer wird im Destruktor zurueckgesetzt.
class CerrCapture {
public:
    CerrCapture() : alt_{std::cerr.rdbuf(puffer_.rdbuf())} {}
    CerrCapture(CerrCapture const&)            = delete;
    CerrCapture& operator=(CerrCapture const&) = delete;
    ~CerrCapture() { std::cerr.rdbuf(alt_); }

    [[nodiscard]] std::string text() const { return puffer_.str(); }
    [[nodiscard]] std::size_t zeilen() const {
        auto const        s = puffer_.str();
        std::size_t const n = static_cast<std::size_t>(std::count(s.begin(), s.end(), '\n'));
        return n;
    }

private:
    std::ostringstream puffer_;
    std::streambuf*    alt_;
};

// Skript-Uhr als NowFn: liefert die Zeitpunkte in fester Folge, der letzte Wert bleibt stehen ->
// Frist- und ttl-Faelle deterministisch ohne Wall-Clock. with_document_lock fragt bis zu dreimal
// (Erwerb, vor der Arbeit, nach der Arbeit).
bl::NowFn scripted_now(std::vector<std::int64_t> ts) {
    return [werte = std::move(ts), i = std::size_t{0}]() mutable -> std::int64_t {
        std::int64_t const v = werte[std::min(i, werte.size() - 1)];
        ++i;
        return v;
    };
}

// Referenz-Implementierung des Record-Union-Merge in der ALTEN Bauform: linearer std::find_if-Scan je
// Element. Sie ist die Messlatte fuer die map-Variante -- identische Identitaets-Definition
// (same_eintrag_identity), identische Konflikt-Aufloesung (detail::pick_*), identische Ausgabe-
// Ordnung; NUR der Lookup unterscheidet sich. Damit misst der Aequivalenz-Test genau die Aenderung.
bl::BestandslogDocument merge_documents_reference(bl::BestandslogDocument const& a, bl::BestandslogDocument const& b) {
    bl::BestandslogDocument out;
    out.genus             = a.genus;
    out.syntax_version    = std::max(a.syntax_version, b.syntax_version);
    out.semantics_version = std::max(a.semantics_version, b.semantics_version);
    out.created_utc       = a.created_utc;
    out.doc_revision      = std::max(a.doc_revision, b.doc_revision) + 1;

    out.bestand = a.bestand;
    for (auto const& be : b.bestand) {
        auto it = std::find_if(out.bestand.begin(), out.bestand.end(),
                               [&](bl::BestandEintrag const& x) { return bl::same_eintrag_identity(x, be); });
        if (it == out.bestand.end())
            out.bestand.push_back(be);
        else
            *it = bl::detail::pick_eintrag(*it, be);
    }
    std::sort(out.bestand.begin(), out.bestand.end(), bl::eintrag_identity_less);

    out.reservierungen = a.reservierungen;
    for (auto const& br : b.reservierungen) {
        auto it = std::find_if(out.reservierungen.begin(), out.reservierungen.end(),
                               [&](bl::BatchReservierung const& x) { return x.id == br.id; });
        if (it == out.reservierungen.end())
            out.reservierungen.push_back(br);
        else
            *it = bl::detail::pick_reservierung(*it, br);
    }
    std::sort(out.reservierungen.begin(), out.reservierungen.end(),
              [](bl::BatchReservierung const& x, bl::BatchReservierung const& y) { return x.id < y.id; });
    return out;
}

// Ruft store_document_merged mit abgefangenem cerr -> Ergebnis UND die Wach-Zeilen sind pruefbar.
struct StoreProbe {
    std::optional<bl::BestandslogDocument> written;
    std::string                            cerr_text;
    std::size_t                            cerr_zeilen = 0;
};

StoreProbe store_probe(bl::BestandTransport const& t, std::string const& doc_key,
                       bl::BestandslogDocument const& local) {
    StoreProbe        p;
    CerrCapture const cap;
    p.written     = bl::store_document_merged(t, doc_key, local);
    p.cerr_text   = cap.text();
    p.cerr_zeilen = cap.zeilen();
    return p;
}

// Dasselbe fuer den gelockten Zyklus.
struct LockProbe {
    bl::LockOutcome outcome = bl::LockOutcome::lock_unavailable;
    std::string     cerr_text;
    std::size_t     cerr_zeilen = 0;
};

template <class Fn>
LockProbe lock_probe(bl::BestandTransport const& t, std::string const& doc_key, bl::LockOwner const& me, int ttl_s,
                     bl::NowFn const& now_fn, Fn&& fn, int section_budget_s = bl::kSectionBudgetSeconds) {
    LockProbe         p;
    CerrCapture const cap;
    p.outcome     = bl::with_document_lock(t, doc_key, me, ttl_s, now_fn, std::forward<Fn>(fn), section_budget_s);
    p.cerr_text   = cap.text();
    p.cerr_zeilen = cap.zeilen();
    return p;
}

// Zufalls-Dokument aus KLEINEN Wert-Pools: die Pools erzwingen Kollisionen (gleiche Identitaet in a
// und b, Doppel-Identitaeten INNERHALB eines Dokuments) -- genau die Faelle, in denen sich Lookup-
// Bauformen unterscheiden koennten. Fester Seed -> reproduzierbar.
bl::BestandslogDocument random_document(std::mt19937& rng, std::size_t n_bestand, std::size_t n_res) {
    static constexpr char const* kSimd[] = {"", "sse42", "avx2", "avx512"};
    static constexpr char const* kOpt[]  = {"O2", "O3"};
    static constexpr char const* kZeit[] = {"2026-07-23T10:00:00Z", "2026-07-24T11:00:00Z", "2026-07-25T12:00:00Z"};
    static constexpr int         kKeys   = 6; // kleiner Schluessel-Raum -> viele Treffer

    auto pick = [&rng](int n) { return static_cast<std::size_t>(std::uniform_int_distribution<int>{0, n - 1}(rng)); };

    bl::BestandslogDocument d;
    d.doc_revision = std::uniform_int_distribution<std::uint64_t>{0, 9}(rng);
    for (std::size_t i = 0; i < n_bestand; ++i) {
        bl::BestandEintrag e;
        e.key_sha512 = std::string(128, static_cast<char>('a' + pick(kKeys))); // 128 hex-Zeichen wie im Ernstfall
        e.zelle.opt  = kOpt[pick(2)];
        e.zelle.simd = kSimd[pick(4)];
        e.pfad       = "pfad/" + std::to_string(pick(4));
        e.bytes      = std::uniform_int_distribution<std::uint64_t>{1, 1000}(rng);
        e.stempel    = "[a,b,c]";
        e.done_utc   = kZeit[pick(3)];
        d.bestand.push_back(std::move(e));
    }
    static constexpr bl::BatchStatus kStatus[] = {bl::BatchStatus::offen, bl::BatchStatus::released,
                                                  bl::BatchStatus::done};
    for (std::size_t i = 0; i < n_res; ++i) {
        bl::BatchReservierung r;
        r.id     = "uuid-" + std::to_string(pick(3)) + "/" + std::to_string(pick(8));
        r.status = kStatus[pick(3)];
        r.eta_s  = pick(2) == 0 ? "" : "1800.000"; // leere eta_s ist der Gleichstands-Tiebreak
        // CEB-Bindung mal gesetzt, mal nicht -> die Aequivalenz-Wache deckt auch v2-/v3-Mischungen ab.
        if (pick(2) == 0) {
            r.ceb_legende    = "[a,b,c]";
            r.ceb_key_sha512 = std::string(128, static_cast<char>('a' + pick(kKeys)));
        }
        d.reservierungen.push_back(std::move(r));
    }
    return d;
}

} // namespace

// ---------------------------------------------------------------------------
// Lock-Serialisierung + Staleness.
// ---------------------------------------------------------------------------
TEST(G3BestandslogLock, SerializeParseRoundtrip) {
    bl::LockRecord r{"uuid-A", "prod1", 111, 1000, 30};
    auto           back = bl::parse_lock(bl::serialize_lock(r));
    ASSERT_TRUE(back.has_value());
    EXPECT_EQ(*back, r);
    EXPECT_FALSE(bl::parse_lock("garbage without owner").has_value());
}

TEST(G3BestandslogLock, Staleness) {
    bl::LockRecord r{"uuid-A", "prod1", 111, 1000, 30};
    EXPECT_FALSE(bl::lock_is_stale(r, 1000));
    EXPECT_FALSE(bl::lock_is_stale(r, 1030)); // exakt ttl -> noch frisch
    EXPECT_TRUE(bl::lock_is_stale(r, 1031));  // > ttl -> stale
}

// ---------------------------------------------------------------------------
// Lock-Erwerb: leer / fremd-frisch / stale-Bruch / release nur eigenen.
// ---------------------------------------------------------------------------
TEST(G3BestandslogLock, AcquireOnEmpty) {
    FakeStore s;
    auto      t = s.transport();
    EXPECT_TRUE(bl::try_acquire_lock(t, kLockKey, owner_a(), 30, 1000));
    EXPECT_EQ(lock_owner_in(s), "uuid-A");
}

TEST(G3BestandslogLock, SecondOwnerBlockedWhileFresh) {
    FakeStore s;
    auto      t = s.transport();
    ASSERT_TRUE(bl::try_acquire_lock(t, kLockKey, owner_a(), 30, 1000));
    // B versucht 5s spaeter -> A frisch -> B bekommt nicht.
    EXPECT_FALSE(bl::try_acquire_lock(t, kLockKey, owner_b(), 30, 1005));
    EXPECT_EQ(lock_owner_in(s), "uuid-A");
}

TEST(G3BestandslogLock, StaleLockBroken) {
    FakeStore s;
    auto      t = s.transport();
    ASSERT_TRUE(bl::try_acquire_lock(t, kLockKey, owner_a(), 30, 1000));
    // B kommt 200s spaeter -> A stale (>30) -> B bricht und uebernimmt.
    EXPECT_TRUE(bl::try_acquire_lock(t, kLockKey, owner_b(), 30, 1200));
    EXPECT_EQ(lock_owner_in(s), "uuid-B");
}

TEST(G3BestandslogLock, ReleaseOnlyOwn) {
    FakeStore s;
    auto      t = s.transport();
    ASSERT_TRUE(bl::try_acquire_lock(t, kLockKey, owner_a(), 30, 1000));
    // B versucht A's Lock zu released -> bleibt A's.
    bl::release_lock(t, kLockKey, owner_b());
    EXPECT_EQ(lock_owner_in(s), "uuid-A");
    // A released -> Lock weg.
    bl::release_lock(t, kLockKey, owner_a());
    EXPECT_EQ(lock_owner_in(s), "");
}

// Skriptbares Interleaving: B schreibt seinen Token GENAU zwischen A's store und A's Zweit-Verify
// -> A erkennt das verlorene Race (Zweit-Verify sieht B) und bekommt den Lock NICHT. Der Rest-Race
// ist damit sichtbar UND unschaedlich (der Union-Merge traegt die Korrektheit).
TEST(G3BestandslogLock, ZweitVerifyDetectsLostRace) {
    FakeStore s;
    auto      t        = s.transport();
    s.after_store_hook = [&]() {
        // B draengt sich dazwischen und ueberschreibt das Lock-Objekt mit SEINEM Token.
        bl::LockRecord b_rec{"uuid-B", "prod2", 222, 1000, 30};
        s.objs[kLockKey] = bl::serialize_lock(b_rec);
    };
    EXPECT_FALSE(bl::try_acquire_lock(t, kLockKey, owner_a(), 30, 1000));
    EXPECT_EQ(lock_owner_in(s), "uuid-B"); // B hat gewonnen; A hat es erkannt
}

// ---------------------------------------------------------------------------
// Record-Union-Merge: Doppel-Halt harmlos (beide Reservierungen ueberleben), Konflikt monoton.
// ---------------------------------------------------------------------------
TEST(G3BestandslogLock, UnionMergeKeepsBothReservations) {
    bl::BestandslogDocument a;
    a.doc_revision = 3;
    a.reservierungen.push_back(mk_res("uuid-A/0", bl::BatchStatus::offen, "prod1"));
    bl::BestandslogDocument b;
    b.doc_revision = 5;
    b.reservierungen.push_back(mk_res("uuid-B/0", bl::BatchStatus::offen, "prod2"));

    auto m = bl::merge_documents(a, b);
    ASSERT_EQ(m.reservierungen.size(), 2u);
    EXPECT_EQ(m.reservierungen[0].id, "uuid-A/0"); // sortiert
    EXPECT_EQ(m.reservierungen[1].id, "uuid-B/0");
    EXPECT_EQ(m.doc_revision, 6u); // max(3,5)+1
}

TEST(G3BestandslogLock, MergeConflictIsMonotone) {
    // Gleiche id, aber a=offen, b=done -> done gewinnt (Fortschritt geht nie verloren).
    bl::BestandslogDocument a;
    a.reservierungen.push_back(mk_res("uuid-A/0", bl::BatchStatus::offen));
    bl::BestandslogDocument b;
    b.reservierungen.push_back(mk_res("uuid-A/0", bl::BatchStatus::done));

    auto m1 = bl::merge_documents(a, b);
    ASSERT_EQ(m1.reservierungen.size(), 1u);
    EXPECT_EQ(m1.reservierungen[0].status, bl::BatchStatus::done);
    // Kommutativ in der Fortschritts-Wahl (Reihenfolge egal fuer den Status).
    auto m2 = bl::merge_documents(b, a);
    ASSERT_EQ(m2.reservierungen.size(), 1u);
    EXPECT_EQ(m2.reservierungen[0].status, bl::BatchStatus::done);
}

TEST(G3BestandslogLock, MergeBestandLaterDoneUtcWins) {
    bl::BestandslogDocument a;
    a.bestand.push_back(mk_eintrag("k", 10, "2026-07-23T10:00:00Z"));
    bl::BestandslogDocument b;
    b.bestand.push_back(mk_eintrag("k", 20, "2026-07-23T11:00:00Z"));
    auto m = bl::merge_documents(a, b);
    ASSERT_EQ(m.bestand.size(), 1u);
    EXPECT_EQ(m.bestand[0].bytes, 20u); // spaeteres done_utc
}

// ---------------------------------------------------------------------------
// §62-NACHTRAG-4: der Bestands-Union laeuft ueber das TUPEL (key_sha512 + Zell-Koordinaten). Gleicher
// Fingerprint unter verschiedener ISA darf NICHT verschmelzen -- sonst verlore der zweite Bau seinen
// Pfad. Zusaetzlich: die Ausgabe ist nach dem Tupel sortiert, also eingabe-reihenfolge-unabhaengig.
// ---------------------------------------------------------------------------
TEST(G3BestandslogLock, MergeBestandKeepsCellsApart) {
    auto with_cell = [](std::string key, std::string simd, std::uint64_t bytes) {
        auto e       = mk_eintrag(std::move(key), bytes, "2026-07-23T10:00:00Z");
        e.zelle.opt  = "O2";
        e.zelle.simd = std::move(simd);
        return e;
    };

    bl::BestandslogDocument a;
    a.bestand.push_back(with_cell("k", "avx2", 10));
    bl::BestandslogDocument b;
    b.bestand.push_back(with_cell("k", "avx512", 20));

    auto const m = bl::merge_documents(a, b);
    ASSERT_EQ(m.bestand.size(), 2u); // ZWEI Eintraege trotz identischem key_sha512
    EXPECT_EQ(m.bestand[0].key_sha512, m.bestand[1].key_sha512);
    EXPECT_EQ(m.bestand[0].zelle.simd, "avx2"); // sortiert nach dem Tupel
    EXPECT_EQ(m.bestand[1].zelle.simd, "avx512");

    // Reihenfolge-unabhaengig: der umgekehrte Merge liefert exakt dieselbe Liste.
    auto const m_rev = bl::merge_documents(b, a);
    EXPECT_EQ(m_rev.bestand, m.bestand);

    // Und bei GLEICHER Zelle greift die alte Regel weiter (spaeteres done_utc gewinnt).
    bl::BestandslogDocument c;
    c.bestand.push_back(with_cell("k", "avx2", 99));
    c.bestand[0].done_utc = "2026-07-23T12:00:00Z";
    auto const m_same     = bl::merge_documents(a, c);
    ASSERT_EQ(m_same.bestand.size(), 1u);
    EXPECT_EQ(m_same.bestand[0].bytes, 99u);
}

// ---------------------------------------------------------------------------
// store_document_merged: fetch->merge->store (nie blinde Ersetzung). Doppel-Halt ueber den Store.
// ---------------------------------------------------------------------------
TEST(G3BestandslogLock, StoreDocumentMergedUnionsWithRemote) {
    FakeStore s;
    auto      t = s.transport();
    // Remote hat schon B's Reservierung.
    bl::BestandslogDocument remote;
    remote.doc_revision = 4;
    remote.reservierungen.push_back(mk_res("uuid-B/0", bl::BatchStatus::offen));
    s.objs[kDocKey] = bl::emit_document(remote);

    // Lokal will A seine Reservierung eintragen (blind waere ein Verlust von B).
    bl::BestandslogDocument local;
    local.doc_revision = 4;
    local.reservierungen.push_back(mk_res("uuid-A/0", bl::BatchStatus::offen));

    auto written = bl::store_document_merged(t, kDocKey, local);
    ASSERT_TRUE(written.has_value());
    EXPECT_EQ(written->reservierungen.size(), 2u); // B NICHT verloren
    EXPECT_EQ(written->doc_revision, 5u);          // max(4,4)+1

    // Und der Store traegt jetzt das gemergte Dokument.
    auto reparsed = bl::parse_bestandslog(s.objs[kDocKey]);
    ASSERT_TRUE(reparsed.has_value());
    EXPECT_EQ(reparsed->reservierungen.size(), 2u);
}

TEST(G3BestandslogLock, StoreDocumentMergedOnEmptyBumpsRevision) {
    FakeStore               s;
    auto                    t = s.transport();
    bl::BestandslogDocument local;
    local.doc_revision = 5;
    auto written       = bl::store_document_merged(t, kDocKey, local);
    ASSERT_TRUE(written.has_value());
    EXPECT_EQ(written->doc_revision, 6u);
}

// "Lock endet mit erster pro-forma": Lock nehmen -> erste pro-forma-Reservierung schreiben ->
// Lock SOFORT wieder freigeben. Danach: Lock-Objekt weg, Reservierung persistiert (B11).
TEST(G3BestandslogLock, LockEndsWithFirstProForma) {
    FakeStore s;
    auto      t = s.transport();

    ASSERT_TRUE(bl::try_acquire_lock(t, kLockKey, owner_a(), 30, 1000));

    bl::BestandslogDocument local;
    bl::BatchReservierung   pf = mk_res("uuid-A/0", bl::BatchStatus::offen, "prod1");
    pf.typ                     = bl::BatchTyp::tier;
    pf.pro_forma_bis_utc       = "2026-07-23T12:30:00Z"; // pro-forma-Frist; eta_s bleibt leer (noch keine ETA)
    local.reservierungen.push_back(pf);
    ASSERT_TRUE(bl::store_document_merged(t, kDocKey, local).has_value());

    bl::release_lock(t, kLockKey, owner_a());

    EXPECT_EQ(lock_owner_in(s), ""); // Lock ist weg -> war nur kurze Schreib-Exklusivitaet
    auto doc = bl::parse_bestandslog(s.objs[kDocKey]);
    ASSERT_TRUE(doc.has_value());
    ASSERT_EQ(doc->reservierungen.size(), 1u);
    EXPECT_EQ(doc->reservierungen[0].pro_forma_bis_utc, "2026-07-23T12:30:00Z");
}

// ---------------------------------------------------------------------------
// OWNER-PFLICHT (N7-D5). Vorher genuegte die ANWESENHEIT des owner-Keys: ein leerer Wert kam durch,
// und dann verglich try_acquire_lock "" != "" (falsch -> fremder frischer Lock ueberschrieben) bzw.
// release_lock "" == "" (wahr -> fremder Lock geloescht). Beide Richtungen sind hier gewacht.
// ---------------------------------------------------------------------------
TEST(G3BestandslogLock, ParseLockRejectsEmptyOwner) {
    EXPECT_FALSE(bl::parse_lock("owner=;host=prod1;pid=111;ts=1000;ttl=90").has_value()); // Wert leer
    EXPECT_FALSE(bl::parse_lock("host=prod1;pid=111;ts=1000;ttl=90").has_value());        // Key fehlt
    EXPECT_TRUE(bl::parse_lock("owner=uuid-A;host=prod1;pid=111;ts=1000;ttl=90").has_value());
}

TEST(G3BestandslogLock, EmptyOwnerStealsNoLock) {
    FakeStore s;
    auto      t = s.transport();
    ASSERT_TRUE(bl::try_acquire_lock(t, kLockKey, owner_a(), 30, 1000));

    bl::LockOwner const anonym{"", "prod2", 222};
    EXPECT_FALSE(bl::try_acquire_lock(t, kLockKey, anonym, 30, 1005)); // fremd + frisch
    EXPECT_EQ(lock_owner_in(s), "uuid-A");

    // Fremd und STALE: auch dann nicht -- wer keinen Token hat, bricht keinen Lock, den er nicht
    // halten koennte, und hinterlaesst auch kein wertloses Lock-Objekt.
    auto const vorher = s.objs[kLockKey];
    EXPECT_FALSE(bl::try_acquire_lock(t, kLockKey, anonym, 30, 1200));
    EXPECT_EQ(s.objs[kLockKey], vorher);
}

TEST(G3BestandslogLock, EmptyOwnerReleasesNoLock) {
    FakeStore s;
    auto      t = s.transport();
    ASSERT_TRUE(bl::try_acquire_lock(t, kLockKey, owner_a(), 30, 1000));

    bl::release_lock(t, kLockKey, bl::LockOwner{"", "prod2", 222});
    EXPECT_EQ(lock_owner_in(s), "uuid-A"); // fremder Lock ueberlebt die anonyme Freigabe

    // Und ein von einem kaputten Peer geschriebenes Lock mit LEEREM owner ist fuer niemanden "eigen".
    s.objs[kLockKey] = "owner=;host=prod2;pid=222;ts=1000;ttl=90";
    bl::release_lock(t, kLockKey, bl::LockOwner{"", "prod2", 222});
    EXPECT_TRUE(s.objs.contains(kLockKey));
}

TEST(G3BestandslogLock, TtlIstDie30MinutenObergrenze) {
    // OWNER-GESETZ, nicht Ingenieurs-Wahl (LED:3225, Praezisierung-3 vom 22.07.): "Das lock fuer
    // Schreiben eines Bestandslogs, endet mit der ersten pro forma 30 Minuten Reservierung
    // spaetestens." Die ttl ist die OBERGRENZE fuer den Stale-Bruch (30 min = 1800 s), nicht die
    // Dauer eines Zyklus -- den begrenzt das Sektions-Budget. Diese Zahl ist gepinnt, damit sie
    // nicht wieder zu einem Agenten-Schaetzwert wird.
    EXPECT_EQ(bl::kLockTtlSeconds, 1800);
    EXPECT_EQ(bl::LockRecord{}.ttl_s, 1800);
    EXPECT_EQ(bl::kSectionBudgetSeconds, 30); // Kurz-Zyklus-Wache ("nur millisekunden" + Spielraum)
}

// ---------------------------------------------------------------------------
// VOLL-WIPE-WACHE: ein vorhandenes, nicht leeres Remote-Dokument wird NIE ueberschrieben, wenn es
// nicht parsbar oder grammatik-fremd ist. Der Objekt-Store schreibt ohne Versionierung und ohne
// Backup, und im Reservierungs-Pfad ist der lokale Stand ein Dokument mit EINER Reservierung und
// LEEREM bestand -- ein solcher Schreibvorgang loeschte den Lagerbestand eines mehrtaegigen Laufs.
// ---------------------------------------------------------------------------
TEST(G3BestandslogLock, WipeGuardKeepsUnparsableRemote) {
    FakeStore s;
    auto      t       = s.transport();
    s.objs[kDocKey]   = "das ist kein Bestandslog";
    auto const vorher = s.objs[kDocKey];

    bl::BestandslogDocument local; // genau der Reservierungs-Pfad: eine Zeile, leerer bestand
    local.reservierungen.push_back(mk_res("uuid-A/0", bl::BatchStatus::offen, "prod1"));

    auto const p = store_probe(t, kDocKey, local);
    EXPECT_FALSE(p.written.has_value());
    EXPECT_EQ(s.objs[kDocKey], vorher); // Byte-identisch stehen gelassen
    EXPECT_EQ(p.cerr_zeilen, 1u);
    EXPECT_NE(p.cerr_text.find("[bestandslog] FEHLER"), std::string::npos) << p.cerr_text;
}

TEST(G3BestandslogLock, WipeGuardKeepsRemoteWithUnknownEnumValue) {
    // Der reale Fall (B7): wohlgeformtes XML, aber ein Enum-Wert, den dieser Leser nicht kennt ->
    // parse_bestandslog lehnt das GANZE Dokument ab. Ein neuerer Schreiber darf den Bestand nicht
    // verlieren, nur weil ein aelterer Leser seine Grammatik nicht versteht.
    FakeStore s;
    auto      t = s.transport();

    bl::BestandslogDocument remote;
    remote.bestand.push_back(mk_eintrag(std::string(128, 'a'), 4096, "2026-07-25T10:00:00Z"));
    remote.reservierungen.push_back(mk_res("uuid-B/0", bl::BatchStatus::offen, "prod2"));
    std::string roh = bl::emit_document(remote);
    auto const  pos = roh.find("status=\"offen\"");
    ASSERT_NE(pos, std::string::npos);
    roh.replace(pos, std::string_view{"status=\"offen\""}.size(), "status=\"zukunft\"");
    s.objs[kDocKey]   = roh;
    auto const vorher = s.objs[kDocKey];

    bl::BestandslogDocument local;
    local.reservierungen.push_back(mk_res("uuid-A/0", bl::BatchStatus::offen, "prod1"));

    auto const p = store_probe(t, kDocKey, local);
    EXPECT_FALSE(p.written.has_value());
    EXPECT_EQ(s.objs[kDocKey], vorher);
    EXPECT_EQ(p.cerr_zeilen, 1u);
    EXPECT_NE(p.cerr_text.find("[bestandslog] FEHLER"), std::string::npos) << p.cerr_text;
}

TEST(G3BestandslogLock, SyntaxGuardRefusesHigherGrammar) {
    // document_syntax_supported wird jetzt VOR dem Merge gerufen (vorher: 0 Produktions-Aufrufer,
    // waehrend der Kommentar den Schutz behauptete). Hoehere Wire-Grammatik -> nicht anfassen.
    FakeStore s;
    auto      t = s.transport();

    bl::BestandslogDocument zukunft;
    zukunft.syntax_version = bl::kSyntaxVersion + 1;
    zukunft.bestand.push_back(mk_eintrag(std::string(128, 'b'), 8192, "2026-07-25T10:00:00Z"));
    s.objs[kDocKey]   = bl::emit_document(zukunft);
    auto const vorher = s.objs[kDocKey];

    bl::BestandslogDocument local;
    local.reservierungen.push_back(mk_res("uuid-A/0", bl::BatchStatus::offen, "prod1"));

    auto const p = store_probe(t, kDocKey, local);
    EXPECT_FALSE(p.written.has_value());
    EXPECT_EQ(s.objs[kDocKey], vorher);
    EXPECT_EQ(p.cerr_zeilen, 1u);
    EXPECT_NE(p.cerr_text.find("syntax_version"), std::string::npos) << p.cerr_text;

    // Gegenprobe: eine AELTERE Grammatik bleibt vertraeglich und wird normal gemergt.
    bl::BestandslogDocument alt;
    alt.syntax_version = 1;
    alt.bestand.push_back(mk_eintrag(std::string(128, 'b'), 8192, "2026-07-25T10:00:00Z"));
    s.objs[kDocKey] = bl::emit_document(alt);
    auto const q    = store_probe(t, kDocKey, local);
    ASSERT_TRUE(q.written.has_value());
    EXPECT_EQ(q.cerr_zeilen, 0u);
    EXPECT_EQ(q.written->bestand.size(), 1u);        // Bestand des Remote uebernommen
    EXPECT_EQ(q.written->reservierungen.size(), 1u); // lokale Reservierung dazu
}

TEST(G3BestandslogLock, BlankRemoteIsFirstWrite) {
    // Leeres/fehlendes Remote bleibt die Erst-Anlage (kein Bestand kann verloren gehen), und
    // doc_revision steigt auch in diesem Zweig.
    FakeStore s;
    auto      t = s.transport();

    bl::BestandslogDocument local;
    local.doc_revision = 7;
    local.reservierungen.push_back(mk_res("uuid-A/0", bl::BatchStatus::offen, "prod1"));

    s.objs[kDocKey] = "";
    auto const leer = store_probe(t, kDocKey, local);
    ASSERT_TRUE(leer.written.has_value());
    EXPECT_EQ(leer.written->doc_revision, 8u);
    EXPECT_EQ(leer.cerr_zeilen, 0u);

    s.objs[kDocKey]       = "\n  \n";
    auto const whitespace = store_probe(t, kDocKey, local);
    ASSERT_TRUE(whitespace.written.has_value());
    EXPECT_EQ(whitespace.written->doc_revision, 8u);
    EXPECT_EQ(whitespace.cerr_zeilen, 0u);

    s.objs.erase(kDocKey);
    auto const fehlt = store_probe(t, kDocKey, local);
    ASSERT_TRUE(fehlt.written.has_value());
    EXPECT_EQ(fehlt.written->doc_revision, 8u);
    EXPECT_EQ(fehlt.cerr_zeilen, 0u);
}

// ---------------------------------------------------------------------------
// MERGE UEBER std::map: der Lookup wechselt von O(|a|*|b|) Linear-Scan auf O((n+m) log n), die
// SEMANTIK nicht. Messlatte ist merge_documents_reference (die alte find_if-Bauform) auf
// Zufallsmengen mit erzwungenen Kollisionen.
// ---------------------------------------------------------------------------
TEST(G3BestandslogLock, MergeMapMatchesLinearReference) {
    std::mt19937 rng{20260726u};
    for (int runde = 0; runde < 30; ++runde) {
        auto const a = random_document(rng, 120, 40);
        auto const b = random_document(rng, 150, 50);

        auto const neu = bl::merge_documents(a, b);
        ASSERT_EQ(neu, merge_documents_reference(a, b)) << "Runde " << runde;
        ASSERT_EQ(bl::merge_documents(b, a), merge_documents_reference(b, a)) << "Runde " << runde << " rueckwaerts";

        // Ordnung stabil: die Ausgabe ist nach dem Identitaets-Tupel bzw. der id sortiert -> der Emit
        // bleibt byte-stabil, unabhaengig von der Eingabe-Reihenfolge.
        EXPECT_TRUE(std::is_sorted(neu.bestand.begin(), neu.bestand.end(), bl::eintrag_identity_less));
        EXPECT_TRUE(
            std::is_sorted(neu.reservierungen.begin(), neu.reservierungen.end(),
                           [](bl::BatchReservierung const& x, bl::BatchReservierung const& y) { return x.id < y.id; }));
    }
}

TEST(G3BestandslogLock, MergeMapHandlesDuplicateIdentitiesLikeReference) {
    // (1) Doppel-Identitaet INNERHALB von a (der Parser schliesst sie nicht aus): der Merge faellt
    // nichts still zusammen, sondern trifft -- wie der alte find_if -- das ERSTE Vorkommen.
    bl::BestandslogDocument a;
    a.bestand.push_back(mk_eintrag(std::string(128, 'a'), 10, "2026-07-23T10:00:00Z"));
    a.bestand.push_back(mk_eintrag(std::string(128, 'a'), 11, "2026-07-23T09:00:00Z"));
    bl::BestandslogDocument b;
    b.bestand.push_back(mk_eintrag(std::string(128, 'a'), 99, "2026-07-25T10:00:00Z"));

    auto const neu = bl::merge_documents(a, b);
    EXPECT_EQ(neu, merge_documents_reference(a, b));
    ASSERT_EQ(neu.bestand.size(), 2u);
    std::vector<std::uint64_t> bytes{neu.bestand[0].bytes, neu.bestand[1].bytes};
    std::sort(bytes.begin(), bytes.end());
    EXPECT_EQ(bytes, (std::vector<std::uint64_t>{11u, 99u})); // spaeteres done_utc gewann im ersten Slot

    // (2) Doppel-Identitaet INNERHALB von b, in a unbekannt: der zweite b-Eintrag mergt in den
    // frisch angehaengten hinein statt ein Duplikat anzulegen.
    bl::BestandslogDocument leer;
    bl::BestandslogDocument doppelt;
    doppelt.bestand.push_back(mk_eintrag(std::string(128, 'c'), 1, "2026-07-23T10:00:00Z"));
    doppelt.bestand.push_back(mk_eintrag(std::string(128, 'c'), 2, "2026-07-24T10:00:00Z"));
    auto const zwei = bl::merge_documents(leer, doppelt);
    EXPECT_EQ(zwei, merge_documents_reference(leer, doppelt));
    ASSERT_EQ(zwei.bestand.size(), 1u);
    EXPECT_EQ(zwei.bestand[0].bytes, 2u);

    // (3) Dasselbe fuer die Reservierungen (Union per id).
    bl::BestandslogDocument res_a;
    res_a.reservierungen.push_back(mk_res("uuid-A/0", bl::BatchStatus::offen, "prod1"));
    res_a.reservierungen.push_back(mk_res("uuid-A/0", bl::BatchStatus::offen, "prod1"));
    bl::BestandslogDocument res_b;
    res_b.reservierungen.push_back(mk_res("uuid-A/0", bl::BatchStatus::done, "prod1"));
    auto const res = bl::merge_documents(res_a, res_b);
    EXPECT_EQ(res, merge_documents_reference(res_a, res_b));
    ASSERT_EQ(res.reservierungen.size(), 2u);
}

// ---------------------------------------------------------------------------
// syntax_version 3: die CEB-Bindung (ceb_legende / ceb_key_sha512) reist durch den Merge, ohne dass
// der Merge sie kennt -- die Konflikt-Aufloesung waehlt ganze RECORDS, nicht Felder.
// ---------------------------------------------------------------------------
TEST(G3BestandslogLock, MergeCarriesCebBindingThrough) {
    auto mit_bindung = [](std::string id, bl::BatchStatus status) {
        auto r           = mk_res(std::move(id), status, "prod1");
        r.typ            = bl::BatchTyp::planer_block;
        r.ceb_legende    = "[a,b,c]";
        r.ceb_key_sha512 = std::string(128, 'e');
        return r;
    };

    // Fremde ids: beide Records ueberleben, die Bindung unangetastet.
    bl::BestandslogDocument a;
    a.reservierungen.push_back(mit_bindung("uuid-A/plan/0", bl::BatchStatus::offen));
    bl::BestandslogDocument b;
    b.reservierungen.push_back(mk_res("uuid-B/0", bl::BatchStatus::offen, "prod2")); // v2-Form, ohne Bindung

    auto const m = bl::merge_documents(a, b);
    ASSERT_EQ(m.reservierungen.size(), 2u);
    EXPECT_EQ(m.reservierungen[0].ceb_legende, "[a,b,c]");
    EXPECT_EQ(m.reservierungen[0].ceb_key_sha512, std::string(128, 'e'));
    EXPECT_TRUE(m.reservierungen[1].ceb_legende.empty()); // v2-Record bleibt v2-Record
    EXPECT_EQ(m, merge_documents_reference(a, b));

    // GLEICHE id, Fortschritt gewinnt: der GANZE Record des Gewinners zaehlt. Traegt der Gewinner die
    // Bindung, reist sie mit; traegt er sie nicht, ist sie fort. Das ist die bestehende Record-Regel,
    // hier bewusst festgenagelt -- ein Feld-weiser Merge waere eine andere Semantik und ist nicht gebaut.
    bl::BestandslogDocument offen_mit;
    offen_mit.reservierungen.push_back(mit_bindung("uuid-A/plan/0", bl::BatchStatus::offen));
    bl::BestandslogDocument done_ohne;
    done_ohne.reservierungen.push_back(mk_res("uuid-A/plan/0", bl::BatchStatus::done, "prod1"));

    auto const gewinner_ohne = bl::merge_documents(offen_mit, done_ohne);
    ASSERT_EQ(gewinner_ohne.reservierungen.size(), 1u);
    EXPECT_EQ(gewinner_ohne.reservierungen[0].status, bl::BatchStatus::done);
    EXPECT_TRUE(gewinner_ohne.reservierungen[0].ceb_legende.empty());
    EXPECT_EQ(gewinner_ohne, merge_documents_reference(offen_mit, done_ohne));

    // Umgekehrt: traegt der done-Record die Bindung, ueberlebt sie den Merge in beiden Richtungen.
    bl::BestandslogDocument done_mit;
    done_mit.reservierungen.push_back(mit_bindung("uuid-A/plan/0", bl::BatchStatus::done));
    bl::BestandslogDocument offen_ohne;
    offen_ohne.reservierungen.push_back(mk_res("uuid-A/plan/0", bl::BatchStatus::offen, "prod1"));
    for (auto const& m2 : {bl::merge_documents(offen_ohne, done_mit), bl::merge_documents(done_mit, offen_ohne)}) {
        ASSERT_EQ(m2.reservierungen.size(), 1u);
        EXPECT_EQ(m2.reservierungen[0].status, bl::BatchStatus::done);
        EXPECT_EQ(m2.reservierungen[0].ceb_legende, "[a,b,c]");
    }
}

TEST(G3BestandslogLock, StoreDocumentMergedAcceptsV2RemoteAndKeepsCebBinding) {
    // Der Weg, den der Planer nimmt: das Remote ist noch ein v2-Dokument (kein ceb_*), lokal steht ein
    // planer_block MIT Bindung. Der Syntax-Guard laesst v2 durch (nach unten vertraeglich), der Merge
    // vereinigt, und das geschriebene Dokument traegt die Bindung samt v3-Stempel.
    FakeStore s;
    auto      t = s.transport();

    bl::BestandslogDocument remote;
    remote.syntax_version = 2;
    remote.reservierungen.push_back(mk_res("uuid-B/0", bl::BatchStatus::offen, "prod2"));
    s.objs[kDocKey] = bl::emit_document(remote);

    bl::BestandslogDocument local;
    auto                    block = mk_res("uuid-A/plan/0", bl::BatchStatus::offen, "prod1");
    block.typ                     = bl::BatchTyp::planer_block;
    block.ceb_legende             = "[a,b,c]";
    block.ceb_key_sha512          = std::string(128, 'e');
    local.reservierungen.push_back(block);

    auto const p = store_probe(t, kDocKey, local);
    ASSERT_TRUE(p.written.has_value());
    EXPECT_EQ(p.cerr_zeilen, 0u);
    EXPECT_EQ(p.written->syntax_version, bl::kSyntaxVersion); // max(2, 3) -> 3
    ASSERT_EQ(p.written->reservierungen.size(), 2u);

    auto const doc = bl::parse_bestandslog(s.objs[kDocKey]);
    ASSERT_TRUE(doc.has_value());
    ASSERT_EQ(doc->reservierungen.size(), 2u);
    EXPECT_EQ(doc->reservierungen[0].ceb_legende, "[a,b,c]"); // sortiert: uuid-A/plan/0 zuerst
    EXPECT_EQ(doc->reservierungen[0].ceb_key_sha512, std::string(128, 'e'));
}

// ---------------------------------------------------------------------------
// with_document_lock: Acquire -> Frist-Wache -> Arbeit -> Release, EIN Versuch je Zyklus.
// ---------------------------------------------------------------------------
TEST(G3BestandslogLock, LockKeyForIsTheDocumentSidecar) {
    EXPECT_EQ(bl::lock_key_for(kDocKey), kLockKey); // EINE Ableitung fuer Schreiber und Brecher
}

TEST(G3BestandslogLock, WithDocumentLockNormalCase) {
    FakeStore s;
    auto      t = s.transport();

    bl::BestandslogDocument local;
    local.reservierungen.push_back(mk_res("uuid-A/0", bl::BatchStatus::offen, "prod1"));

    auto const now = scripted_now({1000, 1000, 1001});
    auto const p   = lock_probe(t, kDocKey, owner_a(), bl::kLockTtlSeconds, now,
                                [&]() { return bl::store_document_merged(t, kDocKey, local).has_value(); });

    EXPECT_EQ(p.outcome, bl::LockOutcome::ok);
    EXPECT_EQ(p.cerr_zeilen, 0u);    // gruener Zyklus schweigt
    EXPECT_EQ(lock_owner_in(s), ""); // Lock endet mit dem Schreibvorgang
    auto const doc = bl::parse_bestandslog(s.objs[kDocKey]);
    ASSERT_TRUE(doc.has_value());
    EXPECT_EQ(doc->reservierungen.size(), 1u);
}

TEST(G3BestandslogLock, WithDocumentLockYieldsToForeignFreshLock) {
    FakeStore s;
    auto      t = s.transport();
    ASSERT_TRUE(bl::try_acquire_lock(t, kLockKey, owner_b(), bl::kLockTtlSeconds, 1000));

    bool       gelaufen = false;
    auto const now      = scripted_now({1005});
    auto const p        = lock_probe(t, kDocKey, owner_a(), bl::kLockTtlSeconds, now, [&]() {
        gelaufen = true;
        return true;
    });

    EXPECT_EQ(p.outcome, bl::LockOutcome::lock_unavailable);
    EXPECT_FALSE(gelaufen);                // NICHTS geschrieben
    EXPECT_EQ(p.cerr_zeilen, 0u);          // Kollision ist der Normalfall -> der Aufrufer meldet
    EXPECT_EQ(lock_owner_in(s), "uuid-B"); // fremder Lock unberuehrt
    EXPECT_FALSE(s.objs.contains(kDocKey));
}

TEST(G3BestandslogLock, WithDocumentLockRefusesToWriteBeyondSectionBudget) {
    // Frist-Wache: schon der Erwerb hat mehr als das SEKTIONS-BUDGET gekostet -> NICHT schreiben,
    // Lock freigeben, Wiederholung ist Sache des Aufrufers. Gewacht wird gegen das Budget, NICHT
    // gegen die ttl: die ttl ist die 30-Minuten-Obergrenze fuer den Stale-Bruch, aus ihr abgeleitete
    // Schwellen (900 s) waeren als Wache eines millisekunden-kurzen Zyklus wertlos.
    FakeStore s;
    auto      t = s.transport();

    bool       gelaufen = false;
    auto const now      = scripted_now({1000, 1031}); // 31 s > budget 30 s (Default)
    auto const p        = lock_probe(t, kDocKey, owner_a(), bl::kLockTtlSeconds, now, [&]() {
        gelaufen = true;
        return true;
    });

    EXPECT_EQ(p.outcome, bl::LockOutcome::deadline_exceeded);
    EXPECT_FALSE(gelaufen);
    EXPECT_EQ(p.cerr_zeilen, 1u);
    EXPECT_NE(p.cerr_text.find("[bestandslog] FEHLER"), std::string::npos) << p.cerr_text;
    EXPECT_NE(p.cerr_text.find("budget=30"), std::string::npos) << p.cerr_text;
    EXPECT_EQ(lock_owner_in(s), "");        // Lock freigegeben, nicht liegen gelassen
    EXPECT_FALSE(s.objs.contains(kDocKey)); // und nichts geschrieben

    // Genau AUF dem Budget ist noch erlaubt (die Wache greift erst DARUEBER).
    auto const grenze = scripted_now({2000, 2030, 2031});
    auto const q      = lock_probe(t, kDocKey, owner_a(), bl::kLockTtlSeconds, grenze, [&]() {
        gelaufen = true;
        return true;
    });
    EXPECT_EQ(q.outcome, bl::LockOutcome::ok);
    EXPECT_TRUE(gelaufen);
}

TEST(G3BestandslogLock, WithDocumentLockHonoursExplicitSectionBudget) {
    // Ein Aufrufer darf sein Budget selbst setzen (der Parameter steht hinter fn und hat einen
    // Default, damit die bestehende Sechs-Argument-Aufrufform gueltig bleibt).
    FakeStore s;
    auto      t = s.transport();

    bool       gelaufen = false;
    auto const now      = scripted_now({1000, 1006}); // 6 s > eigenes Budget 5 s
    auto const p        = lock_probe(
        t, kDocKey, owner_a(), bl::kLockTtlSeconds, now,
        [&]() {
            gelaufen = true;
            return true;
        },
        5);

    EXPECT_EQ(p.outcome, bl::LockOutcome::deadline_exceeded);
    EXPECT_FALSE(gelaufen);
    EXPECT_NE(p.cerr_text.find("budget=5"), std::string::npos) << p.cerr_text;
}

TEST(G3BestandslogLock, WithDocumentLockEtaFormHoldsUntilEtaIsFixed) {
    // Der ETA-Fall (Owner: der EINZIGE laengere Schreib-Lock-Fall) ist derselbe Zyklus mit
    // section_budget_s = ttl_s: das Lock wird exklusiv gehalten, bis die ETA feststeht -- bis maximal
    // zur 30-Minuten-Obergrenze. Eine 200-s-Sektion ist damit regulaer und schweigt.
    FakeStore s;
    auto      t = s.transport();

    bool       gelaufen = false;
    auto const now      = scripted_now({1000, 1200, 1201}); // 200 s Rechenzeit vor dem Schreiben
    auto const p        = lock_probe(
        t, kDocKey, owner_a(), bl::kLockTtlSeconds, now,
        [&]() {
            gelaufen = true;
            return true;
        },
        bl::kLockTtlSeconds);

    EXPECT_EQ(p.outcome, bl::LockOutcome::ok);
    EXPECT_TRUE(gelaufen);
    EXPECT_EQ(p.cerr_zeilen, 0u); // exklusiv erlaubt -> keine Budget-Warnung
}

TEST(G3BestandslogLock, WithDocumentLockReportsOverrunSection) {
    // Nach-Wache: die Arbeit selbst hat das Budget gesprengt. Geschrieben ist dann schon (der
    // Union-Merge traegt die Kollision), aber die Invariante war verletzt -> sichtbar, mit BEIDEN
    // Grenzen in der Zeile (Budget und ttl-Obergrenze).
    FakeStore s;
    auto      t = s.transport();

    auto const now = scripted_now({1000, 1005, 1041}); // Sektion 41 s > budget 30 s
    auto const p   = lock_probe(t, kDocKey, owner_a(), bl::kLockTtlSeconds, now, [&]() { return true; });

    EXPECT_EQ(p.outcome, bl::LockOutcome::ok);
    EXPECT_EQ(p.cerr_zeilen, 1u);
    EXPECT_NE(p.cerr_text.find("[bestandslog] warn"), std::string::npos) << p.cerr_text;
    EXPECT_NE(p.cerr_text.find("budget=30"), std::string::npos) << p.cerr_text;
    EXPECT_NE(p.cerr_text.find("1800"), std::string::npos) << p.cerr_text;
}

TEST(G3BestandslogLock, WithDocumentLockReportsFailedWork) {
    // fn liefert bool = "Arbeit gelungen" -> ein verschluckter Store-Fehler ist an dieser Naht
    // nicht mehr moeglich; der Lock wird trotzdem freigegeben.
    FakeStore s;
    auto      t = s.transport();

    auto const now = scripted_now({1000, 1000, 1000});
    auto const p   = lock_probe(t, kDocKey, owner_a(), bl::kLockTtlSeconds, now, [&]() { return false; });

    EXPECT_EQ(p.outcome, bl::LockOutcome::work_failed);
    EXPECT_EQ(lock_owner_in(s), "");
}

TEST(G3BestandslogLock, WithDocumentLockRejectsEmptyOwner) {
    FakeStore s;
    auto      t = s.transport();

    bool       gelaufen = false;
    auto const now      = scripted_now({1000});
    auto const p        = lock_probe(t, kDocKey, bl::LockOwner{"", "prod1", 111}, bl::kLockTtlSeconds, now, [&]() {
        gelaufen = true;
        return true;
    });

    EXPECT_EQ(p.outcome, bl::LockOutcome::lock_unavailable);
    EXPECT_FALSE(gelaufen);
    EXPECT_EQ(p.cerr_zeilen, 1u); // Konfigurations-Fehler des Hosts -> genau eine Zeile
    EXPECT_NE(p.cerr_text.find("[bestandslog] FEHLER"), std::string::npos) << p.cerr_text;
    EXPECT_TRUE(s.objs.empty()); // kein Lock-Objekt, kein Dokument
}

TEST(G3BestandslogLock, WithDocumentLockReleasesOnThrow) {
    // Wirft die Arbeit, muss der Lock trotzdem weg sein -- sonst sperrte ein Abbruch das Dokument
    // bis zum ttl-Bruch (RAII-Halter, nicht Straight-Line-Release).
    FakeStore s;
    auto      t = s.transport();

    auto const now = scripted_now({1000, 1000});
    EXPECT_THROW(
        {
            (void)bl::with_document_lock(t, kDocKey, owner_a(), bl::kLockTtlSeconds, now, []() -> bool {
                throw std::runtime_error("Abbruch mitten im Schreibvorgang");
            });
        },
        std::runtime_error);
    EXPECT_EQ(lock_owner_in(s), "");
}

TEST(G3BestandslogLock, LockOutcomeNames) {
    EXPECT_EQ(bl::to_string(bl::LockOutcome::ok), "ok");
    EXPECT_EQ(bl::to_string(bl::LockOutcome::lock_unavailable), "lock_unavailable");
    EXPECT_EQ(bl::to_string(bl::LockOutcome::deadline_exceeded), "deadline_exceeded");
    EXPECT_EQ(bl::to_string(bl::LockOutcome::work_failed), "work_failed");
}
