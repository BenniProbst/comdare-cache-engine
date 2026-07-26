// test_g3_artifact_cache_transport -- G3 / #46b Lagerhaltung, Folge-A (T1).
//
// Der Objekt-per-Key-Weg des ArtifactCache + der eine Binder auf die BestandTransport-Naht.
//
// DETERMINISTISCH OHNE MINIO: getestet wird die INERTE Instanz -- ein default-konstruierter
// ArtifactCache hat weder Endpoint noch Bucket, minio_enabled() ist false. Das ist bewusst KEIN
// from_env(): der Test darf nicht davon abhaengen, dass die Umgebung zufaellig keine
// COMDARE_MINIO_*-Variablen traegt. Kein mc wird gestartet, kein Netz, keine TEMP-Datei entsteht.
//
// Was das beweist, ist genau das Sicherheitsversprechen der Naht: ohne konfigurierte Ebene B melden
// alle vier Verben ehrlich "nichts geschehen" (nullopt/false) statt eines falschen Erfolgs, und der
// daraus gebundene Transport verhaelt sich fuer das Bestandslog wie ein leerer Store. Der scharfe
// mc-Pfad ist Infrastruktur (Shellout) und gehoert nicht in einen Unit-Test.

#include "artifact_transport/artifact_cache.hpp"
#include "bestandslog/artifact_cache_transport.hpp"
#include "bestandslog/builder_registration.hpp" // LagerRunState (der Konsument der gebundenen Naht)

#include <gtest/gtest.h>

#include <string>

namespace at = comdare::cache_engine::builder::artifact_transport;
namespace bl = comdare::cache_engine::builder::bestandslog;

namespace {
constexpr char const* kDocKey = "bestandslog/binary_bestand.xml";
}

// ---------------------------------------------------------------------------
// Die vier Verben auf einer inerten Instanz: kein Effekt, kein falscher Erfolg.
// ---------------------------------------------------------------------------
TEST(G3ArtifactCacheTransport, ObjectVerbsAreInertWithoutMinio) {
    at::ArtifactCache const cache; // default == weder Ebene B noch Ebene C konfiguriert
    ASSERT_FALSE(cache.minio_enabled());
    ASSERT_TRUE(cache.inert());

    EXPECT_FALSE(cache.object_fetch(kDocKey).has_value());
    EXPECT_FALSE(cache.object_store(kDocKey, "<bestandslog/>"));
    EXPECT_FALSE(cache.object_remove(kDocKey));
    EXPECT_FALSE(cache.object_stat(kDocKey).has_value());
}

// ---------------------------------------------------------------------------
// Der Binder setzt alle vier Verben (die Naht ist vollstaendig belegt, nicht halb).
// ---------------------------------------------------------------------------
TEST(G3ArtifactCacheTransport, BinderPopulatesAllFourVerbs) {
    at::ArtifactCache const cache;
    auto const              t = bl::make_bestand_transport(cache);

    EXPECT_TRUE(static_cast<bool>(t.fetch));
    EXPECT_TRUE(static_cast<bool>(t.store));
    EXPECT_TRUE(static_cast<bool>(t.remove));
    EXPECT_TRUE(static_cast<bool>(t.stat));
}

// Der gebundene Transport reicht die inerte Antwort unveraendert durch -- er erfindet nichts.
TEST(G3ArtifactCacheTransport, BoundTransportForwardsInertAnswers) {
    at::ArtifactCache const cache;
    auto const              t = bl::make_bestand_transport(cache);

    EXPECT_FALSE(t.fetch(kDocKey).has_value());
    EXPECT_FALSE(t.store(kDocKey, "<bestandslog/>"));
    EXPECT_FALSE(t.remove(kDocKey));
    EXPECT_FALSE(t.stat(kDocKey).has_value());
}

// ---------------------------------------------------------------------------
// Das Bestandslog auf einem inerten Transport: leeres Lager, kein Phantom-Bestand, und der
// fehlgeschlagene Schreibweg wird als Store-Fehler SICHTBAR (nullopt) statt still geschluckt.
// ---------------------------------------------------------------------------
TEST(G3ArtifactCacheTransport, LagerRunStateOnInertTransport) {
    at::ArtifactCache const cache;
    auto const              t = bl::make_bestand_transport(cache);

    bl::LagerRunState st;
    st.load(t, kDocKey);
    EXPECT_EQ(st.lager_size(), 0u); // nichts zu laden -> leeres Lager
    EXPECT_EQ(st.lager_hits(), 0u);

    // Ohne Beobachtung gibt es nichts zu schreiben -> 0 (kein Fehler, es wurde nichts versucht).
    auto const nichts = st.flush(t, kDocKey, "2026-07-26T00:00:00Z");
    ASSERT_TRUE(nichts.has_value());
    EXPECT_EQ(*nichts, 0u);

    // Mit Beobachtung schlaegt der Schreibweg fehl und das MELDET der flush (nullopt), statt einen
    // Erfolg vorzutaeuschen.
    bl::ZellKoordinaten const zelle{.combo = "default", .opt = "O2", .simd = "avx2"};
    EXPECT_EQ(st.observe(std::string(128, 'a'), zelle, "tier/0.dll", 1, "", "utc"), bl::DedupOutcome::fresh_register);
    EXPECT_FALSE(st.flush(t, kDocKey, "2026-07-26T00:00:01Z").has_value());
}

// ---------------------------------------------------------------------------
// ObjectMeta -> ObjectStat wird feldweise uebersetzt; mtime bleibt ehrlich 0 == unbekannt.
// ---------------------------------------------------------------------------
TEST(G3ArtifactCacheTransport, ObjectMetaDefaultsAreHonest) {
    at::ObjectMeta const m;
    EXPECT_EQ(m.size, 0u);
    EXPECT_EQ(m.mtime_epoch_s, 0); // 0 == unbekannt, kein erfundener Zeitstempel

    bl::ObjectStat const s{.size = m.size, .mtime_epoch_s = m.mtime_epoch_s};
    EXPECT_EQ(s.size, m.size);
    EXPECT_EQ(s.mtime_epoch_s, m.mtime_epoch_s);
}
