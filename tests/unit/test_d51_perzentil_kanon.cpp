// D5-1 PERZENTIL-KANON (2026-08-09) -- die EINE Perzentil-Definition des Mess-Baums.
//
// SELBSTCHECK DIESER DATEI
//   ZUSICHERT: (1) stats::nearest_rank_index/percentile_ns rechnen die LEHRBUCH-Formel
//              k = ceil(q*n) - 1 auf dem aufsteigend sortierten Feld (0-basiert), belegt gegen
//              eine ZWEITIMPLEMENTIERTE, definitionsbasierte Referenz (Brute Force, anderer
//              Algorithmus) und gegen von Hand gerechnete Pins;
//              (2) im Auswertungs-Baum (libs/cache_engine/builder + libs/cache_engine/harness)
//              existiert GENAU EINE Quantil->Index-Umrechnung ("Definitionen: 1");
//              (3) der Name nearest_rank_p kommt in libs/cache_engine NICHT MEHR vor;
//              (4) die real rendernden Ausgabepfade (workload-CSV, ABI-Trace-JSON, merged_p50_ns)
//              liefern bei GERADER Stichprobenlaenge exakt den Kanon-Wert (Bit-Gleichheit auf
//              std::int64_t);
//              (5) die Median-Regel des best_binary_selector, vals[(n-1)/2], IST der Kanon-Fall
//              q=0.5 -- als Zahlengleichheit ueber n=1..64, nicht als Behauptung.
//   ZUSICHERT NICHT: nichts ueber die super-Werkzeuge (Code/04_csv_to_latex, Code/05_diagram_generator)
//              -- die liegen in einem anderen Repo und haengen dort an einem eigenen, VERALTETEN
//              ce-Vendor-Stand; das ist Paket D5-3/D5-2, nicht D5-1. Nichts ueber die vier weiteren
//              Median-Bauarten (best_binary_selector-Binary, eta_kalibrierung::median_t_s,
//              HDR-Histogramm, Iterator-Gruppenmedian) -- das ist D5-2/D5-5. Nichts ueber die
//              ALTBESTAENDE an Messdaten: die bleiben liegen und werden nur als ungueltig MARKIERT.
//              Der Form-Zensus (2) sieht nur die zwei genannten Wurzeln; organ_axes/, ext/ und tests/
//              sind bewusst ausserhalb.
//
// TDD-VERTRAG: T-1 (diese Datei lief ROT, bevor der Kanon gebaut wurde), T-2 (jede Zusicherung
// prueft einen WERT), T-3 (die Vergleichszahlen sind von Hand nach der Lehrbuch-Definition
// gerechnet, nicht aus dem Pruefling gezogen), T-5 (das Orakel ist zweitimplementiert:
// definitionsbasierte Suche statt Index-Formel).
//
// SEED: kSeed ist EINMAL aus /dev/urandom gewuerfelt (nicht aus einer Doku abgeschrieben) und wird
// bei jedem Lauf GEDRUCKT -- ein Fehlschlag ist damit exakt nachstellbar.

#include <builder/anatomy_commands/tier_observe_trace_abi.hpp>
#include <builder/commands/latency_stats.hpp>
#include <builder/measurement_snapshot.hpp>
#include <builder/workload_driver/workload_orchestrator.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#ifndef COMDARE_D51_ZENSUS_ROOT_BUILDER
#error "D5-1: COMDARE_D51_ZENSUS_ROOT_BUILDER (Pfad libs/cache_engine/builder) muss gesetzt sein."
#endif
#ifndef COMDARE_D51_ZENSUS_ROOT_HARNESS
#error "D5-1: COMDARE_D51_ZENSUS_ROOT_HARNESS (Pfad libs/cache_engine/harness) muss gesetzt sein."
#endif
#ifndef COMDARE_D51_ZENSUS_ROOT_CE
#error "D5-1: COMDARE_D51_ZENSUS_ROOT_CE (Pfad libs/cache_engine) muss gesetzt sein."
#endif

