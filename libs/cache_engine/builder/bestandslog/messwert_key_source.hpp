#pragma once
// -----------------------------------------------------------------------------
// messwert_key_source.hpp -- LAG-P2: der EINE reale mess_bestand_key_of-Provider.
//
// SELBSTCHECK: diese Datei sichert zu, dass eine GEMESSENE ZELLE einen stabilen 128-hex-Schluessel
// bekommt, der die vermessene Binary UND die Maschine unterscheidet. Sie sichert NICHT zu, dass die
// Messung gueltig war (das entscheidet der Iterator ueber die Existenz der result.csv, bevor er
// ueberhaupt fragt), und sie rechnet NICHTS selbst -- der Digest kommt aus der MesswertKeyPolicy der
// B3-Factory ueber messwert_key_hex.
//
// DER BEFUND, DEN SIE SCHLIESST (OE-E-Dossier G-E3 / LAG-P2). Der Schreiber des zweiten Genus stand
// seit G-E3 fertig da (messwert_registrierung.hpp: load/observe/flush), und der Iterator konsumierte
// ihn vollstaendig -- laden vor dem Lauf, beobachten je gemessener Zelle, flushen am Ende. Nur die
// drei Konfigurations-Felder mess_bestand_doc_key / mess_bestand_key_of / mess_bestand_versions
// hatten NULL externe Zuweiser. mess_bestandslog_active wurde damit NIE true: das Lager fuehrte
// Binaries, aber keine Messungen. Es fehlte nicht der Mechanismus, es fehlte der Schluessel.
//
// DIE SCHWESTER-DATEI IST DAS MUSTER: fingerprint_key_source.hpp liefert den bestand_key_of des
// BINARY-Genus, indem sie das `.fingerprint`-Sidecar einer gebauten Binary liest. Diese Datei tut
// dasselbe fuer das MESSWERT-Genus und benutzt dieselbe Lese-Wahrheit
// (experiment::read_fingerprint_sidecar, inklusive Trim und 128-hex-Formwache aus AUF-A3/A2). Zwei
// Leser mit je eigenem Trim waeren genau die Drift, die jene Datei seit der A2-Eichung verhindert.
//
// WORAUS DER SCHLUESSEL BESTEHT -- UND WARUM NICHT AUS DEN MESSWERTEN.
// Komponenten (feste Reihenfolge): [0] der 128-hex-Fingerprint der vermessenen Binary,
// [1] die Hardware-Identitaet der messenden Maschine. Die ZELLE [d,e,f] kommt NICHT hier hinein --
// der Iterator klammert sie DANEBEN (lager_key_from_hex(key, zelle), Section 62-NACHTRAG-4), sonst
// wuerden avx2 und avx512 derselben Permutation wechselseitig wegdedupliziert.
//
// Der Fingerprint IST die voll-permutative Identitaet (er entsteht ueber alle Stempel-Zeilen), damit
// deckt sich [0] mit der Ledger-Formel "Mess-Bestand keyt voll-permutativ + Hardware-Identitaet".
//
// DIE MESSWERTE SELBST GEHEN BEWUSST NICHT IN DEN SCHLUESSEL. Das ist die Stelle, an der ein
// naheliegender Entwurf den Owner-KERN vom 08.08.2026 gebrochen haette: "Die operation bei Validen
// Messdaten ist skip fuer die XLSX, sofern von der exakt gleichen binary gemessen wird." Ein Digest
// ueber die result.csv wuerde bei jeder Wiederholung ANDERS ausfallen (Messwerte streuen) -- es gaebe
// nie einen lager_hit, also nie einen SKIP, und das Lager wuechse bei jedem Lauf um eine Zeile, die
// dieselbe Zelle beschreibt. Der SKIP haengt an der IDENTITAET des Gemessenen, nicht an seinem Wert.
//
// KEINE FUSION (Section 66-N3): der Binary-Fingerprint wird als KOMPONENTE verhasht, der
// Messwert-Schluessel ist deshalb ein anderer Wert als der Fingerprint. Die vermessene Binary wird
// zusaetzlich im Feld `stempel` des Eintrags REFERENZIERT -- lesbar, ohne in den Schluessel gemischt
// zu sein.
//
// FEHLERKLASSEN -- alle fuehren zu nullopt und damit zu MesswertOutcome::no_key (fail-closed, kein
// Eintrag ohne Anker). Sie sind bewusst STILL, aus demselben Grund wie bei fingerprint_key_source:
// der Provider laeuft je gemessener Zelle, eine Zeile je Zelle waere bei 2^17 Zellen ein
// Log-Bombardement. Sichtbar werden sie am no_key-Verhalten des MesswertRunState.
//   sidecar_fehlt / sidecar_leer / laenge_verstoss / zeichen_verstoss -- geerbt von
//     read_fingerprint_sidecar (die vier Klassen sind dort benannt)
//   hardware_leer -- die Hardware-Identitaet ist leer. Ein Messwert ohne Maschinen-Zuordnung waere
//     auf zwei Maschinen derselbe Schluessel: prod2 wuerde eine Messung von prod1 als eigene
//     ausweisen und sie ueberspringen. Genau der Fall, den die Kampagne nicht ueberleben darf.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (Section erlaubt), nur stdlib + die zwei
// Bestandslog-/Sidecar-Header. Kein Runtime-Switch, kein variant, keine Env-Abfrage -- das Gate
// sitzt beim Host (AUF-B3), exakt wie beim Binary-Genus.
// -----------------------------------------------------------------------------

