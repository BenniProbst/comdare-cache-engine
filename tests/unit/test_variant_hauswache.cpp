// test_variant_hauswache -- A1-RIEGEL (#117, Owner-Order 21.08.2026): der HAUSWEITE
// std::variant-Quelltext-Scan ueber libs/cache_engine, rekursiv, kommentarfest, mit
// exakt eingefrorener Allowlist.
//
// ============================================================================================
// WARUM ES DIESE WACHE BRAUCHT (Befund backups-workflow/20260821-a1-variant-audit/befund.md)
// ============================================================================================
// Das A1-Audit hat bewiesen: die Tier-Binary-Bauform ist heute variant-frei (BFS ueber die
// transitiven Include-Huellen aller drei emittierten Tier-TU-Formen, Schnitt mit allen
// variant-Traegern leer). Aber KEIN Test sicherte diesen Zustand: die HY-A1-Zonen-Wache
// (test_hy_a1_dock_contract.cpp, Test E) scannt AUSDRUECKLICH nur hybrid/, und Block D des
// Striktheits-Guards (test_striktheit_metaprog_guard.cpp:78-95) ist ein Typ-Praedikat ueber
// die Top-Level-Typen der 11 Referenz-Compositions -- ein neuer variant in organ_axes/,
// anatomy/, topics/, builder/ oder measurement/ blieb testseitig unsichtbar. Diese TU
// schliesst genau diese Luecke: sie uebernimmt die Bauform der Zonen-Wache (Quelltext lesen,
// Kommentare entfernen, STELLEN zaehlen -- nicht Dateien) und weitet sie auf den ganzen
// libs/cache_engine-Baum aus.
//
// DIE ALLOWLIST IST EINGEFROREN (D2-G4/Z-Muster, beide Richtungen benannt):
//   ist > soll  == SCHLEICH-VARIANT (jemand hat in einer erlaubten Datei nachgelegt),
//   ist < soll  == TOTE AUSNAHME   (die Ausnahme hat ihren Gegenstand verloren -- Zahl
//                                   bewusst mitziehen, nicht stillschweigend gruen bleiben).
// Die Soll-Zahlen wurden am Objekt erhoben (unabhaengiges Orakel-Skript, 22.08.2026, Stand
// ed9f1a3c) und hier als Literale eingefroren (T-5: das Orakel rechnet nicht im Test).
//
// WAS DIESE WACHE NICHT KANN, ausdruecklich benannt: sie liest QUELLTEXT, keine Typen. Eine
// variant-Nutzung ohne eines der Muster (z.B. ueber einen Alias aus einem Fremd-Header oder
// exotisches Whitespace im include) faengt sie nicht -- dafuer stehen daneben weiter die
// HY-A1-Zonen-Wache (Datei-Identitaets-Aussage in hybrid/) und Block D (Typ-Praedikat).
// Drei Wachen, drei Blickwinkel; keine ersetzt eine andere.
//
// DIE builder!=build-FALLE (Fallen-Register): uebersprungen werden NUR Verzeichnisse, die
// exakt "build" heissen oder mit "build-" beginnen -- NIE "builder". Die Gegenprobe unten
// verlangt eine dreistellige builder/-Dateizahl im Nenner; frisst ein kuenftiger Filter den
// builder/-Baum, wird die Wache ROT statt still kleiner.
//
// @autoritaet Owner-Order 21.08.2026 (A1: variant-Audit vor GN-9) + par.23 (variant-Verbot
//             fuer statische/Organ-Achsen); Ausnahmen-Kanon: Owner-E1 (hybrid_dock_array),
//             par.23-Quarantaene-Cluster, Expected/Result-Fehler-Summen (BuildError).
// @befund     ~/backups-workflow/20260821-a1-variant-audit/befund.md (Abschnitte 1, 2, 4)

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string g_wurzel; // argv[1]: Pfad zu libs/cache_engine (der zu scannende Quellbaum)

// ============================================================================================
// DIE EINGEFRORENE ALLOWLIST -- Pfade relativ zur Scan-Wurzel, generic-Separatoren ('/')
// ============================================================================================
struct AllowEintrag {
    std::string_view rel;
    std::size_t soll{}; // NSDMI: cppcheck uninitMemberVarNoCtor (CI 16085 Job 382984); Verwendung bleibt Aggregat-Init
    std::string_view grund;
};

