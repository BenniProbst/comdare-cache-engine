#pragma once
// abi/anatomy_fingerprint.hpp -- K7b-3 (Section 62-B / Section 64): der SHA-512-Fingerprint der einkompilierten
// Anatomy-Stempel-Zeilen (organ/system/measurement), consteval berechnet aus der K7b-1-Primitive.
//
// D3 (Manager-Entscheid 2026-07-22): Preimage-Ordnung fix. A13-M3 (Owner-E2 "Merge Zeile kann daher nicht
// existieren" + OF-M3-1 Option A + F7-Konsolidierung 01.08.) hat sie NEU gefasst -- die Glied-Liste steht als
// EINE Quelle in anatomy_fingerprint_glieder() unten, alle Rechen-Stellen ziehen daraus.
// O-2/C-2 (Owner-Entscheid abend-5, 05.08.2026 = OPTION A "Achsen-Vollstaendigkeits-Neuanker"): die Liste ist auf
// ACHT Glieder gewachsen -- die CEB-Laufzeit-Hauptachsen (Compiler inkl. Flags, opt_level, atomic128, bt/gate/ceb)
// und die Enabled-Mengen-Signatur der Build-Achsen sind ab fingerprint_format=3 IDENTITAETS-wirksam, nicht mehr
// bloss Provenienz im build_version-Suffix. Sie werden INJIZIERT (K-1-Traeger + Compile-Define); die Ordnung
// bleibt an dieser einen Stelle.
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
#include <stdexcept> // NB/CX-1: die RT-Injektivitaets-Wache der injizierten Glieder ist FAIL-LOUD
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
/// Werteset-Segment (<64), das Overlay-Glied als SHA-512-Hex-String (128) und die Separator-Bytes.
/// Das zweite, SEPARATE Budget ist das 50-KB-Funktionsrumpf-Budget der Primitive
/// (sha512::fits_compile_time_budget, ctsha512.hpp:164-166, kMaxFunctionBodyBytes = 50 * 1024);
/// 4096 liegt eine Groessenordnung darunter. Beide Budgets sind eingehalten.
///
/// O-2/C-2 (Format 3, 05.08.2026) -- DER BUDGET-NACHWEIS IST AB HIER MASCHINELL, nicht mehr nur ein
/// Absatz. Format 3 haengt ZWEI Glieder an (Toolchain, bvset), und das bvset-Glied ist das erste, das
/// mit der Flotten-Groesse waechst: es klammert je Build-Achse ALLE enabled Varianten mit Namen und
/// Feldern (build_variant_set_signature.hpp). Eine Prosa-Abschaetzung waere hier genau die Sorte
/// Behauptung, die spaeter still bricht -- deshalb stehen die Teil-Budgets unten als Konstanten und
/// ihre Summe als static_assert. Wer ein Glied verlaengert, bricht compile-time, statt den
/// consteval-Puffer irgendwann unerklaerlich zu ueberlaufen. Die LEBENDE Messung der laengsten realen
/// bvset-Signatur steht dort, wo sie entsteht: driver_build_variant_signature.hpp prueft ihre
/// tatsaechliche Laenge gegen kAnatomyFingerprintBvsetMax.
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
/// Format 3 (O-2/C-2, 05.08.2026, Owner-Entscheid abend-5 = OPTION A "Achsen-Vollstaendigkeits-Neuanker"):
///   das TOOLCHAIN-Glied [5] (Compiler-Haupt-Achse inkl. ihrer Flags, opt_level, atomic128, bt/gate/ceb --
///   die CEB-Laufzeit-Hauptachsen als CT-Glieder der Tier-Binary) und das BVSET-Glied [6] (Enabled-Mengen-
///   Signatur der Build-Achsen) kommen dazu; das Overlay-Glied wandert ans ENDE ([5] -> [7]).
///   DER BUMP IST DER PUNKT: er macht die Layout-Evolution deterministisch. Ohne ihn haetten Format-2- und
///   Format-3-Binaries mit leeren neuen Gliedern DENSELBEN Digest -- ein Bestand aus der Zeit VOR der
///   Toolchain-Verankerung wuerde weiter uebersprungen, obwohl seine Toolchain-Identitaet nie geprueft
///   wurde. Mit dem Bump mismatcht die gesamte Flotte EINMAL und baut fail-closed neu (F7-Uebergangsregel,
///   KEIN Grandfathering). Das ist eine bewusste, einmalige Invalidierungswelle.
inline constexpr std::string_view kAnatomyFingerprintFormat = "fingerprint_format=3";

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

