#pragma once
// support/pmc_stdout_maskierung.hpp -- Test-Support: Maskierung der host-LEBENDIGEN PMC-Zeilenklasse
// vor Byte-Vergleichen von Planer-stdout auf PROZESS-Ebene (A2.2: EINE Quelle, kein Kopieren je TU).
//
// WARUM ES DIESEN HELFER GIBT (CI 16095, Job 383091): der Planer traegt GENAU EINE deklarierte
// host-abhaengige Zeilenklasse -- `pmc_befund=` im Plan-Text (I-PMC-2, Owner 10.08.2026: "erkennt
// jeder Planer auf jeder Maschine fuer sich"; experiment_plan_director.hpp nennt sie woertlich "die
// EINZIGE host-abhaengige Zeile des --dump-plan") und ihre Kommentar-Schwester "# PMC-BEFUND ..."
// der YAML-/CMake-Emissionen (pmc_befund_zeile, V-1). Ihr WERT ist eine PMU-Momentaufnahme JE
// PROZESSLAUF: unter PMU-Multiplexing (fremde perf-Nutzer, parallele ctest-Nachbarn der CI-Zelle)
// kippt ein grenzwertiges Event zwischen zwei Prozesslaeufen (383091: events=2/4 vs 3/4; die Klasse
// ist aelter als der pmcpaket-Umzug und im Probe-Kern pmc_event_biss.hpp dokumentiert, CI 16073/
// 382856). Ein Byte-Vergleich ZWEIER Prozesslaeufe muss deshalb auf den STABILEN Anteil pinnen --
// alles andere waere eine Zusicherung, die das Design ausdruecklich nicht gibt (V7).
//
// WAS DIE MASKIERUNG TUT (und was nicht):
//   - Sie ersetzt AUSSCHLIESSLICH den Zeilen-REST der zwei deklarierten Formen durch einen festen
//     Platzhalter; Einzug und Zeilen-Position bleiben erhalten. ALLE anderen Bytes reisen
//     unveraendert durch -- Mutationsanker der Verwender bleiben fuer sie scharf.
//   - Sie ZAEHLT die maskierten Zeilen (Struktur-Nenner). Verwender MUESSEN die Anzahl beidseitig
//     vergleichen und -- wo das Design die Existenz zusichert (plan dump: I-PMC-2; plan ci: V-1
//     "geht JEDER Treiber-Bau-Emission voran") -- eine Mindest-Anzahl fordern. So ist
//     Maskierungs-Drift LAUT: aendert die Emission ihr Praefix, greift die Maskierung nicht mehr
//     und die Mindest-Anzahl reisst den Test (keine stille Falsch-Null).
//   - Die PRODUKT-Emission ist unberuehrt (fail-loud #83): der Plan traegt weiter den Live-Befund.
//
// FORM (c) -- DEFINE-FLAECHE (Lead-Entscheid 23.08. nach CI 16095, Job 383091, Hunks @@ -54/-65/-104):
// beim LAGE-Kipp der Probe (unbrauchbar 0/4 <-> amd/intel) kippen die zwei PMC-Defines der
// emittierten "- cmake ..."-Zeilen per ANWESENHEIT (ceb_pmc_compile_define,
// experiment_plan_director.hpp: Unbrauchbar => leer, sonst " -DCOMDARE_ENABLE_PMC=ON
// -DCOMDARE_PMC_VENDOR=<amd|intel>"; das fuehrende Leerzeichen gehoert zum Token). Der Hunk @@ -65
// traegt KEINE begleitende "# PMC-BEFUND"-Zeile -- Form (b) allein deckt die Flaeche also NIE.
// Form (c) entfernt die zwei EXAKTEN Token-Literale beidseitig aus "- cmake "-Zeilen (die Zeile
// bleibt in der Unbrauchbar-Form zurueck => beidseitig identisch) und ZAEHLT die Funde.
// LEAD-LEITPLANKE (woertlich entschieden): KEIN beidseitiger Pflicht-Vergleich des Define-Zaehlers
// in den Verwendern -- die Anwesenheits-Asymmetrie ist die LEGITIME Folge des Lage-Kipps; der
// Zaehler dient der K13-Bissprobe und der Diagnose. Formen (a)/(b) behalten ihren strengen Nenner.
// DEKLARIERTE GRENZE: der Tier-Preflight-Zweig (PMC-TESTAT, Director-Z.1758ff) verzweigt ebenfalls
// an der Lage, liegt aber NUR in der --emit-tier-ci-Emission (Stufe 2) -- kein Gegenstand der
// plan-dump/ci/cmake/validate-Verwender; ein kuenftiger Byte-Vergleich jener Emission braucht eine
// eigene Behandlung.
//
// header-only, C++23. ASCII-only. Kein Zustand.

