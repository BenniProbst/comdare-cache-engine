// test_g3_lager_presence -- G3 / #46b Lagerhaltung, Folge-A (T3-Naht).
//
// Die Presence-Naht zwischen dem geladenen Lager-Index und dem Miss-Scan des Planers. Kern-Abnahmen:
//   (a) DEFAULT-NEUTRAL -- ohne Host-Provider meldet alles "fehlt" (== Ist-Verhalten ohne Lager).
//   (b) KONSERVATIVER MISS -- jede Unaufloesbarkeit (nullopt, ungueltiges Hex) ergibt "fehlt", nie
//       einen falschen Treffer.
//   (c) ZELL-GENAUIGKEIT (§62-NACHTRAG-4) -- derselbe Fingerprint unter anderer ISA ist ein Miss.
//   (d) SEAM-ONLY -- make_index_key_fn ist duck-typed; hier genuegt eine Mini-View mit operator[]
//       und .binary_id. Weder Iterator noch StaticBinaryView werden gebraucht (oder angefasst).
// Deterministisch, ohne minio, ohne Baum-Schicht.

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

bl::ZellKoordinaten const kZelle{.combo = "default", .opt = "O2", .simd = "avx2"};
bl::ZellKoordinaten const kZelleAndereIsa{.combo = "default", .opt = "O2", .simd = "avx512"};

// Die MINIMALE View, die make_index_key_fn verlangt: operator[](i) -> etwas mit .binary_id.
// Genau das ist der duck-typed Vertrag -- kein experiment_tree, kein Iterator.
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

} // namespace

// ---------------------------------------------------------------------------
// (a) Default-neutral: ohne BinaryIdKeyFn ist nichts aufloesbar -> alles "fehlt".
// ---------------------------------------------------------------------------
TEST(G3LagerPresence, WithoutProviderEverythingIsMissing) {
    MiniView const    view{{"bin-0", "bin-1"}};
    bl::LagerRunState st;

    auto const key_of  = bl::make_index_key_fn(view, bl::BinaryIdKeyFn{}); // leerer Provider
    auto const present = bl::make_lager_presence(st, key_of, kZelle);

    ASSERT_TRUE(static_cast<bool>(present));
    EXPECT_FALSE(present(0));
    EXPECT_FALSE(present(1));

    // Auch die Key-Funktion selbst meldet ehrlich "nichts bekannt".
    EXPECT_FALSE(key_of(0).has_value());
}

// Ohne IndexKeyFn (gar keine Naht) gilt dasselbe -- kein Schluessel-Weg, kein Treffer.
TEST(G3LagerPresence, WithoutIndexKeyFnEverythingIsMissing) {
    bl::LagerRunState st;
    auto const        present = bl::make_lager_presence(st, bl::IndexKeyFn{}, kZelle);
    EXPECT_FALSE(present(0));
    EXPECT_FALSE(present(42));
}

// ---------------------------------------------------------------------------
// (b)+(c) Treffer nur bei aufloesbarem Fingerprint UND passender Zelle.
// ---------------------------------------------------------------------------
TEST(G3LagerPresence, HitOnlyForResolvableKeyAndMatchingCell) {
    std::map<std::string, std::string> objs;
    bl::LagerRunState                  st;
    load_state(st, objs, {eintrag(hexkey('a'), kZelle)});
    ASSERT_EQ(st.lager_size(), 1u);

    // bin-0 -> 'a' (im Lager), bin-1 -> 'b' (nicht im Lager), bin-2 -> kein Fingerprint bekannt,
    // bin-3 -> ein kaputtes Hex (Sidecar beschaedigt).
    MiniView const          view{{"bin-0", "bin-1", "bin-2", "bin-3"}};
    bl::BinaryIdKeyFn const by_id = [](std::string const& id) -> std::optional<std::string> {
        if (id == "bin-0") return hexkey('a');
        if (id == "bin-1") return hexkey('b');
        if (id == "bin-3") return std::string{"kein-gueltiges-hex"};
        return std::nullopt;
    };

    auto const present = bl::make_lager_presence(st, bl::make_index_key_fn(view, by_id), kZelle);
    EXPECT_TRUE(present(0));  // im Lager, gleiche Zelle
    EXPECT_FALSE(present(1)); // anderer Fingerprint
    EXPECT_FALSE(present(2)); // kein Fingerprint -> konservativer Miss
    EXPECT_FALSE(present(3)); // ungueltiges Hex -> konservativer Miss

    // DIESELBE Binary unter einer anderen ISA ist ein Miss (der Kern von NACHTRAG-4).
    auto const present_avx512 = bl::make_lager_presence(st, bl::make_index_key_fn(view, by_id), kZelleAndereIsa);
    EXPECT_FALSE(present_avx512(0));

    // Der Scan ist rein lesend -- er faelscht keine Dedup-Statistik.
    EXPECT_EQ(st.lager_hits(), 0u);
    EXPECT_EQ(st.pending_fresh(), 0u);
}

