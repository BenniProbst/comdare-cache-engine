// measurement/axis_version_stamp.hpp -- Formatter der einkompilierten Achsen-Versions-Stempel-Zeilen
// (Bau W12-A, Section 43). Der Kern-Baustein von kSystemAxisVersionLine / kOrganAxisVersionLine.
//
// Section 43 (User-Direktive): jede Haupt-Achse traegt ihren gewaehlten Algorithmus + dessen Version als
// string_view in die Tier-Binary (Versionierung -> chirurgische Cache-Invalidierung). Diese Datei baut die
// lesbare Zeile "achse=algorithmus@X.Y.Z;..." je Achsen-Kategorie.
//
// KRITISCH (Byte-Wache, Section 43): Dies ist eine SEPARATE Formatier-Welt zur .algos-Signatur
// (compose_algo_signature). Der Stempel nutzt die X.Y.Z-VOLL-Form (algo_semver_string), die .algos-Sig
// behaelt die ROHE Version verbatim -- kein gemeinsamer Formatter (Entscheid W12-A-3: getrennte Felder).
// FLAG-GRAMMATIK v2 (Owner-KERN 07.08.2026): mit dem Wegfall des 'v'-Praefixes fallen rohe und gerenderte
// Form zeichengleich zusammen. Die TRENNUNG der beiden Welten bleibt trotzdem bestehen -- sie ist eine
// Aussage ueber die QUELLE (Literal vs. Stempel), nicht ueber die Schreibweise.
// Aenderungen hier beruehren die .algos-Sig / den 131072er-Cache NICHT (eigener Test + Byte-Wache belegen es).
//
// Reihenfolge = Eingabe-Reihenfolge; der Aufrufer liefert die kanonische compose-Ordnung (Entscheid W12-A-5).
// NUR Haupt-Achsen (Section 42.b: Unter-Achsen sind Laufzeit, nie Stempel-Bestandteil). Metaprog: reine
// Freifunktionen + POD, keine vtable, kein std::variant.

#pragma once

#include <cache_engine/measurement/algo_semver.hpp>

#include <span>
#include <string>
#include <string_view>

namespace comdare::cache_engine::measurement {

/// Eine Haupt-Achsen-Zuordnung fuer die Stempel-Zeile: Achse -> gewaehlter Algorithmus -> dessen roher
/// algo_version-String (der Formatter parst ihn zur X.Y.Z-Voll-Form).
struct AxisVersionEntry {
    std::string_view axis;         // Haupt-Achsen-Name (z.B. "search_algo")
    std::string_view algorithm;    // gewaehlter Algorithmus == W::name() (z.B. "bst")
    std::string_view algo_version; // roher W::algo_version ("1.0.0.c"); WIRD ueber algo_semver_string kanonisiert
};

/// FLAG-GRAMMATIK v2 -- DIE FORM-WACHE DER EINTRAEGE, als constexpr-PRAEDIKAT statt als Nebenbedingung.
///
/// WOZU SIE DA IST (Owner-Direktive 07.08.2026: der Bestand SOLL brechen, aber LAUT): der Renderer unten
/// prueft selbst nicht und schluckt jede Zeichenfolge (seit K02/F2-7a constexpr-faehig, an der
/// Prueflosigkeit aendert das nichts) -- eine Fehlform faellt in algo_semver_string auf "0.0.0"
/// und reist als "Version unbekannt" weiter. Genau das ist die Alias-Identitaet, gegen die die
/// B11-Wachen eine Ebene tiefer gebaut sind: zwei ROH verschiedene Literale mit demselben Stempel-Segment.
/// AM OBJEKT GEMESSEN (Bissprobe 07.08.2026, vor dieser Wache): eine Composition mit dem ALT-Literal
/// "v1.0.0c" an allen 18 Organ-Achsen uebersetzte KLAGLOS und stempelte
/// "search_algo=algoALT@0.0.0;...persistence_target=algoALT@0.0.0" -- achtzehnmal ein Nicht-Stand, ohne
/// eine einzige Meldung.
///
/// WELCHE HAERTE HIER RICHTIG IST -- und welche NICHT: geprueft wird PARSBARKEIT
/// (version_is_parsable_or_documented_sentinel), NICHT die ce-CPU-Pflicht. Der Unterschied ist wichtig:
/// diese Zeile stempelt auch Fremd-Prueflinge und Test-Kompositionen, deren Versionen flaglos sein
/// duerfen; die CPU-Pflicht gilt fuer ce-EIGENE Registry-Varianten und wird dort (axis_variant_version_
/// table.hpp, die Registry-Header) durchgesetzt. Was hier NIE passieren darf, ist der STILLE Kollaps --
/// und genau den faengt die Parsbarkeit. Dieselbe Schaerfe fuehrt abi::anatomy_stamp_entries auf der
/// Ruecklese-Seite (dotted_version_is_wellformed).
///
/// Der dokumentierte Sentinel "0.0.0" bleibt ausdruecklich zulaessig: er ist die ABSICHT "Version
/// unbekannt" (axis_variant_version_table emittiert ihn fuer versionslose Eintraege), kein Unfall.
[[nodiscard]] constexpr bool axis_version_entries_are_wellformed(std::span<AxisVersionEntry const> entries) noexcept {
    for (AxisVersionEntry const& e : entries)
        if (!version_is_parsable_or_documented_sentinel(e.algo_version)) return false;
    return true;
}

/// Baut die Stempel-Zeile "achse=algorithmus@X.Y.Z[.flag]*;..." (kanonische Form via algo_semver_string).
/// Leere Eingabe -> "". SEPARATE Welt zur .algos-Sig: hier die kanonische Form, dort die rohe Version.
///
/// SIE PRUEFT NICHT SELBST: die Wache oben ist ein CONSTEXPR-Praedikat und gehoert an die Stelle, an der
/// die Eintraege noch TYPEN sind (organ_stamp_line<Comp>, system_stamp_line) -- nur dort kann sie
/// compile-time brechen. Hier waere sie ein Laufzeit-Zweig und damit genau die stille Degradierung, die
/// sie verhindern soll.
/// K02/F2-7a (2026-08-21): constexpr statt inline (verhaltensneutral) -- die Genus-Leistungs-
/// Komposition wertet diese EINE Grammatik-Funktion compile time aus; ein zweiter CT-Renderer waere
/// exakt die Drift-Quelle, gegen die der Kommentar oben argumentiert. Prueflos bleibt sie trotzdem.
[[nodiscard]] constexpr std::string build_axis_version_stamp_line(std::span<AxisVersionEntry const> entries) {
    std::string out;
    for (AxisVersionEntry const& e : entries) {
        if (!out.empty()) out += ';';
        out += e.axis;
        out += '=';
        out += e.algorithm;
        out += '@';
        out += algo_semver_string(e.algo_version); // X.Y.Z-Voll-Form (NICHT die rohe Version)
    }
    return out;
}

} // namespace comdare::cache_engine::measurement