#include <cstddef>
#include <string>
#include <string_view>

namespace comdare::test {

/// Ergebnis der PMC-Maskierung: der stabile Text + die Anzahl maskierter Zeilen (Struktur-Nenner).
struct PmcMaskiert {
    std::string text;                ///< Eingabe mit strukturerhaltend maskierter PMC-Zeilenklasse
    std::size_t pmc_zeilen{};        ///< wie oft die Zeilenklasse (a)/(b) auftrat -- Pflicht-Vergleichsgroesse
    std::size_t pmc_define_tokens{}; ///< Form (c): entfernte Define-Paare -- KEIN beidseitiger Pflicht-
                                     ///< Vergleich (Anwesenheits-Kipp legitim, s. Kopf); Biss-/Diagnosewert
};

/// Maskiert AUSSCHLIESSLICH die deklarierte host-lebendige PMC-Klasse (s. Kopf):
///   (a) Plan-Text:  Zeile beginnt am Zeilenanfang mit `pmc_befund=`  (I-PMC-2-Emission)
///   (b) YAML/CMake: optionaler Einzug aus ' '/'\t', dann `# PMC-BEFUND` (pmc_befund_zeile, V-1)
///   (c) YAML/CMake: in Zeilen, deren Inhalt nach dem Einzug mit `- cmake ` beginnt, jedes
///       Vorkommen der zwei exakten PMC-Define-Token-Literale (vendor-ids amd|intel aus der
///       pmc_vendor_registry, s. vendor_id() in pmc_host_probe.hpp)
[[nodiscard]] inline PmcMaskiert pmc_zeilen_maskieren(std::string const& roh) {
    // Form (c): fuehrendes Leerzeichen gehoert zum Token (ceb_pmc_compile_define emittiert es).
    static constexpr std::string_view kDefineTokens[] = {
        " -DCOMDARE_ENABLE_PMC=ON -DCOMDARE_PMC_VENDOR=amd",
        " -DCOMDARE_ENABLE_PMC=ON -DCOMDARE_PMC_VENDOR=intel",
    };
    PmcMaskiert m{};
    std::size_t pos = 0;
    while (pos < roh.size()) {
        std::size_t const nl    = roh.find('\n', pos);
        std::size_t const ende  = (nl == std::string::npos) ? roh.size() : nl;
        std::string       zeile = roh.substr(pos, ende - pos);
        if (zeile.rfind("pmc_befund=", 0) == 0) { // (a) Plan-Text-Form
            zeile = "pmc_befund=<HOST-LEBENDIG>";
            ++m.pmc_zeilen;
        } else {
            std::size_t const inhalt = zeile.find_first_not_of(" \t"); // (b)/(c): Einzug ueberspringen
            if (inhalt != std::string::npos && zeile.compare(inhalt, 12, "# PMC-BEFUND") == 0) { // (b)
                zeile.replace(inhalt, std::string::npos, "# PMC-BEFUND <HOST-LEBENDIG>");
                ++m.pmc_zeilen;
            } else if (inhalt != std::string::npos && zeile.compare(inhalt, 8, "- cmake ") == 0) { // (c)
                for (std::string_view const token : kDefineTokens) {
                    for (std::size_t fund = zeile.find(token); fund != std::string::npos;
                         fund             = zeile.find(token, fund)) {
                        zeile.erase(fund, token.size());
                        ++m.pmc_define_tokens;
                    }
                }
            }
        }
        m.text += zeile;
        if (nl != std::string::npos) m.text += '\n';
        pos = ende + 1;
    }
    return m;
}

} // namespace comdare::test
