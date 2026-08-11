// -----------------------------------------------------------------------------
// test_d37b_modus_riegel.cpp -- D3-7b: DIE ZWEI LAUF-MODI SCHLIESSEN SICH AUS, UND ZWAR DURCH CODE.
//
// SELBSTCHECK dieses Blocks: jede Zahl unten erhebt dieses Programm SELBST und druckt sie MIT NENNER;
// keine ist aus einer Doku oder einem Review abgeschrieben. kKoeder ist frisch gewuerfelt
// (head -c12 /dev/urandom | base32 | tr A-Z a-z) und steht NUR hier -- er belegt beim Lauf, dass die
// ausgefuehrte Binary aus DIESER Quelle stammt und nicht aus einem alten Objekt.
//
// DER ZUSTAND, DEN ES ZU VERHINDERN GILT (am Objekt gemessen 11.08.2026, nicht theoretisch):
//   provision_only=1 UND pruef_only=1. Der Iterator kehrt im provision_only-Zweig zurueck, BEVOR der
//   pruef_only-Zweig beginnt -- 0 .so werden gegatet, any_pruef_ok bleibt 0. Die Bilanz-Zeile der
//   Fassade trug fuer denselben Lauf trotzdem das Token "(pruef-only)", und der Exit-Code kam aus dem
//   Pruef-Zweig (0 geprueft => exit 1), obwohl der Lauf eine erfolgreiche PROVISIONIERUNG war. Die
//   Zeile behauptete also einen Modus, den der Lauf nie gefahren hat.
//
// WARUM DAS ERREICHBAR WAR: die zwei Schalter sind zwei unabhaengige bool, sie kommen im Betrieb aus
// zwei getrennten Umgebungsvariablen (COMDARE_GOLDEN_N_PROVISION_ONLY / COMDARE_PRUEF_ONLY), und der
// Planer setzt sie in getrennten Bloecken, ohne die erste je zurueckzunehmen. Drei Kommentare
// behaupteten die gegenseitige Ausschliessung; durchgesetzt hat sie 0 Zeilen Code.
//
// WAS DIESER TEST BEHAUPTET -- AUSSAGEN, KEINE ANWESENHEIT (T-2):
//   (A) FASSADE: run_profile lehnt den Doppel-Auftrag mit exit_code 7 ab, und zwar VOR dem
//       Profil-Parse. Der Beleg dafuer ist die GEGENRICHTUNG im selben Aufruf: derselbe unlesbare
//       Pfad liefert OHNE Konflikt 5 (Parse-Fehler). Waere der Riegel hinter dem Parse, kaeme auch
//       im Konflikt-Fall 5 -- die 7 ist also nicht nur "ein Fehler", sondern beweist die POSITION.
//       Nenner: alle 4 Belegungen der zwei Schalter.
//   (B) TOKEN: die Bilanz-Zeile fuehrt HOECHSTENS EIN Modus-Token. Geprueft ueber alle 4 Belegungen,
//       inklusive der Aussage "kein Ergebnis enthaelt beide Modus-Token zugleich".
//   (C) ITERATOR: run_lazy_static_then_dynamic verweigert den Doppel-Auftrag VOR jedem Bau -- gemessen
//       an der WIRKUNG, nicht an einem Rueckgabe-Feld: die injizierte CompileFn wird 0 mal gerufen.
//       Gegenrichtung: mit nur EINEM Schalter wird sie > 0 mal gerufen. Ein Riegel, der immer
//       verweigert, waere so wertlos wie einer, der nie verweigert; beide Richtungen fahren hier.
//
// ORAKEL-UNABHAENGIGKEIT (T-5): die Erwartungen stehen als LITERALE hier (7, 5, " (provision-only)",
// " (pruef-only)", 0 Compile-Aufrufe). Keine Erwartung ruft die Funktion, die geprueft wird.
//
// Doktrin: Google Test, C++23, ASCII-Kommentare.
// -----------------------------------------------------------------------------

#include "profile_facade/profile_run_entry.hpp" // Prueflinge: run_profile / lauf_modus_zusatz

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // Pruefling: run_lazy_static_then_dynamic

#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace tlz = ::comdare::cache_engine::thesis_lazy;
namespace fs  = std::filesystem;

namespace {

// K13: FRISCH GEWUERFELT (head -c12 /dev/urandom | base32 | tr A-Z a-z), nicht abgeschrieben.
constexpr char const* kKoeder = "zu7uvu36f7kgsasxxopa====";

// Die vier Belegungen der zwei Schalter -- DER NENNER aller Aussagen unten.
struct Belegung {
    char const* name;
    bool        provision_only;
    bool        pruef_only;
};
constexpr Belegung kBelegungen[] = {
    {"beide aus", false, false},
    {"nur provision_only", true, false},
    {"nur pruef_only", false, true},
    {"BEIDE zugleich (der Konflikt)", true, true},
};
constexpr std::size_t kBelegungenAnzahl = sizeof(kBelegungen) / sizeof(kBelegungen[0]);

// Zwei statische Blaetter + eine dynamische Dimension -- kleinstmoeglicher echter Baum (Muster
// test_tp1_planer_filter_iterator).
ex::ExperimentTree mach_baum(std::shared_ptr<ex::ExperimentNodeFactory> const& f) {
    ex::ExperimentTree t{f};
    t.build({
        ex::AxisLevel{"traversal", {"ART"}, true, "", ""},
        ex::AxisLevel{"node", {"N4", "N16"}, true, "", ""},
        ex::AxisLevel{"concurrency", {"1"}, false, "thread_count", ""},
    });
    return t;
}

} // namespace

