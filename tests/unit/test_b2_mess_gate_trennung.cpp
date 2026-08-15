// tests/unit/test_b2_mess_gate_trennung.cpp -- B2 GATE-TRENNUNG G2(macro)/G3(micro) (15.08.2026).
//
// DER KOEDER (T-1, rot zuerst): dieser Test VERLANGT die Ausstattung "Macro AN, Micro AUS" als eigenen
// Gate-Zustand des Tier-Kompilats. Am Vor-B2-Stand ist das UNERFUELLBAR -- G2 (fill_observer_v3) und
// G3 (fill_segment_timing_v3) teilen EIN Gate COMDARE_CE_ENABLE_STATISTICS (mess_achsen_naht.hpp,
// Abschnitt "EHRLICHE GRENZE DIESER SCHEIBE"): [wallclock,macro] und [wallclock,micro] erzeugen
// denselben GATE-Zustand und unterscheiden sich nur im Deklarations-Define. Der Rot-Nachweis am
// Basis-Stand 0817c7bf ist im Bericht der Scheibe woertlich protokolliert.
//
// NACH B2 gilt (und genau das prueft dieser Test literal):
//   G2 (macro) : COMDARE_CE_ENABLE_STATISTICS      -- unveraendert, #ifdef, alle Bestands-Stellen.
//   G3 (micro) : COMDARE_CE_ENABLE_SEGMENT_TIMING  -- eigenes Gate, wertbasiert (#if), die EINE
//                Ableitung wohnt in cache_engine/abi/mess_gate_segment_timing.hpp.
//   Naht       : mess_achsen_defines() entscheidet das G3-Gate IMMER MIT, sobald G2 emittiert wird
//                (=1 bei micro, =0 sonst) -- aus der EIGENEN Menge der CEB (KON37-01: die CEB baut
//                hoehere Stufen nur nach ihren eigenen Messeigenschaften, nie aus der Env).
//   Vererbung  : OHNE explizites G3-Define folgt G3 dem Alt-Gate G2 (Ableitungs-Header). Jeder
//                CMake-/[all]-Bestandsbau behaelt damit seinen Gate-Zustand (G2+G3 an) unveraendert.
//
// PAAR-DOKTRIN: jede Koeder-Haelfte (K*) traegt ihre Gegenprobe (G*). Die G*-Haelften sind am
// HEUTIGEN Stand UND nach B2 gruen -- sie beweisen, dass der Koeder das Bestehende nicht umdeutet;
// die K*-Haelften sind am heutigen Stand rot und nach B2 gruen -- sie beweisen die Trennung.
//
// ASCII-only. Header-only-Konsum der Mess-Naht; kein Makro-Zustand dieser TU geht in die Pruefung ein
// (alle Aussagen laufen ueber die REINE Abbildung mess_achsen_defines_for_legend und den Spiegel).

#include <profile_facade/mess_achsen_naht.hpp>

#include <cache_engine/abi/mess_gates_glied.hpp>

#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace pf  = ::comdare::cache_engine::profile_facade;
namespace cea = ::comdare::cache_engine::abi;

namespace {

int g_fail = 0;

void check_true(std::string const& was, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << "\n";
    if (!ok) ++g_fail;
}

[[nodiscard]] bool hat(std::vector<std::string> const& v, std::string_view d) {
    return std::find(v.begin(), v.end(), std::string{d}) != v.end();
}

/// Traegt der Vektor IRGENDEIN Define mit diesem Praefix? (fuer die "gar nicht entschieden"-Faelle)
[[nodiscard]] bool hat_praefix(std::vector<std::string> const& v, std::string_view praefix) {
    for (auto const& d : v)
        if (std::string_view{d}.substr(0, praefix.size()) == praefix) return true;
    return false;
}

/// Der GATE-Teil eines mess-gates-Glieds: alle Felder OHNE die drei Deklarations-Segmente (tw/tm/tmi).
/// Bewusst ueber die Feld-Zerlegung des Glied-Headers gebaut, nicht ueber einen Zweit-Parser.
[[nodiscard]] std::string gate_teil(std::string const& glied) {
    std::string aus;
    for (std::size_t i = 0;; ++i) {
        std::string_view const f = cea::mess_gates_feld(glied, i);
        if (f.empty()) break;
        // Deklarations-Segmente beginnen mit 't' (tw/tm/tmi) -- alle Gate-Segmente (m/s/st/x) nicht.
        if (f.front() == 't') continue;
        if (!aus.empty()) aus += ';';
        aus += std::string{f};
    }
    return aus;
}

} // namespace

