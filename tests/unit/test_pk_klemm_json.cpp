// test_pk_klemm_json -- B07 (K01/F2-5, Designplan par.4 Z44 "PK-KlemmJson n-Spalten", Klasse K2
// anwesenheit-statt-bedingung): die PERZENTIL-KLEMM-Pfade des Kanons, beobachtet im JSON-Trace,
// mit vollem Spalten-Nenner.
//
// VORPOSTEN-EXPLORE (BAULISTE-Auflage, Ordnung 8 "D5-1 entsperrt PK-KanonWert -> PK-Kreuztest ->
// PK-KlemmJson"): beide Vorposten sind INHALTLICH GEDECKT durch test_d51_perzentil_kanon.cpp --
// PK-KanonWert == D51PerzentilKanon.KanonIndexGegenLehrbuchPin (+GeradeLaengeWertePin),
// PK-Kreuztest == D51PerzentilKanon.PercentileNsGegenZweitimplementiertesOrakel
// (+KreuzTestGeraderLaengeBitGleich). HIER NEU ist die KLEMM-Seite am JSON (test_d54_trace_schema
// prueft die Kanon-WERTE im Schema, aber keinen einzigen Klemm-Eingang).
//
// DIE KLEMM-PFADE DES KANONS (latency_stats.hpp, nearest_rank_index/percentile_ns):
//   (a) rang<=1 -> 0            : n=1 klemmt JEDES Quantil auf das eine Sample.
//   (b) rang>=n -> n-1          : die OBERE Klemme faellt genau dann, wenn ceil(q*n) > n-1+1 rundet
//                                 -- fuer q=0.99 bei n=50 (ceil(49.5)=50>=50). KONTRAST (K2-Heilform):
//                                 bei n=100 faellt sie NICHT (Index 98 = ZWEITgroesster Wert) -- eine
//                                 "immer max"-Fassung wuerde hier auffliegen.
//   (c) samples.empty() -> 0ns  : die leere Kurve schreibt 0 in ALLE ihre Felder, waehrend die
//                                 gefuellten Kurven echte Werte tragen (kein Zeilen-Abbruch).
//
// N-SPALTEN-NENNER: die Spaltenliste IST kLatenzFelder (tier_trace_schema.hpp, "REIHENFOLGE IST
// VERTRAG"): 9 Felder, hier ZUSAETZLICH als unabhaengig eingefrorene Literal-Liste (T-5: das Orakel
// zitiert nicht den Pruefling) UND in Positions-Bindung (jeder Schluessel VOR seinem Nachfolger).
//
// T-11c-MUTATIONSANKER: eine Spalte aus der Literal-Liste nehmen laesst den 9/9-Nenner brechen;
// die obere Klemme im Kanon lockern bricht KlemmeObenFaelltBeiFuenfzig literal.

#include "anatomy_commands/tier_trace_schema.hpp" // der Pruefling: Schema + schreibe_geteilten_json_vorspann

#include <gtest/gtest.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace {

namespace ts = ::comdare::cache_engine::builder::anatomy_commands::trace_schema;

/// Mini-Snapshot (Duck-Typing wie AbiFillLevelSnapshot: write_ns/read_ns/delete_ns + Korrelation).
struct Snap {
    std::vector<std::int64_t> write_ns;
    std::vector<std::int64_t> read_ns;
    std::vector<std::int64_t> delete_ns;
    std::uint64_t             observe_wall_ns = 42;
    double                    fill_level      = 0.5;
};

[[nodiscard]] std::string json_von(Snap const& cp) {
    std::ostringstream os;
    ts::schreibe_geteilten_json_vorspann(os, /*index=*/0, cp);
    os << "}";
    return os.str();
}

/// Der eine Feldwert "name":wert aus dem JSON (positionsunabhaengig gelesen, wertgenau).
[[nodiscard]] std::string feldwert(std::string const& json, std::string const& name) {
    std::string const nadel = "\"" + name + "\":";
    auto const        pos   = json.find(nadel);
    if (pos == std::string::npos) return "<FEHLT>";
    auto const start = pos + nadel.size();
    auto const ende  = json.find_first_of(",}", start);
    return json.substr(start, ende - start);
}

// Die 9 Spalten, UNABHAENGIG eingefroren (T-5) -- Reihenfolge == Vertrag (tier_trace_schema.hpp).
constexpr char const* kSpalten[9] = {"write_p50_ns", "write_p95_ns",  "write_p99_ns",  "read_p50_ns",  "read_p95_ns",
                                     "read_p99_ns",  "delete_p50_ns", "delete_p95_ns", "delete_p99_ns"};