// -- (A) DIE FASSADE: der Riegel liegt VOR dem Profil-Parse. ---------------------------------------
TEST(D37bModusRiegel, RunProfileLehntDenDoppelAuftragVorDemProfilParseAb) {
    std::cout << "[d37b] Koeder=" << kKoeder << "\n";

    // Ein Pfad, den es garantiert nicht gibt: OHNE Konflikt muss run_profile daran scheitern (5),
    // MIT Konflikt darf es gar nicht erst so weit kommen (7).
    fs::path const kein_profil =
        ::comdare::test::user_tmp_dir() / "comdare_d37b_riegel" / "es-gibt-mich-nicht.profile.xml";
    std::error_code ec;
    fs::remove_all(kein_profil.parent_path(), ec);
    ASSERT_FALSE(fs::exists(kein_profil)) << "Vorbedingung verletzt: der Pfad darf NICHT existieren";

    std::size_t geprueft = 0;
    std::size_t wie_erwartet = 0;
    for (auto const& b : kBelegungen) {
        tlz::RunProfileArgs a;
        a.profile_path   = kein_profil;
        a.out_csv        = kein_profil.parent_path() / "result.csv";
        a.provision_only = b.provision_only;
        a.pruef_only     = b.pruef_only;

        tlz::RunProfileResult const r = tlz::run_profile(a);

        // LITERAL, nicht aus dem Pruefling abgeleitet: 7 = Modus-Konflikt, 5 = Profil unlesbar.
        int const erwartet = (b.provision_only && b.pruef_only) ? 7 : 5;
        ++geprueft;
        if (r.exit_code == erwartet) ++wie_erwartet;
        EXPECT_EQ(r.exit_code, erwartet) << "Belegung '" << b.name << "': exit_code";
    }
    std::cout << "  [NENNER] exit_code wie erwartet in " << wie_erwartet << " von " << geprueft
              << " Schalter-Belegungen (Grundgesamtheit: " << kBelegungenAnzahl << ")\n";
    EXPECT_EQ(wie_erwartet, kBelegungenAnzahl);

    // DIE EIGENTLICHE POSITIONS-AUSSAGE, noch einmal als eigener Satz: die zwei Codes sind
    // VERSCHIEDEN. Waere der Riegel hinter dem Parse, waeren beide 5 -- und diese Zeile riesse.
    tlz::RunProfileArgs nur_pruef;
    nur_pruef.profile_path = kein_profil;
    nur_pruef.out_csv      = kein_profil.parent_path() / "result.csv";
    nur_pruef.pruef_only   = true;
    tlz::RunProfileArgs beide = nur_pruef;
    beide.provision_only      = true;
    EXPECT_NE(tlz::run_profile(beide).exit_code, tlz::run_profile(nur_pruef).exit_code)
        << "der Konflikt-Abbruch ist von einem Parse-Fehler nicht unterscheidbar -- dann steht der "
           "Riegel an der falschen Stelle oder gar nicht";

    fs::remove_all(kein_profil.parent_path(), ec);
}

// -- (B) DAS TOKEN: hoechstens EINES je Zeile, ueber alle vier Belegungen. -------------------------
TEST(D37bModusRiegel, BilanzZeileFuehrtHoechstensEinModusToken) {
    std::size_t hoechstens_eins = 0;
    for (auto const& b : kBelegungen) {
        std::string const zusatz = tlz::lauf_modus_zusatz(b.provision_only, b.pruef_only);
        std::size_t const treffer =
            (zusatz.find("(provision-only)") != std::string::npos ? 1u : 0u) +
            (zusatz.find("(pruef-only)") != std::string::npos ? 1u : 0u);
        std::cout << "  [TOKEN] " << b.name << " -> '" << zusatz << "' (Modus-Token: " << treffer << ")\n";
        if (treffer <= 1) ++hoechstens_eins;
        EXPECT_LE(treffer, 1u) << "Belegung '" << b.name
                               << "': zwei Modus-Token in einer Zeile sind fuer jeden Feld-Zerleger "
                                  "mehrdeutig";
    }
    std::cout << "  [NENNER] hoechstens EIN Modus-Token in " << hoechstens_eins << " von " << kBelegungenAnzahl
              << " Schalter-Belegungen\n";
    EXPECT_EQ(hoechstens_eins, kBelegungenAnzahl);

    // Die exakten Formen -- LITERALE, damit eine stille Umbenennung den super-seitigen Zerleger nicht
    // unbemerkt verliert.
    EXPECT_EQ(tlz::lauf_modus_zusatz(false, false), std::string{});
    EXPECT_EQ(tlz::lauf_modus_zusatz(true, false), std::string{" (provision-only)"});
    EXPECT_EQ(tlz::lauf_modus_zusatz(false, true), std::string{" (pruef-only)"});
    // Der Konflikt-Fall ist KEIN Modus: er darf keines der beiden Modus-Token tragen.
    std::string const konflikt = tlz::lauf_modus_zusatz(true, true);
    EXPECT_EQ(konflikt.find("(provision-only)"), std::string::npos);
    EXPECT_EQ(konflikt.find("(pruef-only)"), std::string::npos);
    EXPECT_NE(konflikt.find("lauf-modus-konflikt"), std::string::npos)
        << "der Konflikt-Fall muss benannt sein, nicht leer -- sonst ist er von 'kein Modus' nicht zu "
           "unterscheiden";
}