// -- NB/CX-1: DIE RT-INJEKTIVITAETS-WACHE DER INJIZIERTEN GLIED-WERTE ----------------------------------
//
// DER BEFUND, DEN SIE HEILT (Codex-Nachreview [BLOCKER], am Code bestaetigt): die '\n'-Freiheit war NUR
// fuer die per #define injizierten Compile-Zeit-Konstanten bewiesen (die static_asserts unten). Die
// LAUFZEIT-Naht nahm beliebige std::string_view entgegen und validierte NICHTS. Die Kollision ist konkret
// und braucht keinen SHA-Bruch:
//     A = {Toolchain="TC\nX", bvset="BV"}   und   B = {Toolchain="TC", bvset="X\nBV"}
// erzeugen BYTE-IDENTISCHE Preimages, also denselben Fingerprint fuer zwei verschiedene Baue. Die feste
// Glied-ANZAHL rettet die Zerlegung nur, solange KEIN Glied den Separator traegt -- diese Voraussetzung war
// fuer die Laufzeit-Werte unbewiesen. Sie ist ab hier eine Pflicht mit Wache, kein Zufall mehr.
//
// DREI REGELN, alle drei aus der Injektivitaet abgeleitet (nicht aus Geschmack):
//   (1) KEIN '\n' (und kein '\r')  -- der Domain-Separator darf in keinem Glied vorkommen (OF-M3-1).
//   (2) ZEICHENVORRAT              -- nur Zeichen, die real vorkommen (alnum + die Trenn-/Klammer-Zeichen
//                                     der Stempel-Grammatik). Alles andere ist ein Hinweis darauf, dass ein
//                                     fremder String in den Slot geraten ist (Pfad, Fehlertext, Rohdaten).
//   (3) KEIN LEERER SCHLUESSEL     -- ein Segment "=wert" bzw. ";=wert" waere kein Paar mehr; die Grenze
//                                     zwischen Schluessel und Wert liesse sich verschieben.
// Die Regeln gelten fuer die INJIZIERTEN Glieder (Toolchain, bvset, Overlay). Die einkompilierten
// Stempel-ZEILEN [1]-[3] werden bewusst NUR auf (1) geprueft: ihr Zeichenvorrat gehoert den Achsen-Namen
// und darf nicht von diesem Header eingeengt werden.

/// Der Zeichenvorrat der INJIZIERTEN Glied-Werte. Er ist die Vereinigung dessen, was die realen Glieder
/// wirklich tragen: Achsen-Ids und Versionen (alnum, '_', '.', '@'), die Segment-Trenner (';', '='), die
/// Flag-/Mengen-Klammern ('{', '}', '[', ']'), Compiler-Flags ('-', '+') sowie ',' (Werteset) und ':'/'/'
/// (Pfad-freie Ids mit Doppelpunkt-Namensraum). Bewusst NICHT enthalten: Whitespace jeder Art.
[[nodiscard]] constexpr bool anatomy_glied_zeichen_erlaubt(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '=' || c == '@' ||
           c == ';' || c == '.' || c == ',' || c == '+' || c == '-' || c == '_' || c == ':' || c == '/' ||
           c == '[' || c == ']' || c == '{' || c == '}';
}

/// Die Format-Wache fuer einen injizierten Glied-Wert. LEER ist immer wohlgeformt (== die Identitaet).
[[nodiscard]] constexpr bool injizierter_glied_wert_ist_wohlgeformt(std::string_view v) noexcept {
    if (v.empty()) return true;
    if (v.front() == '=') return false; // (3) leerer Schluessel am Anfang
    for (std::size_t i = 0; i < v.size(); ++i) {
        char const c = v[i];
        if (c == kAnatomyFingerprintSeparator || c == '\r') return false; // (1)
        if (!anatomy_glied_zeichen_erlaubt(c)) return false;              // (2)
        if (c == '=') {
            char const vor = v[i - 1]; // i > 0 ist durch die front()-Pruefung oben sichergestellt
            if (vor == ';' || vor == '[' || vor == '{') return false; // (3) leerer Schluessel im Segment
        }
    }
    return true;
}

/// FAIL-LOUD: ein nicht wohlgeformter Wert bricht -- compile-hart, wenn er eine Compile-Zeit-Konstante ist
/// (der Wurf macht den Ausdruck zu keinem konstanten Ausdruck mehr), sonst zur Laufzeit mit benannter
/// Fehlerklasse. Ein stiller Ersatzwert waere hier das Schlimmste: er wuerde eine FALSCHE Identitaet
/// zementieren, und zwar genau an der Stelle, an der niemand mehr nachsieht.
constexpr void require_injizierter_glied_wert(std::string_view glied, std::string_view wert) {
    if (injizierter_glied_wert_ist_wohlgeformt(wert)) return;
    throw std::invalid_argument(
        std::string{"fehlerklasse=stempel_injektivitaet: das injizierte Preimage-Glied '"} + std::string{glied} +
        "' verletzt die Format-Wache (kein '\\n'/'\\r', kein leerer Schluessel, nur der Stempel-Zeichenvorrat). "
        "Die Injektivitaet der Glied-Zerlegung haengt daran -- zwei verschiedene Baue koennten sonst dasselbe "
        "Preimage und damit denselben Fingerprint bekommen (falscher Skip). Wert: '" +
        std::string{wert} + "'");
}

