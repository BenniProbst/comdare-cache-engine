// test_c14_messwert_presence_wache -- #97/E-12 C-14-SCHWESTER (T-6, beide Genera): die SKIP-Wache
// der MESS-Presence-Naht, MesswertRunState::lager_contains (Genus::measurement).
//
// GEGENSTAND (KON3-06-Klasse, Messung 09.08.2026; Audit-Fund S97-F1): dieselbe (hex, zelle)-
// Tupel-Frage wie an der Binary-Presence-Naht existiert im MEASUREMENT-Genus --
// MesswertRunState::lager_contains ist laut eigenem Docstring "Lesender Zugriff fuer eine
// Mess-Presence-Naht (war diese Zelle schon gemessen?)". Ein kollabierter Leer-Zellen-Bestand
// (je Kennung EIN Eintrag mit leerer Zelle) traefe bei leerer Lauf-Zelle via lager_key_from_hex
// jede Anfrage (leer-vs-leer matcht) -- exakt das Loch, das C-14 auf der Binary-Seite schliesst
// (lager_presence.hpp / test_c14_lager_presence_wache). GOAL VI.4 verlangt den SKIP "fuer
// Messdaten UND Binaries"; T-6 verlangt die Schwesterstelle in BEIDEN Genera im selben Zug.
//
// PLATZIERUNG (Abwaegung, s. Kopf der Wache in messwert_registrierung.hpp): auf der Binary-Seite
// sitzt die Wache an der NAHT (make_lager_presence), NICHT im rohen LagerRunState::lager_contains
// -- der dortige Koeder ASSERTet das rohe Tupel. Im Mess-Genus IST lager_contains selbst die
// (kuenftige) Presence-Naht; die Wache sitzt deshalb direkt an ihr. Der rohe (unbewachte)
// Index-Weg dieses Genus bleibt observe() -- dessen lager_hit-Dedup dient hier als
// Koeder-Vorbedingung (der Koeder KANN beissen; T-11c-Koeder-Form).
//
// WAS DIESE TU BEWEIST (Spiegel von test_c14_lager_presence_wache):
//   (a) KOEDER            -- kollabierter Leer-Zellen-Bestand + leere Lauf-Zelle =>
//                            lager_contains false fuer JEDE Kennung. VOR der Schwester-Wache
//                            war sie true: der falsch begruendete Mess-SKIP. Genau dieser Test
//                            ist der ROT-zuerst-Beweis (T-1).
//   (b) GEGENEINGANG      -- belegte Lauf-Zelle + belegter, passender Eintrag => true. Die Wache
//                            verschenkt keinen berechtigten "schon gemessen"-Befund (T-4).
//   (c) TUPEL-SCHAERFE    -- belegte Lauf-Zelle gegen NUR-Leer-Zellen-Bestand => false, und
//                            leere Lauf-Zelle gegen BELEGTEN Bestand => false. Die Wache haengt
//                            an der Lauf-Zelle, die Tupel-Gleichheit an beiden Seiten -- beide
//                            Richtungen ausdruecklich.
//   (d) NENNER FREMD      -- die Miss-Zaehlung laeuft gegen die testlokale Kennungs-Liste
//                            (keys.size()), nie gegen eine Zahl aus dem Prueflig (T-3).
//
// Harness-Muster wie test_c14_lager_presence_wache (Dokument-Transport inert, ohne minio, ohne
// Baum-Schicht), nur auf Genus::measurement gestempelt. Deterministisch.

#include "bestandslog/messwert_registrierung.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace bl = comdare::cache_engine::builder::bestandslog;

namespace {

constexpr char const* kDocKey = "bestandslog/messwert_bestand.xml";

std::string hexkey(char c) { return std::string(128, c); }

bl::ZellKoordinaten const kZelleLeer{};
bl::ZellKoordinaten const kZelle{.combo = "default", .opt = "O2", .simd = "avx2"};

// Messwert-Lager mit den Eintraegen aus seed (measurement-Genus) vorbelegen und einen
// MesswertRunState darauf laden (Muster: load_state in test_c14_lager_presence_wache).
void load_state(bl::MesswertRunState& st, std::map<std::string, std::string>& objs,
                std::vector<bl::BestandEintrag> const& seed) {
    bl::BestandslogDocument doc;
    doc.genus     = bl::Genus::measurement;
    doc.bestand   = seed;
    objs[kDocKey] = bl::emit_document(doc);

    bl::BestandTransport t;
    t.fetch = [&objs](std::string const& k) -> std::optional<std::string> {
        auto it = objs.find(k);
        if (it == objs.end()) return std::nullopt;
        return it->second;
    };
    st.load(t, kDocKey);
}

bl::BestandEintrag eintrag(std::string key_hex, bl::ZellKoordinaten zelle) {
    bl::BestandEintrag e;
    e.key_sha512 = std::move(key_hex);
    e.zelle      = std::move(zelle);
    return e;
}

} // namespace