// Beide Zellen im Lager -> beide Laeufe finden ihre eigene.
TEST(G3LagerPresence, BothCellsPresentResolveIndependently) {
    std::map<std::string, std::string> objs;
    bl::LagerRunState                  st;
    load_state(st, objs, {eintrag(hexkey('a'), kZelle), eintrag(hexkey('a'), kZelleAndereIsa)});
    ASSERT_EQ(st.lager_size(), 2u);

    MiniView const          view{{"bin-0"}};
    bl::BinaryIdKeyFn const by_id = [](std::string const&) -> std::optional<std::string> { return hexkey('a'); };

    EXPECT_TRUE(bl::make_lager_presence(st, bl::make_index_key_fn(view, by_id), kZelle)(0));
    EXPECT_TRUE(bl::make_lager_presence(st, bl::make_index_key_fn(view, by_id), kZelleAndereIsa)(0));
    // Eine dritte, nie gebaute Zelle bleibt ein Miss.
    EXPECT_FALSE(bl::make_lager_presence(st, bl::make_index_key_fn(view, by_id),
                                         {.combo = "default", .opt = "O3", .simd = "avx2"})(0));
}

// ---------------------------------------------------------------------------
// (d) Die Naht sitzt am view_index: make_index_key_fn geht ueber view[i].binary_id.
// ---------------------------------------------------------------------------
TEST(G3LagerPresence, IndexKeyFnGoesThroughBinaryId) {
    MiniView const           view{{"bin-A", "bin-B"}};
    std::vector<std::string> gesehen;
    bl::BinaryIdKeyFn const  by_id = [&gesehen](std::string const& id) -> std::optional<std::string> {
        gesehen.push_back(id);
        return hexkey('a');
    };

    auto const key_of = bl::make_index_key_fn(view, by_id);
    EXPECT_EQ(key_of(1).value_or(""), hexkey('a'));
    EXPECT_EQ(key_of(0).value_or(""), hexkey('a'));
    ASSERT_EQ(gesehen.size(), 2u);
    EXPECT_EQ(gesehen[0], "bin-B"); // der Index waehlt die binary_id, nicht die Aufruf-Reihenfolge
    EXPECT_EQ(gesehen[1], "bin-A");
}

// Die gebaute PresenceFn passt in die Planer-Signatur (I1b) -- ohne Anpassung am SlicePlanner.
TEST(G3LagerPresence, ResultIsAPlannerPresenceFn) {
    std::map<std::string, std::string> objs;
    bl::LagerRunState                  st;
    load_state(st, objs, {eintrag(hexkey('a'), kZelle)});

    MiniView const          view{{"bin-0", "bin-1"}};
    bl::BinaryIdKeyFn const by_id = [](std::string const& id) -> std::optional<std::string> {
        return id == "bin-0" ? std::optional<std::string>{hexkey('a')} : std::nullopt;
    };

    bl::PresenceFn const present = bl::make_lager_presence(st, bl::make_index_key_fn(view, by_id), kZelle);

    bl::SlicePlanQueue queue;
    {
        bl::SlicePlanner planner(queue, {0, 1}, 4, present);
        planner.join();
    }
    auto const plan = queue.pop();
    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(plan->view_indices, (std::vector<std::size_t>{0, 1}));
    EXPECT_EQ(plan->missing_count, 1u); // nur bin-1 fehlt
}