/// K-1 (Bauplan A13, "ceb_version_stamp.hpp-Falle"): der Overlay-Hash reist als BENANNTER TYP, nicht als
/// vierter string_view.
///
/// DIE FALLE, DIE ER SCHLIESST: bis A13-M2 lautete die Signatur
/// anatomy_fingerprint_hex(organ, system, measurement, merge, overlay = kOverlaySourceHash). Faellt merge
/// ersatzlos weg (Owner-E2) und blieben alle Parameter string_view, waere der Bestands-Aufruf
/// anatomy_fingerprint_hex("", "", kCebMeasurementStamp, "") WEITER GUELTIG -- das vierte "" rutschte still
/// von merge auf overlay. Es kompiliert, die Semantik ist verschoben, niemand merkt es. Ein string_view
/// konvertiert NICHT implizit nach OverlayHash (explicit) -> jeder Alt-Aufruf bricht compile-hart.
///
/// NB/CX-1: der Traeger ist zugleich die WACHE. Die Pruefung im Konstruktor ist kein Zusatz, sondern der
/// einzige Ort, an dem sie NICHT umgangen werden kann -- jeder Weg ins Preimage fuehrt durch ihn. Er ist
/// deshalb nicht mehr `noexcept`: ein ungueltiger Wert MUSS heraus, statt zu terminieren.
///
/// NB2-2 (Codex-Zweitreview [KRITISCH], am Code bestaetigt): "der einzige Ort, an dem sie nicht umgangen
/// werden kann" war zum Vor-Stand schlicht UNWAHR -- das Feld hiess `value`, war public und frei
/// mutierbar. `OverlayHash h{"ok"}; h.value = "a\nb";` fuehrte an der Konstruktor-Wache VORBEI und trug
/// den Domain-Separator ins Preimage; damit war die ganze NB/CX-1-Zusage eine Absichtserklaerung. Der
/// Wert wohnt ab hier PRIVAT und wird nur lesend herausgegeben. Der Traeger ist damit das, was er
/// behauptet zu sein: ein Beweis-Tragender Typ, dessen Invariante ab Konstruktion gilt.
class OverlayHash {
public:
    constexpr explicit OverlayHash(std::string_view v) : wert_{v} { require_injizierter_glied_wert("overlay", v); }

    [[nodiscard]] constexpr std::string_view wert() const noexcept { return wert_; }

private:
    std::string_view wert_;
};

/// O-2/C-2 -- DIE TOOLCHAIN-NAHT (Glied [5]). Owner-KERN abend-5: "ALLE Laufzeit Hauptachsen wie Compiler
/// auf der CEB [sind] zwangsweise compile time Hauptachsen auf der entstehenden Tier-Binary ... und auch im
/// Fingerprint mit ihrer Versionierung verankert."
///
/// WIE DER WERT HIERHER KOMMT: per COMPILE-DEFINE, exakt die sanktionierte Pre-Build-Define-Klasse
/// (Muster COMDARE_SYSTEM_CELL_VALUES / COMDARE_OVERLAY_SOURCE_HASH / COMDARE_GN_ALGO_SIG). Die CEB kennt
/// die Toolchain-Wahl DIESER Permutation zur Laufzeit und friert sie beim Bau der Tier-Binary als
/// CT-Konstante ein -- das ist die dokumentierte "dynamisch-Vorstufe -> statisch-Folgestufe"-Bruecke
/// (Paragraf 24.C ACHSEN-KETTEN-STATIK). Gerendert wird der Wert von abi/toolchain_stamp_glied.hpp; dieser
/// Header haelt nur den SLOT, damit die Glied-Ordnung an EINER Stelle wohnt.
///
/// DEFAULT LEER == IDENTITAET: ohne Define traegt das Glied nichts bei (ausser seinem Separator). Die
/// Verdrahtung der per-Perm-Werte (Realversions-Probe G-C4 an der CEB, Emission je Zelle) ist die
/// FOLGE-Scheibe C-3 des Buendels -- sie beruehrt profile_run_entry/experiment_plan_director, die zum
/// Zeitpunkt dieses Commits einer anderen Welle gehoeren. Der Slot ist bewusst VORHER gebaut: der
/// Format-Bump und die Frozen-Neuanker kosten so GENAU EINEN Anker, nicht zwei.
#ifndef COMDARE_TOOLCHAIN_STAMP_GLIED
#define COMDARE_TOOLCHAIN_STAMP_GLIED ""
#endif
inline constexpr std::string_view kToolchainStampGlied = COMDARE_TOOLCHAIN_STAMP_GLIED;