int main() {
    std::cout << "== B2: GATE-TRENNUNG G2(macro)/G3(micro) -- Koeder + Gegenproben ==\n";
    std::string const g3an  = "-DCOMDARE_CE_ENABLE_SEGMENT_TIMING=1";
    std::string const g3aus = "-DCOMDARE_CE_ENABLE_SEGMENT_TIMING=0";
    std::string const g2an  = "-DCOMDARE_CE_ENABLE_STATISTICS=1";

    auto const wc  = pf::mess_achsen_defines_for_legend("[wallclock]");
    auto const ma  = pf::mess_achsen_defines_for_legend("[wallclock,macro]");
    auto const mi  = pf::mess_achsen_defines_for_legend("[wallclock,micro]");
    auto const all = pf::mess_achsen_defines_for_legend("[all]");

    std::cout << "\n---- KOEDER K1-K4: Macro-an/Micro-aus ist ein EIGENER Gate-Zustand ----\n";
    // K1: die Bestellung "[wallclock,macro]" baut den Observer OHNE die Segment-Timer -- das G3-Gate
    // wird ausdruecklich AUS-entschieden (=0), nicht bloss weggelassen (Weglassen hiesse Vererbung).
    check_true("K1 [wallclock,macro] entscheidet G3 AUS (" + g3aus + " im Vektor)", hat(ma, g3aus));
    // K2/K3: micro (und die Vollmenge) bauen die Segment-Timer ein -- G3 AN-entschieden.
    check_true("K2 [wallclock,micro] entscheidet G3 AN (" + g3an + " im Vektor)", hat(mi, g3an));
    check_true("K3 [all] entscheidet G3 AN (" + g3an + " im Vektor)", hat(all, g3an));
    // K4: der GATE-Teil des neunten Glieds unterscheidet macro von micro -- nicht nur der
    // Deklarations-Teil. Vor B2 ist der Gate-Teil beider Legenden byte-gleich (die "EHRLICHE GRENZE").
    std::string const glied_ma = pf::mess_gates_glied_for_legend("[wallclock,macro]");
    std::string const glied_mi = pf::mess_gates_glied_for_legend("[wallclock,micro]");
    std::cout << "    gate_teil(macro) = " << gate_teil(glied_ma) << "\n";
    std::cout << "    gate_teil(micro) = " << gate_teil(glied_mi) << "\n";
    check_true("K4 GATE-Teil des Glieds: macro != micro (Trennung ist im Kompilat, nicht nur deklariert)",
               gate_teil(glied_ma) != gate_teil(glied_mi));

    std::cout << "\n---- GEGENPROBEN G1-G5: der Bestand behaelt seine Bedeutung ----\n";
    // G1: [wallclock] traegt weder G2 noch irgendeine G3-Entscheidung -- unterhalb des Observers gibt
    // es keine Segment-Timer-Schicht, also auch nichts zu entscheiden.
    check_true("G1 [wallclock] ohne G2 (wie seit M-1/D-1)", !hat(wc, g2an));
    check_true("G1 [wallclock] ohne jede G3-Entscheidung (kein -DCOMDARE_CE_ENABLE_SEGMENT_TIMING*)",
               !hat_praefix(wc, "-DCOMDARE_CE_ENABLE_SEGMENT_TIMING"));
    // G2-Emission unveraendert: macro/micro/all tragen den Observer, wallclock nicht (M-1/H-B).
    check_true("G2 Observer-Gate unveraendert in [wallclock,macro]", hat(ma, g2an));
    check_true("G2 Observer-Gate unveraendert in [wallclock,micro]", hat(mi, g2an));
    check_true("G2 Observer-Gate unveraendert in [all]", hat(all, g2an));
    // G3-Schichtung: nie eine G3-AN-Entscheidung ohne G2 im selben Vektor (die #error-Wache des
    // Ableitungs-Headers haelt dieselbe Bedingung compile-hart fest).
    for (auto const* n : {"[wallclock]", "[wallclock,macro]", "[wallclock,micro]", "[all]"}) {
        auto const d = pf::mess_achsen_defines_for_legend(n);
        check_true(std::string{"G3 Schichtung in "} + n + ": G3=1 nur zusammen mit G2", !hat(d, g3an) || hat(d, g2an));
    }
    // K5/K6 (KOEDER-Haelften, am Vor-B2-Stand rot wie K1-K4): REIHENFOLGE bindend -- Gates in
    // Schichten-Reihenfolge VOR den Deklarationen; das G3-Define steht direkt hinter dem G2-Define.
    auto pos = [](std::vector<std::string> const& v, std::string_view d) {
        return static_cast<std::size_t>(std::find(v.begin(), v.end(), std::string{d}) - v.begin());
    };
    check_true("K5 [all]: G1 vor G2 vor G3 vor Deklarationen",
               pos(all, "-DCOMDARE_MEASUREMENT_ON=1") == 0 && pos(all, g2an) == 1 && pos(all, g3an) == 2 &&
                   pos(all, "-DCOMDARE_MEASUREMENT_TOOLING_WALLCLOCK=1") == 3);
    check_true("K6 [wallclock,macro]: G3-AUS-Entscheid direkt hinter G2", pos(ma, g2an) + 1 == pos(ma, g3aus));
    // G5: Determinismus -- dieselbe Legende liefert denselben Vektor und dasselbe Glied.
    check_true("G5 Determinismus Vektor", pf::mess_achsen_defines_for_legend("[all]") == all);
    check_true("G5 Determinismus Glied", pf::mess_gates_glied_for_legend("[wallclock,macro]") == glied_ma);

    std::cout << "\n== Fehler: " << g_fail << " ==\n";
    return g_fail == 0 ? 0 : 1;
}
