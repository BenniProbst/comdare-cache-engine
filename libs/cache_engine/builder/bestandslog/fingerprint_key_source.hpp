#pragma once
// -----------------------------------------------------------------------------
// fingerprint_key_source.hpp -- G4b-1 / AUF-A1..A4: der EINE reale bestand_key_of-Provider.
//
// Er liest den Lager-Index-Schluessel einer fertig gebauten Binary aus deren
// `.fingerprint`-Sidecar (I2, geschrieben von write_fingerprint_sidecar) und liefert ihn als
// 128-hex-String. Damit wird aus dem bisher NUR host-injizierten Platzhalter der Naht
// LazyRunConfig::bestand_key_of (cache_engine_builder_iterator.hpp:216) ein produktiver Wert.
//
// WARUM NICHT IN artifact_cache_transport.hpp (AUF-A1): jene Datei ist die EINE Kante
// bestandslog -> artifact_transport (ihr Kopf sagt das ausdruecklich). Ein key_of, das eine LOKALE
// Datei neben der Binary liest, hat mit dem ArtifactCache nichts zu tun -- es beruehrt weder mc noch
// minio noch einen Objekt-Key. Getrennte Datei, damit die Kante ihre Aussage behaelt.
//
// DRIFT-FREIHEIT (AUF-A2): das Suffix `.fingerprint` steht ausschliesslich in
// build_orchestrator/fingerprint_sidecar.hpp. Dieser Header inkludiert NUR jenen (nicht
// build_orchestrator.hpp) -- der Schreiber und der Leser des Sidecars teilen eine Wahrheit ueber
// den Dateinamen, ohne dass der Leser die Orchestrator-Welt (<thread>/<condition_variable>/spawn)
// mitzieht.
//
// TYP (AUF-A4): der Rueckgabetyp ist EXAKT der Iterator-Feldtyp
//   std::function<std::optional<std::string>(std::filesystem::path const&)>
// und ausdruecklich NICHT experiment::FingerprintFn (build_orchestrator.hpp:174), die
// std::string -> std::string ist. Die beiden Namen sind benachbart und bedeuten Verschiedenes: die
// FingerprintFn BERECHNET den Fingerprint aus einer binary_id (Schreib-Seite), dieses key_of LIEST
// ihn von der Platte (Lese-Seite).
//
// TRIMMEND (AUF-A3, Befund B26): write_fingerprint_sidecar schreibt OHNE abschliessenden Newline.
// Jede fremd oder per Werkzeug erzeugte Datei mit '\n' (oder CRLF unter Windows) haette 129 bzw.
// 130 Zeichen -- key_from_hex (bestandslog_index.hpp:73) verlangt exakt 128 und haette still
// nullopt geliefert, also DedupOutcome::no_key: die Binary waere fuer das Lager unsichtbar, ohne
// eine einzige Log-Zeile. Deshalb wird VOR der Pruefung getrimmt.
//
// FEHLERKLASSEN dieser Naht -- alle vier fuehren zu nullopt und damit zu DedupOutcome::no_key.
// Sie sind bewusst STILL (kein Log): der Provider laeuft je gebauter Binary in den Bau-Workern,
// eine Zeile je Binary waere bei 2^17 Binaries ein Log-Bombardement. Sichtbar werden sie am
// no_key-Zaehler des LagerRunState, nicht hier.
//   sidecar_fehlt      -- Datei nicht vorhanden oder nicht oeffenbar (der Normalfall bei
//                         hydrierten Binaries: push_tier_binary schiebt `.fingerprint` NICHT mit,
//                         s. KNOWN GAP AUF-A5)
//   sidecar_leer       -- Datei da, Inhalt nach dem Trim leer (abgebrochener Schreibvorgang)
//   laenge_verstoss    -- getrimmt != 128 Zeichen
//   zeichen_verstoss   -- 128 Zeichen, aber mindestens eines nicht hexadezimal
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), nur stdlib + der Sidecar-Pfad-Header.
// Kein Runtime-Switch, kein variant, keine Env-Abfrage (das Gate sitzt beim Host, AUF-B3).
// -----------------------------------------------------------------------------

#include "../build_orchestrator/fingerprint_sidecar.hpp" // fingerprint_sidecar_path -- die EINE Suffix-Wahrheit

#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <system_error> // std::error_code fuer das exception-freie exists()

namespace comdare::cache_engine::builder::bestandslog {

namespace detail {

// Whitespace nach POSIX-Sicht, ohne <cctype>/Locale (der Inhalt ist reiner ASCII-Hex).
[[nodiscard]] inline bool is_trimmable(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

// Beidseitiger Trim (AUF-A3). Gibt eine Sicht in `s` zurueck -- der Aufrufer haelt `s` am Leben.
[[nodiscard]] inline std::string_view trim_view(std::string const& s) noexcept {
    std::size_t b = 0;
    std::size_t e = s.size();
    while (b < e && is_trimmable(s[b])) ++b;
    while (e > b && is_trimmable(s[e - 1])) --e;
    return std::string_view{s}.substr(b, e - b);
}

// Genau die Zeichenmenge, die key_from_hex (bestandslog_index.hpp:75-80) akzeptiert -- beide
// Schreibweisen. Bewusst KEINE Normalisierung auf Kleinbuchstaben: der Wert reist unveraendert zum
// Konsumenten (LagerRunState::observe), und diese Naht erfindet keinen Inhalt.
[[nodiscard]] inline bool is_hex_128(std::string_view v) noexcept {
    if (v.size() != 128) return false;
    for (char const c : v) {
        bool const hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (!hex) return false;
    }
    return true;
}

} // namespace detail

// Der Provider. `output` ist der Binary-Ausgabepfad, den der Orchestrator gebaut hat (BuildResult::
// output) -- das Sidecar liegt daneben als `<output>.fingerprint`.
[[nodiscard]] inline std::function<std::optional<std::string>(std::filesystem::path const&)> make_fingerprint_key_fn() {
    return [](std::filesystem::path const& output) -> std::optional<std::string> {
        std::error_code ec;
        auto const      sidecar = experiment::fingerprint_sidecar_path(output);
        // exists() mit error_code: ein I/O-Fehler auf dem Pfad ist sidecar_fehlt, keine Exception
        // (der Provider laeuft in den Bau-Workern).
        if (!std::filesystem::exists(sidecar, ec) || ec) return std::nullopt; // sidecar_fehlt
        std::ifstream f{sidecar, std::ios::binary};
        if (!f) return std::nullopt; // sidecar_fehlt
        std::string const raw{std::istreambuf_iterator<char>{f}, std::istreambuf_iterator<char>{}};
        auto const        v = detail::trim_view(raw);
        if (v.empty()) return std::nullopt;              // sidecar_leer
        if (!detail::is_hex_128(v)) return std::nullopt; // laenge_verstoss / zeichen_verstoss
        return std::string{v};
    };
}

} // namespace comdare::cache_engine::builder::bestandslog