/// O-2/C-2 -- DIE BVSET-NAHT (Glied [6]). F7-Spez (b): "Variant-/Treiber-Enable-Menge (bvset-Signatur) als
/// Preimage-Glied -- deckt page_type/general_hardware".
///
/// WARUM ALS DEFINE UND NICHT ALS INCLUDE: die Signatur entsteht in builder/build_variant_set_signature.hpp
/// aus den REALEN Enabled-Typlisten der Achsen-Registries (driver_build_variant_signature.hpp). abi/ darf
/// builder/ nicht sehen -- ein Include waere ein Schichtungs-Bruch und wuerde ausserdem die
/// CMake-generierten Flags-Header in jede abi-TU ziehen. Der Wert wird deshalb INJIZIERT (K-1-Muster,
/// benannter Traeger-Typ unten), auf dem Perm-Pfad ueber dieses Define.
///
/// KONSEQUENZ (plan-gedeckt, F7-(b)): COMDARE_VARIANT_GATE wird funktional obsolet -- die Enable-Mengen
/// diskriminieren ab Format 3 den Fingerprint SELBST, statt einen eigenen String-Vergleich zu brauchen.
/// Das `.variant`-Sidecar bleibt reine Provenienz. DEFAULT LEER == IDENTITAET (wie oben).
#ifndef COMDARE_BUILD_VARIANT_SET_SIGNATURE
#define COMDARE_BUILD_VARIANT_SET_SIGNATURE ""
#endif
inline constexpr std::string_view kBuildVariantSetSignatureGlied = COMDARE_BUILD_VARIANT_SET_SIGNATURE;

/// K-1 (dieselbe Begruendung wie bei OverlayHash): Toolchain- und bvset-Glied reisen als BENANNTE TYPEN.
///
/// DIE FALLE, DIE SIE SCHLIESSEN: die Glied-Folge besteht ab Format 3 aus DREI aufeinanderfolgenden
/// injizierbaren Zeichenketten (Toolchain, bvset, Overlay). Waeren sie drei nackte string_view-Parameter,
/// koennte jeder Aufrufer sie in beliebiger Reihenfolge reichen -- es kompiliert, der Digest ist falsch,
/// und der Fehler zeigt sich erst als "der Cache greift nie" bzw. als falscher Skip. Drei disjunkte Typen
/// machen jede Verwechslung compile-hart.
///
/// NB/CX-1: beide Traeger pruefen ihren Wert im Konstruktor (s. OverlayHash). Damit ist die
/// Injektivitaets-Pflicht der zur CEB-LAUFZEIT gereichten Werte kein Kommentar mehr, sondern Mechanik:
/// lazy_adhoc_fingerprint_for baut die Traeger aus Laufzeit-Strings, also laeuft jeder Laufzeit-Wert durch
/// dieselbe Wache wie die Compile-Defines.
///
/// NB2-2: beide Traeger sind ab hier GEKAPSELT (Begruendung ausfuehrlich bei OverlayHash) -- ein public
/// mutierbares `.value` machte die Konstruktor-Wache zu einer blossen Empfehlung.
class ToolchainGlied {
public:
    constexpr explicit ToolchainGlied(std::string_view v) : wert_{v} { require_injizierter_glied_wert("toolchain", v); }

    [[nodiscard]] constexpr std::string_view wert() const noexcept { return wert_; }

private:
    std::string_view wert_;
};

class BvsetGlied {
public:
    constexpr explicit BvsetGlied(std::string_view v) : wert_{v} { require_injizierter_glied_wert("bvset", v); }

    [[nodiscard]] constexpr std::string_view wert() const noexcept { return wert_; }

private:
    std::string_view wert_;
};

/// Anzahl der Preimage-Glieder. FEST -- die Injektivitaet der '\n'-Zerlegung haengt an der festen Anzahl.
/// A13-M3 (Format 2): 6. O-2/C-2 (Format 3): 8 (Toolchain + bvset kommen dazu).
inline constexpr std::size_t kAnatomyFingerprintGliedCount = 8;

/// W10-C3: die POSITION der System-Zeile in der Glied-Folge, benannt statt als nackte 2.
///
/// WOZU: der Laufzeit-Zwilling des Lager-Keys (BinaryKeyPolicy, bestandslog_factory.hpp) bekommt die
/// fertigen Glieder als span und muss GENAU EIN Glied -- die System-Zeile -- um die Zellwerte
/// vervollstaendigen. Ohne benannte Konstante stuende dort eine nackte 2, die bei jeder kuenftigen
/// Umsortierung der Glied-Folge STILL auf das falsche Glied zeigte. Die Konstante wohnt hier, weil hier
/// auch die Ordnung wohnt (dieselbe Begruendung wie fuer anatomy_fingerprint_glieder selbst).
inline constexpr std::size_t kAnatomyFingerprintSystemGlied = 2;

/// O-2/C-2: die POSITIONEN der drei injizierbaren Schwanz-Glieder, ebenfalls benannt statt nackt.
///
/// WARUM DAS OVERLAY-GLIED ANS ENDE WANDERT (5 -> 7): es ist das einzige Glied, dessen Inhalt noch
/// aussteht (L14/OF-M3-2 Fallback B, Codegen erst in Phase 6). Ein noch leeres Glied gehoert an den
/// Schwanz, damit die Positionen der GEFUELLTEN Glieder stabil bleiben, wenn es scharfgeschaltet wird.
/// Der Umzug selbst ist ungefaehrlich, weil die Format-Kennung [0] jede Layout-Evolution deterministisch
/// vom Alt-Bestand trennt -- genau der Zweck, fuer den F7 sie verlangt hat.
inline constexpr std::size_t kAnatomyFingerprintToolchainGlied = 5;
inline constexpr std::size_t kAnatomyFingerprintBvsetGlied     = 6;
inline constexpr std::size_t kAnatomyFingerprintOverlayGlied   = 7;