TEST(PkKlemmJson, NeunSpaltenNennerInVertragsReihenfolge) {
    // Nenner-Gleichheit beider Quellen: Literal-Liste (9) == Schema-Groesse (fremde Quelle).
    ASSERT_EQ(std::size(kSpalten), ts::kLatenzFelder.size())
        << "Spalten-Nenner: das Schema hat die Spaltenzahl bewegt -- Literal-Liste nachziehen";
    Snap cp;
    cp.write_ns            = {1, 2, 3, 4};
    cp.read_ns             = {5, 6, 7, 8};
    cp.delete_ns           = {9, 10, 11, 12};
    std::string const json = json_von(cp);
    // 9/9 anwesend UND in Positions-Bindung (jede Spalte VOR ihrem Nachfolger -- Reihenfolge ist Vertrag).
    std::size_t vorher = 0;
    for (std::size_t i = 0; i < std::size(kSpalten); ++i) {
        auto const pos = json.find(std::string{"\""} + kSpalten[i] + "\":");
        ASSERT_NE(pos, std::string::npos) << kSpalten[i] << " fehlt im JSON (Nenner " << i << "/9)";
        if (i > 0) { EXPECT_GT(pos, vorher) << kSpalten[i] << " steht vor seinem Vorgaenger -- Vertragsbruch"; }
        vorher = pos;
    }
}

TEST(PkKlemmJson, KlemmeUntenEinSampleTraegtAlleQuantile) {
    // Klemm-Pfad (a): n=1 -> rang<=1 -> Index 0 fuer p50/p95/p99. Zellgenau je Kurve.
    Snap cp;
    cp.write_ns            = {111};
    cp.read_ns             = {222};
    cp.delete_ns           = {333};
    std::string const json = json_von(cp);
    for (char const* f : {"write_p50_ns", "write_p95_ns", "write_p99_ns"}) EXPECT_EQ(feldwert(json, f), "111") << f;
    for (char const* f : {"read_p50_ns", "read_p95_ns", "read_p99_ns"}) EXPECT_EQ(feldwert(json, f), "222") << f;
    for (char const* f : {"delete_p50_ns", "delete_p95_ns", "delete_p99_ns"}) EXPECT_EQ(feldwert(json, f), "333") << f;
}

TEST(PkKlemmJson, KlemmeObenFaelltBeiFuenfzigUndNichtBeiHundert) {
    // Klemm-Pfad (b) MIT KONTRAST (K2-Heilform: die Bedingung, nicht die Anwesenheit).
    // n=50, Werte 1000*(k+1): p99-Rang ceil(0.99*50)=ceil(49.5)=50 >= n -> KLEMME -> Index 49 -> 50000 (max).
    Snap klemmt;
    for (std::int64_t k = 0; k < 50; ++k) klemmt.write_ns.push_back(1000 * (k + 1));
    klemmt.read_ns   = {1};
    klemmt.delete_ns = {1};
    EXPECT_EQ(feldwert(json_von(klemmt), "write_p99_ns"), "50000")
        << "n=50: die obere Klemme muss auf das Maximum fallen";

    // KONTRAST n=100: p99-Rang ceil(99.0)=99 < 100 -> KEINE Klemme -> Index 98 -> 99000 (ZWEITgroesster).
    Snap frei;
    for (std::int64_t k = 0; k < 100; ++k) frei.write_ns.push_back(1000 * (k + 1));
    frei.read_ns   = {1};
    frei.delete_ns = {1};
    EXPECT_EQ(feldwert(json_von(frei), "write_p99_ns"), "99000")
        << "n=100: OHNE Klemmfall ist p99 der zweitgroesste Wert -- eine 'immer max'-Fassung faellt hier auf";
}

TEST(PkKlemmJson, LeereKurveSchreibtNullOhneZeilenAbbruch) {
    // Klemm-Pfad (c): delete_ns LEER -> alle drei delete_-Felder 0, die gefuellten Kurven unberuehrt.
    Snap cp;
    cp.write_ns            = {700, 800};
    cp.read_ns             = {500};
    cp.delete_ns           = {};
    std::string const json = json_von(cp);
    for (char const* f : {"delete_p50_ns", "delete_p95_ns", "delete_p99_ns"})
        EXPECT_EQ(feldwert(json, f), "0") << f << ": leere Kurve muss 0 tragen, nicht fehlen";
    EXPECT_EQ(feldwert(json, "read_p50_ns"), "500") << "die gefuellte Nachbar-Kurve bleibt unberuehrt";
    EXPECT_EQ(feldwert(json, "write_p99_ns"), "800") << "n=2, p99: Rang ceil(1.98)=2 >= n -> Klemme -> max";
    // Und der Zeilen-Rahmen haelt (kein Abbruch mitten im Objekt):
    EXPECT_EQ(json.back(), '}');
    EXPECT_NE(json.find("\"checkpoint\":0"), std::string::npos);
}

} // namespace