// ---------------------------------------------------------------------------
// (a) KOEDER (KON3-06-Klasse im Mess-Genus): der kollabierte Leer-Zellen-Bestand -- je Kennung EIN
// Eintrag mit leerer Zelle -- darf bei leerer Lauf-Zelle KEINEN einzigen "schon gemessen"-Befund
// begruenden. Der Index enthaelt die Eintraege nachweislich (lager_size + observe-lager_hit auf dem
// rohen Tupel); die WACHE an der Mess-Presence-Naht muss trotzdem "nicht gemessen" melden. Eine
// Implementierung ohne Wache faellt hier durch (ROT vor dem Schwester-Fix, 2026-08-22 belegt).
// ---------------------------------------------------------------------------
TEST(C14MesswertPresenceWache, KoederKollabierterLeerZellenBestandSkipptNie) {
    // NENNER FREMD: die testlokale Kennungs-Liste (T-3) -- nie eine Zahl aus dem Prueflig.
    std::vector<std::string> const keys{hexkey('a'), hexkey('b'), hexkey('c')};

    std::map<std::string, std::string> objs;
    bl::MesswertRunState               st;
    load_state(st, objs,
               {eintrag(hexkey('a'), kZelleLeer), eintrag(hexkey('b'), kZelleLeer), eintrag(hexkey('c'), kZelleLeer)});

    // VORBEDINGUNG des Koeders, am Index selbst belegt: der Leer-Zellen-Bestand IST geladen UND
    // das Leer-Tupel MATCHT im Index -- observe() ist der rohe, unbewachte Index-Weg dieses Genus
    // und quittiert lager_hit. (Ohne diese Zeilen koennte ein leerer Index den Test still gruen
    // machen -- der Koeder muss beissen KOENNEN, damit sein Nicht-Beissen etwas beweist.)
    ASSERT_EQ(st.lager_size(), std::size_t{3});
    ASSERT_EQ(st.observe(hexkey('a'), kZelleLeer, "p", 0, "s", "t"), bl::MesswertOutcome::lager_hit);
    ASSERT_EQ(st.observe(hexkey('b'), kZelleLeer, "p", 0, "s", "t"), bl::MesswertOutcome::lager_hit);
    ASSERT_EQ(st.observe(hexkey('c'), kZelleLeer, "p", 0, "s", "t"), bl::MesswertOutcome::lager_hit);

    std::size_t const fenster = keys.size();
    ASSERT_EQ(fenster, std::size_t{3});
    std::size_t misses = 0;
    for (std::size_t i = 0; i < fenster; ++i) {
        EXPECT_FALSE(st.lager_contains(keys[i], kZelleLeer))
            << "leere Lauf-Zelle begruendete einen Mess-SKIP fuer Kennung " << i;
        if (!st.lager_contains(keys[i], kZelleLeer)) ++misses;
    }
    EXPECT_EQ(misses, fenster) << "jede Kennung muss als 'nicht gemessen' gemeldet werden (konservativer Miss)";
}

// ---------------------------------------------------------------------------
// (b) GEGENEINGANG: belegte Lauf-Zelle + belegter, passender Eintrag => true. Die Wache darf den
// BERECHTIGTEN "schon gemessen"-Befund nicht mit verschlucken -- sonst waere sie von "immer false"
// nicht zu unterscheiden.
// ---------------------------------------------------------------------------
TEST(C14MesswertPresenceWache, GegeneingangBelegteZelleBleibtTreffer) {
    std::map<std::string, std::string> objs;
    bl::MesswertRunState               st;
    load_state(st, objs, {eintrag(hexkey('a'), kZelle)});

    EXPECT_TRUE(st.lager_contains(hexkey('a'), kZelle))
        << "belegte Lauf-Zelle + belegter Eintrag ist der berechtigte 'schon gemessen'-Befund";
}

// ---------------------------------------------------------------------------
// (c) TUPEL-SCHAERFE, beide Richtungen ausdruecklich:
//     belegte Lauf-Zelle gegen NUR-Leer-Zellen-Bestand => false (Tupel-Ungleichheit; der
//     kollabierte Eintrag taugt auch fuer eine BELEGTE Zelle nicht als Beleg), und
//     leere Lauf-Zelle gegen BELEGTEN Bestand => false (die Wache haengt an der Lauf-Zelle,
//     nicht am Eintrag: ohne benennbare Zelle gibt es nichts, wofuer der Treffer gelten wuerde).
// ---------------------------------------------------------------------------
TEST(C14MesswertPresenceWache, TupelSchaerfeBeideRichtungen) {
    {
        std::map<std::string, std::string> objs;
        bl::MesswertRunState               st;
        load_state(st, objs, {eintrag(hexkey('a'), kZelleLeer)});
        EXPECT_FALSE(st.lager_contains(hexkey('a'), kZelle))
            << "Leer-Zellen-Eintrag darf eine belegte Lauf-Zelle nicht decken";
    }
    {
        std::map<std::string, std::string> objs;
        bl::MesswertRunState               st;
        load_state(st, objs, {eintrag(hexkey('a'), kZelle)});
        EXPECT_FALSE(st.lager_contains(hexkey('a'), kZelleLeer))
            << "leere Lauf-Zelle darf auch gegen belegten Bestand nicht treffen";
    }
}