// -- BUDGET-NACHWEIS (O-2/C-2), maschinell statt als Absatz --------------------------------------------
// Je Glied eine Obergrenze; ihre Summe plus die (GliedCount-1) Separator-Bytes MUSS in den
// consteval-Puffer passen. Die Zahlen sind an den Ist-Daten begruendet (s. Budget-Beleg oben):
//   Organ 768   -- 546 (laengste reale binary_id) + 18*12 Versions-Schwaenze = 762, aufgerundet
//   System 256  -- 3 Achsen + Meta-Meta-Klammer (~150 gemessen)
//   Mess 256    -- Tooling-Vollmenge + load_framework-Klammer (<200 gemessen)
//   Werteset    -- exakt das Puffer-Budget seines eigenen Renderers (kSubAxisValuesetSegmentMax)
//   Toolchain 512 -- 9 Felder, davon zwei mit Flag-Klammer; die realen Flags sind kurz ("-O3", "-mcx16")
//   bvset 1536  -- das einzige mit der Flotte wachsende Glied; LEBEND gemessen in
//                  driver_build_variant_signature.hpp gegen genau diese Konstante
//   Overlay 128 -- ein SHA-512-Hex
inline constexpr std::size_t kAnatomyFingerprintFormatMax      = 32;
inline constexpr std::size_t kAnatomyFingerprintOrganMax       = 768;
inline constexpr std::size_t kAnatomyFingerprintSystemMax      = 256;
inline constexpr std::size_t kAnatomyFingerprintMeasurementMax = 256;
inline constexpr std::size_t kAnatomyFingerprintValuesetMax    = kSubAxisValuesetSegmentMax;
inline constexpr std::size_t kAnatomyFingerprintToolchainMax   = 512;
inline constexpr std::size_t kAnatomyFingerprintBvsetMax       = 1536;
inline constexpr std::size_t kAnatomyFingerprintOverlayMax     = 128;

inline constexpr std::size_t kAnatomyFingerprintBudgetSum =
    kAnatomyFingerprintFormatMax + kAnatomyFingerprintOrganMax + kAnatomyFingerprintSystemMax +
    kAnatomyFingerprintMeasurementMax + kAnatomyFingerprintValuesetMax + kAnatomyFingerprintToolchainMax +
    kAnatomyFingerprintBvsetMax + kAnatomyFingerprintOverlayMax + (kAnatomyFingerprintGliedCount - 1);

static_assert(kAnatomyFingerprintBudgetSum <= kAnatomyFingerprintPreimageMax,
              "O-2/C-2 BUDGET: die Summe der Glied-Obergrenzen plus Separatoren passt nicht mehr in "
              "kAnatomyFingerprintPreimageMax. Entweder ein Glied-Budget senken oder den Puffer heben -- "
              "und in beiden Faellen den Beleg oben nachziehen, statt die Grenze still zu verschieben.");

// Die EINGEFROREREN Glieder halten ihr Budget schon compile-time ein (die injizierten pruefen ihre
// Traeger-Header bzw. der lebende bvset-Zwilling in driver_build_variant_signature.hpp).
static_assert(kAnatomyFingerprintFormat.size() <= kAnatomyFingerprintFormatMax);
static_assert(kSubAxisValuesetSegment.size() <= kAnatomyFingerprintValuesetMax);
static_assert(kToolchainStampGlied.size() <= kAnatomyFingerprintToolchainMax);
static_assert(kBuildVariantSetSignatureGlied.size() <= kAnatomyFingerprintBvsetMax);
static_assert(kOverlaySourceHash.size() <= kAnatomyFingerprintOverlayMax);

// '\n'-FREIHEIT der neuen Glieder (OF-M3-1): die Injektivitaet der Zerlegung haengt daran, dass KEIN Glied
// den Domain-Separator traegt. Fuer die per Define injizierten Glieder ist das hier compile-time bewiesen;
// fuer die zur CEB-Laufzeit gereichten Werte traegt die Injektions-Naht dieselbe Pflicht (C-3).
static_assert(kAnatomyFingerprintFormat.find(kAnatomyFingerprintSeparator) == std::string_view::npos);
static_assert(kToolchainStampGlied.find(kAnatomyFingerprintSeparator) == std::string_view::npos,
              "Das Toolchain-Glied darf den Domain-Separator '\\n' nicht enthalten.");
static_assert(kBuildVariantSetSignatureGlied.find(kAnatomyFingerprintSeparator) == std::string_view::npos,
              "Das bvset-Glied darf den Domain-Separator '\\n' nicht enthalten.");
static_assert(kOverlaySourceHash.find(kAnatomyFingerprintSeparator) == std::string_view::npos,
              "Das Overlay-Glied darf den Domain-Separator '\\n' nicht enthalten.");

