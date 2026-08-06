// test_m1h_stufen_und_pmc_wache.cpp -- M-1/H-A + M-1/H-2 BISS (06.08.2026).
//
// ZWEI ZU BEWEISENDE AUSSAGEN:
//   H-A  Eine SPEZIFISCHE Mess-Combo aus der Env, ohne dass diese CEB dafuer gebaut wurde, wird
//        ABGEWIESEN. Sie ist der uebersprungene CT-EINBAU der STUFE 2 der Mess-Stufen-Doktrin.
//   H-2  Die einkompilierte Mess-Achse und die einkompilierte PMC-Ausstattung muessen zusammenpassen.
//        Sonst liefern zwei Laeufe mit demselben Stempel und demselben Schluessel andere Zahlen (F6).
//
// WARUM ES DIESE WACHEN BRAUCHT -- BEIDE BEFUNDE SIND AM OBJEKT GEMESSEN, NICHT VERMUTET:
//
// H-A: eine und dieselbe Default-CEB-Binary, nur COMDARE_MEASUREMENT_COMBO variiert:
//        env=[all]        ceb_key=004251f4...  tier_defines= ...MEASUREMENT_ON ...CE_ENABLE_STATISTICS + 3 Dekl.
//        env=[wallclock]  ceb_key=004251f4...  tier_defines= ...MEASUREMENT_ON ...TOOLING_WALLCLOCK
//        env=[macro]      ceb_key=004251f4...  tier_defines= ...MEASUREMENT_ON ...CE_ENABLE_STATISTICS ...
//      Vier verschiedene TIER-AUSSTATTUNGEN, EIN Schluessel, KEIN Wurf. Die Doktrin sagt dagegen:
//      MESS ist DREISTUFIG -- Planer (RT-Freigabe) -> CEB (CT-EINBAU) -> Tier (CT-EINBAU). Hier lief die
//      Achse Planer -> Env -> Tier-Compile-Kommando und uebersprang Stufe 2. Owner-KERN F2 woertlich:
//      "bei einem neuen Messsystem [muss] auch die CEB ... neu gebaut werden".
//      Die zwei bestehenden Widerspruchs-Wachen (F-B2) standen BEIDE im #ifdef-Zweig; der #else-Zweig --
//      und genau der ist der Default-Bau, denn COMDARE_MEASUREMENT_COMBO ist per CACHE STRING "" leer --
//      hatte keine.
//
// H-2: Legende [micro], gleiche Maschine, beide Varianten gebaut UND ausgefuehrt:
//        COMDARE_ENABLE_PMC=OFF  -> NullPmcSource       cache_misses_l1 = 0
//        COMDARE_ENABLE_PMC=ON   -> LinuxPerfPmcSource  cache_misses_l1 = 1898596
//        glied3 und ceb_key in BEIDEN Faellen IDENTISCH.
//
// DIESE TU LAEUFT IM CT-LOSEN ZUSTAND (sie linkt comdare_measurement_combo_ct NICHT) -- also in exakt
// dem Zustand, in dem die H-A-Luecke sass. Das ist Absicht: der Biss beisst dort, wo der Defekt war.
// Die H-2-Entscheidung wird deshalb an ihrer REINEN Form gefuehrt (pruefe_pmc_ausstattung), die alle
// vier Kombinationen ohne Praeprozessor-Akrobatik durchlaeuft; der Live-Aufruf wird zusaetzlich als
// No-op im CT-losen Zustand nachgewiesen.
//
// ASCII-only. Keine Mess-CSV-Beruehrung, keine ABI-Flaeche, kein Preimage-Format.

#include <profile_facade/mess_achsen_naht.hpp>

#include <cache_engine/measurement/measurement_tooling_registry.hpp>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

namespace pf = ::comdare::cache_engine::profile_facade;
namespace cm = ::comdare::cache_engine::measurement;

