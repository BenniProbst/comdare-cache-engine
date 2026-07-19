// measurement/axis_version_stamp.hpp -- Formatter der einkompilierten Achsen-Versions-Stempel-Zeilen
// (Bau W12-A, Section 43). Der Kern-Baustein von kSystemAxisVersionLine / kOrganAxisVersionLine.
//
// Section 43 (User-Direktive): jede Haupt-Achse traegt ihren gewaehlten Algorithmus + dessen Version als
// string_view in die Tier-Binary (Versionierung -> chirurgische Cache-Invalidierung). Diese Datei baut die
// lesbare Zeile "achse=algorithmus@X.Y.Z;..." je Achsen-Kategorie.
//
// KRITISCH (Byte-Wache, Section 43): Dies ist eine SEPARATE Formatier-Welt zur .algos-Signatur
// (compose_algo_signature). Der Stempel nutzt die X.Y.Z-VOLL-Form (algo_semver_string), die .algos-Sig
// behaelt die ROHE Version ("v1") -- kein gemeinsamer Formatter (Entscheid W12-A-3: getrennte Felder).
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
    std::string_view algo_version; // roher W::algo_version ("v1"); WIRD zu X.Y.Z geparst
};

/// Baut die Stempel-Zeile "achse=algorithmus@X.Y.Z;..." (Voll-Form via algo_semver_string). Leere Eingabe -> "".
/// SEPARATE Welt zur .algos-Sig: hier X.Y.Z, dort die rohe Version.
[[nodiscard]] inline std::string build_axis_version_stamp_line(std::span<AxisVersionEntry const> entries) {
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