// NB/CX-1: dieselbe VOLLE Format-Wache, die ab jetzt jeder LAUFZEIT-Wert durchlaeuft, gilt auch fuer die
// per Define injizierten Konstanten -- sonst waere ausgerechnet der produktive Weg (Compile-Define) laxer
// gegatet als der Ausnahme-Weg (Laufzeit-Injektion). Ein Verstoss bricht hier compile-hart mit Namen.
static_assert(injizierter_glied_wert_ist_wohlgeformt(kToolchainStampGlied),
              "NB/CX-1: COMDARE_TOOLCHAIN_STAMP_GLIED verletzt die Injektivitaets-Format-Wache.");
static_assert(injizierter_glied_wert_ist_wohlgeformt(kBuildVariantSetSignatureGlied),
              "NB/CX-1: COMDARE_BUILD_VARIANT_SET_SIGNATURE verletzt die Injektivitaets-Format-Wache.");
static_assert(injizierter_glied_wert_ist_wohlgeformt(kOverlaySourceHash),
              "NB/CX-1: COMDARE_OVERLAY_SOURCE_HASH verletzt die Injektivitaets-Format-Wache.");

/// anatomy_fingerprint_glieder(...) -- DIE EINE QUELLE der Preimage-Ordnung. Jede Rechen-Stelle (der
/// consteval-Hex unten, der Laufzeit-Zwilling lazy_adhoc_fingerprint_for, der Lager-Key-Ableiter
/// derive_key_from_lines ueber seinen Aufrufer) zieht ihre Glieder HIER heraus. Wer die Ordnung aendert,
/// aendert sie an genau dieser Stelle -- vorher drifteten vier Kopien auseinander (Risiko R6).
///
/// Ordnung (O-2/C-2, Format 3):
///   [0] fingerprint_format-Kennung  (F7: Layout-Evolution mismatcht deterministisch)
///   [1] Organ-Zeile                 (18 Haupt-Achsen, achse=algo@X.Y.Z)
///   [2] System-Zeile                (3 Haupt-Achsen + Meta-Meta-Klammer-Anhang)
///   [3] Mess-Tooling-Zeile          (Haupt-Wahl + load_framework-Anhang; die CT-Mess-AUSSTATTUNG dieser
///                                    Tier-Binary -- NICHT der CEB-Schluessel, Paragraf 62-D trennt beide)
///   [4] Sub-Achsen-Werteset-Segment (F7-VERIFY: sonst wuerde ein Werteset-Bump unter dem SHA512-only-Gate
///                                    STILL reused -- die Organ-Zeile traegt den Sub-Schwanz ausdruecklich nicht)
///   [5] Toolchain-Glied             (NEU: Compiler-Haupt-Achse inkl. Flags, opt_level, atomic128, ext/bt/
///                                    gate/ceb -- die CEB-Laufzeit-Hauptachsen als CT-Glieder; heilt C1)
///   [6] bvset-Glied                 (NEU: Enabled-Mengen-Signatur der Build-Achsen; heilt C6)
///   [7] Overlay-Source-Hash         (heute leer, OF-M3-2 = Fallback B; ans Ende gewandert)
///
/// Die drei Schwanz-Glieder haben Defaults, weil sie INJIZIERT werden: wer sie nicht kennt, reicht sie
/// nicht -- und rechnet dann byte-identisch zur Identitaet. Das ist dieselbe Zusage wie bei
/// SystemCellValues (W10) und macht die Naht ohne Flotten-weiten Umbau baubar.
[[nodiscard]] constexpr std::array<std::string_view, kAnatomyFingerprintGliedCount>
anatomy_fingerprint_glieder(std::string_view organ, std::string_view system, std::string_view measurement,
                            ToolchainGlied toolchain = ToolchainGlied{kToolchainStampGlied},
                            BvsetGlied     bvset     = BvsetGlied{kBuildVariantSetSignatureGlied},
                            OverlayHash    overlay   = OverlayHash{kOverlaySourceHash}) noexcept {
    return {kAnatomyFingerprintFormat, organ,            system,       measurement,
            kSubAxisValuesetSegment,   toolchain.wert(), bvset.wert(), overlay.wert()};
}

/// W10-C3: die Positions-Konstante ist BEWIESEN, nicht behauptet -- wer die Glied-Ordnung oben umbaut,
/// bricht hier compile-time, statt den Zellwert still ins Organ-Glied zu schreiben.
static_assert(anatomy_fingerprint_glieder("ORGAN", "SYSTEM", "MESS")[kAnatomyFingerprintSystemGlied] == "SYSTEM",
              "kAnatomyFingerprintSystemGlied zeigt nicht mehr auf die System-Zeile der Glied-Folge.");
static_assert(kAnatomyFingerprintSystemGlied < kAnatomyFingerprintGliedCount);

