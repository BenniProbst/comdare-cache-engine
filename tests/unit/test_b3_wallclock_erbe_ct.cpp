// test_b3_wallclock_erbe_ct.cpp -- B3 (19.08.2026, KON37-01/KON34-04): DER CT-ZWEIG-BEWEIS DES
// WALLCLOCK-ERBES. Die Schalter-Hoheit liegt bei der CEB, das Tier erbt.
//
// DIESE TU wird MIT -DCOMDARE_MEASUREMENT_COMBO_CT="[macro]" kompiliert (tests/unit/CMakeLists.txt)
// -- als EINZIGE im Baum mit einer w-LOSEN spezifischen Bestellung. Nur hier ist der Erb-Anschluss
// des CT-Zweigs (resolve_live_measurement_combo_legend -> mess_combo_legende_mit_wallclock_erbe)
// ueberhaupt beobachtbar: die beiden Bestands-CT-TUs (test_w2_combo_ct_stamp "[wallclock]",
// test_b2_gate_zustand_g2an_g3aus "[wallclock,macro]") nennen wallclock selbst, ihr Erbe ist ein
// No-op -- eine entfernte Erb-Durchleitung bliebe dort GRUEN. Diese TU macht sie ROT (Mutations-
// Nachweis im Bericht der Scheibe: resolve ohne Erbe-Durchleitung -> Faelle (b)-(e) fallen).
//
// DIE ZU BEWEISENDE AUSSAGE (gruppe5.md POSTEN 4, SOLL woertlich): "w erscheint im Tier-Stempel
// genau dann, wenn die erzeugende CEB w traegt und einbaut" -- eine [macro]-bestellte CEB baut G1
// (das wallclock-Instrument) unvermeidlich in jede Tier-Binary ein, also fuehrt ihr Glied [3] w,
// ohne dass der Bediener es nennen musste. Der M-1/H-1-Pflicht-Wurf ist gefallen (der Umzug der
// Schalter-Hoheit); die WACHEN der Aufloesung (Env-Synchronitaet gegen die BESTELLUNG) stehen
// unveraendert VOR dem Erbe.
//
// Die TU-eigenen Defines tragen exakt den Satz, den mess_achsen_defines fuer die GEERBTE
// Bestellung emittiert (G1+G2, G3 explizit 0, Deklarationen WALLCLOCK+MACRO) -- dieselbe
// Konsistenz-Doktrin wie test_b2_gate_zustand_g2an_g3aus.
//
// Plain-main-Test (Muster test_w2_combo_ct_stamp.cpp): check_true, exit 0 = alle OK. ASCII-only.

#include "lazy_adhoc_source_gen.hpp" // measurement_stamp_from_env (die EINE Stempel-Naht)

#include <cache_engine/abi/anatomy_version_stamp.hpp> // measurement_stamp_line_from_combo_legend

#include <cstdlib> // setenv/unsetenv (Env-Kanal der Widerspruchs-Wache)
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace pf  = ::comdare::cache_engine::profile_facade;
namespace tlz = ::comdare::cache_engine::thesis_lazy;
namespace abi = ::comdare::cache_engine::abi;

