#pragma once
// abi/anatomy_fingerprint.hpp -- K7b-3 (Section 62-B / Section 64): der SHA-512-Fingerprint der vier einkompilierten
// Anatomy-Stempel-Zeilen (organ/system/measurement/merge), consteval berechnet aus der K7b-1-Primitive.
//
// D3 (User-GO 2026-07-22): Preimage == concat(organ + system + measurement + merge) in DIESER fixen Reihenfolge.
// D2: der Fingerprint reist als 128-hex nullterminierte Zeile ({char const*, uint64}) im AnatomyVersionLines-POD.
// GOLDEN-NEUTRAL: die Berechnung passiert INNEN im COMDARE_ANATOMY_VERSION_STAMP*-Makro -> der emittierte Quelltext
// (der Makro-Call, 2/3/4-arg) bleibt byte-identisch; der Fingerprint materialisiert erst in der Makro-Expansion,
// nicht im emittierten .cpp -> golden-CRC 0xF1C1F26A1232073B unberuehrt. Saat fuer den #46b-std::map-Lookup (ein
// kompakter, stabiler Provenienz-Schluessel je Tier-Binary).

#include <sha512/ctsha512.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace comdare::cache_engine::abi {

/// Obergrenze des Preimage-Puffers: die laengste Stempel-Zeile ist die 18-Achsen-Organ-Zeile; die Summe der
/// Glieder bleibt weit darunter. Wird die Grenze je ueberschritten, bricht die consteval-Auswertung sichtbar
/// (der Puffer-Zugriff ist dann kein konstanter Ausdruck mehr).
///
/// BUDGET-BELEG (A8.3, O-8 Schritt 8; an den Ist-Daten gemessen, nicht geschaetzt): die laengste reale
/// binary_id in golden_fullpilot_320_binary_ids.txt ist 546 Zeichen (18 Segmente "achse=wert"). Die
/// Organ-Zeile ist dieselbe Segment-Folge mit ';' statt '/' plus je Segment "@X.Y.Z" -- bei maximal
/// dreistelligen Versions-Gliedern also hoechstens 546 + 18*12 = 762 Zeichen. System-Zeile (3 Achsen)
/// ~110, Mess- und Merge-Zeile zusammen deutlich unter 400. Das 5. Glied ist ein SHA-512-Hex-String
/// (128 Zeichen). Summe damit unter ~1400 von 4096 -- mehr als der doppelte Kopfraum.
/// Das zweite, SEPARATE Budget ist das 50-KB-Funktionsrumpf-Budget der Primitive
/// (sha512::fits_compile_time_budget, ctsha512.hpp:164-166, kMaxFunctionBodyBytes = 50 * 1024);
/// 4096 liegt eine Groessenordnung darunter. Beide Budgets sind eingehalten.
inline constexpr std::size_t kAnatomyFingerprintPreimageMax = 4096;

/// Das 5. Preimage-Glied: der HASH ALLER SOURCE-CODE-DATEIEN IM OVERLAY (Owner-Abnahme 26.07.,
/// Ledger 88: "consteval-SHA512-Zeile ueber Achsen-Strings + Versionen + Overlay-Source-Hashes").
///
/// WIE ER HIERHER KOMMT: ausdruecklich per PRE-BUILD-CODEGEN, nie zur Laufzeit -- der Codegen hasht die
/// Overlay-Quellen und reicht das Ergebnis als Compile-Define herein (Muster COMDARE_GN_ALGO_SIG). Ein
/// consteval-Hash kann keine Dateien lesen; jede Laufzeit-Variante waere ein Bruch der Doktrin
/// "compile-time only" und wuerde die Binary nicht mehr eindeutig machen.
///
/// HEUTE LEER, UND ZWAR EHRLICH: der Codegen existiert noch nicht (0 Treffer fuer eine
/// Overlay-Hash-Quelle im Baum). Ein leeres Glied traegt nichts zum Preimage bei -- der Fingerprint
/// bleibt damit exakt der bisherige, und die Naht ist trotzdem gebaut und an EINER Stelle. Sobald der
/// Codegen das Define setzt, wandert der Hash ohne jede weitere Aenderung in alle Fingerprints.
/// OFFEN und bewusst NICHT geraten: WELCHE Dateimenge "das Overlay" ist (Verzeichnis-Schnitt,
/// Sortier-Ordnung, Hash je Datei vs. ueber die Konkatenation). Das ist eine Identitaets-Entscheidung
/// je Tier-Binary und gehoert dem Owner, nicht diesem Header.
#ifndef COMDARE_OVERLAY_SOURCE_HASH
#define COMDARE_OVERLAY_SOURCE_HASH ""
#endif
inline constexpr std::string_view kOverlaySourceHash = COMDARE_OVERLAY_SOURCE_HASH;

/// anatomy_fingerprint_hex(organ, system, measurement, merge) -- 128-hex SHA-512 (nullterminiert, array<char, 129>)
/// von concat(organ + system + measurement + merge). consteval: reine Compile-Zeit-Ableitung der vier Stempel-Literale
/// (leere Zeilen -> leerer Anteil; der ce-only-/Katalog-Pfad {measurement="", merge=""} ergibt den Fingerprint von
/// concat(organ + system), deterministisch und ohne den emittierten Quelltext zu beruehren).
/// A8.3 (O-8 Schritt 8): das 5. Glied `overlay_source_hash` steht am ENDE der fixen Reihenfolge --
/// als Anhang, nicht als Einschub. Ein Einschub wuerde jeden bestehenden Fingerprint aendern, ohne dass
/// sich am Inhalt der vier Zeilen etwas geaendert haette; der Anhang laesst sie unberuehrt, solange das
/// Glied leer ist. Die Reihenfolge darf nur GEMEINSAM an allen Orten der Kette geaendert werden
/// (dieser Header, der Laufzeit-Zwilling lazy_adhoc_fingerprint_for, der Bestandslog-Index).
[[nodiscard]] consteval std::array<char, 129> anatomy_fingerprint_hex(std::string_view organ, std::string_view system,
                                                                      std::string_view measurement,
                                                                      std::string_view merge,
                                                                      std::string_view overlay_source_hash
                                                                      = kOverlaySourceHash) noexcept {
    std::array<std::uint8_t, kAnatomyFingerprintPreimageMax> preimage{};
    std::size_t                                              n      = 0;
    auto                                                     append = [&](std::string_view s) {
        for (char c : s) preimage[n++] = static_cast<std::uint8_t>(c);
    };
    append(organ);
    append(system);
    append(measurement);
    append(merge);
    append(overlay_source_hash);
    auto const digest = ::comdare::cache_engine::sha512::sha512(std::span<const std::uint8_t>{preimage.data(), n});
    auto const hex    = ::comdare::cache_engine::sha512::to_hex(digest); // array<char, 128>
    std::array<char, 129> out{};
    for (std::size_t i = 0; i < 128; ++i) out[i] = hex[i];
    out[128] = '\0';
    return out;
}

} // namespace comdare::cache_engine::abi