/// O-2/C-2: dieselbe Beweis-Form fuer die drei neuen/verschobenen Positionen. Sie sind KEINE Kommentare:
/// bestandslog_factory vervollstaendigt Glied [2] anhand seiner Konstante, und die Konsumenten der
/// Toolchain-/bvset-Naht adressieren ihre Glieder ebenso. Eine Umsortierung ohne Nachzug bricht hier.
static_assert(anatomy_fingerprint_glieder("O", "S", "M", ToolchainGlied{"TC"}, BvsetGlied{"BV"},
                                          OverlayHash{"OV"})[kAnatomyFingerprintToolchainGlied] == "TC",
              "kAnatomyFingerprintToolchainGlied zeigt nicht mehr auf das Toolchain-Glied.");
static_assert(anatomy_fingerprint_glieder("O", "S", "M", ToolchainGlied{"TC"}, BvsetGlied{"BV"},
                                          OverlayHash{"OV"})[kAnatomyFingerprintBvsetGlied] == "BV",
              "kAnatomyFingerprintBvsetGlied zeigt nicht mehr auf das bvset-Glied.");
static_assert(anatomy_fingerprint_glieder("O", "S", "M", ToolchainGlied{"TC"}, BvsetGlied{"BV"},
                                          OverlayHash{"OV"})[kAnatomyFingerprintOverlayGlied] == "OV",
              "kAnatomyFingerprintOverlayGlied zeigt nicht mehr auf das Overlay-Glied.");
static_assert(kAnatomyFingerprintOverlayGlied == kAnatomyFingerprintGliedCount - 1,
              "Das Overlay-Glied ist per O-2/C-2 das SCHWANZ-Glied (noch leer, L14/Phase 6).");
static_assert(kAnatomyFingerprintSystemGlied < kAnatomyFingerprintToolchainGlied &&
                  kAnatomyFingerprintToolchainGlied < kAnatomyFingerprintBvsetGlied &&
                  kAnatomyFingerprintBvsetGlied < kAnatomyFingerprintOverlayGlied,
              "Die benannten Positionen muessen aufsteigend und paarweise verschieden sein.");

// -- NB2-2: DIE EINE PREIMAGE-KONSTRUKTION -------------------------------------------------------------
//
// DER BEFUND, DEN SIE HEILT (Codex-Zweitreview [KRITISCH], am Code bestaetigt): es gab ZWEI Preimage-
// Bildungen. Die Laufzeit-Form (anatomy_fingerprint_preimage) trug seit NB/CX-1 die Separator-Wache; die
// consteval-Form in anatomy_fingerprint_hex baute ihren Puffer in einer EIGENEN, UNGEPRUEFTEN Schleife.
// Damit galt die Injektivitaets-Zusage genau auf dem Weg NICHT, der die einkompilierten Stempel-Literale
// hasht -- also auf dem produktiven. Die Kollision braucht keinen SHA-Bruch:
//     A = {organ="A\nB", system="C"}   und   B = {organ="A", system="B\nC"}
// ergeben BYTE-IDENTISCHE Preimages, also denselben Fingerprint fuer zwei verschiedene Tier-Binaries.
// "Zwei Schleifen, EINE Ordnung" war als Kommentar wahr und als Mechanik falsch: die Ordnung war geteilt,
// die WACHE nicht.
//
// DIE AUFLOESUNG: EINE constexpr-Kernfunktion, die beide Wege benutzen. Sie unterscheiden sich nur noch in
// der SENKE -- der consteval-Weg braucht einen uint8-Puffer (ein reinterpret_cast auf std::string::data()
// waere in einem konstanten Ausdruck verboten), der Laufzeit-Weg einen std::string. Die Ordnung, der
// Separator UND die Wache stehen ab hier physisch nur noch EINMAL da; eine der beiden Seiten kann gar
// nicht mehr laxer sein als die andere.

namespace detail {

/// Die Separator-Wache EINES Glieds -- fail-loud auf beiden Wegen. Zur LAUFZEIT wirft sie mit benannter
/// Fehlerklasse; in einem konstanten Ausdruck ist ein Wurf per Definition kein konstanter Ausdruck mehr,
/// die consteval-Auswertung bricht also COMPILE-HART mit Verweis auf genau diese Stelle.
/// Die Positions-Ziffer wird von Hand gesetzt statt via std::to_string: letzteres ist nicht constexpr und
/// wuerde den Kern fuer den consteval-Weg unbrauchbar machen.
constexpr void require_glied_ohne_separator(std::size_t position, std::string_view glied) {
    if (glied.find(kAnatomyFingerprintSeparator) == std::string_view::npos) return;
    std::string wo{"?"};
    if (position < 10) wo[0] = static_cast<char>('0' + static_cast<int>(position));
    throw std::invalid_argument(
        std::string{"fehlerklasse=stempel_injektivitaet: das Preimage-Glied an Position "} + wo +
        " traegt den Domain-Separator '\\n'. Die Zerlegung des Preimage waere damit mehrdeutig, zwei "
        "verschiedene Glied-Saetze koennten denselben Fingerprint ergeben (falscher Skip).");
}

/// Die Laufzeit-Senke: ein wachsender std::string.
struct PreimageStringSenke {
    std::string    aus;
    constexpr void put(char c) { aus.push_back(c); }
};

/// Die consteval-Senke: ein exakt budgetierter Byte-Puffer (die Budget-static_asserts oben decken ihn).
template <std::size_t N>
struct PreimageBytesSenke {
    std::array<std::uint8_t, N> aus{};
    std::size_t                 n = 0;
    constexpr void              put(char c) { aus[n++] = static_cast<std::uint8_t>(c); }
};

} // namespace detail