constexpr std::array<AllowEintrag, 8> kAllowlist{{
    {"hybrid/hybrid_dock_array.hpp", 4,
     "HY-A1: die EINE owner-erlaubte variant-Stelle des Hauses (Owner-E1; include + Typ + 2x visit)"},
    {"include/cache_engine/abi/algorithm_baustein.hpp", 3,
     "par.23-QUARANTAENE, test-only (REV-7.6-Cluster; include + Typ + variant_alternative)"},
    {"include/cache_engine/abi/baustein_variants.hpp", 0,
     "par.23-QUARANTAENE: variants NUR via algorithm_axis-Indirektion -- 0 direkte Muster-Stellen; "
     "ein direkter std::variant-Einbau hier wuerde beissen (Negativ-Zusicherung)"},
    {"include/cache_engine/concepts/pressure_state.hpp", 2,
     "par.23-QUARANTAENE (REV-5.2-Aera State-Pattern), test-only (include + Typ)"},
    {"include/cache_engine/measurement/axis_error.hpp", 6,
     "Expected/Result-Naht: BuildError + ProbeErrorSum (include + 2x Typ + 3x visit)"},
    {"include/cache_engine/measurement/numa_page_probe.hpp", 2,
     "Expected/Result-Naht: Fehler-Summe der Probe (include + visit)"},
    {"include/cache_engine/measurement/numa_cpu_pin_process_probe.hpp", 2,
     "Expected/Result-Naht: Fehler-Summe der Probe (include + visit)"},
    {"system_axes/operating_system_probe.hpp", 2, "Expected/Result-Naht: BuildError der OS-Probe (include + visit)"},
}};

// Die Gesamt-Stellen-Zahl ist an die Tafel GEBUNDEN: wer eine Soll-Zahl aendert, zieht die
// Summe compile-hart mit (und umgekehrt) -- zwei Zahlen, eine Wahrheit.
constexpr std::size_t kSollStellenGesamt = []() {
    std::size_t s = 0;
    for (auto const& e : kAllowlist) s += e.soll;
    return s;
}();
static_assert(kSollStellenGesamt == 21,
              "HAUSWACHE: die eingefrorene Soll-Summe (Orakel 22.08.2026 @ ed9f1a3c) ist 21. Wer eine "
              "Allowlist-Zeile aendert, aendert BEIDE Zahlen bewusst -- nie nur eine.");

// Nenner-Untergrenzen (Stand der Erhebung: 1324 Dateien gesamt, davon 170 unter builder/).
// Bewusst mit Puffer nach unten: Umzuege duerfen schrumpfen, ein leergelaufener Scan nicht.
constexpr std::size_t kMinDateienGesamt  = 1000;
constexpr std::size_t kMinBuilderDateien = 100;

// ============================================================================================
// MUSTER-SATZ -- deckt alle Verwendungs-Formen des A1-Zensus (Beschaffung, Typ, Zugriff)
// ============================================================================================
constexpr std::array<std::string_view, 8> kMuster{{
    "std::variant<",
    "std::visit",
    "std::monostate",
    "std::get_if",
    "std::holds_alternative",
    "std::variant_alternative",
    "include <variant>",
    "include<variant>",
}};

constexpr std::array<std::string_view, 9> kEndungen{
    {".hpp", ".cpp", ".h", ".cc", ".hh", ".ipp", ".tpp", ".inl", ".cxx"}};

[[nodiscard]] bool endung_erfasst(std::string const& dateiname) {
    for (auto const e : kEndungen) {
        if (dateiname.size() >= e.size() && dateiname.compare(dateiname.size() - e.size(), e.size(), e) == 0)
            return true;
    }
    return false;
}

// Kommentar-Entferner in der Bauform der HY-A1-Zonen-Wache (test_hy_a1_dock_contract.cpp):
// //-Zeilenreste fallen, /* */-Bloecke fallen (auch mehrzeilig), Code-Reste bleiben.
[[nodiscard]] std::string ohne_kommentare(std::string const& roh) {
    std::string        raus;
    bool               in_block = false;
    std::istringstream zeilen{roh};
    std::string        z;
    while (std::getline(zeilen, z)) {
        std::string sauber;
        for (std::size_t i = 0; i < z.size(); ++i) {
            if (in_block) {
                if (i + 1 < z.size() && z[i] == '*' && z[i + 1] == '/') {
                    in_block = false;
                    ++i;
                }
                continue;
            }
            if (i + 1 < z.size() && z[i] == '/' && z[i + 1] == '/') break; // Zeilen-Kommentar
            if (i + 1 < z.size() && z[i] == '/' && z[i + 1] == '*') {
                in_block = true;
                ++i;
                continue;
            }
            sauber += z[i];
        }
        raus += sauber;
        raus += '\n';
    }
    return raus;
}

