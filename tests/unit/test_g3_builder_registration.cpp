// test_g3_builder_registration -- G3 / #46b Lagerhaltung, Scheibe I1.
//
// Die Bestandslog-Laufzeit-Zustandsmaschine des Bau-Loops (LagerRunState) mit Fake-Transport:
// Lager-Index laden, per-Binary-Klassifikation (Dedup-Hit / frisch / no_key), Flush + Union-Merge.
// Deterministisch, ohne minio.

#include "bestandslog/builder_registration.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>

namespace bl = comdare::cache_engine::builder::bestandslog;

namespace {

// In-Memory-Objekt-Store (Fake-Transport).
struct FakeStore {
    std::map<std::string, std::string> objs;

    bl::BestandTransport transport() {
        bl::BestandTransport t;
        t.fetch = [this](std::string const& k) -> std::optional<std::string> {
            auto it = objs.find(k);
            if (it == objs.end()) return std::nullopt;
            return it->second;
        };
        t.store = [this](std::string const& k, std::string const& c) -> bool {
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

constexpr char const* kDocKey = "bestandslog/binary_bestand.xml";

// Ein gueltiger 128-hex-Key (aus einer bekannten Ziffer aufgebaut).
std::string hexkey(char c) { return std::string(128, c); }

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
    EXPECT_EQ(st.observe("", "pfad", 10, "", "utc"), bl::DedupOutcome::no_key);
    EXPECT_EQ(st.observe("nothex", "pfad", 10, "", "utc"), bl::DedupOutcome::no_key); // falsche Laenge
    EXPECT_EQ(st.pending_fresh(), 0u);
}

TEST(G3BuilderRegistration, ObserveFreshThenFlush) {
    FakeStore         s;
    auto              t = s.transport();
    bl::LagerRunState st;
    st.load(t, kDocKey);

    EXPECT_EQ(st.observe(hexkey('a'), "tier/0.dll", 428032, "[d,e,f]", "2026-07-23T12:00:00Z"),
              bl::DedupOutcome::fresh_register);
    EXPECT_EQ(st.observe(hexkey('b'), "tier/1.dll", 12345, "[a,b,c]", "2026-07-23T12:01:00Z"),
              bl::DedupOutcome::fresh_register);
    EXPECT_EQ(st.pending_fresh(), 2u);

    auto n = st.flush(t, kDocKey, "2026-07-23T12:02:00Z");
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, 2u);
    EXPECT_EQ(st.pending_fresh(), 0u);

    // Der Store traegt jetzt ein Binary-Bestandslog mit beiden Eintraegen.
    auto doc = bl::parse_bestandslog(s.objs[kDocKey]);
    ASSERT_TRUE(doc.has_value());
    EXPECT_EQ(doc->genus, bl::Genus::binary);
    EXPECT_EQ(doc->bestand.size(), 2u);

    // Zweiter Flush ohne neue Beobachtung -> nichts zu tun.
    auto n2 = st.flush(t, kDocKey, "2026-07-23T12:03:00Z");
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
    e.pfad       = "tier/0.dll";
    seed.bestand.push_back(e);
    s.objs[kDocKey] = bl::emit_document(seed);

    bl::LagerRunState st;
    st.load(t, kDocKey);
    EXPECT_EQ(st.lager_size(), 1u);

    // 'a' ist schon im Lager -> Hit (kein Neu-Eintrag); 'b' ist neu -> fresh.
    EXPECT_EQ(st.observe(hexkey('a'), "tier/0.dll", 428032, "", "utc"), bl::DedupOutcome::lager_hit);
    EXPECT_EQ(st.observe(hexkey('b'), "tier/1.dll", 999, "", "utc"), bl::DedupOutcome::fresh_register);
    EXPECT_EQ(st.lager_hits(), 1u);
    EXPECT_EQ(st.pending_fresh(), 1u);

    auto n = st.flush(t, kDocKey, "utc2");
    ASSERT_TRUE(n.has_value());
    EXPECT_EQ(*n, 1u); // nur 'b' frisch

    // Der Merge vereint das vorbestehende 'a' + das neue 'b'.
    auto doc = bl::parse_bestandslog(s.objs[kDocKey]);
    ASSERT_TRUE(doc.has_value());
    EXPECT_EQ(doc->bestand.size(), 2u);
}

TEST(G3BuilderRegistration, StoreFailurePropagates) {
    // Transport mit fehlschlagendem store -> flush liefert nullopt.
    bl::BestandTransport t;
    t.fetch = [](std::string const&) -> std::optional<std::string> { return std::nullopt; };
    t.store = [](std::string const&, std::string const&) -> bool { return false; };
    bl::LagerRunState st;
    st.load(t, kDocKey);
    EXPECT_EQ(st.observe(hexkey('c'), "p", 1, "", "u"), bl::DedupOutcome::fresh_register);
    auto n = st.flush(t, kDocKey, "u");
    EXPECT_FALSE(n.has_value());
}