// -- (C) DER ITERATOR: verweigert VOR jedem Bau; und verweigert NICHT, wenn nur einer gesetzt ist. --
TEST(D37bModusRiegel, IteratorVerweigertDenDoppelAuftragVorJedemBau) {
    fs::path const  basis = ::comdare::test::user_tmp_dir() / "comdare_d37b_iterator";
    std::error_code ec;
    fs::remove_all(basis, ec);

    auto           factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree baum = mach_baum(factory);
    ASSERT_EQ(baum.static_binary_view().size(), std::size_t{2}) << "Vorbedingung: 2 statische Blaetter";

    ex::BuildSelection sel;
    sel.indices    = {0, 1};
    sel.provenance = "explicit";

    auto const ram_stub = []() -> std::uint64_t { return ~std::uint64_t{0}; };
    auto const gen_stub = [](std::string const&) { return std::string{"// d37b-riegel-stub\n"}; };

    // Ein Lauf mit gezaehlten Compile-Aufrufen. Die ZAHL ist die Aussage: 0 heisst "nichts gebaut".
    auto fahre = [&](bool provision_only, bool pruef_only, char const* unterordner) {
        std::size_t compile_aufrufe = 0;
        auto compile_zaehler = [&compile_aufrufe](ex::BuildJob const&) -> int {
            ++compile_aufrufe;
            return 0;
        };
        ex::LazyRunConfig cfg;
        cfg.source_dir         = basis / unterordner / "src";
        cfg.output_dir         = basis / unterordner / "dll";
        cfg.per_binary_subdirs = true;
        cfg.build_parallelism  = 1;
        cfg.provision_only     = provision_only;
        cfg.pruef_only         = pruef_only;
        ex::LazyRunResult const r =
            ex::run_lazy_static_then_dynamic(baum, sel, compile_zaehler, gen_stub, ram_stub, cfg);
        std::cout << "  [ITER] provision_only=" << (provision_only ? 1 : 0) << " pruef_only=" << (pruef_only ? 1 : 0)
                  << " -> compile_aufrufe=" << compile_aufrufe << " selected=" << r.selected << " built=" << r.built
                  << " pruef_ok=" << r.pruef_ok << " pruef_failed=" << r.pruef_failed << "\n";
        return std::pair<std::size_t, ex::LazyRunResult>{compile_aufrufe, r};
    };

    // GEGENRICHTUNG ZUERST (der Riegel muss beweisen, dass er NICHT immer beisst): nur provision_only
    // -- der Lauf baut wirklich, also ist der Zaehler > 0. Ohne diesen Fall waere der Konflikt-Fall
    // unten von "der Iterator baut nie" nicht zu unterscheiden.
    auto const [ohne_konflikt_calls, ohne_konflikt] = fahre(true, false, "nur_provision");
    EXPECT_GT(ohne_konflikt_calls, std::size_t{0}) << "ohne Konflikt muss real gebaut werden";
    EXPECT_EQ(ohne_konflikt.selected, std::size_t{2});
    EXPECT_GT(ohne_konflikt.built, std::size_t{0});

    // DER KONFLIKT: 0 Compile-Aufrufe, 0 selektiert, 0 gebaut, 0 geprueft -- der Lauf hinterlaesst
    // nichts, was ein Folgelauf als Bestand missdeuten koennte.
    auto const [konflikt_calls, konflikt] = fahre(true, true, "beide");
    EXPECT_EQ(konflikt_calls, std::size_t{0}) << "der Riegel liegt hinter dem Bau -- es wurde kompiliert";
    EXPECT_EQ(konflikt.selected, std::size_t{0});
    EXPECT_EQ(konflikt.built, std::size_t{0});
    EXPECT_EQ(konflikt.pruef_ok, std::size_t{0});
    EXPECT_EQ(konflikt.pruef_failed, std::size_t{0});

    // Und die Platte: der Konflikt-Lauf darf sein Ausgabe-Verzeichnis nicht einmal angelegt haben.
    EXPECT_FALSE(fs::exists(basis / "beide" / "dll"))
        << "der Riegel greift zu spaet: das Ausgabe-Verzeichnis existiert bereits";

    fs::remove_all(basis, ec);
}
