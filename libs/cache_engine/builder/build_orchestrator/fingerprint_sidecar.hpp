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
// I2 (HISTORIK, Stand bis 2026-08-05): das VIERTE Sidecar `<output>.fingerprint`
// -- der 128-hex K7b-Fingerprint der Binary (Lager-Index-Schluessel,
// bestand_key_of liest es). Bewusst SEPARAT von .version/.algos/.variant (das
// ist der kompakte Provenienz-Anker, kein Skip-Kriterium). perm.dll.* bleiben
// byte-genau unveraendert.
//
// A2-EICHUNG (GATE 5, F7 KONSOLIDIERT:72, 2026-08-05) -- LEBENDE WAHRHEIT: der
// Absatz darueber gilt fuer die Trennung der DATEIEN weiter, seine Klammer
// "kein Skip-Kriterium" ist seit der Eichung UEBERHOLT. `.fingerprint` ist
// jetzt BEIDES in einem: DAS Skip-Kriterium von dll_is_current UND der
// Lager-Index-Schluessel. Genau das ist die F7-Forderung "eine Schluessel-Welt":
// Skip-Gate == minio-Key == Bestandslog key_sha512. .version/.algos/.variant
// bleiben als Provenienz-Legende bzw. Transport-Vollstaendigkeits-Marke
// bestehen, entscheiden aber ueber KEINEN Skip mehr.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), nur stdlib.
// -----------------------------------------------------------------------------

#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace comdare::cache_engine::builder::experiment {

[[nodiscard]] inline std::filesystem::path fingerprint_sidecar_path(std::filesystem::path const& output) {
    return std::filesystem::path{output.string() + ".fingerprint"};
}

namespace detail {

// Whitespace nach POSIX-Sicht, ohne <cctype>/Locale (der Inhalt ist reiner ASCII-Hex).
[[nodiscard]] inline bool fp_is_trimmable(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

// Beidseitiger Trim (AUF-A3). Gibt eine Sicht in `s` zurueck -- der Aufrufer haelt `s` am Leben.
[[nodiscard]] inline std::string_view fp_trim_view(std::string const& s) noexcept {
    std::size_t b = 0;
    std::size_t e = s.size();
    while (b < e && fp_is_trimmable(s[b])) ++b;
    while (e > b && fp_is_trimmable(s[e - 1])) --e;
    return std::string_view{s}.substr(b, e - b);
}

// Genau die Zeichenmenge, die key_from_hex (bestandslog_index.hpp:75-80) akzeptiert -- beide
// Schreibweisen. Bewusst KEINE Normalisierung auf Kleinbuchstaben: der Wert reist unveraendert zum
// Konsumenten, und diese Naht erfindet keinen Inhalt.
[[nodiscard]] inline bool fp_is_hex_128(std::string_view v) noexcept {
    if (v.size() != 128) return false;
    for (char const c : v) {
        bool const hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (!hex) return false;
    }
    return true;
}

} // namespace detail

// A2-EICHUNG (GATE 5): die EINE LESE-WAHRHEIT des `.fingerprint`-Sidecars. Beide Leser der Naht
// benutzen ab jetzt genau diese Funktion:
//   (1) dll_is_current (build_orchestrator.hpp) -- das Skip-Gate,
//   (2) make_fingerprint_key_fn (bestandslog/fingerprint_key_source.hpp) -- der Lager-Index-Binder.
// Vorher las nur (2), und (1) verglich drei ANDERE Dateien; damit war die Gleichheit von Skip-Wert
// und Lager-Schluessel eine Frage der Disziplin. Jetzt ist sie eine Code-Konstruktion: EIN Leser,
// EIN Trim, EINE Formwache. Wer die Semantik aendert, aendert sie fuer beide Seiten gleichzeitig.
//
// FEHLERKLASSEN (alle vier -> nullopt; fuer (1) heisst nullopt FAIL-CLOSED = Neubau, fuer (2)
// DedupOutcome::no_key). Bewusst STILL (kein Log): die Funktion laeuft je Binary in den Bau-Workern,
// eine Zeile je Binary waere bei 2^17 Binaries ein Log-Bombardement.
//   sidecar_fehlt      -- Datei nicht vorhanden oder nicht oeffenbar
//   sidecar_leer       -- Datei da, Inhalt nach dem Trim leer (abgebrochener Schreibvorgang)
//   laenge_verstoss    -- getrimmt != 128 Zeichen
//   zeichen_verstoss   -- 128 Zeichen, aber mindestens eines nicht hexadezimal
//
// TRIMMEND (AUF-A3, Befund B26): write_fingerprint_sidecar schreibt OHNE abschliessenden Newline.
// Jede fremd oder per Werkzeug erzeugte Datei mit '\n' (oder CRLF unter Windows) haette 129 bzw. 130
// Zeichen und waere sonst still ungueltig. Deshalb wird VOR der Pruefung getrimmt.
[[nodiscard]] inline std::optional<std::string> read_fingerprint_sidecar(std::filesystem::path const& output) {
    std::error_code ec;
    auto const      sidecar = fingerprint_sidecar_path(output);
    // exists() mit error_code: ein I/O-Fehler auf dem Pfad ist sidecar_fehlt, keine Exception
    // (der Leser laeuft in den Bau-Workern).
    if (!std::filesystem::exists(sidecar, ec) || ec) return std::nullopt; // sidecar_fehlt
    std::ifstream f{sidecar, std::ios::binary};
    if (!f) return std::nullopt; // sidecar_fehlt
    std::string const raw{std::istreambuf_iterator<char>{f}, std::istreambuf_iterator<char>{}};
    auto const        v = detail::fp_trim_view(raw);
    if (v.empty()) return std::nullopt;                 // sidecar_leer
    if (!detail::fp_is_hex_128(v)) return std::nullopt; // laenge_verstoss / zeichen_verstoss
    return std::string{v};
}

} // namespace comdare::cache_engine::builder::experiment
