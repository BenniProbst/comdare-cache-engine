#pragma once
// abi/anatomy_fingerprint.hpp -- K7b-3 (Section 62-B / Section 64): der SHA-512-Fingerprint der einkompilierten
// Anatomy-Stempel-Zeilen (organ/system/measurement), consteval berechnet aus der K7b-1-Primitive.
//
// D3 (Manager-Entscheid 2026-07-22): Preimage-Ordnung fix. A13-M3 (Owner-E2 "Merge Zeile kann daher nicht
// existieren" + OF-M3-1 Option A + F7-Konsolidierung 01.08.) hat sie NEU gefasst -- die Glied-Liste steht als
// EINE Quelle in anatomy_fingerprint_glieder() unten, alle Rechen-Stellen ziehen daraus.
// D2: der Fingerprint reist als 128-hex nullterminierte Zeile ({char const*, uint64}) im AnatomyVersionLines-POD.
// GOLDEN-NEUTRAL: die Berechnung passiert INNEN im COMDARE_ANATOMY_VERSION_STAMP*-Makro -> der emittierte Quelltext
// (der Makro-Call, 2/3-arg) bleibt byte-identisch; der Fingerprint materialisiert erst in der Makro-Expansion,
// nicht im emittierten .cpp -> golden-CRC 0xF1C1F26A1232073B unberuehrt. Saat fuer den #46b-std::map-Lookup (ein
// kompakter, stabiler Provenienz-Schluessel je Tier-Binary).

#include "subaxis_valueset_segment.hpp" // A13-M3: das Sub-Achsen-Werteset-Segment als Preimage-Glied

#include <sha512/ctsha512.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace comdare::cache_engine::abi {

/// Obergrenze des Preimage-Puffers: die laengste Stempel-Zeile ist die 18-Achsen-Organ-Zeile; die Summe der
/// Glieder bleibt weit darunter. Wird die Grenze je ueberschritten, bricht die consteval-Auswertung sichtbar
/// (der Puffer-Zugriff ist dann kein konstanter Ausdruck mehr).
///
/// BUDGET-BELEG (A8.3, O-8 Schritt 8; an den Ist-Daten gemessen, nicht geschaetzt): die laengste reale
/// binary_id in golden_fullpilot_320_binary_ids.txt ist 546 Zeichen (18 Segmente "achse=wert"). Die
/// Organ-Zeile ist dieselbe Segment-Folge mit ';' statt '/' plus je Segment "@X.Y.Z" -- bei maximal
/// dreistelligen Versions-Gliedern also hoechstens 546 + 18*12 = 762 Zeichen. System-Zeile (3 Achsen +
/// Meta-Meta-Anhang) ~150, Mess-Zeile unter 200. Dazu die A13-M3-Glieder: die Format-Kennung (<32), das
/// Werteset-Segment (<64), das 5. Glied als SHA-512-Hex-String (128) und fuenf Separator-Bytes. Summe
/// damit unter ~1400 von 4096 -- mehr als der doppelte Kopfraum.
/// Das zweite, SEPARATE Budget ist das 50-KB-Funktionsrumpf-Budget der Primitive
/// (sha512::fits_compile_time_budget, ctsha512.hpp:164-166, kMaxFunctionBodyBytes = 50 * 1024);
/// 4096 liegt eine Groessenordnung darunter. Beide Budgets sind eingehalten.
inline constexpr std::size_t kAnatomyFingerprintPreimageMax = 4096;

/// A13-M3 / OF-M3-1 = Option A (Owner-Entscheid 03.08.2026, Befund GA-01 [BLOCK]): der DOMAIN-SEPARATOR.
///
/// Bis A13-M2 entstand das Preimage als reine Byte-Konkatenation variabel langer Felder OHNE Trenner. Damit
/// war es NICHT injektiv: jede Verschiebung einer Feldgrenze ergibt dasselbe Preimage -- literal
/// demonstriert an fp("","",X) == fp(X,"","") == fp("",X,"") und an der Ein-Zeichen-Grenzverschiebung
/// zwischen Organ- und System-Zeile. Genau darauf ruht aber das SHA512-ONLY-Skip-Gate (F7): "der deckt die
/// anderen Stempel allein". Ein nicht-injektives Preimage macht diese Zusage unbeweisbar.
///
/// '\n' ist BEWEISBAR kollisionsfrei, nicht bloss unwahrscheinlich: der Stempel-Zeichenvorrat ist auf
/// alnum + "=@;.+_[]" festgelegt (C-literal-Sicherheit der Makro-Materialisierung, anatomy_module_abi_v1.hpp)
/// -- ein Zeilenumbruch kann in keinem Glied vorkommen. Bei FESTER Glied-ANZAHL ist die Zerlegung des
/// Preimage damit eindeutig, also die Abbildung Glieder -> Preimage injektiv.
inline constexpr char kAnatomyFingerprintSeparator = '\n';

