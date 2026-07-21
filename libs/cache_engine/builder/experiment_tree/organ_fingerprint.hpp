#pragma once
// organ_fingerprint.hpp -- Cache-Resthygiene-2 (2026-07-21): die REINE sort+concat-Naht des Chunk-Organ-Fingerprints.
//
// Der S1-F1-Whole-Chunk-Marker traegt algo_sig = sha256 ueber `find <dll_dir> -name perm.dll.algos | LC_ALL=C sort |
// xargs cat`. Um die Marker-Wache SCHARF zu machen, muss der Treiber-Modus (--chunk-organ-fingerprint) BYTE-GENAU
// dasselbe Pre-Image (vor sha256) erzeugen -- OHNE DLL-Bau, rein aus dem Katalog (compose_algo_signature je Binary).
//
// Diese Datei kapselt NUR die reihenfolge-/konkatenations-KRITISCHE Logik (leicht, nur stdlib) -- damit sie EXAKT
// gegen `find|LC_ALL=C sort|xargs cat` kreuz-validiert werden kann, ohne die umbrella-schwere Entry-/Katalog-Kette.
//
// SORT-INVARIANTE (kreuz-validiert): die per-Binary-Ordner heissen <dll_dir>/<stem>/perm.dll.algos; `find|LC_ALL=C sort`
// sortiert nach VOLLPFAD. Der stem (orch_make_stem) besteht NUR aus [A-Za-z0-9_] (alle >= 0x30 > '/' 0x2F). Nach einem
// Praefix-stem folgt im Pfad das '/' (0x2F), das KLEINER ist als jede alnum/_-Fortsetzung -> ein Praefix-stem sortiert
// stets vor seinen Erweiterungen -- in Pfad-Sort UND stem-Sort gleich. Also: stem-Sort (std::string<) == LC_ALL=C-
// Pfadsort. .algos werden OHNE Trenner konkateniert (write_algos_sidecar schreibt kein trailing newline == xargs cat).

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// Sortiert (nach stem, byte-weise == LC_ALL=C-Pfadsort) und konkateniert OHNE Trenner die algo_sig-Werte -> EXAKT die
/// Bytes von `find <dll_dir> -name perm.dll.algos | LC_ALL=C sort | xargs cat`. pairs = (stem, algo_sig); leere algo_sig
/// gehoert NICHT hinein (im find nicht enthalten -- der Aufrufer filtert). Rein, stdlib-only, deterministisch.
[[nodiscard]] inline std::string
organ_fingerprint_preimage_from_pairs(std::vector<std::pair<std::string, std::string>> pairs) {
    std::sort(pairs.begin(), pairs.end(), [](auto const& x, auto const& y) { return x.first < y.first; });
    std::string out;
    for (auto const& p : pairs) out += p.second;
    return out;
}

} // namespace comdare::cache_engine::builder::experiment
