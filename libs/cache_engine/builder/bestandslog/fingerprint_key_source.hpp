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
//   sidecar_fehlt      -- Datei nicht vorhanden oder nicht oeffenbar. [ALT-WORTLAUT, HISTORIK bis
//                         #13: "der Normalfall bei hydrierten Binaries: push_tier_binary schiebt
//                         `.fingerprint` NICHT mit, s. KNOWN GAP AUF-A5".] NACHGEFUEHRT 2026-08-05
//                         (A2-Eichung): der KNOWN GAP AUF-A5 ist seit #13 GESCHLOSSEN --
//                         `perm.dll.fingerprint` steht in kOptionalTierSidecars
//                         (artifact_transport/artifact_cache.hpp:92) und reist damit in BEIDEN
//                         Richtungen mit (push_tier_binary, pull_tier_binary, prunable_artifacts
//                         lesen dieselbe eine Liste). Hydrierte Binaries sind also der REGELFALL
//                         MIT Anker; sidecar_fehlt bedeutet jetzt Alt-/Fremd-Bestand ohne Anker.
//   sidecar_leer       -- Datei da, Inhalt nach dem Trim leer (abgebrochener Schreibvorgang)
//   laenge_verstoss    -- getrimmt != 128 Zeichen
//   zeichen_verstoss   -- 128 Zeichen, aber mindestens eines nicht hexadezimal
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), nur stdlib + der Sidecar-Pfad-Header.
// Kein Runtime-Switch, kein variant, keine Env-Abfrage (das Gate sitzt beim Host, AUF-B3).
// -----------------------------------------------------------------------------

// fingerprint_sidecar_path -- die EINE Suffix-Wahrheit; seit der A2-Eichung (2026-08-05) zusaetzlich
// read_fingerprint_sidecar -- die EINE Lese-Wahrheit (Trim + 128-hex-Formwache), die dieser Provider
// und das Skip-Gate dll_is_current gemeinsam benutzen.
#include "../build_orchestrator/fingerprint_sidecar.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace comdare::cache_engine::builder::bestandslog {

// Der Provider. `output` ist der Binary-Ausgabepfad, den der Orchestrator gebaut hat (BuildResult::
// output) -- das Sidecar liegt daneben als `<output>.fingerprint`.
//
// A2-EICHUNG (GATE 5, 2026-08-05): der Rumpf (exception-freies exists, Trim AUF-A3, 128-hex-Wache,
// die vier Fehlerklassen oben) ist seither KEINE zweite Implementierung mehr, sondern eine
// DELEGATION an experiment::read_fingerprint_sidecar (build_orchestrator/fingerprint_sidecar.hpp).
// Verhalten byte-identisch -- der Grund fuer den Umzug ist die F7-Schluessel-Welt: seit der Eichung
// liest AUCH das Skip-Gate dll_is_current dieses Sidecar. Zwei Leser mit je eigenem Trim waeren
// genau die Drift, die dieser Header seit AUF-A2 auf der PFAD-Seite verhindert -- jetzt gilt sie
// auch fuer den INHALT. Die Typ-Unterscheidung aus AUF-A4 bleibt davon unberuehrt.
[[nodiscard]] inline std::function<std::optional<std::string>(std::filesystem::path const&)> make_fingerprint_key_fn() {
    return [](std::filesystem::path const& output) -> std::optional<std::string> {
        return experiment::read_fingerprint_sidecar(output);
    };
}

} // namespace comdare::cache_engine::builder::bestandslog