// ---------------------------------------------------------------------------
// Lager-TP1(B) / G-A2+G-E2: die PresenceFn ist jetzt BAU-FILTER -- die drei Pflicht-Abnahmen des
// Teilpakets gegen einen ECHTEN LagerRunState (Dokument-Interface, baum-agnostisch).
// ---------------------------------------------------------------------------
TEST(G3LagerPresence, TeilbestandErgibtMissingKleinerFenster) {
    // Lager traegt 2 von 5 Binaries dieser Zelle -> der Planer erkennt PER BINARY genau 3 Fehlende
    // (LEDGER:3397), nie den Gesamt-Batch.
    std::map<std::string, std::string> objs;
    bl::LagerRunState                  st;
    load_state(st, objs, {eintrag(hexkey('a'), kZelle), eintrag(hexkey('c'), kZelle)});
    ASSERT_EQ(st.lager_size(), 2u);

    MiniView const          view{{"bin-a", "bin-b", "bin-c", "bin-d", "bin-e"}};
    bl::BinaryIdKeyFn const by_id = [](std::string const& id) -> std::optional<std::string> {
        return hexkey(id.back()); // bin-a -> 'a'... (abgeleitet, nicht je Fall getippt)
    };
    bl::PresenceFn const present = bl::make_lager_presence(st, bl::make_index_key_fn(view, by_id), kZelle);

    bl::SlicePlanQueue queue;
    {
        bl::SlicePlanner planner(queue, {0, 1, 2, 3, 4}, 8, present);
        planner.join();
    }
    auto const plan = queue.pop();
    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(plan->missing_count, 3u);
    EXPECT_LT(plan->missing_count, plan->view_indices.size()) << "Teilbestand: missing < Fenstergroesse.";

    // Und der BAU-FILTER baut exakt diese drei -- in Fenster-Reihenfolge (b, d, e).
    auto const gefiltert = bl::filter_window_for_build(plan->view_indices, present);
    EXPECT_EQ(gefiltert.zu_bauen, (std::vector<std::size_t>{1, 3, 4}));
    EXPECT_EQ(gefiltert.bestand_uebersprungen, 2u);
}

TEST(G3LagerPresence, LeeresLogErgibtVollesFenster) {
    // Leerer Bestand -> konservativ fehlt alles -> volles Fenster wird gebaut, nichts uebersprungen.
    std::map<std::string, std::string> objs;
    bl::LagerRunState                  st;
    load_state(st, objs, {});
    ASSERT_EQ(st.lager_size(), 0u);

    MiniView const          view{{"bin-a", "bin-b", "bin-c"}};
    bl::BinaryIdKeyFn const by_id = [](std::string const& id) -> std::optional<std::string> {
        return hexkey(id.back());
    };
    bl::PresenceFn const present = bl::make_lager_presence(st, bl::make_index_key_fn(view, by_id), kZelle);

    auto const gefiltert = bl::filter_window_for_build({0, 1, 2}, present);
    EXPECT_EQ(gefiltert.zu_bauen, (std::vector<std::size_t>{0, 1, 2}));
    EXPECT_EQ(gefiltert.bestand_uebersprungen, 0u);
}

TEST(G3LagerPresence, SkipZaehlerArithmetikStimmt) {
    // Die Buchungs-Invariante des Iterators: zu_bauen + uebersprungen == Fenster, und missing_count
    // des Planers == zu_bauen des Filters (EINE Miss-Wahrheit, zwei Aufrufer).
    std::map<std::string, std::string> objs;
    bl::LagerRunState                  st;
    load_state(st, objs, {eintrag(hexkey('b'), kZelle)});

    MiniView const          view{{"bin-a", "bin-b", "bin-c", "bin-d"}};
    bl::BinaryIdKeyFn const by_id = [](std::string const& id) -> std::optional<std::string> {
        return hexkey(id.back());
    };
    bl::PresenceFn const present = bl::make_lager_presence(st, bl::make_index_key_fn(view, by_id), kZelle);

    std::vector<std::size_t> const fenster{0, 1, 2, 3};
    auto const                     gefiltert = bl::filter_window_for_build(fenster, present);
    EXPECT_EQ(gefiltert.zu_bauen.size() + gefiltert.bestand_uebersprungen, fenster.size());

    bl::SlicePlanQueue queue;
    {
        bl::SlicePlanner planner(queue, fenster, 8, present);
        planner.join();
    }
    auto const plan = queue.pop();
    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(plan->missing_count, gefiltert.zu_bauen.size());
}