// ALLE Vorkommen ALLER Muster zaehlen (Fortschritt pos+1 wie das Orakel -- identische Arithmetik,
// sonst waere die eingefrorene Tafel gegen eine andere Zaehlung geeicht).
[[nodiscard]] std::size_t stellen_zaehlen(std::string const& inhalt) {
    std::size_t n = 0;
    for (auto const m : kMuster) {
        for (std::size_t pos = inhalt.find(m); pos != std::string::npos; pos = inhalt.find(m, pos + 1)) ++n;
    }
    return n;
}

} // namespace

// ============================================================================================
// (A) DIE HAUSWEITE BILANZ -- ein Durchlauf, drei Urteile je Datei, vier Nenner-Zusicherungen
// ============================================================================================
TEST(VariantHauswache, HausweiteBilanzMitEingefrorenerAllowlist) {
    namespace fs = std::filesystem;
    fs::path const wurzel{g_wurzel};
    ASSERT_TRUE(fs::is_directory(wurzel)) << wurzel << " -- Scan-Wurzel fehlt (argv[1] pruefen)";

    std::map<std::string, std::size_t> ist;                 // rel-Pfad -> Stellen (nur Traeger)
    std::size_t                        dateien_gesamt  = 0; // Nenner
    std::size_t                        stellen_gesamt  = 0;
    std::size_t                        builder_dateien = 0; // Gegenprobe zur builder!=build-Falle
    std::vector<std::string>           gescannt_rel;        // fuer die Allowlist-Deckungs-Pruefung

    for (auto it = fs::recursive_directory_iterator{wurzel}; it != fs::recursive_directory_iterator{}; ++it) {
        if (it->is_directory()) {
            std::string const name = it->path().filename().string();
            // NUR exakte Bau-Artefakt-Namen ueberspringen -- NIE "builder" (Fallen-Register).
            if (name == "build" || name.rfind("build-", 0) == 0) it.disable_recursion_pending();
            continue;
        }
        if (!it->is_regular_file()) continue;
        std::string const dateiname = it->path().filename().string();
        if (!endung_erfasst(dateiname)) continue;

        ++dateien_gesamt;
        std::string const rel = fs::relative(it->path(), wurzel).generic_string();
        gescannt_rel.push_back(rel);
        if (rel.rfind("builder/", 0) == 0) ++builder_dateien;

        std::ifstream ein{it->path()};
        ASSERT_TRUE(ein.is_open()) << it->path();
        std::ostringstream puffer;
        puffer << ein.rdbuf();
        std::size_t const s = stellen_zaehlen(ohne_kommentare(puffer.str()));
        if (s != 0) {
            ist[rel] = s;
            stellen_gesamt += s;
        }
    }

    // (1) ALLOWLIST-DECKUNG: jeder Eintrag muss als Datei existieren UND gescannt worden sein --
    //     auch die soll==0-Eintraege (sonst wuerde ein Umzug/Loeschen die Ausnahme still toeten).
    std::size_t allow_gedeckt = 0;
    for (auto const& e : kAllowlist) {
        bool gefunden = false;
        for (auto const& r : gescannt_rel) {
            if (r == e.rel) {
                gefunden = true;
                break;
            }
        }
        EXPECT_TRUE(gefunden) << "ALLOWLIST-EINTRAG OHNE DATEI (Umzug/Loeschung?): " << e.rel
                              << " -- Eintrag mitziehen, nicht liegen lassen. Grund der Ausnahme: " << e.grund;
        if (gefunden) ++allow_gedeckt;
    }

    // (2) JE ALLOWLIST-EINTRAG: ist == soll, beide Fehlrichtungen benannt.
    for (auto const& e : kAllowlist) {
        auto const        gefunden = ist.find(std::string{e.rel});
        std::size_t const wert     = (gefunden == ist.end()) ? 0u : gefunden->second;
        if (wert > e.soll) {
            ADD_FAILURE() << "SCHLEICH-VARIANT in Allowlist-Datei " << e.rel << ": ist=" << wert << " soll=" << e.soll
                          << " -- die Ausnahme deckt NUR den eingefrorenen Bestand (" << e.grund << ")";
        } else if (wert < e.soll) {
            ADD_FAILURE() << "TOTE AUSNAHME " << e.rel << ": ist=" << wert << " soll=" << e.soll
                          << " -- der Gegenstand ist geschrumpft; Soll-Zahl (und kSollStellenGesamt) bewusst "
                             "mitziehen ("
                          << e.grund << ")";
        }
    }

    // (3) JEDE NICHT-ALLOWLIST-DATEI: 0 Stellen. Treffer werden GESAMMELT gemeldet, nicht nur der erste.
    std::vector<std::string> schleich;
    for (auto const& [rel, wert] : ist) {
        bool erlaubt = false;
        for (auto const& e : kAllowlist) {
            if (rel == e.rel) {
                erlaubt = true;
                break;
            }
        }
        if (!erlaubt) schleich.push_back(rel + " (stellen=" + std::to_string(wert) + ")");
    }
    if (!schleich.empty()) {
        std::string liste;
        for (auto const& s : schleich) liste += "\n    " + s;
        ADD_FAILURE() << "SCHLEICH-VARIANT ausserhalb der Allowlist (" << schleich.size() << " Datei(en)):" << liste
                      << "\n  par.23: std::variant ist fuer statische/Organ-Achsen VERBOTEN. Entweder die "
                         "Stelle entfernen oder -- mit Owner-Entscheid -- als benannten Allowlist-Eintrag "
                         "mit Soll-Zahl eintragen (beide Zahlen: Zeile + kSollStellenGesamt).";
    }

    // (4) NENNER: eine Wache ueber einem leergelaufenen Scan waere gruen und wertlos.
    EXPECT_GE(dateien_gesamt, kMinDateienGesamt)
        << "Scan-Nenner eingebrochen -- Wache ohne Gegenstand (Wurzel falsch? Endungs-Filter kaputt?)";
    EXPECT_GE(builder_dateien, kMinBuilderDateien)
        << "builder/-Dateien fehlen im Nenner -- die builder!=build-Falle hat zugebissen (Skip-Regel pruefen)";
    EXPECT_EQ(stellen_gesamt, kSollStellenGesamt)
        << "Gesamt-Stellen weichen von der eingefrorenen Summe ab -- die Detail-Urteile oben nennen die Datei";
    EXPECT_EQ(allow_gedeckt, kAllowlist.size());

    std::cout << "[HAUSWACHE] dateien=" << dateien_gesamt << " stellen=" << stellen_gesamt << "/" << kSollStellenGesamt
              << " allowlist=" << allow_gedeckt << "/" << kAllowlist.size() << " builder=" << builder_dateien
              << " traeger=" << ist.size() << "\n";
}

