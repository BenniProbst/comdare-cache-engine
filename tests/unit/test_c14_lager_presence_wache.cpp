// test_c14_lager_presence_wache -- #97/E-12 C-14: die Bestandslog-SKIP-Wache an der Presence-Naht.
//
// GEGENSTAND (KON3-06, Messung 09.08.2026): der real gemessene Bestand traegt je binary_id GENAU EINEN
// Eintrag mit LEERER Zelle (320 geprueften IDs war der Fingerprint unter [O2,avx2]/[O3,avx512]
// bit-identisch, Alt-Format). Laeuft der Host ohne gesetzte Zell-Env, ist die Lauf-Zelle ebenfalls
// leer (profile_run_entry.hpp:697ff, woertlich "Ungesetzt => alle drei leer =>
// ZellKoordinaten::empty()") -- und make_lager_presence stellte die Frage lager_contains(hex, {}).
// Der EINE kollabierte Leer-Zellen-Eintrag begruendete damit den SKIP fuer JEDE Matrix-Zelle.
// C-14 schliesst das an der Naht, an der der SKIP begruendet wird: eine LEERE Lauf-Zelle ist NIE
// ein SKIP-Beleg (konservativer Miss; SKIP nur bei BELEGTEM Eintrag je binary_id).
//
// WAS DIESE TU BEWEIST:
//   (a) KOEDER            -- kollabierter Leer-Zellen-Bestand + leere Lauf-Zelle => presence false
//                            fuer JEDEN view-Index. VOR C-14 war sie true: der falsch begruendete
//                            SKIP. Genau dieser Test ist der ROT-zuerst-Beweis (T-1).
//   (b) GEGENEINGANG      -- belegte Lauf-Zelle + belegter, passender Eintrag => presence true.
//                            Die Wache verschenkt keinen berechtigten Skip (T-4).
//   (c) TUPEL-SCHAERFE    -- belegte Lauf-Zelle gegen NUR-Leer-Zellen-Bestand => false, und
//                            leere Lauf-Zelle gegen BELEGTEN Bestand => false. Die Wache haengt an
//                            der Lauf-Zelle, die Tupel-Gleichheit an beiden Seiten -- beide
//                            Richtungen ausdruecklich.
//   (d) NENNER FREMD      -- die Miss-Zaehlung laeuft gegen die Fenstergroesse der testlokalen
//                            View (ids.size()), nie gegen eine Zahl aus dem Prueflig (T-3).
//
// Harness-Muster wie test_g3_lager_presence.cpp (MiniView duck-typed, Dokument-Transport inert,
// ohne minio, ohne Baum-Schicht). Deterministisch.

#include "bestandslog/lager_presence.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace bl = comdare::cache_engine::builder::bestandslog;

namespace {

constexpr char const* kDocKey = "bestandslog/binary_bestand.xml";

std::string hexkey(char c) { return std::string(128, c); }

bl::ZellKoordinaten const kZelleLeer{};
bl::ZellKoordinaten const kZelle{.combo = "default", .opt = "O2", .simd = "avx2"};

// Die MINIMALE View, die make_index_key_fn verlangt (duck-typed Vertrag, s. test_g3_lager_presence).
struct MiniSpec {
    std::string binary_id;
};
struct MiniView {
    std::vector<std::string> ids;