namespace {

int g_fail = 0;

void check_true(std::string const& what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

/// Setzt die Env fuer die Dauer eines Falls und raeumt sie IMMER wieder ab -- ein vergessenes setenv
/// wuerde die folgenden Faelle still faerben.
struct ScopedCombo {
    explicit ScopedCombo(char const* v) {
        if (v == nullptr)
            ::unsetenv("COMDARE_MEASUREMENT_COMBO");
        else
            ::setenv("COMDARE_MEASUREMENT_COMBO", v, 1);
    }
    ~ScopedCombo() { ::unsetenv("COMDARE_MEASUREMENT_COMBO"); }
    ScopedCombo(ScopedCombo const&)            = delete;
    ScopedCombo& operator=(ScopedCombo const&) = delete;
};

struct Lauf {
    bool        warf = false;
    std::string was;
    std::string legend;
};

[[nodiscard]] Lauf loese_auf(char const* env) {
    ScopedCombo const guard{env};
    Lauf              r;
    try {
        r.legend = pf::resolve_live_measurement_combo_legend();
    } catch (std::exception const& e) {
        r.warf = true;
        r.was  = e.what();
    }
    return r;
}

// ---------------------------------------------------------------------------------------------
// (a) NENNER + VORAUSSETZUNG. Ohne diesen Fall waere jeder Wurf unten wertlos: er zeigt, dass die
//     Aufloesung im CT-losen Zustand ueberhaupt etwas liefert (und nicht generell wirft), und dass
//     diese TU wirklich im CT-losen Zweig laeuft -- sonst pruefte sie eine andere Codebahn als die,
//     in der der Defekt sass.
// ---------------------------------------------------------------------------------------------
void fall_a_nenner() {
    std::cout << "\n---- (a) NENNER: diese TU laeuft im CT-losen Zweig, und der liefert die Vollmenge ----\n";
    check_true("(a) diese TU hat KEINE einkompilierte Combo (ct_measurement_combo_legend() ist leer) -- "
               "der Biss beisst also im Zweig, in dem die Luecke sass",
               pf::ct_measurement_combo_legend().empty());
    Lauf const ungesetzt = loese_auf(nullptr);
    std::cout << "    env UNGESETZT -> '" << ungesetzt.legend << "'" << (ungesetzt.warf ? " WURF" : "") << "\n";
    check_true("(a) env UNGESETZT wirft NICHT", !ungesetzt.warf);
    check_true("(a) env UNGESETZT == '[all]' (byte-stabiler Default des gesamten Bestandes)",
               ungesetzt.legend == "[all]");
}

// ---------------------------------------------------------------------------------------------
// (b) H-A, DIE KERN-AUSSAGE: jede SPEZIFISCHE Env-Combo wird abgewiesen. Es werden alle 7 nicht-leeren
//     Teilmengen gefahren; die Zahl wird AUSGEGEBEN, nicht behauptet.
// ---------------------------------------------------------------------------------------------
void fall_b_spezifische_env_wird_abgewiesen() {
    std::cout << "\n---- (b) H-A: eine spezifische Env-Combo ohne CT-Einbau wird abgewiesen ----\n";
    std::size_t gefahren = 0;
    std::size_t geworfen = 0;
    for (unsigned mask = 1; mask < (1u << cm::kMeasurementToolingCount); ++mask) {
        std::string legend = "[";
        for (std::size_t i = 0; i < cm::kMeasurementToolingCount; ++i)
            if ((mask >> i) & 1u) {
                if (legend.size() > 1) legend += ',';
                legend += std::string{cm::kMeasurementToolingRegistry[i].id};
            }
        legend += "]";
        Lauf const l = loese_auf(legend.c_str());
        ++gefahren;
        if (l.warf) ++geworfen;
        std::cout << "    env=" << legend << "  ->  " << (l.warf ? std::string{"WURF"} : "'" + l.legend + "'") << "\n";
        check_true("(b) env=" + legend + " wird abgewiesen", l.warf);
        check_true("(b) env=" + legend + " meldet die Fehlerklasse",
                   l.warf && l.was.find("fehlerklasse=konfiguration_widerspruch") != std::string::npos);
        check_true("(b) env=" + legend + " nennt die fehlende STUFE 2 beim Namen (die Meldung muss den "
                                         "Bediener zum CT-Einbau fuehren, nicht bloss meckern)",
                   l.warf && l.was.find("COMDARE_MEASUREMENT_COMBO_CT") != std::string::npos &&
                       l.was.find("-DCOMDARE_MEASUREMENT_COMBO=") != std::string::npos);
    }
    std::cout << "  spezifische Combos gefahren: " << gefahren << " (erwartet 7), davon abgewiesen: " << geworfen
              << "\n";
    check_true("(b) 7 spezifische Combos wirklich gefahren (der Test kann nicht leer gruen sein)", gefahren == 7);
    check_true("(b) ALLE 7 abgewiesen", geworfen == 7);
}

// ---------------------------------------------------------------------------------------------
// (c) DIE GEGENPROBE ZU (b) -- ohne sie waere (b) auch dann gruen, wenn die Wache ALLES abwiese, und
//     genau das waere die schlimmere Regression: der gesamte heutige Bestand ist [all].
// ---------------------------------------------------------------------------------------------
void fall_c_vollmenge_bleibt_zulaessig() {
    std::cout << "\n---- (c) GEGENPROBE: die Vollmenge geht weiter durch (Bestand byte-stabil) ----\n";
    char const* const vollmengen[] = {"[all]", "all", "[]", ""};
    for (char const* v : vollmengen) {
        Lauf const l = loese_auf(v);
        std::cout << "    env='" << v << "'  ->  " << (l.warf ? std::string{"WURF"} : "'" + l.legend + "'") << "\n";
        check_true(std::string{"(c) env='"} + v + "' wirft NICHT", !l.warf);
    }
    // Und die Semantik dahinter, benannt statt bloss "wirft nicht": das Praedikat, aus dem die Wache
    // ihre Entscheidung zieht, muss die Vollmengen-Schreibweisen von den spezifischen trennen.
    check_true("(c) Praedikat: '[all]' ist Vollmenge", pf::combo_legend_ist_vollmenge("[all]"));
    check_true("(c) Praedikat: '[]' ist Vollmenge", pf::combo_legend_ist_vollmenge("[]"));
    check_true("(c) Praedikat: '' ist Vollmenge", pf::combo_legend_ist_vollmenge(""));
    check_true("(c) Praedikat: '[wallclock]' ist KEINE Vollmenge", !pf::combo_legend_ist_vollmenge("[wallclock]"));
    check_true("(c) Praedikat: '[wallclock,macro,micro]' ist KEINE Vollmenge -- die AUFZAEHLUNG aller drei "
               "ist eine spezifische Bestellung und verlangt denselben CT-Einbau",
               !pf::combo_legend_ist_vollmenge("[wallclock,macro,micro]"));
}

// ---------------------------------------------------------------------------------------------
// (d) H-2: die PMC-Ausstattung gegen die Mess-Achse, ueber ALLE VIER Kombinationen. Zwei muessen
//     durchgehen und zwei werfen -- eine Wache, die immer wirft, ist keine Wache.
// ---------------------------------------------------------------------------------------------
void fall_d_pmc_vier_kombinationen() {
    std::cout << "\n---- (d) H-2: micro <-> COMDARE_ENABLE_PMC, alle vier Kombinationen ----\n";
    struct Fall {
        bool        micro;
        bool        pmc;
        bool        soll_werfen;
        char const* name;
    };
    Fall const faelle[] = {
        {false, false, false, "micro=NEIN pmc=AUS -> zulaessig (nichts versprochen, nichts erhoben)"},
        {true, true, false, "micro=JA   pmc=AN  -> zulaessig (versprochen und erhoben)"},
        {true, false, true, "micro=JA   pmc=AUS -> WURF (PMC-Spalten waeren durchgehend 0)"},
        {false, true, true, "micro=NEIN pmc=AN  -> WURF (Zahlen, die der Stempel nicht deklariert)"},
    };
    std::size_t wuerfe = 0;
    std::size_t durch  = 0;
    for (auto const& f : faelle) {
        bool        warf = false;
        std::string was;
        try {
            pf::pruefe_pmc_ausstattung("[probe]", f.micro, f.pmc);
        } catch (std::exception const& e) {
            warf = true;
            was  = e.what();
        }
        if (warf)
            ++wuerfe;
        else
            ++durch;
        std::cout << "    " << f.name << "  ->  " << (warf ? "WURF" : "durch") << "\n";
        check_true(std::string{"(d) "} + f.name, warf == f.soll_werfen);
        if (f.soll_werfen)
            check_true(std::string{"(d) Fehlerklasse gemeldet: "} + f.name,
                       warf && was.find("fehlerklasse=mess_ausstattung_widerspruch") != std::string::npos);
    }
    std::cout << "  Kombinationen: 4, davon Wuerfe: " << wuerfe << " (erwartet 2), durch: " << durch
              << " (erwartet 2)\n";
    check_true("(d) genau 2 Wuerfe -- die Wache weist nicht alles ab", wuerfe == 2);
    check_true("(d) genau 2 gehen durch -- die Wache laesst nicht alles durch", durch == 2);
}

// ---------------------------------------------------------------------------------------------
// (e) H-2, die BESTANDS-BILANZ: ohne einkompilierte Combo ist der Live-Aufruf ein No-op. Ohne diesen
//     Fall koennte die Wache den gesamten heutigen Bestand ([all], COMDARE_ENABLE_PMC=OFF per
//     CMakeLists.txt:67, Sidecar-Bestand 0) blockieren, ohne dass es jemand merkt.
// ---------------------------------------------------------------------------------------------
void fall_e_bestandsbilanz() {
    std::cout << "\n---- (e) BESTANDS-BILANZ: ohne CT-Combo ist die PMC-Wache ein No-op ----\n";
    for (char const* v : {static_cast<char const*>(nullptr), "[all]"}) {
        ScopedCombo const guard{v};
        bool              warf = false;
        try {
            pf::pruefe_pmc_gegen_mess_achse();
        } catch (std::exception const&) {
            warf = true;
        }
        std::cout << "    env=" << (v == nullptr ? "UNGESETZT" : v) << "  ->  " << (warf ? "WURF" : "no-op") << "\n";
        check_true(std::string{"(e) pruefe_pmc_gegen_mess_achse() ist No-op bei env="} +
                       (v == nullptr ? "UNGESETZT" : v),
                   !warf);
    }
}

// ---------------------------------------------------------------------------------------------
// (f) H-B: das Observer-Praedikat ist an DIESELBE Menge gebunden wie das Gate-Define. Der CSV-Renderer
//     zieht sein "n/a" daraus; liefe es je auseinander, schriebe eine [wallclock]-Zeile wieder Nullen.
// ---------------------------------------------------------------------------------------------
void fall_f_observer_praedikat() {
    std::cout << "\n---- (f) H-B: Observer-Praedikat gegen die vier baubaren Combos ----\n";
    struct P {
        char const* legend;
        bool        erwartet;
    };
    P const faelle[] = {{"[wallclock]", false},
                        {"[wallclock,macro]", true},
                        {"[wallclock,micro]", true},
                        {"[all]", true}};
    for (auto const& f : faelle) {
        bool const ist = pf::mess_menge_hat_observer_gate(pf::mess_tooling_menge_from_legend(f.legend));
        std::cout << "    " << f.legend << "  ->  Observer " << (ist ? "JA" : "NEIN") << "\n";
        check_true(std::string{"(f) "} + f.legend + ": Observer-Ausstattung wie erwartet", ist == f.erwartet);
    }
    check_true("(f) GEGENPROBE: nicht alle vier liefern denselben Wert (sonst pruefte (f) nichts)",
               pf::mess_menge_hat_observer_gate(pf::mess_tooling_menge_from_legend("[wallclock]")) !=
                   pf::mess_menge_hat_observer_gate(pf::mess_tooling_menge_from_legend("[all]")));
}

} // namespace

int main() {
    std::cout << "== M-1/H-A + H-2 BISS: die Stufen-Wache und die PMC-Ausstattungs-Invariante ==\n";
    fall_a_nenner();
    fall_b_spezifische_env_wird_abgewiesen();
    fall_c_vollmenge_bleibt_zulaessig();
    fall_d_pmc_vier_kombinationen();
    fall_e_bestandsbilanz();
    fall_f_observer_praedikat();
    std::cout << "\n== Fehler: " << g_fail << " ==\n";
    return g_fail == 0 ? 0 : 1;
}