/// A13-M3 / F7 ("FORMAT-VERSION: das Preimage-Layout traegt eine eigene fingerprint_format-Kennung als erstes
/// Glied -- Layout-Evolution mismatcht dann deterministisch statt still zu kollidieren"): die Kennung steht
/// als ERSTES Glied im Preimage. Ohne sie waere jede kuenftige Layout-Aenderung (Glied dazu/weg/umsortiert)
/// eine STILLE Kollisionsflaeche gegen Alt-Fingerprints; mit ihr verschiebt sich der Digest deterministisch.
///
/// Format 1 (historisch, bis A13-M2): concat(organ + system + measurement + merge + overlay), OHNE Trenner.
/// Format 2 (A13-M3): '\n'-getrennte Glied-Folge, merge ENTFAELLT (Owner-E2), Werteset-Segment kommt dazu.
inline constexpr std::string_view kAnatomyFingerprintFormat = "fingerprint_format=2";

/// Das letzte Preimage-Glied: der HASH ALLER SOURCE-CODE-DATEIEN IM OVERLAY (Owner-Abnahme 26.07.,
/// Ledger 88: "consteval-SHA512-Zeile ueber Achsen-Strings + Versionen + Overlay-Source-Hashes").
///
/// WIE ER HIERHER KOMMT: ausdruecklich per PRE-BUILD-CODEGEN, nie zur Laufzeit -- der Codegen hasht die
/// Overlay-Quellen und reicht das Ergebnis als Compile-Define herein (Muster COMDARE_GN_ALGO_SIG). Ein
/// consteval-Hash kann keine Dateien lesen; jede Laufzeit-Variante waere ein Bruch der Doktrin
/// "compile-time only" und wuerde die Binary nicht mehr eindeutig machen.
///
/// HEUTE LEER, UND ZWAR EHRLICH: der Codegen existiert noch nicht (0 Treffer fuer eine
/// Overlay-Hash-Quelle im Baum). Ein leeres Glied traegt (ausser seinem Separator) nichts zum Preimage bei;
/// die Naht ist trotzdem gebaut und an EINER Stelle. Sobald der Codegen das Define setzt, wandert der Hash
/// ohne jede weitere Aenderung in alle Fingerprints.
/// OFFEN und bewusst NICHT geraten: WELCHE Dateimenge "das Overlay" ist (Verzeichnis-Schnitt,
/// Sortier-Ordnung, Hash je Datei vs. ueber die Konkatenation). Das ist eine Identitaets-Entscheidung
/// je Tier-Binary und gehoert dem Owner, nicht diesem Header. A13-M3/OF-M3-2 = FALLBACK B (Entscheid
/// 03.08.2026): die drei Owner-Festlegungen lagen zum M3-Start NICHT vor -> das Glied bleibt "", KEIN
/// Overlay-Codegen im Fenster; die Scharfschaltung ist ein deklariert spaeteres Fingerprint-Ereignis.
#ifndef COMDARE_OVERLAY_SOURCE_HASH
#define COMDARE_OVERLAY_SOURCE_HASH ""
#endif
inline constexpr std::string_view kOverlaySourceHash = COMDARE_OVERLAY_SOURCE_HASH;

/// K-1 (Bauplan A13, "ceb_version_stamp.hpp-Falle"): der Overlay-Hash reist als BENANNTER TYP, nicht als
/// vierter string_view.
///
/// DIE FALLE, DIE ER SCHLIESST: bis A13-M2 lautete die Signatur
/// anatomy_fingerprint_hex(organ, system, measurement, merge, overlay = kOverlaySourceHash). Faellt merge
/// ersatzlos weg (Owner-E2) und blieben alle Parameter string_view, waere der Bestands-Aufruf
/// anatomy_fingerprint_hex("", "", kCebMeasurementStamp, "") WEITER GUELTIG -- das vierte "" rutschte still
/// von merge auf overlay. Es kompiliert, die Semantik ist verschoben, niemand merkt es. Ein string_view
/// konvertiert NICHT implizit nach OverlayHash (explicit) -> jeder Alt-Aufruf bricht compile-hart.
struct OverlayHash {
    std::string_view value;

