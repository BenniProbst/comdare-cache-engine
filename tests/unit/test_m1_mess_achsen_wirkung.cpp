// test_m1_mess_achsen_wirkung.cpp -- M-1/D-1 BISS (06.08.2026).
//
// DIE ZU BEWEISENDE AUSSAGE, in einem Satz: ZWEI VERSCHIEDENE MESS-ACHSEN-KONFIGURATIONEN ERZEUGEN
// VERSCHIEDENE TIER-DEFINES.
//
// WARUM DIESER TEST EXISTIERT (Befund D-1, am Objekt gemessen, Stand b241a272):
// perm_mess_defines() (profile_facade/profile_run_facade.cpp) setzte -DCOMDARE_MEASUREMENT_ON=1 und
// -DCOMDARE_CE_ENABLE_STATISTICS=1 als LITERALE, unabhaengig von jeder Tooling-Wahl. Ein vollstaendiger
// Scan nach MeasurementTooling|measurement_tooling ueber libs/ und apps/ fand 105 Treffer in 14 Dateien
// und KEINEN, der Messcode ein- oder ausschaltet. Das Preimage-Glied [3] deklarierte damit eine
// Mess-Ausstattung, die der Bau ignorierte -- eine [wallclock]-Binary und eine [micro]-Binary bekamen
// byte-gleiche Compile-Kommandos. Der Stempel log, und NICHTS BRACH. Genau diese Bruchlosigkeit ist
// der Grund, warum es hier eine Wache braucht und nicht nur eine Korrektur.
//
// DER BISS BEISST AN DER REINEN ABBILDUNG (mess_achsen_defines), nicht am Env/Makro-Zustand: sie ist die
// EINE Funktion, aus der live_mess_achsen_defines() und damit perm_mess_defines() ihren Mess-Teil zieht
// (profile_run_facade.cpp, ein Aufruf; perm_compile_flags ist der EINE Assembler, aus dem alle vier
// make_gpp_compile_fn-Baustellen ihren defines-Kanal ziehen). Wird die Abbildung wieder achsen-blind,
// faellt dieser Test -- unabhaengig davon, wie der Bau gerade konfiguriert ist.
//
// ROT-NACHWEIS (im Bericht der Scheibe woertlich protokolliert): setzt man mess_achsen_defines() auf den
// abgeloesten Ist-Stand zurueck (der Menge-Parameter wird ignoriert, der Vektor ist konstant), fallen die
// Faelle (b), (c), (d) und (f) -- 8 [ERR]-Zeilen. Das ist der Beweis, dass der Test die AENDERUNG prueft
// und nicht die Umgebung.
//
// ASCII-only. Keine Mess-CSV-Beruehrung, keine ABI-Flaeche, kein Preimage-Format.

#include <profile_facade/mess_achsen_naht.hpp>

#include <mess_axes/measurement_tooling_registry.hpp>

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace pf = ::comdare::cache_engine::profile_facade;
namespace cm = ::comdare::cache_engine::measurement;