    [[nodiscard]] MiniSpec operator[](std::size_t i) const { return MiniSpec{ids[i]}; }
};

// Lager mit den Eintraegen aus seed (binary-Genus) vorbelegen und einen LagerRunState darauf laden.
void load_state(bl::LagerRunState& st, std::map<std::string, std::string>& objs,
                std::vector<bl::BestandEintrag> const& seed) {
    bl::BestandslogDocument doc;
    doc.genus     = bl::Genus::binary;
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

// Der Provider-Weg des Tests: binary_id -> deterministisches 128-hex (testlokal, KEINE Ableitung
// aus dem Prueflig).
bl::BinaryIdKeyFn make_by_id(std::map<std::string, std::string> keymap) {
    return [keymap = std::move(keymap)](std::string const& id) -> std::optional<std::string> {
        auto it = keymap.find(id);
        if (it == keymap.end()) return std::nullopt;
        return it->second;
    };
}

} // namespace

// ---------------------------------------------------------------------------
// (a) KOEDER (KON3-06): der kollabierte Leer-Zellen-Bestand -- je binary_id EIN Eintrag mit leerer
// Zelle -- darf bei leerer Lauf-Zelle KEINEN einzigen SKIP begruenden. Der Index enthaelt die
// Eintraege nachweislich (lager_contains bejaht das Tupel (hex, {})); die WACHE an der Presence-
// Naht muss trotzdem "fehlt" melden. Eine Implementierung ohne Wache faellt hier durch (ROT vor
// dem C-14-Fix, 2026-08-21 belegt).
// ---------------------------------------------------------------------------
TEST(C14LagerPresenceWache, KoederKollabierterLeerZellenBestandSkipptNie) {
    MiniView const view{{"bin-0", "bin-1", "bin-2"}};

    std::map<std::string, std::string> objs;
    bl::LagerRunState                  st;
    load_state(st, objs,
               {eintrag(hexkey('a'), kZelleLeer), eintrag(hexkey('b'), kZelleLeer), eintrag(hexkey('c'), kZelleLeer)});

    auto const by_id = make_by_id({{"bin-0", hexkey('a')}, {"bin-1", hexkey('b')}, {"bin-2", hexkey('c')}});

    // VORBEDINGUNG des Koeders, am Index selbst belegt: das Leer-Zellen-Tupel LIEGT im Lager.
    // (Ohne diese Zeile koennte ein leerer Index den Test still gruen machen -- der Koeder
    // muss beissen KOENNEN, damit sein Nicht-Beissen etwas beweist.)
    ASSERT_TRUE(st.lager_contains(hexkey('a'), kZelleLeer));
    ASSERT_TRUE(st.lager_contains(hexkey('b'), kZelleLeer));
    ASSERT_TRUE(st.lager_contains(hexkey('c'), kZelleLeer));

    bl::PresenceFn const present = bl::make_lager_presence(st, bl::make_index_key_fn(view, by_id), kZelleLeer);

    // NENNER FREMD: die Fenstergroesse kommt aus der testlokalen View, nicht aus dem Prueflig.
    std::size_t const fenster = view.ids.size();
    ASSERT_EQ(fenster, std::size_t{3});
    std::size_t misses = 0;
    for (std::size_t i = 0; i < fenster; ++i) {
        EXPECT_FALSE(present(i)) << "leere Lauf-Zelle begruendete einen SKIP fuer view-Index " << i;
        if (!present(i)) ++misses;
    }
    EXPECT_EQ(misses, fenster) << "jede Zelle muss als 'fehlt' gemeldet werden (konservativer Miss)";
}

// ---------------------------------------------------------------------------
// (b) GEGENEINGANG: belegte Lauf-Zelle + belegter, passender Eintrag => presence true. Die Wache
// darf den BERECHTIGTEN Skip nicht mit verschlucken -- sonst waere sie von "immer false" nicht zu
// unterscheiden.
// ---------------------------------------------------------------------------
TEST(C14LagerPresenceWache, GegeneingangBelegteZelleSkipptWeiter) {
    MiniView const view{{"bin-0"}};

    std::map<std::string, std::string> objs;
    bl::LagerRunState                  st;
    load_state(st, objs, {eintrag(hexkey('a'), kZelle)});

    auto const by_id = make_by_id({{"bin-0", hexkey('a')}});

    bl::PresenceFn const present = bl::make_lager_presence(st, bl::make_index_key_fn(view, by_id), kZelle);
    EXPECT_TRUE(present(0)) << "belegte Lauf-Zelle + belegter Eintrag ist der berechtigte Skip";
}

// ---------------------------------------------------------------------------
// (c) TUPEL-SCHAERFE, beide Richtungen ausdruecklich:
//     belegte Lauf-Zelle gegen NUR-Leer-Zellen-Bestand => false (Tupel-Ungleichheit; der
//     kollabierte Eintrag taugt auch fuer eine BELEGTE Zelle nicht als Beleg), und
//     leere Lauf-Zelle gegen BELEGTEN Bestand => false (die Wache haengt an der Lauf-Zelle,
//     nicht am Eintrag: ohne benennbare Zelle gibt es nichts, wofuer der Skip gelten wuerde).
// ---------------------------------------------------------------------------
TEST(C14LagerPresenceWache, TupelSchaerfeBeideRichtungen) {
    MiniView const view{{"bin-0"}};

    auto const by_id = make_by_id({{"bin-0", hexkey('a')}});

    {
        std::map<std::string, std::string> objs;
        bl::LagerRunState                  st;
        load_state(st, objs, {eintrag(hexkey('a'), kZelleLeer)});
        bl::PresenceFn const present = bl::make_lager_presence(st, bl::make_index_key_fn(view, by_id), kZelle);
        EXPECT_FALSE(present(0)) << "Leer-Zellen-Eintrag darf eine belegte Lauf-Zelle nicht decken";
    }
    {
        std::map<std::string, std::string> objs;
        bl::LagerRunState                  st;
        load_state(st, objs, {eintrag(hexkey('a'), kZelle)});
        bl::PresenceFn const present = bl::make_lager_presence(st, bl::make_index_key_fn(view, by_id), kZelleLeer);
        EXPECT_FALSE(present(0)) << "leere Lauf-Zelle darf auch gegen belegten Bestand nicht skippen";
    }
}