namespace {

int g_fail = 0;

void check_true(std::string const& what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

/// Env setzen/abraeumen je Fall -- ein vergessenes setenv faerbte die Folgefaelle still
/// (Muster test_m1h_stufen_und_pmc_wache.cpp).
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

// ---------------------------------------------------------------------------------------------
// (a) NENNER: diese TU laeuft wirklich im CT-Zweig und wirklich mit der w-LOSEN Bestellung --
//     sonst pruefte alles Folgende eine andere Codebahn (der W2-Nachbar deckt "[wallclock]").
// ---------------------------------------------------------------------------------------------
void fall_a_nenner() {
    std::cout << "\n---- (a) NENNER: CT-Zweig mit w-loser Bestellung '[macro]' ----\n";
    std::string const ct = pf::ct_measurement_combo_legend();
    std::cout << "    ct_measurement_combo_legend() = '" << ct << "'\n";
    check_true("(a) die einkompilierte Bestellung ist '[macro]' (w-los, spezifisch)", ct == "[macro]");
    check_true("(a) die Bestellung ist KEINE Vollmenge (sonst waere das Erbe hier unbeobachtbar)",
               !pf::combo_legend_ist_vollmenge(ct));
}

// ---------------------------------------------------------------------------------------------
// (b) DER KERN: die Aufloesung ERBT wallclock im CT-Zweig. Env synchron zur BESTELLUNG (roh),
//     Rueckgabe ist die GEERBTE Legende -- die CEB-Entscheidung, die Stempel UND Bau lesen.
// ---------------------------------------------------------------------------------------------
void fall_b_resolve_erbt() {
    std::cout << "\n---- (b) KERN: resolve erbt '[macro]' -> '[wallclock,macro]' ----\n";
    ScopedCombo const guard{"[macro]"};
    std::string const aufgeloest = pf::resolve_live_measurement_combo_legend();
    std::cout << "    resolve_live_measurement_combo_legend() = '" << aufgeloest << "'\n";
    check_true("(b) Rueckgabe ist die geerbte Legende '[wallclock,macro]'", aufgeloest == "[wallclock,macro]");
    check_true("(b) Rueckgabe ist NICHT mehr die rohe Bestellung '[macro]' (das Erbe ist gelaufen)",
               aufgeloest != "[macro]");
}

// ---------------------------------------------------------------------------------------------
// (c) STEMPEL-SEITE: die Mess-Zeile (Glied [3]) FUEHRT wallclock. Byte-gleich zum Renderer der
//     geerbten Legende, verschieden vom Renderer der rohen -- am Vor-B3-Stand (resolve ohne Erbe)
//     ist genau das rot.
// ---------------------------------------------------------------------------------------------
void fall_c_stempel_fuehrt_w() {
    std::cout << "\n---- (c) STEMPEL: Glied-[3]-Zeile fuehrt wallclock ----\n";
    ScopedCombo const guard{"[macro]"};
    std::string const zeile = tlz::measurement_stamp_from_env();
    std::cout << "    measurement_stamp_from_env() = '" << zeile << "'\n";
    check_true("(c) Zeile == Renderer der GEERBTEN Legende (EINE Aufloesung, keine zweite Wahrheit)",
               zeile == abi::measurement_stamp_line_from_combo_legend("[wallclock,macro]"));
    check_true("(c) Zeile nennt wallclock (w im Tier-Stempel, weil die CEB es einbaut -- das SOLL)",
               zeile.find("measurement_tooling=wallclock@") != std::string::npos);
    check_true("(c) Zeile != Renderer der ROHEN Bestellung (sonst verschwiege der Stempel w -- F2)",
               zeile != abi::measurement_stamp_line_from_combo_legend("[macro]"));
}

// ---------------------------------------------------------------------------------------------
// (d) BAU-SEITE: die Tier-Defines tragen die geerbte Deklaration und sind byte-gleich zum
//     w-nennenden Zwilling -- Stempel und Bau koennen nicht auseinanderlaufen, weil beide nur
//     die eine geerbte Aufloesung lesen.
// ---------------------------------------------------------------------------------------------
void fall_d_bau_erbt() {
    std::cout << "\n---- (d) BAU: live-Defines tragen die geerbte wallclock-Deklaration ----\n";
    ScopedCombo const              guard{"[macro]"};
    std::vector<std::string> const live = pf::live_mess_achsen_defines();
    std::string                    join;
    for (auto const& s : live) {
        if (!join.empty()) join += ' ';
        join += s;
    }
    std::cout << "    live_mess_achsen_defines() = " << join << "\n";
    bool hat_w = false;
    for (auto const& s : live) hat_w = hat_w || s == "-DCOMDARE_MEASUREMENT_TOOLING_WALLCLOCK=1";
    check_true("(d) -DCOMDARE_MEASUREMENT_TOOLING_WALLCLOCK=1 ist enthalten (geerbt)", hat_w);
    check_true("(d) live-Vektor == Vektor des w-nennenden Zwillings '[wallclock,macro]'",
               live == pf::mess_achsen_defines_for_legend("[wallclock,macro]"));
}

// ---------------------------------------------------------------------------------------------
// (e) GLIED-[8]-VORHERSAGE: dieselbe eine Aufloesung -- die Host-Vorhersage des Mess-Gates-Glieds
//     dieser CEB ist die des w-nennenden Zwillings.
// ---------------------------------------------------------------------------------------------
void fall_e_glied8_vorhersage() {
    std::cout << "\n---- (e) GLIED [8]: live-Vorhersage == Zwillings-Vorhersage ----\n";
    ScopedCombo const guard{"[macro]"};
    std::string const live = pf::live_mess_gates_glied();
    std::cout << "    live_mess_gates_glied() = '" << live << "'\n";
    check_true("(e) live == mess_gates_glied_for_legend('[wallclock,macro]')",
               live == pf::mess_gates_glied_for_legend("[wallclock,macro]"));
}

// ---------------------------------------------------------------------------------------------
// (f) DIE WACHEN STEHEN UNVERAENDERT VOR DEM ERBE: sie vergleichen die BESTELL-Kanaele (Env gegen
//     CT-Define) roh. Insbesondere ist die GEERBTE Form KEIN gueltiger Env-Wert -- die Env spiegelt
//     die Bestellung, nicht das Erbe. Ein Erbe, das die Wachen aufweichte, waere eine Regression.
// ---------------------------------------------------------------------------------------------
void fall_f_wachen_unbewegt() {
    std::cout << "\n---- (f) WACHEN: Env-Synchronitaet gegen die ROHE Bestellung, unbewegt ----\n";
    struct F {
        char const* env;
        char const* name;
    };
    F const faelle[] = {
        {nullptr, "Env FEHLT bei spezifischem CT-Einbau"},
        {"", "Env LEER bei spezifischem CT-Einbau"},
        {"[wallclock,macro]", "Env traegt die GEERBTE Form statt der Bestellung"},
        {"[all]", "Env traegt die Vollmenge statt der Bestellung"},
    };
    for (auto const& f : faelle) {
        ScopedCombo const guard{f.env};
        bool              warf = false;
        std::string       was;
        try {
            (void)pf::resolve_live_measurement_combo_legend();
        } catch (std::exception const& e) {
            warf = true;
            was  = e.what();
        }
        std::cout << "    " << f.name << "  ->  " << (warf ? "WURF" : "DURCH") << "\n";
        check_true(std::string{"(f) "} + f.name + " wirft", warf);
        check_true(std::string{"(f) "} + f.name + " meldet die Fehlerklasse",
                   warf && was.find("fehlerklasse=konfiguration_widerspruch") != std::string::npos);
    }
    // Gegenprobe (eine Wache, die alles abweist, ist keine): die synchrone rohe Bestellung geht durch.
    ScopedCombo const guard{"[macro]"};
    bool              warf = false;
    try {
        (void)pf::resolve_live_measurement_combo_legend();
    } catch (std::exception const&) { warf = true; }
    check_true("(f) GEGENPROBE: Env == rohe Bestellung '[macro]' geht durch", !warf);
}

} // namespace

int main() {
    std::cout << "== B3-BISS: der CT-Zweig erbt wallclock -- Hoheit CEB, Tier erbt (KON37-01/KON34-04) ==\n";
    fall_a_nenner();
    fall_b_resolve_erbt();
    fall_c_stempel_fuehrt_w();
    fall_d_bau_erbt();
    fall_e_glied8_vorhersage();
    fall_f_wachen_unbewegt();
    std::cout << "\n== Fehler: " << g_fail << " ==\n";
    return g_fail == 0 ? 0 : 1;
}