namespace {

int g_fail = 0;

void check_true(std::string const& what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

[[nodiscard]] std::string join(std::vector<std::string> const& v) {
    std::string out;
    for (auto const& s : v) {
        if (!out.empty()) out += ' ';
        out += s;
    }
    return out;
}

[[nodiscard]] bool hat(std::vector<std::string> const& v, std::string_view needle) {
    return std::find(v.begin(), v.end(), std::string{needle}) != v.end();
}

/// Ruft die Abbildung und meldet (warf?, Meldung, Vektor) -- EIN Helfer fuer alle Faelle, damit kein Fall
/// einen eigenen try/catch-Dialekt entwickelt.
struct Lauf {
    bool                     warf = false;
    std::string              was;
    std::vector<std::string> defs;
};

[[nodiscard]] Lauf ruf(std::string_view legend) {
    Lauf r;
    try {
        r.defs = pf::mess_achsen_defines_for_legend(legend);
    } catch (std::exception const& e) {
        r.warf = true;
        r.was  = e.what();
    }
    return r;
}

void zeig(char const* titel, Lauf const& l) {
    std::cout << "  " << titel << " -> ";
    if (l.warf)
        std::cout << "WURF: " << l.was << "\n";
    else
        std::cout << join(l.defs) << "\n";
}

// ---------------------------------------------------------------------------------------------
// (a) NENNER: das Verfahren findet ueberhaupt etwas. Eine nackte Null ist kein Befund -- wenn die
//     Vollmenge keinen nicht-leeren Vektor liefert, sind alle folgenden Ungleichungen wertlos.
// ---------------------------------------------------------------------------------------------
void fall_a_nenner() {
    std::cout << "\n---- (a) NENNER: die Vollmenge liefert einen nicht-leeren Define-Vektor ----\n";
    Lauf const all = ruf("[all]");
    zeig("[all]", all);
    check_true("(a) [all] wirft nicht", !all.warf);
    check_true("(a) [all] liefert mindestens 2 Defines (sonst misst dieser Test nichts)", all.defs.size() >= 2);
    check_true("(a) Registry hat 3 Toolings (die Achse, ueber die hier geurteilt wird, existiert)",
               cm::kMeasurementToolingCount == 3);
}

// ---------------------------------------------------------------------------------------------
// (b) DIE KERN-AUSSAGE: verschiedene Mess-Achsen -> verschiedene Defines. Paarweise ueber alle drei
//     Einzel-Toolings UND gegen die Vollmenge. VIER paarweise Ungleichungen, keine davon trivial.
// ---------------------------------------------------------------------------------------------
void fall_b_paarweise_verschieden() {
    std::cout << "\n---- (b) BISS: paarweise VERSCHIEDENE Define-Vektoren je Mess-Achsen-Wahl ----\n";
    // M-1/H-1: die zulaessigen Combos NENNEN wallclock. "[macro]" allein ist ab H-1 ein Wurf, weil G1
    // (das wallclock-Instrument) auch dort einkompiliert wird -- die Gate-Aussagen unten sind deshalb an
    // "[wallclock,macro]" bzw. "[wallclock,micro]" gefuehrt. Die Aussage von (b)/(c) ist unveraendert:
    // sie vergleicht Mengen MIT und OHNE das Observer-Gate.
    Lauf const wc  = ruf("[wallclock]");
    Lauf const ma  = ruf("[wallclock,macro]");
    Lauf const mi  = ruf("[wallclock,micro]");
    Lauf const all = ruf("[all]");
    zeig("[wallclock]      ", wc);
    zeig("[wallclock,macro]", ma);
    zeig("[wallclock,micro]", mi);
    zeig("[all]            ", all);
    check_true("(b) keine der vier zulaessigen Combos wirft", !wc.warf && !ma.warf && !mi.warf && !all.warf);
    check_true("(b) [wallclock]       != [wallclock,macro]", wc.defs != ma.defs);
    check_true("(b) [wallclock]       != [wallclock,micro]", wc.defs != mi.defs);
    check_true("(b) [wallclock,macro] != [wallclock,micro]", ma.defs != mi.defs);
    check_true("(b) [wallclock]       != [all]", wc.defs != all.defs);
    check_true("(b) [wallclock,macro] != [all]", ma.defs != all.defs);
    check_true("(b) [wallclock,micro] != [all]", mi.defs != all.defs);
}

// ---------------------------------------------------------------------------------------------
// (c) DIE REALE GATE-WIRKUNG, benannt statt bloss "verschieden": [wallclock] bekommt das
//     STATISTICS-Gate NICHT. Das ist die einzige am Objekt belegte Trennung -- fill_observer_v3
//     (abi_adapter.hpp:1426-1714) und fill_segment_timing_v3 (:1784-2057) liegen beide darunter.
//     G1 (IMeasurableWorkload::run_workload, :589-1123) haengt an MEASUREMENT_ON und gilt fuer JEDE
//     Ausstattung -- deshalb steht MEASUREMENT_ON in allen vier Vektoren.
// ---------------------------------------------------------------------------------------------
void fall_c_gate_wirkung() {
    std::cout << "\n---- (c) GATE-WIRKUNG: [wallclock] traegt das Observer-Gate NICHT ----\n";
    Lauf const wc  = ruf("[wallclock]");
    Lauf const ma  = ruf("[wallclock,macro]");
    Lauf const mi  = ruf("[wallclock,micro]");
    Lauf const all = ruf("[all]");
    check_true("(c) G1: MEASUREMENT_ON in [wallclock]", hat(wc.defs, "-DCOMDARE_MEASUREMENT_ON=1"));
    check_true("(c) G1: MEASUREMENT_ON in [wallclock,macro]", hat(ma.defs, "-DCOMDARE_MEASUREMENT_ON=1"));
    check_true("(c) G1: MEASUREMENT_ON in [wallclock,micro]", hat(mi.defs, "-DCOMDARE_MEASUREMENT_ON=1"));
    check_true("(c) G1: MEASUREMENT_ON in [all]", hat(all.defs, "-DCOMDARE_MEASUREMENT_ON=1"));
    check_true("(c) G2/G3: CE_ENABLE_STATISTICS FEHLT in [wallclock] -- DIE HEILUNG VON D-1",
               !hat(wc.defs, "-DCOMDARE_CE_ENABLE_STATISTICS=1"));
    check_true("(c) G2/G3: CE_ENABLE_STATISTICS in [wallclock,macro]",
               hat(ma.defs, "-DCOMDARE_CE_ENABLE_STATISTICS=1"));
    check_true("(c) G2/G3: CE_ENABLE_STATISTICS in [wallclock,micro]",
               hat(mi.defs, "-DCOMDARE_CE_ENABLE_STATISTICS=1"));
    check_true("(c) G2/G3: CE_ENABLE_STATISTICS in [all]", hat(all.defs, "-DCOMDARE_CE_ENABLE_STATISTICS=1"));
    // M-1/H-B: DIESELBE Bedingung als benannte Funktion -- der CSV-Renderer zieht aus ihr sein n/a.
    // Wenn diese vier Zeilen je auseinanderlaufen, ist die eine Quelle zu zweien geworden.
    auto menge = [](std::string_view l) { return pf::mess_tooling_menge_from_legend(l); };
    check_true("(c/H-B) Observer-Praedikat == Gate-Define, [wallclock]",
               pf::mess_menge_hat_observer_gate(menge("[wallclock]")) ==
                   hat(wc.defs, "-DCOMDARE_CE_ENABLE_STATISTICS=1"));
    check_true("(c/H-B) Observer-Praedikat == Gate-Define, [wallclock,macro]",
               pf::mess_menge_hat_observer_gate(menge("[wallclock,macro]")) ==
                   hat(ma.defs, "-DCOMDARE_CE_ENABLE_STATISTICS=1"));
    check_true("(c/H-B) Observer-Praedikat == Gate-Define, [wallclock,micro]",
               pf::mess_menge_hat_observer_gate(menge("[wallclock,micro]")) ==
                   hat(mi.defs, "-DCOMDARE_CE_ENABLE_STATISTICS=1"));
    check_true("(c/H-B) Observer-Praedikat == Gate-Define, [all]",
               pf::mess_menge_hat_observer_gate(menge("[all]")) == hat(all.defs, "-DCOMDARE_CE_ENABLE_STATISTICS=1"));
}

// ---------------------------------------------------------------------------------------------
// (h) M-1/H-1: DIE DEKLARATIONS-PFLICHT FUER G1. Eine Combo, die wallclock nicht nennt, es aber
//     unvermeidlich einbaut (G1 == das wallclock-Instrument), wird abgewiesen. Ohne diese Wache
//     bewegte ein Versions-Sprung der wallclock-Mess-Achse das [macro]-Objekt, aber NICHT dessen
//     Fingerprint -- dll_is_current meldete "current" ueber eine Mess-Code-Versionsgrenze hinweg (F2).
//     VOLLSTAENDIGKEIT: die 7 nicht-leeren Teilmengen zerfallen in 4 zulaessige (wallclock genannt)
//     und 3 abgewiesene. Beide Zahlen werden AUSGEGEBEN, nicht behauptet -- ein Test, der nur die
//     Wuerfe zaehlt, waere auch gruen, wenn die Wache ALLES abwiese.
// ---------------------------------------------------------------------------------------------
void fall_h_g1_deklarationspflicht() {
    std::cout << "\n---- (h) H-1: eine Combo ohne 'wallclock' wird abgewiesen (G1 ist das wallclock-Instrument) ----\n";
    std::size_t zulaessig  = 0;
    std::size_t abgewiesen = 0;
    for (unsigned mask = 1; mask < (1u << cm::kMeasurementToolingCount); ++mask) {
        std::string legend   = "[";
        bool        nennt_wc = false;
        for (std::size_t i = 0; i < cm::kMeasurementToolingCount; ++i)
            if ((mask >> i) & 1u) {
                if (legend.size() > 1) legend += ',';
                legend += std::string{cm::kMeasurementToolingRegistry[i].id};
                if (cm::kMeasurementToolingRegistry[i].tooling == cm::MeasurementTooling::WallClock) nennt_wc = true;
            }
        legend += "]";
        Lauf const l = ruf(legend);
        std::cout << "    " << legend << (nennt_wc ? "  (nennt wallclock)  " : "  (nennt wallclock NICHT)  ")
                  << (l.warf ? std::string{"WURF"} : join(l.defs)) << "\n";
        if (nennt_wc) {
            ++zulaessig;
            check_true(std::string{"(h) "} + legend + " ist zulaessig", !l.warf);
        } else {
            ++abgewiesen;
            check_true(std::string{"(h) "} + legend + " wird abgewiesen", l.warf);
            check_true(std::string{"(h) "} + legend + " meldet die Fehlerklasse",
                       l.warf && l.was.find("fehlerklasse=konfiguration_widerspruch") != std::string::npos);
        }
    }
    std::cout << "  zulaessig: " << zulaessig << " (erwartet 4), abgewiesen: " << abgewiesen << " (erwartet 3)\n";
    check_true("(h) genau 4 Teilmengen sind zulaessig (die Wache weist nicht alles ab)", zulaessig == 4);
    check_true("(h) genau 3 Teilmengen werden abgewiesen (die Wache laesst nicht alles durch)", abgewiesen == 3);
    check_true("(h) BYTE-BILANZ: die Vollmenge [all] bleibt zulaessig -- der gesamte Bestand ist unberuehrt",
               !ruf("[all]").warf);
}

// ---------------------------------------------------------------------------------------------
// (d) INJEKTIVITAET (Owner-KERN F6) ueber alle BAUBAREN Teilmengen der Registry. Das ist die
//     Aussage, die der Gate-Teil ALLEIN heute nicht tragen koennte: {macro} und {micro} teilen ein
//     Gate. Der Deklarations-Teil (COMDARE_MEASUREMENT_TOOLING_<ID>) traegt sie.
//     M-1/H-1 hat den Definitionsbereich verkleinert: von den 7 nicht-leeren Teilmengen sind die 4
//     wallclock-tragenden baubar, die 3 uebrigen werden abgewiesen (Buchhaltung in Fall (h)).
//     GEGENPROBE gegen ein trivial gruenes Ergebnis: es werden wirklich 4 Mengen gebildet und
//     4*3/2 = 6 Paare verglichen -- beide Zahlen werden ausgegeben, nicht behauptet.
// ---------------------------------------------------------------------------------------------
void fall_d_injektiv() {
    std::cout << "\n---- (d) INJEKTIVITAET (F6): alle 7 nicht-leeren Tooling-Mengen paarweise verschieden ----\n";
    std::vector<std::string>              legenden;
    std::vector<std::vector<std::string>> vektoren;
    for (unsigned mask = 1; mask < (1u << cm::kMeasurementToolingCount); ++mask) {
        std::string legend = "[";
        for (std::size_t i = 0; i < cm::kMeasurementToolingCount; ++i)
            if ((mask >> i) & 1u) {
                if (legend.size() > 1) legend += ',';
                legend += std::string{cm::kMeasurementToolingRegistry[i].id};
            }
        legend += "]";
        Lauf const l = ruf(legend);
        if (l.warf) {
            // M-1/H-1: die 3 Teilmengen ohne 'wallclock' sind ab jetzt KEIN baubarer Zustand mehr (die
            // Vollstaendigkeits-Buchhaltung dazu fuehrt Fall (h)). Injektivitaet ist die Aussage ueber die
            // BAUBAREN Wahlen -- ueber die abgewiesenen etwas zu behaupten, waere Unsinn.
            std::cout << "    " << legend << "  ->  ABGEWIESEN (H-1: nennt wallclock nicht)\n";
            continue;
        }
        std::cout << "    " << legend << "  ->  " << join(l.defs) << "\n";
        legenden.push_back(legend);
        vektoren.push_back(l.defs);
    }
    std::cout << "  baubare Mengen: " << vektoren.size() << " (erwartet 4 = die wallclock-tragenden)\n";
    check_true("(d) alle 4 baubaren Mengen aufgeloest", vektoren.size() == 4);
    std::size_t paare  = 0;
    std::size_t gleich = 0;
    for (std::size_t i = 0; i < vektoren.size(); ++i)
        for (std::size_t j = i + 1; j < vektoren.size(); ++j) {
            ++paare;
            if (vektoren[i] == vektoren[j]) {
                std::cout << "  [ERR] KOLLISION: " << legenden[i] << " == " << legenden[j] << "\n";
                ++gleich;
            }
        }
    std::cout << "  Paare verglichen: " << paare << " (erwartet 6), Kollisionen: " << gleich << "\n";
    check_true("(d) 6 Paare wirklich verglichen (der Test kann nicht leer gruen sein)", paare == 6);
    check_true("(d) KEINE zwei Mess-Achsen-Wahlen kollidieren auf denselben Define-Vektor", gleich == 0);
}

// ---------------------------------------------------------------------------------------------
// (e) BYTE-BILANZ: die Vollmenge ([all] == der gesamte golden-/CI-Bestand) traegt die Gates in
//     Schichten-Reihenfolge und mit exakt diesem Wortlaut. [HISTORIE bis B2: ZWEI Gates im
//     Vor-M-1-Wortlaut.] Seit B2 (15.08.2026, Gate-Trennung G2/G3 -- das DEKLARIERTE
//     GOLDEN-EREIGNIS der Naht, s. mess_achsen_naht.hpp BYTE-BILANZ B2) steht das dritte Gate
//     -DCOMDARE_CE_ENABLE_SEGMENT_TIMING=1 an Position 2. Wer diese Zeilen bricht, bewegt den
//     produktiven Bau -- genau dafuer stehen sie hier.
// ---------------------------------------------------------------------------------------------
void fall_e_byte_bilanz() {
    std::cout << "\n---- (e) BYTE-BILANZ: [all] traegt die Gates in Schichten-Reihenfolge (B2-Wortlaut) ----\n";
    Lauf const all = ruf("[all]");
    check_true("(e) [all] wirft nicht", !all.warf);
    check_true("(e) Gate 1 an Position 0: -DCOMDARE_MEASUREMENT_ON=1",
               !all.defs.empty() && all.defs[0] == "-DCOMDARE_MEASUREMENT_ON=1");
    check_true("(e) Gate 2 an Position 1: -DCOMDARE_CE_ENABLE_STATISTICS=1",
               all.defs.size() > 1 && all.defs[1] == "-DCOMDARE_CE_ENABLE_STATISTICS=1");
    check_true("(e) Gate 3 an Position 2: -DCOMDARE_CE_ENABLE_SEGMENT_TIMING=1 (B2)",
               all.defs.size() > 2 && all.defs[2] == "-DCOMDARE_CE_ENABLE_SEGMENT_TIMING=1");
    // Die leere Legende ist der UNGESETZT-Fall der Stempel-Seite; die Bau-Seite loest ihn nach "[all]"
    // auf (resolve_live_measurement_combo_legend), NICHT hier -- hier ist er ein Wurf.
    check_true("(e) [all] deklariert alle 3 Toolings (3 Gates + 3 Deklarationen, B2)",
               all.defs.size() == 3 + cm::kMeasurementToolingCount);
}

// ---------------------------------------------------------------------------------------------
// (f) FEHLERKLASSEN (fail-loud statt stiller Vollmenge): leere Legende, unbekannte id. Beides darf
//     NICHT still zu einer Ausstattung fuehren, die der Bediener nicht bestellt hat.
// ---------------------------------------------------------------------------------------------
void fall_f_fehlerklassen() {
    std::cout << "\n---- (f) FEHLERKLASSEN: leere Legende und Tippfehler werfen ----\n";
    Lauf const leer      = ruf("");
    Lauf const tipp      = ruf("[wallclok]");
    Lauf const leerinnen = ruf("[]");
    zeig("''         ", leer);
    zeig("[wallclok] ", tipp);
    zeig("[]         ", leerinnen);
    check_true("(f) leere Legende wirft (kein baubarer Zustand)", leer.warf);
    check_true("(f) leere Legende meldet die Fehlerklasse",
               leer.warf && leer.was.find("fehlerklasse=konfiguration_widerspruch") != std::string::npos);
    check_true("(f) unbekanntes Tooling wirft (kein stiller Ausstattungs-Verlust)", tipp.warf);
    check_true("(f) unbekanntes Tooling meldet die Fehlerklasse",
               tipp.warf && tipp.was.find("fehlerklasse=konfiguration_widerspruch") != std::string::npos);
    check_true("(f) '[]' ist die Vollmenge (Section-64-Semantik, wie im Stempel-Renderer)",
               !leerinnen.warf && leerinnen.defs == ruf("[all]").defs);
}

// ---------------------------------------------------------------------------------------------
// (g) EINE AUFLOESUNG, ZWEI VERBRAUCHER: der Deklarations-Define-Name kommt aus der Registry-id und
//     ist kein Literal je Tooling. Ein viertes Tooling bekaeme seinen Namen automatisch.
// ---------------------------------------------------------------------------------------------
void fall_g_namen_aus_registry() {
    std::cout << "\n---- (g) Deklarations-Namen stammen aus der Registry (kein Literal je Tooling) ----\n";
    for (std::size_t i = 0; i < cm::kMeasurementToolingCount; ++i) {
        std::string const name = pf::mess_tooling_deklarations_define(i);
        std::cout << "    id='" << cm::kMeasurementToolingRegistry[i].id << "'  ->  " << name << "\n";
        std::string erwartet{"-DCOMDARE_MEASUREMENT_TOOLING_"};
        for (char const c : cm::kMeasurementToolingRegistry[i].id)
            erwartet.push_back(c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c);
        erwartet += "=1";
        check_true("(g) Name aus id gebildet", name == erwartet);
    }
}

} // namespace

int main() {
    std::cout << "== M-1/D-1 BISS: die Mess-Achse wirkt auf die Tier-Defines ==\n";
    fall_a_nenner();
    fall_b_paarweise_verschieden();
    fall_c_gate_wirkung();
    fall_h_g1_deklarationspflicht(); // M-1/H-1 -- steht vor (d), weil (d) seinen Definitionsbereich nutzt
    fall_d_injektiv();
    fall_e_byte_bilanz();
    fall_f_fehlerklassen();
    fall_g_namen_aus_registry();
    std::cout << "\n== Fehler: " << g_fail << " ==\n";
    return g_fail == 0 ? 0 : 1;
}