    constexpr explicit OverlayHash(std::string_view v) noexcept : value{v} {}
};

/// Anzahl der Preimage-Glieder. FEST -- die Injektivitaet der '\n'-Zerlegung haengt an der festen Anzahl.
inline constexpr std::size_t kAnatomyFingerprintGliedCount = 6;

/// W10-C3: die POSITION der System-Zeile in der Glied-Folge, benannt statt als nackte 2.
///
/// WOZU: der Laufzeit-Zwilling des Lager-Keys (BinaryKeyPolicy, bestandslog_factory.hpp) bekommt die
/// fertigen Glieder als span und muss GENAU EIN Glied -- die System-Zeile -- um die Zellwerte
/// vervollstaendigen. Ohne benannte Konstante stuende dort eine nackte 2, die bei jeder kuenftigen
/// Umsortierung der Glied-Folge STILL auf das falsche Glied zeigte. Die Konstante wohnt hier, weil hier
/// auch die Ordnung wohnt (dieselbe Begruendung wie fuer anatomy_fingerprint_glieder selbst).
inline constexpr std::size_t kAnatomyFingerprintSystemGlied = 2;

/// anatomy_fingerprint_glieder(...) -- DIE EINE QUELLE der Preimage-Ordnung. Jede Rechen-Stelle (der
/// consteval-Hex unten, der Laufzeit-Zwilling lazy_adhoc_fingerprint_for, der Lager-Key-Ableiter
/// derive_key_from_lines ueber seinen Aufrufer) zieht ihre Glieder HIER heraus. Wer die Ordnung aendert,
/// aendert sie an genau dieser Stelle -- vorher drifteten vier Kopien auseinander (Risiko R6).
///
/// Ordnung (A13-M3, Format 2):
///   [0] fingerprint_format-Kennung  (F7: Layout-Evolution mismatcht deterministisch)
///   [1] Organ-Zeile                 (18 Haupt-Achsen, achse=algo@X.Y.Z)
///   [2] System-Zeile                (3 Haupt-Achsen + Meta-Meta-Klammer-Anhang)
///   [3] Mess-Tooling-Zeile          (Haupt-Wahl + load_framework-Anhang)
///   [4] Sub-Achsen-Werteset-Segment (F7-VERIFY: sonst wuerde ein Werteset-Bump unter dem SHA512-only-Gate
///                                    STILL reused -- die Organ-Zeile traegt den Sub-Schwanz ausdruecklich nicht)
///   [5] Overlay-Source-Hash         (heute leer, OF-M3-2 = Fallback B)
[[nodiscard]] constexpr std::array<std::string_view, kAnatomyFingerprintGliedCount>
anatomy_fingerprint_glieder(std::string_view organ, std::string_view system, std::string_view measurement,
                            OverlayHash overlay = OverlayHash{kOverlaySourceHash}) noexcept {
    return {kAnatomyFingerprintFormat, organ, system, measurement, kSubAxisValuesetSegment, overlay.value};
}

/// W10-C3: die Positions-Konstante ist BEWIESEN, nicht behauptet -- wer die Glied-Ordnung oben umbaut,
/// bricht hier compile-time, statt den Zellwert still ins Organ-Glied zu schreiben.
static_assert(anatomy_fingerprint_glieder("ORGAN", "SYSTEM", "MESS")[kAnatomyFingerprintSystemGlied] == "SYSTEM",
              "kAnatomyFingerprintSystemGlied zeigt nicht mehr auf die System-Zeile der Glied-Folge.");
static_assert(kAnatomyFingerprintSystemGlied < kAnatomyFingerprintGliedCount);