namespace stats = ::comdare::cache_engine::builder::commands::stats;
namespace ac    = ::comdare::cache_engine::builder::anatomy_commands;
namespace wd    = ::comdare::cache_engine::builder::workload_driver;
namespace bd    = ::comdare::cache_engine::builder::detail;

namespace {

// EINMAL gewuerfelt: `od -An -N8 -tu8 /dev/urandom` am 2026-08-09.
constexpr std::uint64_t kSeed = 3302311281923860264ULL;

// =============================================================================================
// ORAKEL (T-5) -- ZWEITIMPLEMENTIERT aus der DEFINITION, nicht aus der Index-Formel.
//
// Nearest-Rank-Perzentil, Lehrbuch: P(q) ist der KLEINSTE Stichprobenwert v, fuer den mindestens
// R = ceil(q*n) Stichprobenwerte <= v sind. Diese Referenz sucht v durch ZAEHLEN (Brute Force,
// O(n^2)) -- sie kennt keinen Index, keine Sortier-Position und keine Formel des Pruefliings.
//
// KOEDER-ANKER: die Rang-Rechnung hat GENAU ZWEI Summanden --
//   Summand 1 = das Produkt q*n unter dem ceil,  Summand 2 = die Rang-Korrektur (+ 0).
// Der Mutations-Koeder addiert delta = +-1 auf GENAU EINEN davon und muss den Test rot faerben.
// =============================================================================================
[[nodiscard]] std::size_t orakel_rang(double q, std::size_t n) {
    if (n == 0) return 0;
    if (q <= 0.0) return 1;
    if (q >= 1.0) return n;
    // ceil ohne <cmath>: ganzzahlig hochzaehlen, bis das Produkt erreicht ist. Bewusst eine ANDERE
    // Mechanik als std::ceil im Pruefling -- eine gemeinsame Bibliotheksfunktion waere ein
    // gemeinsamer Fehlerpfad.
    long double const ziel = static_cast<long double>(q) * static_cast<long double>(n);
    std::size_t       r    = 1;
    while (static_cast<long double>(r) + 1e-12L < ziel) ++r;
    return r + 0; // Summand 2 (Rang-Korrektur) -- Koeder-Angriffsstelle
}

[[nodiscard]] std::int64_t orakel_perzentil(std::vector<std::int64_t> const& s, double q) {
    if (s.empty()) return 0;
    std::size_t const rang = orakel_rang(q, s.size());
    std::int64_t      best = 0;
    bool              hat  = false;
    for (std::int64_t kandidat : s) {
        std::size_t anzahl = 0;
        for (std::int64_t x : s) {
            if (x <= kandidat) ++anzahl;
        }
        if (anzahl >= rang && (!hat || kandidat < best)) {
            best = kandidat;
            hat  = true;
        }
    }
    return hat ? best : *std::max_element(s.begin(), s.end());
}

// =============================================================================================
// QUELLTEXT-ZENSUS -- whitespace-kollabiert, damit clang-format-Umbrueche ihn nicht blenden.
// =============================================================================================
[[nodiscard]] std::string kollabiert(std::filesystem::path const& p) {
    std::ifstream in(p, std::ios::binary);
    std::string   roh((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::string   out;
    out.reserve(roh.size());
    bool letzte_war_space = false;
    for (char c : roh) {
        bool const ist_space = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
        if (ist_space) {
            if (!letzte_war_space) out.push_back(' ');
            letzte_war_space = true;
        } else {
            out.push_back(c);
            letzte_war_space = false;
        }
    }
    return out;
}

[[nodiscard]] bool ist_quelldatei(std::filesystem::path const& p) {
    auto const e = p.extension().string();
    return e == ".hpp" || e == ".cpp" || e == ".h" || e == ".cc";
}

/// Zaehlt die Stellen, an denen ein Gleitkomma-Ausdruck in einen Feld-Index umgerechnet wird --
/// die SHAPE einer Perzentil-Definition. Fenster = 120 kollabierte Zeichen hinter dem Index-Cast.
[[nodiscard]] std::size_t zaehle_index_umrechnungen(std::string const& text) {
    constexpr std::string_view kIndexCast  = "static_cast<std::size_t>(";
    constexpr std::string_view kDoubleCast = "static_cast<double>(";
    constexpr std::size_t      kFenster    = 120;
    std::size_t                treffer     = 0;
    for (std::size_t pos = text.find(kIndexCast); pos != std::string::npos; pos = text.find(kIndexCast, pos + 1)) {
        std::size_t const      ende = std::min(text.size(), pos + kIndexCast.size() + kFenster);
        std::string_view const fenster{text.data() + pos, ende - pos};
        if (fenster.find(kDoubleCast) != std::string_view::npos) ++treffer;
    }
    return treffer;
}

[[nodiscard]] std::size_t zaehle_vorkommen(std::string const& text, std::string_view muster) {
    std::size_t n = 0;
    for (std::size_t pos = text.find(muster); pos != std::string::npos; pos = text.find(muster, pos + 1)) ++n;
    return n;
}

// =============================================================================================
// Token-Extraktion aus den REAL rendernden Ausgabepfaden (kein Nachbau -- der echte Serialisierer).
// =============================================================================================
[[nodiscard]] std::int64_t json_feld(std::string const& js, std::string_view name) {
    std::string const schluessel = "\"" + std::string(name) + "\":";
    std::size_t const pos        = js.find(schluessel);
    if (pos == std::string::npos) return -999999;
    return std::stoll(js.substr(pos + schluessel.size()));
}

[[nodiscard]] std::vector<std::string> csv_spalten(std::string const& zeile) {
    std::vector<std::string> out;
    std::string              akt;
    for (char c : zeile) {
        if (c == ',') {
            out.push_back(akt);
            akt.clear();
        } else {
            akt.push_back(c);
        }
    }
    out.push_back(akt);
    return out;
}

[[nodiscard]] std::vector<std::int64_t> gewuerfelte_stichprobe(std::mt19937_64& rng, std::size_t n) {
    std::uniform_int_distribution<std::int64_t> dist(0, 5000);
    std::vector<std::int64_t>                   v(n);
    for (auto& x : v) x = dist(rng);
    return v;
}

} // namespace

// =================================================================================================
// (1) Der Kanon-Index gegen von Hand gerechnete Lehrbuch-Pins (T-3: Zahlen sind FREMD gerechnet).
// =================================================================================================
TEST(D51PerzentilKanon, KanonIndexGegenLehrbuchPin) {
    // k = ceil(q*n) - 1, 0-basiert. Von Hand: n=100,q=0.5 -> ceil(50)-1 = 49. n=100,q=0.95 -> 94.
    // n=100,q=0.99 -> 98. n=4,q=0.5 -> ceil(2)-1 = 1. n=6,q=0.5 -> ceil(3)-1 = 2.
    EXPECT_EQ(stats::nearest_rank_index(100, 0.50), 49u);
    EXPECT_EQ(stats::nearest_rank_index(100, 0.95), 94u);
    EXPECT_EQ(stats::nearest_rank_index(100, 0.99), 98u);
    EXPECT_EQ(stats::nearest_rank_index(100, 0.999), 99u);
    EXPECT_EQ(stats::nearest_rank_index(4, 0.50), 1u);
    EXPECT_EQ(stats::nearest_rank_index(6, 0.50), 2u);
    EXPECT_EQ(stats::nearest_rank_index(6, 0.25), 1u);  // ceil(1.5)-1 = 1
    EXPECT_EQ(stats::nearest_rank_index(6, 0.75), 4u);  // ceil(4.5)-1 = 4
    EXPECT_EQ(stats::nearest_rank_index(10, 0.10), 0u); // ceil(1)-1 = 0
    EXPECT_EQ(stats::nearest_rank_index(10, 0.90), 8u); // ceil(9)-1 = 8
    EXPECT_EQ(stats::nearest_rank_index(7, 0.00), 0u);
    EXPECT_EQ(stats::nearest_rank_index(7, 1.00), 6u);
    EXPECT_EQ(stats::nearest_rank_index(1, 0.50), 0u);
}

// =================================================================================================
// (2) Perzentil-WERTE bei GERADER Laenge -- der Fall, in dem sich die Konventionen unterscheiden.
// =================================================================================================
TEST(D51PerzentilKanon, GeradeLaengeWertePin) {
    std::vector<std::int64_t> const vier{10, 20, 30, 40};
    // Lehrbuch von Hand: p50 -> Index 1 -> 20 (UNTERE Mitte). round(q*(n-1)) haette 30 geliefert.
    EXPECT_EQ(stats::percentile_ns(vier, 0.50).count(), 20);
    EXPECT_EQ(stats::percentile_ns(vier, 0.25).count(), 10); // ceil(1)-1 = 0
    EXPECT_EQ(stats::percentile_ns(vier, 0.75).count(), 30); // ceil(3)-1 = 2
    EXPECT_EQ(stats::percentile_ns(vier, 0.95).count(), 40);
    EXPECT_EQ(stats::percentile_ns(vier, 0.99).count(), 40);

    std::vector<std::int64_t> const sechs{5, 7, 11, 13, 17, 19};
    EXPECT_EQ(stats::percentile_ns(sechs, 0.50).count(), 11);
    EXPECT_EQ(stats::percentile_ns(sechs, 0.25).count(), 7);
    EXPECT_EQ(stats::percentile_ns(sechs, 0.75).count(), 17);
    EXPECT_EQ(stats::percentile_ns(sechs, 0.99).count(), 19);

    std::vector<std::int64_t> hundert(100);
    for (int i = 0; i < 100; ++i) hundert[static_cast<std::size_t>(i)] = i + 1;
    // DIESE VIER ZAHLEN SIND DER PIN-WECHSEL DES PAKETS: vorher 51 / 96 / 100, jetzt 50 / 95 / 99.
    EXPECT_EQ(stats::percentile_ns(hundert, 0.50).count(), 50);
    EXPECT_EQ(stats::percentile_ns(hundert, 0.95).count(), 95);
    EXPECT_EQ(stats::percentile_ns(hundert, 0.99).count(), 99);
    EXPECT_EQ(stats::percentile_ns(hundert, 1.00).count(), 100);
    EXPECT_EQ(stats::percentile_ns(hundert, 0.00).count(), 1);
    EXPECT_EQ(stats::percentile_ns(std::vector<std::int64_t>{}, 0.5).count(), 0);
}

// =================================================================================================
// (3) Zufalls-Seed-Test gegen das ZWEITIMPLEMENTIERTE Orakel. Seed wird gedruckt.
// =================================================================================================
TEST(D51PerzentilKanon, PercentileNsGegenZweitimplementiertesOrakel) {
    std::cout << "[D5-1] Seed: " << kSeed << "\n";
    std::mt19937_64                rng(kSeed);
    std::vector<double> const      quantile{0.0, 0.01, 0.1, 0.25, 0.5, 0.75, 0.9, 0.95, 0.99, 0.999, 1.0};
    std::vector<std::size_t> const laengen{1, 2, 3, 4, 5, 6, 8, 10, 16, 17, 32, 33, 50, 64, 99, 100, 128};
    std::size_t                    faelle = 0;
    for (std::size_t runde = 0; runde < 20; ++runde) {
        for (std::size_t n : laengen) {
            auto const probe = gewuerfelte_stichprobe(rng, n);
            for (double q : quantile) {
                ++faelle;
                ASSERT_EQ(stats::percentile_ns(probe, q).count(), orakel_perzentil(probe, q))
                    << "Seed " << kSeed << " Runde " << runde << " n=" << n << " q=" << q;
            }
        }
    }
    std::cout << "[D5-1] Orakel-Faelle geprueft: " << faelle << "\n";
    EXPECT_EQ(faelle, 20u * 17u * 11u);
}

// =================================================================================================
// (4) DIE WACHE: genau EINE Quantil->Index-Umrechnung im Auswertungs-Baum + Name restlos getilgt.
// =================================================================================================
TEST(D51PerzentilKanon, DefinitionenWacheDrucktEins) {
    std::vector<std::filesystem::path> const wurzeln{COMDARE_D51_ZENSUS_ROOT_BUILDER, COMDARE_D51_ZENSUS_ROOT_HARNESS};
    std::size_t                              definitionen = 0;
    std::size_t                              dateien      = 0;
    std::vector<std::string>                 fundstellen;
    for (auto const& wurzel : wurzeln) {
        ASSERT_TRUE(std::filesystem::exists(wurzel)) << "Zensus-Wurzel fehlt: " << wurzel.string();
        for (auto const& e : std::filesystem::recursive_directory_iterator(wurzel)) {
            if (!e.is_regular_file() || !ist_quelldatei(e.path())) continue;
            ++dateien;
            std::size_t const n = zaehle_index_umrechnungen(kollabiert(e.path()));
            if (n != 0) {
                definitionen += n;
                fundstellen.push_back(e.path().string() + " x" + std::to_string(n));
            }
        }
    }
    std::cout << "[D5-1] Zensus-Wurzeln: " << wurzeln.size() << ", Quelldateien: " << dateien << "\n";
    for (auto const& f : fundstellen) std::cout << "[D5-1]   Fundstelle: " << f << "\n";
    std::cout << "[D5-1] Definitionen: " << definitionen << "\n";
    EXPECT_EQ(definitionen, 1u) << "Es darf GENAU EINE Perzentil-Definition geben (D5-1-Kanon).";
}

TEST(D51PerzentilKanon, NearestRankPIstErsatzlosGeloescht) {
    // GEZAEHLT WIRD DIE CODE-FORM `nearest_rank_p(` -- Deklaration ODER Aufruf. Sie muss 0 sein:
    // ersatzlos geloescht heisst, dass keine Uebersetzungseinheit den Namen mehr aufloesen kann.
    // Die BLOSSE ERWAEHNUNG ohne Klammer bleibt ausdruecklich erlaubt und wird nur GEDRUCKT: die
    // Kopfkommentare halten fest, WELCHE Formel warum verworfen wurde ("Doku wird nie geloescht").
    std::filesystem::path const wurzel{COMDARE_D51_ZENSUS_ROOT_CE};
    ASSERT_TRUE(std::filesystem::exists(wurzel)) << "Zensus-Wurzel fehlt: " << wurzel.string();
    std::size_t              code_form  = 0;
    std::size_t              erwaehnung = 0;
    std::vector<std::string> fundstellen;
    for (auto const& e : std::filesystem::recursive_directory_iterator(wurzel)) {
        if (!e.is_regular_file() || !ist_quelldatei(e.path())) continue;
        std::string const text = kollabiert(e.path());
        erwaehnung += zaehle_vorkommen(text, "nearest_rank_p");
        std::size_t const n = zaehle_vorkommen(text, "nearest_rank_p(");
        if (n != 0) {
            code_form += n;
            fundstellen.push_back(e.path().string() + " x" + std::to_string(n));
        }
    }
    for (auto const& f : fundstellen) std::cout << "[D5-1]   nearest_rank_p( noch in: " << f << "\n";
    std::cout << "[D5-1] nearest_rank_p Code-Form in libs/cache_engine: " << code_form
              << " (blosse Erwaehnungen im Kommentar: " << erwaehnung << ")\n";
    EXPECT_EQ(code_form, 0u) << "nearest_rank_p wird ERSATZLOS geloescht -- kein Alias, keine Weiterleitung.";
}

// =================================================================================================
// (5) KREUZ-TEST GERADER LAENGE: die real rendernden Ausgabepfade gegen den Kanon, Bit-Gleichheit.
// =================================================================================================
TEST(D51PerzentilKanon, KreuzTestGeraderLaengeBitGleich) {
    std::mt19937_64 rng(kSeed ^ 0x5bf03635ULL);
    // GERADE Laenge ist Pflicht: bei ungerader Laenge stimmen fast alle Median-Konventionen ueberein,
    // der Kreuz-Test waere blind.
    std::vector<std::int64_t> const probe = gewuerfelte_stichprobe(rng, 40);
    ASSERT_EQ(probe.size() % 2, 0u);

    std::int64_t const kanon_p50 = stats::percentile_ns(probe, 0.50).count();
    std::int64_t const kanon_p95 = stats::percentile_ns(probe, 0.95).count();
    std::int64_t const kanon_p99 = stats::percentile_ns(probe, 0.99).count();
    std::cout << "[D5-1] Kreuz-Test n=" << probe.size() << " Kanon p50/p95/p99 = " << kanon_p50 << '/' << kanon_p95
              << '/' << kanon_p99 << "\n";

    // (a) Der ECHTE Lastprofil-CSV-Serialisierer.
    wd::WorkloadRunResult r;
    r.profile_name        = "d51";
    r.op_count            = probe.size();
    r.insert_ns           = probe;
    r.lookup_ns           = probe;
    std::string const csv = wd::serialize_workload_run_results_csv(std::vector<wd::WorkloadRunResult>{r});
    std::size_t const nl  = csv.find('\n');
    ASSERT_NE(nl, std::string::npos);
    auto const spalten = csv_spalten(csv.substr(nl + 1));
    ASSERT_GE(spalten.size(), 12u);
    EXPECT_EQ(std::stoll(spalten[4]), kanon_p50) << "insert_p50_ns der Mess-CSV";
    EXPECT_EQ(std::stoll(spalten[5]), kanon_p95) << "insert_p95_ns der Mess-CSV";
    EXPECT_EQ(std::stoll(spalten[6]), kanon_p99) << "insert_p99_ns der Mess-CSV";
    EXPECT_EQ(std::stoll(spalten[8]), kanon_p50) << "lookup_p50_ns der Mess-CSV";

    // (b) Der ECHTE ABI-Trace-JSON-Serialisierer.
    ac::AbiTierObserveTrace  trace;
    ac::AbiFillLevelSnapshot cp;
    cp.fill_level = 40;
    cp.write_ns   = probe;
    cp.read_ns    = probe;
    cp.delete_ns  = probe;
    trace.checkpoints.push_back(cp);
    std::string const js = ac::serialize_abi_tier_trace_json(trace);
    EXPECT_EQ(json_feld(js, "write_p50_ns"), kanon_p50);
    EXPECT_EQ(json_feld(js, "write_p95_ns"), kanon_p95);
    EXPECT_EQ(json_feld(js, "write_p99_ns"), kanon_p99);
    EXPECT_EQ(json_feld(js, "read_p50_ns"), kanon_p50);
    EXPECT_EQ(json_feld(js, "delete_p50_ns"), kanon_p50);

    // (c) Der Median-Konnektor des Mess-PODs (merged_p50_ns).
    wd::WorkloadRunResult m;
    m.insert_ns = probe;
    EXPECT_EQ(bd::merged_p50_ns(m), kanon_p50);
}

// =================================================================================================
// (6) Der Median ist der Fall q=0.5 -- und die Regel des best_binary_selector IST dieser Fall.
// =================================================================================================
TEST(D51PerzentilKanon, MedianIstQGleichEinHalb) {
    // best_binary_selector.cpp waehlt vals[(n-1)/2] ("UNTERE Mitte"). Behauptung: das ist exakt
    // ceil(0.5*n)-1 fuer JEDES n -- also keine eigene Definition, sondern der Kanon-Fall q=0.5.
    for (std::size_t n = 1; n <= 64; ++n) { ASSERT_EQ(stats::nearest_rank_index(n, 0.5), (n - 1) / 2) << "n=" << n; }
    // Und der Median als Wert: {10,20,30,40} -> 20 (nicht 25, nicht 30).
    std::vector<std::int64_t> const vier{10, 20, 30, 40};
    EXPECT_EQ(stats::latency_p50_ns(vier).count(), 20);
    EXPECT_EQ(stats::latency_p50_ns(vier).count(), stats::percentile_ns(vier, 0.5).count());
}