#include "../build_orchestrator/fingerprint_sidecar.hpp" // read_fingerprint_sidecar: EINE Lese-Wahrheit
#include "messwert_registrierung.hpp"                    // messwert_key_hex (die EINE Ableitung)

#include <array>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::builder::bestandslog {

// Der Dateiname der Tier-Binary innerhalb eines bin_dir. Er steht im Haus bereits als Literal an
// sechs Stellen (build_orchestrator.hpp bei job.output, fuenfmal in artifact_transport/
// artifact_cache.hpp -- deren Kopf sagt ausdruecklich, dass der Transport diese Namen als Literale
// fuehrt). Hier ist er ein DEFAULT-PARAMETER statt eines siebten festen Literals: der Aufrufer kann
// ihn setzen, ohne dass eine zweite Wahrheit entsteht. Eine echte Single-Source ueber alle sieben
// Stellen waere eine eigene, breitere Aufraeum-Scheibe -- sie gehoert nicht in dieses Paket.
inline constexpr char const* kTierBinaryDateiname = "perm.dll";

/// Der Provider. `bin_dir` ist das Verzeichnis der GEMESSENEN ZELLE (im Iterator:
/// b.output.parent_path()); daneben liegt die Binary und deren `.fingerprint`-Sidecar.
///
/// hardware_identitaet ist die Kennung der MESSENDEN Maschine (der Host reicht dieselbe Quelle
/// herein, aus der er COMDARE_BESTANDSLOG_MASCHINE bedient). Leer => der Provider liefert
/// grundsaetzlich nullopt: lieber gar kein Messwert-Lager als eines, in dem prod1 und prod2
/// ununterscheidbar sind.
[[nodiscard]] inline std::function<std::optional<std::string>(std::filesystem::path const&)>
make_messwert_key_fn(std::string hardware_identitaet, std::string binary_dateiname = kTierBinaryDateiname) {
    return [hw   = std::move(hardware_identitaet),
            name = std::move(binary_dateiname)](std::filesystem::path const& bin_dir) -> std::optional<std::string> {
        if (hw.empty()) return std::nullopt; // fehlerklasse=hardware_leer (s. Kopf)
        auto const fingerprint = experiment::read_fingerprint_sidecar(bin_dir / name);
        if (!fingerprint) return std::nullopt; // die vier geerbten Sidecar-Fehlerklassen
        std::array<std::string_view, 2> const komponenten{std::string_view{*fingerprint}, std::string_view{hw}};
        return messwert_key_hex(std::span<std::string_view const>{komponenten});
    };
}

} // namespace comdare::cache_engine::builder::bestandslog