/// anatomy_fingerprint_preimage(glieder) -- der LAUFZEIT-Zwilling der Preimage-Bildung: dieselbe Glied-Liste,
/// derselbe Separator. Er steht bewusst UNMITTELBAR neben der consteval-Schleife unten: der consteval-Weg
/// braucht einen uint8-Puffer (ein reinterpret_cast auf std::string::data() waere in einem konstanten
/// Ausdruck verboten), der Laufzeit-Weg einen std::string -- zwei Schleifen, EINE Ordnung, EIN Separator,
/// im selben Sichtfeld.
[[nodiscard]] inline std::string anatomy_fingerprint_preimage(std::span<std::string_view const> glieder) {
    std::string pre;
    for (std::size_t i = 0; i < glieder.size(); ++i) {
        if (i != 0) pre += kAnatomyFingerprintSeparator;
        pre.append(glieder[i]);
    }
    return pre;
}

/// anatomy_fingerprint_hex(organ, system, measurement[, OverlayHash]) -- 128-hex SHA-512 (nullterminiert,
/// array<char, 129>) ueber die '\n'-getrennte Glied-Folge aus anatomy_fingerprint_glieder(). consteval:
/// reine Compile-Zeit-Ableitung der einkompilierten Stempel-Literale (leere Zeilen -> leeres Glied, aber
/// der Separator bleibt -> die Feldgrenze ist erhalten; genau das ist der GA-01-Fix).
[[nodiscard]] consteval std::array<char, 129>
anatomy_fingerprint_hex(std::string_view organ, std::string_view system, std::string_view measurement,
                        OverlayHash overlay = OverlayHash{kOverlaySourceHash}) noexcept {
    auto const glieder = anatomy_fingerprint_glieder(organ, system, measurement, overlay);
    std::array<std::uint8_t, kAnatomyFingerprintPreimageMax> preimage{};
    std::size_t                                              n = 0;
    for (std::size_t i = 0; i < glieder.size(); ++i) {
        if (i != 0) preimage[n++] = static_cast<std::uint8_t>(kAnatomyFingerprintSeparator);
        for (char c : glieder[i]) preimage[n++] = static_cast<std::uint8_t>(c);
    }
    auto const digest = ::comdare::cache_engine::sha512::sha512(std::span<const std::uint8_t>{preimage.data(), n});
    auto const hex    = ::comdare::cache_engine::sha512::to_hex(digest); // array<char, 128>
    std::array<char, 129> out{};
    for (std::size_t i = 0; i < 128; ++i) out[i] = hex[i];
    out[128] = '\0';
    return out;
}

namespace detail {
/// Abhaengiger false-Wert: erst bei INSTANZIIERUNG der Sperr-Ueberladung unten ausgewertet.
template <class...>
inline constexpr bool kFingerprintMergeAritaetEntfallen = false;
} // namespace detail

/// K-1-SPERRE: die alte 4-/5-string_view-Form existiert weiter als Ueberladung, ist aber unbaubar und meldet
/// sich mit BENANNTEM Text. Sie ist bewusst KEIN `= delete` -- die Form `= delete("Grund")` ist C++26 und
/// waere unter -std=c++23 eine Erweiterung; ein Template mit static_assert liefert denselben Effekt
/// standardkonform und mit derselben Lesbarkeit im Fehlertext.
///
/// Sie greift GENAU dann, wenn jemand das 4. Argument als string_view uebergibt (der alte merge-Slot). Der
/// gueltige typisierte 4-arg-Aufruf (..., OverlayHash{...}) trifft sie nicht: OverlayHash konvertiert nicht
/// nach string_view, also ist diese Ueberladung dort nicht einmal viabel.
template <class... Rest>
[[nodiscard]] consteval std::array<char, 129>
anatomy_fingerprint_hex(std::string_view, std::string_view, std::string_view, std::string_view, Rest...) noexcept {
    static_assert(detail::kFingerprintMergeAritaetEntfallen<Rest...>,
                  "A13-M3/K-1: die merge-ZEILE existiert nicht mehr (Owner-E2 02.08.2026) -- das 4. Argument von "
                  "anatomy_fingerprint_hex ist der OverlayHash-TYP, kein string_view. Ein Alt-Aufruf mit vier "
                  "string_view wuerde den merge-Slot still auf den Overlay-Slot schieben; genau das verhindert "
                  "diese Sperre. Aufruf auf die 3-arg-Form ziehen (bzw. OverlayHash{...} explizit angeben).");
    return {};
}

} // namespace comdare::cache_engine::abi