// ============================================================================================
// (B) GEGENPROBEN: der Kommentar-Entferner entfernt wirklich (sonst degeneriert die Wache
//     zur Kommentar-Zaehlerin), und zwar auch ueber Zeilengrenzen.
// ============================================================================================
TEST(VariantHauswache, KommentarEntfernerIstScharf) {
    EXPECT_EQ(ohne_kommentare("// std::variant<X> im Zeilenkommentar\n"), std::string{"\n"});
    EXPECT_EQ(ohne_kommentare("/* std::variant<X> */\n"), std::string{"\n"});
    EXPECT_EQ(stellen_zaehlen(ohne_kommentare("/* std::visit\n   std::variant<int>\n*/\n")), 0u);
    EXPECT_NE(ohne_kommentare("using V = std::variant<int>;\n").find("std::variant<"), std::string::npos);
    // Code VOR einem Zeilen-Kommentar bleibt zaehlbar.
    EXPECT_EQ(stellen_zaehlen(ohne_kommentare("std::visit(f, v); // std::visit im Kommentar\n")), 1u);
}

// ============================================================================================
// (C) GEGENPROBEN: der Muster-Satz beisst auf jede Verwendungs-Form des A1-Zensus -- und
//     NICHT auf Namen, die nur so aehnlich aussehen.
// ============================================================================================
TEST(VariantHauswache, MusterSatzIstScharf) {
    EXPECT_EQ(stellen_zaehlen("using D = std::variant<A, B>;\n"), 1u);
    EXPECT_EQ(stellen_zaehlen("return std::visit(f, v);\n"), 1u);
    EXPECT_EQ(stellen_zaehlen("std::monostate leer{};\n"), 1u);
    EXPECT_EQ(stellen_zaehlen("if (auto* p = std::get_if<A>(&v)) {}\n"), 1u);
    EXPECT_EQ(stellen_zaehlen("bool b = std::holds_alternative<A>(v);\n"), 1u);
    EXPECT_EQ(stellen_zaehlen("using T = std::variant_alternative_t<0, V>;\n"), 1u);
    EXPECT_EQ(stellen_zaehlen("#include <variant>\n"), 1u);
    EXPECT_EQ(stellen_zaehlen("#include<variant>\n"), 1u);
    // Negativ: fremde Namen und blosse Wortteile zaehlen nicht.
    EXPECT_EQ(stellen_zaehlen("using V = my::variant<A>;\n"), 0u);
    EXPECT_EQ(stellen_zaehlen("int variant = 0; visit(v);\n"), 0u);
    EXPECT_EQ(stellen_zaehlen("#include <variant_lite.hpp>\n"), 0u);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 2) {
        std::fprintf(stderr, "Aufruf: %s <pfad-zu-libs/cache_engine>\n", argv[0]);
        return 2;
    }
    g_wurzel = argv[1];
    return RUN_ALL_TESTS();
}