/// anatomy_fingerprint_preimage_emit(glieder, senke) -- DIE EINE Preimage-Konstruktion.
///
/// NB/CX-1 (unveraendert gueltig, jetzt auf BEIDEN Wegen): die Separator-Wache gilt fuer JEDES Glied --
/// auch fuer die drei Stempel-ZEILEN, deren Werte aus Achsen-Namen und Mess-Combos entstehen und damit von
/// aussen beeinflussbar sind. Nur der Separator wird geprueft, nicht der Zeichenvorrat: die Achsen-Namen
/// gehoeren den Achsen, nicht diesem Header (die schaerfere Wache tragen die injizierten Glieder in ihren
/// Traeger-Typen).
template <class Senke>
constexpr void anatomy_fingerprint_preimage_emit(std::span<std::string_view const> glieder, Senke& senke) {
    for (std::size_t i = 0; i < glieder.size(); ++i) {
        detail::require_glied_ohne_separator(i, glieder[i]);
        if (i != 0) senke.put(kAnatomyFingerprintSeparator);
        for (char const c : glieder[i]) senke.put(c);
    }
}

/// anatomy_fingerprint_preimage(glieder) -- der LAUFZEIT-Weg. Er ist ab NB2-2 nur noch die STRING-SENKE
/// ueber dem gemeinsamen Kern; er haelt keine eigene Ordnung und keine eigene Wache mehr. Er bleibt die
/// EINZIGE Laufzeit-Bildung des Preimage (der Lager-Key-Ableiter derive_key_from_lines zieht hier durch).
[[nodiscard]] inline std::string anatomy_fingerprint_preimage(std::span<std::string_view const> glieder) {
    detail::PreimageStringSenke senke;
    anatomy_fingerprint_preimage_emit(glieder, senke);
    return std::move(senke.aus);
}

/// anatomy_fingerprint_hex(organ, system, measurement[, Traeger...]) -- 128-hex SHA-512 (nullterminiert,
/// array<char, 129>) ueber die '\n'-getrennte Glied-Folge aus anatomy_fingerprint_glieder(). consteval:
/// reine Compile-Zeit-Ableitung der einkompilierten Stempel-Literale (leere Zeilen -> leeres Glied, aber
/// der Separator bleibt -> die Feldgrenze ist erhalten; genau das ist der GA-01-Fix).
///
/// NB2-2: der Puffer wird ueber DENSELBEN Kern gefuellt wie der Laufzeit-Weg -- inklusive Wache. Die
/// Funktion ist deshalb NICHT mehr `noexcept`: ein Glied mit Domain-Separator MUSS die consteval-Auswertung
/// abbrechen (kein konstanter Ausdruck), statt still ueber std::terminate zu laufen oder -- schlimmer -- ein
/// mehrdeutiges Preimage zu hashen.
[[nodiscard]] consteval std::array<char, 129>
anatomy_fingerprint_hex(std::string_view organ, std::string_view system, std::string_view measurement,
                        ToolchainGlied toolchain = ToolchainGlied{kToolchainStampGlied},
                        BvsetGlied     bvset     = BvsetGlied{kBuildVariantSetSignatureGlied},
                        OverlayHash    overlay   = OverlayHash{kOverlaySourceHash}) {
    auto const glieder = anatomy_fingerprint_glieder(organ, system, measurement, toolchain, bvset, overlay);
    detail::PreimageBytesSenke<kAnatomyFingerprintPreimageMax> senke{};
    anatomy_fingerprint_preimage_emit(std::span<std::string_view const>{glieder.data(), glieder.size()}, senke);
    auto const digest =
        ::comdare::cache_engine::sha512::sha512(std::span<const std::uint8_t>{senke.aus.data(), senke.n});
    auto const            hex = ::comdare::cache_engine::sha512::to_hex(digest); // array<char, 128>
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
                  "anatomy_fingerprint_hex ist ein BENANNTER Glied-TYP, kein string_view. O-2/C-2 (Format 3) "
                  "haengt zwei weitere injizierbare Glieder an: die Schwanz-Slots heissen jetzt "
                  "ToolchainGlied{...}, BvsetGlied{...}, OverlayHash{...} -- in dieser Reihenfolge. Ein "
                  "Alt-Aufruf mit nackten string_view wuerde sie still gegeneinander verschieben; genau das "
                  "verhindert diese Sperre. Aufruf auf die 3-arg-Form ziehen (bzw. die Traeger-Typen explizit "
                  "angeben).");
    return {};
}

} // namespace comdare::cache_engine::abi
