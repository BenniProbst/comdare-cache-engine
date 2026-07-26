#pragma once
// -----------------------------------------------------------------------------
// fingerprint_sidecar.hpp -- G4b-1 / AUF-A2: der EINE Ort, an dem das Suffix
// `.fingerprint` steht.
//
// EXTRACT, keine Neuerung: die Funktion stand bis 2026-07-26 in
// build_orchestrator.hpp (dort Z.298-300) und ist von dort hierher gezogen
// worden -- unveraendert, gleicher Namespace, gleiche Semantik. Der Orchestrator
// inkludiert diesen Header und benutzt sie fortan von hier.
//
// WARUM EIGENER HEADER (Single Source ohne Gewicht): der Lager-Binder
// bestandslog/fingerprint_key_source.hpp muss denselben Pfad BILDEN, den der
// Orchestrator SCHREIBT. Die beiden Alternativen waeren beide falsch gewesen:
// das Suffix im Binder zu duplizieren (Drift -- zwei Wahrheiten ueber denselben
// Dateinamen) oder build_orchestrator.hpp in den Binder zu ziehen (der zieht
// <thread>/<condition_variable>/<mutex>/spawn und die ganze Orchestrator-Welt
// in eine Datei, die nur einen Dateinamen braucht). Dieser Header kostet
// <filesystem> und <string> -- nichts weiter.
//
// I2: das VIERTE Sidecar `<output>.fingerprint` -- der 128-hex K7b-Fingerprint
// der Binary (Lager-Index-Schluessel, bestand_key_of liest es). Bewusst SEPARAT
// von .version/.algos/.variant (das ist der kompakte Provenienz-Anker, kein
// Skip-Kriterium). perm.dll.* bleiben byte-genau unveraendert.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), nur stdlib.
// -----------------------------------------------------------------------------

#include <filesystem>
#include <string>

namespace comdare::cache_engine::builder::experiment {

[[nodiscard]] inline std::filesystem::path fingerprint_sidecar_path(std::filesystem::path const& output) {
    return std::filesystem::path{output.string() + ".fingerprint"};
}

} // namespace comdare::cache_engine::builder::experiment
