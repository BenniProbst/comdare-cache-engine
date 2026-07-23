// test_g3_bestandslog_lock -- G3 / #46b Lagerhaltung, Scheibe B2.
//
// Deckt die Koordinations-Schicht ab: Transport-Naht (Fake = In-Memory-Map), Owner-Token-Verify-Lock
// mit ttl-Bruch, skriptbare Race-Interleavings (verlorenes Race sichtbar), Record-UNION-Merge
// (Doppel-Halt harmlos, Konflikt monoton) und "Lock endet mit erster pro-forma". Ohne minio.

#include "bestandslog/bestandslog_lock.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>

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
