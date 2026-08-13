// profile_facade/maschinen_deklarations_naht.hpp -- S-3c (13.08.2026): DIE EINE Belegungs-Naht der
// aktiven Maschinen-Deklaration.
//
// WAS SIE SCHLIESST: set_active_machine_declaration (measurement/simd_build_gate.hpp) hatte 0
// Produktions-Aufrufer; der Fallback system_axis_host_supports_simd las per __builtin_cpu_supports
// die echte Host-CPUID (profile_run_entry.hpp, der dortige Kommentar benennt die Luecke selbst:
// "heute der Normalfall, weil die CEB set_active_machine_declaration noch nicht ruft"). Diese Naht
// ist DER EINE AUFRUF -- verdrahtet in run_experiment_profile_facade (profile_run_facade.cpp, NACH
// Parse+validate, VOR der Delegation an die Perm-Schleife). KEIN zweiter Kanal, KEINE Heuristik,
// KEIN CMake-Schalter.
//
// DIE ZWEI IDENTITAETS-BEGRIFFE (O-4, machine_identity.hpp Kopf): der HOSTNAME ist NUR
// Lookup-Schluessel gegen <machine hostname_hint=..> (Instanz-Eigenschaft); die IDENTITAET ist das
// Eigenschafts-Tupel (cpu_fabrication, ram_pair). Genau so belegt diese Naht: Treffer ueber den
// Hint, Belegung ueber das Tupel.
//
// FAIL-CLOSED (der NATUERLICHE KILL-SWITCH): kein Treffer, keine <machines>-Menge oder kein
// ermittelbarer Hostname => NICHTS wird belegt -- das Gate bleibt inert, der ehrliche
// CPUID-Fallback traegt weiter. Sobald die Belegung steht, gewinnt die Deklaration; der Fallback
// verschwindet von selbst (profile_run_entry.hpp, "WARUM DIE HOST-PROBE BLEIBT").
//
// DAS [[nodiscard]]-URTEIL WIRD BEHANDELT, NIE VERSCHLUCKT: Gesetzt/BereitsGesetztIdentisch sind in
// Ordnung; AbgelehntAbweichend (zweiter Treffer mit ANDEREM Tupel -- eine widerspruechliche
// <machines>-Deklaration fuer denselben Host) ist ein KLASSIFIZIERTER Fehler-Log (D1,
// konfig_xml_parse), KEIN Abbruch: die Erstbelegung bleibt, der Lauf misst weiter (dieselbe
// D1-Semantik wie an den Nachbar-Naehten).
//
// ASCII-only, header-only (measurement/ ist durchgaengig header-only; eine .cpp braeuchte einen
// CMake-Eintrag -- Paragraf 73.1: CMake sparsam).

#pragma once

#include "xml_config_parser/xml_config_parser.hpp" // cx::ExperimentMachine (<machines><machine>)

#include <cache_engine/measurement/axis_error.hpp>      // CompilerCompilerErrorClass / error_class_label
#include <cache_engine/measurement/simd_build_gate.hpp> // set_active_machine_declaration + Urteils-Enum

#include <cstddef>
#include <ostream>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::profile_facade {

/// Der Befund der Belegung -- fuer Log/Diagnose UND fuer den Test (T-6): wie viele <machine> den
/// Hostnamen trafen, ob eine Belegung steht, wie viele abweichende Zweit-Treffer abgelehnt wurden.
struct MaschinenDeklarationsBefund {
    std::size_t treffer   = 0;     ///< <machine>-Zeilen mit hostname_hint == live_host
    std::size_t abgelehnt = 0;     ///< davon AbgelehntAbweichend (widerspruechliches Tupel)
    bool        belegt    = false; ///< mindestens einmal Gesetzt/BereitsGesetztIdentisch
};

/// DER EINE AUFRUF (Zielform KON11-01-Muster: eine Naht, alle Verbraucher hoeren mit): haelt
/// `live_host` gegen die hostname_hint-Spalte der <machines>-Menge und belegt beim Treffer die
/// aktive Maschinen-Deklaration mit dem Eigenschafts-Tupel. Leerer Hostname/leerer Hint zaehlt NIE
/// als Treffer (kein Raten). `fehler_log` empfaengt AUSSCHLIESSLICH den klassifizierten
/// Konflikt-Fall; die stillen Faelle (kein Treffer) schreiben NICHTS -- Inertheit ist dort die
/// gewollte Antwort, kein Fehler.
[[nodiscard]] inline MaschinenDeklarationsBefund
belege_aktive_maschinen_deklaration(std::string_view                                               live_host,
                                    std::vector<::comdare::builder::xml::ExperimentMachine> const& machines,
                                    std::ostream&                                                  fehler_log) {
    namespace cm = ::comdare::cache_engine::measurement;
    MaschinenDeklarationsBefund befund{};
    if (live_host.empty()) return befund; // Hostname nicht ermittelbar -> keine Maschine gewaehlt
    for (auto const& mc : machines) {
        if (mc.hostname_hint.empty() || mc.hostname_hint != live_host) continue; // NUR Lookup-Schluessel
        ++befund.treffer;
        switch (cm::set_active_machine_declaration(mc.cpu_fabrication, mc.ram_pair)) {
            case cm::MachineDeclarationSetResult::Gesetzt:
            case cm::MachineDeclarationSetResult::BereitsGesetztIdentisch: befund.belegt = true; break;
            case cm::MachineDeclarationSetResult::AbgelehntAbweichend:
                ++befund.abgelehnt;
                fehler_log << "[Compiler-Compiler-Fehler: "
                           << cm::error_class_label(cm::CompilerCompilerErrorClass::KonfigXmlParse)
                           << "] S-3c: <machine id=\"" << mc.id << "\"> traegt hostname_hint=\"" << mc.hostname_hint
                           << "\" mit ABWEICHENDEM Eigenschafts-Tupel (cpu_fabrication=\"" << mc.cpu_fabrication
                           << "\", ram_pair=\"" << mc.ram_pair
                           << "\") gegenueber der bereits belegten Deklaration desselben Laufs. Die"
                              " Einmal-Belegung lehnt den Zweit-Treffer ab (Determinismus-Schutz:"
                              " keine halbe Perm-Schleife unter anderer Freigabe); die Erstbelegung"
                              " bleibt bestehen, der Lauf misst weiter (D1, kein Abbruch). Die"
                              " <machines>-Menge widerspricht sich fuer diesen Host -- Deklaration"
                              " in der Anwender-XML bereinigen.\n";
                break;
        }
    }
    return befund;
}

} // namespace comdare::cache_engine::profile_facade
