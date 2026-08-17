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

#include "mess_gates_glied.hpp"         // R-3: der Mess-GATE-Zustand DIESER TU als Preimage-Glied [8]
#include "subaxis_valueset_segment.hpp" // A13-M3: das Sub-Achsen-Werteset-Segment als Preimage-Glied

// E-E (07.08.2026): der Overlay-Quell-Hash als Preimage-Glied [7]. Der Header ist GENERIERT
// (tools/overlay_source_hash_gen, verdrahtet in cmake/overlay_source_hash.cmake) und setzt
// COMDARE_OVERLAY_SOURCE_HASH, falls es nicht schon per -D hereingereicht wurde.
//
// HARTES #include, KEIN __has_include: fehlt der Header, MUSS der Bau brechen. Ein weiches
// __has_include machte die Abwesenheit des Codegens still -- die TU rechnete dann mit leerem Glied
// weiter und traege eine FALSCHE Identitaet. Genau diese Klasse von Stille beseitigt E-E; sie darf
// nicht an ihrer eigenen Naht wieder eintreten.
#include <cache_engine/abi/overlay_source_hash_generated.hpp>

#include <sha512/ctsha512.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept> // NB/CX-1: die RT-Injektivitaets-Wache der injizierten Glieder ist FAIL-LOUD
#include <string>
#include <string_view>
#include <type_traits> // T2-D: GliedSterbenderString; S-6b: die CT-Negativ-Proben der Zeilen-Traeger

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
/// Format 4 (R-3, 07.08.2026): das MESS-GATES-Glied [8] kommt dazu (abi/mess_gates_glied.hpp) -- der
///   PRAEPROZESSOR-Zustand der Mess-Gates DIESER Uebersetzungseinheit. Bis Format 3 stand KEIN
///   Gate-Makro im Preimage; dasselbe emittierte .cpp mit und ohne -DCOMDARE_MEASUREMENT_ON=1 ergab
///   zwei verschiedene Binaries mit DEMSELBEN Digest (am Objekt gemessen, tests/unit/
///   r3_mess_gate_stamp_module.cpp: .so-sha256 820533fb vs. e92658ce, sha512_line BEIDE 6739cae7...).
///   dll_is_current (build_orchestrator.hpp) vergleicht genau diese Zahl gegen ein Sidecar -- der Skip
///   war still und falsch. Glied [3] konnte das nicht heilen: es traegt die Mess-Combo als HOST-Literal,
///   also als Behauptung des Bauwerkzeugs, nicht als Wahrheit der TU.
///   WARUM WIEDER EIN BUMP UND NICHT DIE NIEMALS-LEERHEIT DES NEUEN GLIEDS: dieselbe Begruendung wie
///   bei Format 3 -- ein Bestand aus der Zeit VOR der Gate-Verankerung wuerde sonst weiter
///   uebersprungen, obwohl seine Mess-Ausstattung nie geprueft wurde. Der Bump erzwingt die EINE
///   deklarierte Invalidierungswelle. Sie ist im Fenster dieses Commits KOSTENLOS: der
///   .fingerprint-Sidecar-Bestand ist literal 0 (git ls-files '*.fingerprint' -> 0 Zeilen; find ueber
///   den GESAMTEN Arbeitsbaum inkl. Build-Verzeichnisse -> 0 Dateien). Nach dem ersten golden-Batch
///   kostete dieselbe Aenderung einen Voll-Neubau der Flotte plus Messdaten-Entwertung -- der Fix
///   gehoert deshalb VOR das naechste GOLDEN-UPDATE-Fenster und nicht dahinter.
inline constexpr std::string_view kAnatomyFingerprintFormat = "fingerprint_format=4";

/// Das letzte Preimage-Glied: der HASH ALLER SOURCE-CODE-DATEIEN IM OVERLAY (Owner-Abnahme 26.07.,
/// Ledger 88: "consteval-SHA512-Zeile ueber Achsen-Strings + Versionen + Overlay-Source-Hashes").
///
/// WIE ER HIERHER KOMMT: ausdruecklich per PRE-BUILD-CODEGEN, nie zur Laufzeit -- der Codegen hasht die
/// Overlay-Quellen und reicht das Ergebnis als Compile-Define herein (Muster COMDARE_GN_ALGO_SIG). Ein
/// consteval-Hash kann keine Dateien lesen; jede Laufzeit-Variante waere ein Bruch der Doktrin
/// "compile-time only" und wuerde die Binary nicht mehr eindeutig machen.
///
/// -- E-E (07.08.2026): SCHARFGESCHALTET. DIE DREI OWNER-FESTLEGUNGEN LIEGEN VOR ------------------------
///
/// Bis hierher stand an dieser Stelle: "OFFEN und bewusst NICHT geraten: WELCHE Dateimenge 'das Overlay'
/// ist (Verzeichnis-Schnitt, Sortier-Ordnung, Hash je Datei vs. ueber die Konkatenation)" -- A13-M3/
/// OF-M3-2 = FALLBACK B, weil die Festlegungen zum M3-Start fehlten. Sie sind am 07.08.2026 getroffen,
/// und sie stehen ab hier hier, damit die naechste Suche nicht wieder auf einer beantworteten Frage landet:
///   (1) KONKATENATION, nicht Hash je Datei: die Bytes werden aneinandergehaengt, EINMAL SHA-512 darueber.
///   (2) FESTE STATISCHE ORDNUNG JE ACHSEN-KATEGORIE, und zwar die BESTEHENDE: Organ nach
///       kCompositionAxisNames (18), System nach kSystemAxisOrder (3), Mess strukturell EINE Achse
///       (measurement_tooling -- pro Binary wird genau eine gewaehlt, dort ist nichts zu sortieren).
///   (3) DER SCHNITT: ueber die drei kanonischen Achsen-Ordnungen -- je Achse die Dateimenge IHRER
///       EIGENEN Implementierung -- plus libs/cache_engine/anatomy/ als gemeinsame Tier-Substanz.
///
/// DIE MENGE SELBST STEHT NICHT HIER, sondern an EINER Stelle: builder/overlay_source_set.hpp. Dort
/// wohnen der Schnitt, seine Begruendung und die compile-harten Ordnungs-Wachen gegen die beiden
/// kanonischen Quellen. Dieser Header darf sie nicht sehen -- abi/ liegt unter builder/ (dieselbe
/// Schichtungs-Regel, aus der auch das bvset-Glied [6] injiziert statt inkludiert wird).
///
/// WAS DER WERT DECKT, UND WARUM ES DIE ANDEREN GLIEDER NICHT TUN: Glied [5] traegt Bau-SCHALTER,
/// Glied [6] die ENABLED-MENGEN der Build-Achsen, die Glieder [1]/[2]/[3] Achsen-NAMEN und VERSIONEN --
/// alles Behauptungen UEBER den Code. Der INHALT unserer Quelldateien war bis hierher von KEINEM Glied
/// gedeckt: zwei Baue mit identischen Achsen-Strings, Versionen, Toolchain und bvset, aber geaendertem
/// Implementierungs-Quelltext, trugen denselben Fingerprint, dll_is_current uebersprang den Neubau und
/// das Lager lieferte die alte Binary aus (build_orchestrator.hpp, L14). Das endet mit diesem Glied.
///
/// KEIN FORMAT-BUMP, und der Grund ist praezise: anders als bei R-3 droht kein STILLER Skip, weil es
/// keinen Bestand gibt, der uebersprungen werden koennte -- der .fingerprint-Sidecar-Bestand ist literal
/// 0 (find ueber den GESAMTEN Arbeitsbaum inkl. Build-Verzeichnisse -> 0 Dateien, am 07.08. erneut
/// nachgezaehlt). Das Glied wechselt von leer auf belegt; jeder Fingerprint bewegt sich damit ohnehin
/// EINMAL, und zwar fail-closed (Neubau), nicht fail-open (falscher Skip). Waere Bestand da, waere der
/// Bump Pflicht.
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
           c == ';' || c == '.' || c == ',' || c == '+' || c == '-' || c == '_' || c == ':' || c == '/' || c == '[' ||
           c == ']' || c == '{' || c == '}';
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
///
/// -- NB-3/T2-D: DIE LEBENSDAUER-HAELFTE DERSELBEN ZUSAGE ------------------------------------------------
///
/// DER BEFUND (Codex-Zweitreview [MITTEL], am Code bestaetigt): NB2-2 hat den Wert gekapselt, aber er
/// bleibt eine SICHT. Zwei Loecher blieben damit offen, und beide fuehren an der Konstruktor-Wache vorbei:
///   (a) DANGLING -- `ToolchainGlied{compose_live_toolchain_stamp_glied()}` bindet an ein Temporary, das am
///       Ende des Voll-Ausdrucks stirbt. Der Traeger zeigt danach ins Leere; was gehasht wird, ist Zufall.
///   (b) SPAETE MUTATION -- der Puffer HINTER der Sicht kann sich nach der Pruefung noch aendern. Die
///       Zusage "ab Konstruktion geprueft" gilt dann fuer einen Wert, den niemand mehr hasht.
///
/// ZWEI MASSNAHMEN, je eine pro Loch:
///   (a) Der Konstruktor aus einem std::string-RVALUE ist GELOESCHT. Ein Temporary kann sich damit nicht
///       mehr an einen Traeger binden -- der Fehler wird compile-hart und nennt sich beim Namen, statt als
///       sporadisch falscher Digest zu erscheinen. Lvalue-Strings (der Normalfall: eine benannte Variable,
///       die den Aufruf ueberlebt) bleiben unveraendert zulaessig.
///   (b) Die VOLLE Format-Wache laeuft ZUSAETZLICH beim GEBRAUCH, in anatomy_fingerprint_glieder() -- also
///       an der einen Stelle, die weiss, welches Glied welches ist, und unmittelbar bevor der Wert ins
///       Preimage geht. Zwischen dieser Pruefung und dem Hash liegt kein Aufrufer mehr.
///
/// -- T2-D-UEBERNAHME: WARUM (a) NICHT `explicit X(std::string&&) = delete` LAUTEN DARF ------------------
///
/// Die Vorarbeit dieser Wache stand genau so da -- und der Bau brach FLOTTENWEIT. Grund, am Objekt
/// gelesen (gcc-Diagnose "call of overloaded 'ToolchainGlied(<brace-enclosed initializer list>)' is
/// ambiguous", 2 Kandidaten): fuer ein STRING-LITERAL sind beide Konstruktoren gleich gut erreichbar --
/// `char const[N]` -> `std::string_view` und `char const[N]` -> `std::string` sind beides
/// benutzerdefinierte Konvertierungen derselben Rangstufe. Die Ueberladungsaufloesung kann zwischen ihnen
/// nicht waehlen, und der geloeschte Kandidat macht damit ausgerechnet den Normalfall `ToolchainGlied{"TC"}`
/// unbaubar (die static_asserts unten, jedes Test-Literal, jeder consteval-Zwilling in den generierten
/// Anatomy-Modulen). Eine Wache, die den legitimen Gebrauch sperrt, ist keine Wache, sondern ein Ausfall.
///
/// DIE KORREKTUR: der geloeschte Konstruktor wird auf GENAU den Fall verengt, den er treffen soll -- ein
/// std::string-PRVALUE/XVALUE. Als constrained Template greift er erst NACH der Deduktion: fuer ein
/// Literal deduziert S zu `char const(&)[N]` (Bedingung falsch -> Kandidat existiert gar nicht), fuer
/// einen benannten String zu `std::string&` (falsch -> erlaubt, der Traeger ueberlebt ja), und nur fuer
/// ein Temporary zu `std::string` (wahr -> geloescht). Damit trifft die Sperre die Lebensdauer-Falle
/// vollstaendig und sonst nichts.
///
/// WARUM KEIN OWNING-TRAEGER (die Alternative, ehrlich verworfen statt verschwiegen): ein std::string im
/// Traeger wuerde die consteval-Seite kosten. kToolchainStampGlied & Co. sind `inline constexpr` und
/// werden im consteval-Fingerprint der Tier-Binary ausgewertet; ein Traeger mit dynamischem Speicher kann
/// dort nicht als Konstante leben. Der Preis waere die Compile-Zeit-Identitaet selbst -- also genau das,
/// was das Glied leisten soll. Die zwei Massnahmen oben decken beide Loecher, ohne ihn zu zahlen.

/// T2-D: das Praedikat des geloeschten Konstruktors -- GENAU ein std::string-Rvalue, nichts sonst.
/// Es steht als benanntes Konzept da (und nicht dreimal als roher requires-Ausdruck), weil alle drei
/// Traeger dieselbe Zusage geben und eine Divergenz zwischen ihnen niemandem auffiele.
template <class S>
concept GliedSterbenderString = std::is_same_v<S, std::string>;

class OverlayHash {
public:
    constexpr explicit OverlayHash(std::string_view v) : wert_{v} { require_injizierter_glied_wert("overlay", v); }
    /// NB-3/T2-D (a): kein Traeger auf ein sterbendes Temporary.
    template <GliedSterbenderString S>
    explicit OverlayHash(S&&) = delete;

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
///
/// NB-3/T2-D: beide Traeger tragen dieselbe Lebensdauer-Haertung wie OverlayHash (ausfuehrlich dort):
/// geloeschter Rvalue-Konstruktor gegen das Dangling, VOLL-Wache beim Gebrauch gegen die spaete Mutation.
class ToolchainGlied {
public:
    constexpr explicit ToolchainGlied(std::string_view v) : wert_{v} { require_injizierter_glied_wert("toolchain", v); }
    /// NB-3/T2-D (a): kein Traeger auf ein sterbendes Temporary (Verengung s. OverlayHash).
    template <GliedSterbenderString S>
    explicit ToolchainGlied(S&&) = delete;

    [[nodiscard]] constexpr std::string_view wert() const noexcept { return wert_; }

private:
    std::string_view wert_;
};

class BvsetGlied {
public:
    constexpr explicit BvsetGlied(std::string_view v) : wert_{v} { require_injizierter_glied_wert("bvset", v); }
    /// NB-3/T2-D (a): kein Traeger auf ein sterbendes Temporary (Verengung s. OverlayHash).
    template <GliedSterbenderString S>
    explicit BvsetGlied(S&&) = delete;

    [[nodiscard]] constexpr std::string_view wert() const noexcept { return wert_; }

private:
    std::string_view wert_;
};

/// R-3 -- DER TRAEGER DES MESS-GATES-GLIEDS [8]. Dieselbe K-1-Begruendung wie oben: das Preimage traegt
/// ab Format 4 VIER aufeinanderfolgende Zeichenketten im Schwanz; vier nackte string_view koennten
/// beliebig gegeneinander verschoben werden, es kompilierte, und der Digest waere falsch.
///
/// EIN UNTERSCHIED ZU DEN DREI ANDEREN, UND ER IST DER PUNKT DER SCHEIBE: dieser Wert wird NICHT
/// injiziert. Er kommt aus dem Praeprozessor-Zustand der uebersetzten TU (abi/mess_gates_glied.hpp,
/// kMessGatesTuGlied) und wird am Makro-Expansionsort EXPLIZIT gereicht. Ein Host kann ihn deshalb
/// nicht falsch behaupten -- er kann ihn nur VORHERSAGEN (mess_gates_glied_for_legend an der Mess-Naht),
/// und diese Vorhersage benutzt dieselbe Grammatik-Bildung (mess_gates_glied_komponieren).
///
/// DIE WACHE IST DIESELBE: die Grammatik "mg=m1;s1;st1;x1;tw1;tm1;tmi1" (B2) benutzt ausschliesslich Zeichen aus
/// anatomy_glied_zeichen_erlaubt, traegt kein '\n' und keinen leeren Schluessel. Der Traeger prueft das
/// im Konstruktor, anatomy_fingerprint_glieder() ein zweites Mal beim Gebrauch (NB-3/T2-D).
class MessGatesGlied {
public:
    constexpr explicit MessGatesGlied(std::string_view v) : wert_{v} {
        require_injizierter_glied_wert("mess-gates", v);
    }
    /// NB-3/T2-D (a): kein Traeger auf ein sterbendes Temporary (Verengung s. OverlayHash).
    template <GliedSterbenderString S>
    explicit MessGatesGlied(S&&) = delete;

    [[nodiscard]] constexpr std::string_view wert() const noexcept { return wert_; }

private:
    std::string_view wert_;
};

/// -- S-6b: DIE TRANSPOSITIONS-SPERRE DER DREI STEMPEL-ZEILEN -------------------------------------------
///
/// DIE FALLE, DIE SIE SCHLIESST -- dieselbe Klasse wie K-1, nur eine Ebene frueher: die Glieder [1][2][3]
/// (Organ-, System- und Mess-Tooling-Zeile) reisten bis hierher als DREI rohe string_view in fester
/// Positions-Folge. Wer zwei davon vertauscht, bekommt KEINE Diagnose: es uebersetzt, das Preimage ist
/// wohlgeformt, der Digest ist falsch. Sichtbar wird das erst als "der Cache greift nie" -- oder, teurer,
/// als FALSCHER SKIP, wenn zwei verschiedene Tier-Binaries denselben Fingerprint bekommen. Die vier
/// SCHWANZ-Glieder waren gegen genau diese Verwechslung seit K-1 geschuetzt ("drei disjunkte Typen machen
/// jede Verwechslung compile-hart"); ausgerechnet die drei Zeilen, die den Grossteil des Preimage tragen,
/// waren es nicht. Am Objekt belegt (17.08.2026): die Vertauschung zweier Zeilen-Argumente an einer
/// Aufrufstelle uebersetzte mit rc=0 und ohne eine einzige Warnung.
///
/// DREI DISJUNKTE TYPEN schliessen sie. Ein string_view konvertiert nicht implizit in einen von ihnen
/// (explicit), und untereinander konvertieren sie gar nicht -- die Sperre laesst sich weder unterlaufen
/// noch versehentlich befriedigen. Sie greift an jeder Aufrufstelle DIESER BEIDEN FUNKTIONEN zugleich,
/// weil sie in der Signatur wohnt und nicht in einer Wache, die jemand aufrufen muesste.
///
/// NICHT GEDECKT, damit aus dem Satz oben niemand die Vollstaendigkeit ableitet: der span-Eingang
/// derive_key_from_lines (builder/bestandslog/bestandslog_index.hpp) fuehrt die Glieder weiter POSITIONS-
/// gebunden in dasselbe Preimage. Er hat heute keinen Produktions-Aufrufer (die Aufrufer liegen samtlich
/// in tests/), und BinaryKeyPolicy wirft immerhin bei falscher Glied-ZAHL -- gegen eine Vertauschung bei
/// richtiger Zahl hilft das nicht. Die Signatur-Sperre erreicht ihn nicht, weil er bewusst generisch ist
/// (er bedient auch den MESSWERT-Genus). Bekommt der Lager-Weg Produktions-Aufrufer (#57), gehoert die
/// Typisierung dort in denselben Bruch.
///
/// WIE DIE SPERRE SICH MELDET -- und wo nicht mit ihrem Text: rein ROHE Aufrufe (drei Zeichenketten, auch
/// mit viertem Argument im alten merge-Slot) treffen die Roh-Sperre unten und bekommen deren benannte
/// Anleitung. MISCH-Aufrufe (ein Argument schon typisiert, ein anderes noch roh) fallen aus BEIDEN
/// Ueberladungen und melden sich generisch mit "no matching function" -- der Aufruf bricht also, aber ohne
/// die Anleitung. Dasselbe gilt fuer die VERTAUSCHUNG typisierter Argumente, und dort ist es Absicht: der
/// Typ-Name steht in der Diagnose ("...(SystemZeile, OrganZeile, MessZeile)") und sagt das Noetige. Eine
/// zweite variadische Sperre mit gemischtem Praefix waere KEIN Ausweg -- sie wuerde mit der Roh-Sperre
/// mehrdeutig und ersetzte die generische Meldung durch eine Ambiguitaets-Meldung.
///
/// WARUM SIE KEINE FORMAT-WACHE TRAGEN (ehrlich benannt statt stillschweigend weggelassen): die vier
/// Schwanz-Traeger rufen require_injizierter_glied_wert im Konstruktor, diese drei NICHT. Der Grund steht
/// schon bei anatomy_fingerprint_preimage_emit: fuer die ZEILEN gilt allein die Separator-Wache, nicht der
/// engere Zeichenvorrat -- "die Achsen-Namen gehoeren den Achsen, nicht diesem Header". Eine
/// Konstruktor-Wache mit dem engeren Vorrat wuerde legitime Achsen-Namen ablehnen; eine mit NUR der
/// Separator-Pruefung waere eine zweite Fassung derselben Wache, die anatomy_fingerprint_preimage_emit
/// ohnehin ueber JEDES Glied fuehrt -- sie verschoebe bloss den Wurf-Ort. Diese Traeger sind deshalb
/// reine BENENNUNG: sie tragen eine Positions-Zusage, keine Wert-Zusage. Genau daran haengt die
/// Neutralitaet der Scheibe -- kein Wert wird neu geprueft, kein Wert neu abgelehnt, kein Byte bewegt
/// sich (S-6b: die Typen leben in der Makro-EXPANSION, der emittierte Quelltext kennt sie nicht).
///
/// NB-3/T2-D GILT HIER UNVERAENDERT: alle drei loeschen den Konstruktor aus einem std::string-RVALUE, ueber
/// dasselbe GliedSterbenderString-Konzept und mit derselben Verengung (ein String-LITERAL muss weiter
/// passen, sonst braeche der Normalfall -- die Herleitung steht bei OverlayHash). Der Laufzeit-Weg
/// lazy_adhoc_fingerprint_for baut die Zeilen aus benannten std::string, die den Aufruf ueberleben; ein
/// Temporary waere dort dieselbe Dangling-Falle wie bei den Schwanz-Gliedern.
///
/// ABGRENZUNG, DIE NICHT VERWISCHT WERDEN DARF: MessZeile ist Glied [3] -- die Mess-TOOLING-Zeile dieser
/// Tier-Binary, also ihre einkompilierte CT-Ausstattung. Sie ist NICHT der CEB-Schluessel und NICHT der
/// Messwert-Schluessel; Paragraf 62-D trennt beide, und messwert_registrierung.hpp haelt seine eigene
/// Ableitung. Positions-Zusagen dieses Headers duerfen dorthin nie uebertragen werden (dieselbe
/// Grenzziehung, die anatomy_fingerprint_glieder unten fuer den generischen Emitter benennt).
class OrganZeile {
public:
    constexpr explicit OrganZeile(std::string_view v) noexcept : wert_{v} {}
    /// NB-3/T2-D (a): kein Traeger auf ein sterbendes Temporary (Verengung s. OverlayHash).
    template <GliedSterbenderString S>
    explicit OrganZeile(S&&) = delete;

    [[nodiscard]] constexpr std::string_view wert() const noexcept { return wert_; }

private:
    std::string_view wert_;
};

class SystemZeile {
public:
    constexpr explicit SystemZeile(std::string_view v) noexcept : wert_{v} {}
    /// NB-3/T2-D (a): kein Traeger auf ein sterbendes Temporary (Verengung s. OverlayHash).
    template <GliedSterbenderString S>
    explicit SystemZeile(S&&) = delete;

    [[nodiscard]] constexpr std::string_view wert() const noexcept { return wert_; }

private:
    std::string_view wert_;
};

class MessZeile {
public:
    constexpr explicit MessZeile(std::string_view v) noexcept : wert_{v} {}
    /// NB-3/T2-D (a): kein Traeger auf ein sterbendes Temporary (Verengung s. OverlayHash).
    template <GliedSterbenderString S>
    explicit MessZeile(S&&) = delete;

    [[nodiscard]] constexpr std::string_view wert() const noexcept { return wert_; }

private:
    std::string_view wert_;
};

/// S-6b CT-NEGATIV-PROBE, am Eigentuemer statt in einer Test-TU: die Sperre ist nur so viel wert, wie die
/// drei Typen wirklich disjunkt sind. Diese Asserts sind der Beweis dafuer -- sie brechen, sobald jemand
/// einem der Traeger eine Konvertierung anhaengt (implizit ODER explizit) und damit die Vertauschung
/// wieder baubar macht. Sie stehen HIER, weil die Zusage hier entsteht; eine Test-TU koennte sie nur
/// nachtraeglich nachlesen.
static_assert(!std::is_constructible_v<OrganZeile, SystemZeile> && !std::is_constructible_v<OrganZeile, MessZeile> &&
                  !std::is_constructible_v<SystemZeile, OrganZeile> &&
                  !std::is_constructible_v<SystemZeile, MessZeile> && !std::is_constructible_v<MessZeile, OrganZeile> &&
                  !std::is_constructible_v<MessZeile, SystemZeile>,
              "S-6b: die drei Zeilen-Traeger muessen PAARWEISE unkonstruierbar auseinander sein -- sonst "
              "kompiliert eine Vertauschung wieder still und der Fingerprint waere falsch.");
static_assert(!std::is_convertible_v<std::string_view, OrganZeile> &&
                  !std::is_convertible_v<std::string_view, SystemZeile> &&
                  !std::is_convertible_v<std::string_view, MessZeile>,
              "S-6b: kein Zeilen-Traeger darf IMPLIZIT aus einer Zeichenkette entstehen -- sonst waere die "
              "Sperre an jeder Aufrufstelle mit Literalen wieder offen.");
static_assert(std::is_constructible_v<OrganZeile, std::string_view> &&
                  std::is_constructible_v<SystemZeile, std::string_view> &&
                  std::is_constructible_v<MessZeile, std::string_view>,
              "S-6b: der EXPLIZITE Weg aus einer Zeichenkette muss offen bleiben -- er ist der Normalfall.");
static_assert(!std::is_convertible_v<OrganZeile, std::string_view> &&
                  !std::is_convertible_v<SystemZeile, std::string_view> &&
                  !std::is_convertible_v<MessZeile, std::string_view>,
              "S-6b: kein Zeilen-Traeger darf ZURUECK nach string_view konvertieren -- sonst waere die "
              "Roh-Sperre unten auch fuer typisierte Aufrufe viabel und meldete den falschen Grund.");

/// Anzahl der Preimage-Glieder. FEST -- die Injektivitaet der '\n'-Zerlegung haengt an der festen Anzahl.
/// A13-M3 (Format 2): 6. O-2/C-2 (Format 3): 8 (Toolchain + bvset kommen dazu).
/// R-3 (Format 4): 9 (das Mess-Gates-Glied kommt dazu).
inline constexpr std::size_t kAnatomyFingerprintGliedCount = 9;

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

/// R-3: das Mess-Gates-Glied haengt sich HINTEN an -- die Bestands-Nummern [0]..[7] bleiben, wo sie
/// sind. Das ist keine Bequemlichkeit: bestandslog_factory (Glied [2]) und die Konsumenten der
/// Toolchain-/bvset-/Overlay-Naht adressieren ihre Glieder ueber genau diese Konstanten, und eine
/// Umsortierung waere ein zweites Byte-Ereignis ohne Gewinn. Das Overlay-Glied verliert damit seine
/// Schwanz-Stellung; ihre urspruengliche Begruendung ("ein noch leeres Glied gehoert an den Schwanz,
/// damit die Positionen der GEFUELLTEN Glieder stabil bleiben") gilt fuer das Mess-Gates-Glied NICHT --
/// es ist niemals leer (der Aus-Zustand ist "mg=m0;s0;st0;x0;tw0;tm0;tmi0" seit B2, davor ohne das
/// <st>-Feld), es kann also nicht "spaeter scharfgeschaltet" werden und braucht die Schwanz-Stellung
/// nicht.
inline constexpr std::size_t kAnatomyFingerprintMessGatesGlied = 8;

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
//   mess-gates 64 -- die feste Grammatik "mg=m1;s1;st1;x1;tw1;tm1;tmi1" misst seit B2 (15.08.2026,
//                  G3-Gate-Verfeinerung -- genau der Fall, fuer den dieses Budget Raum liess) 28
//                  Zeichen; 64 laesst weiter Raum fuer ein viertes Mess-Tooling in der Registry,
//                  ohne das Budget neu zu verhandeln.
inline constexpr std::size_t kAnatomyFingerprintFormatMax      = 32;
inline constexpr std::size_t kAnatomyFingerprintOrganMax       = 768;
inline constexpr std::size_t kAnatomyFingerprintSystemMax      = 256;
inline constexpr std::size_t kAnatomyFingerprintMeasurementMax = 256;
inline constexpr std::size_t kAnatomyFingerprintValuesetMax    = kSubAxisValuesetSegmentMax;
inline constexpr std::size_t kAnatomyFingerprintToolchainMax   = 512;
inline constexpr std::size_t kAnatomyFingerprintBvsetMax       = 1536;
inline constexpr std::size_t kAnatomyFingerprintOverlayMax     = 128;
inline constexpr std::size_t kAnatomyFingerprintMessGatesMax   = 64;

inline constexpr std::size_t kAnatomyFingerprintBudgetSum =
    kAnatomyFingerprintFormatMax + kAnatomyFingerprintOrganMax + kAnatomyFingerprintSystemMax +
    kAnatomyFingerprintMeasurementMax + kAnatomyFingerprintValuesetMax + kAnatomyFingerprintToolchainMax +
    kAnatomyFingerprintBvsetMax + kAnatomyFingerprintOverlayMax + kAnatomyFingerprintMessGatesMax +
    (kAnatomyFingerprintGliedCount - 1);

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
static_assert(kMessGatesTuGlied.size() <= kAnatomyFingerprintMessGatesMax,
              "R-3: das mess-gates-Glied sprengt sein Budget -- kAnatomyFingerprintMessGatesMax heben UND "
              "den Budget-Beleg oben nachziehen, statt die Grenze still zu verschieben.");
// Die Zeile darueber prueft nur die Grammatik DIESER TU. Seit die Bildung ihre Kapazitaet aus den
// Segmenten rechnet (mess_gates_glied.hpp, kMessGatesGliedMaxLen), laesst sich die staerkere Aussage
// treffen: KEINE der baubaren Gate-Kombinationen sprengt das Budget. Damit faellt ein zu langes
// Segment hier auf, statt erst in der einen TU, die es zufaellig baut.
static_assert(kMessGatesGliedMaxLen <= kAnatomyFingerprintMessGatesMax,
              "R-3: die LAENGSTE baubare mess-gates-Grammatik sprengt kAnatomyFingerprintMessGatesMax. Ein "
              "neues Gate oder ein laengeres Segment braucht ein groesseres Budget UND den Beleg oben -- "
              "nicht das Glueck, dass die gerade uebersetzte TU unter der Grenze bleibt.");

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
// R-3: dieselbe Wache fuer das TU-Glied. Es ist zwar nicht injiziert, aber es reist durch DASSELBE
// Preimage -- eine Grammatik, die den Domain-Separator oder einen leeren Schluessel traegt, machte die
// Zerlegung genauso mehrdeutig. Der Wert entsteht per Praeprozessor, also faellt der Verstoss hier
// compile-hart auf, nicht erst an einer Laufzeit-Naht.
static_assert(kMessGatesTuGlied.find(kAnatomyFingerprintSeparator) == std::string_view::npos,
              "Das Mess-Gates-Glied darf den Domain-Separator '\\n' nicht enthalten.");
static_assert(injizierter_glied_wert_ist_wohlgeformt(kMessGatesTuGlied),
              "R-3: kMessGatesTuGlied verletzt die Injektivitaets-Format-Wache.");
static_assert(!kMessGatesTuGlied.empty(),
              "R-3/B2: das Mess-Gates-Glied ist NIEMALS leer -- der Aus-Zustand heisst "
              "'mg=m0;s0;st0;x0;tw0;tm0;tmi0'. Ein leerer Wert waere die Identitaet und wuerde eine gate-lose "
              "TU mit dem CEB-Default (der bewusst leeren Nicht-Tier-Identitaet) kollidieren lassen.");

namespace detail {
/// Abhaengiger false-Wert: erst bei INSTANZIIERUNG der beiden Sperr-Ueberladungen ausgewertet, damit die
/// Sperre nur den fehlerhaften Aufruf trifft und nicht schon die Deklaration.
///
/// EINE Konstante fuer BEIDE Sperren (_glieder und _hex): sie geben dieselbe Zusage, und zwei getrennte
/// Praedikate koennten auseinanderlaufen, ohne dass es jemandem auffiele -- dieselbe Begruendung, aus der
/// GliedSterbenderString oben als EIN benanntes Konzept dasteht und nicht dreimal als roher requires.
template <class...>
inline constexpr bool kFingerprintRohZeilenGesperrt = false;
} // namespace detail

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
///   [7] Overlay-Source-Hash         (heute leer, OF-M3-2 = Fallback B)
///   [8] Mess-Gates-Glied            (R-3, Format 4: der PRAEPROZESSOR-Zustand der Mess-Gates DIESER
///                                    Uebersetzungseinheit -- TU-WAHRHEIT, nicht Host-Injektion. Glied [3]
///                                    nennt die Mess-Combo als Host-LITERAL; erst dieses Glied macht den
///                                    Gate-Zustand des Kompilats identitaets-wirksam)
///
/// Die drei Schwanz-Glieder haben Defaults, weil sie INJIZIERT werden: wer sie nicht kennt, reicht sie
/// nicht -- und rechnet dann byte-identisch zur Identitaet. Das ist dieselbe Zusage wie bei
/// SystemCellValues (W10) und macht die Naht ohne Flotten-weiten Umbau baubar.
///
/// -- NB-3/T2-D: DIE VOLL-WACHE BEIM GEBRAUCH -----------------------------------------------------------
///
/// Die drei injizierten Werte werden HIER ein zweites Mal geprueft -- nicht aus Misstrauen gegen den
/// Konstruktor, sondern weil die Traeger SICHTEN halten (ausfuehrliche Begruendung bei OverlayHash). Die
/// Konstruktor-Wache beweist etwas ueber den Wert ZUM ZEITPUNKT DER KONSTRUKTION; gehasht wird er aber
/// hier. Was dazwischen mit dem Puffer geschieht, weiss kein Traeger. Diese Funktion ist die einzige
/// Stelle, die (a) unmittelbar vor dem Preimage steht und (b) weiss, WELCHES Glied welches ist -- der
/// gemeinsame Emitter darunter kann das nicht: er bedient auch den MESSWERT-Genus, dessen Komponenten an
/// denselben Positionen etwas voellig anderes bedeuten (bestandslog_index derive_key_from_lines ist
/// bewusst generisch). Eine positionsbasierte Voll-Wache im Emitter wuerde dort Fremd-Glieder nach einer
/// Grammatik pruefen, die fuer sie nie galt.
///
/// Die Funktion ist deshalb NICHT mehr `noexcept`: ein verletzter Wert MUSS heraus (zur Laufzeit als
/// benannte Fehlerklasse, im konstanten Ausdruck als harter Compile-Fehler), statt ueber std::terminate zu
/// laufen oder -- schlimmer -- ein mehrdeutiges Preimage zu hashen.
///
/// DIGEST-NEUTRAL: fuer jeden Wert, der die Wache besteht (also fuer jeden legitimen), aendert sich kein
/// Byte. Nur Werte, die vorher STILL ein mehrdeutiges Preimage erzeugt haetten, werden jetzt abgelehnt.
///
/// -- R-3: WARUM DER DEFAULT DES MESS-GATES-GLIEDS DIE LEERE IDENTITAET IST UND NICHT kMessGatesTuGlied
///
/// Diese Funktion ist `constexpr` und damit implizit `inline` -- ihre Default-Argumente sind Teil einer
/// Entitaet mit EXTERNER Bindung. kMessGatesTuGlied ist TU-abhaengig (interne Bindung, per Definition
/// je Uebersetzungseinheit verschieden). Stuende sie hier als Default, haetten zwei TUs desselben
/// Programms verschiedene Definitionen derselben inline-Funktion: ein stiller ODR-Verstoss (IFNDR), und
/// Misch-Programme EXISTIEREN real (test_a8s4_release_pfad_neutralitaet uebersetzt eine Release-TU neben
/// Mess-TUs). Zusaetzlich haenge kCebFingerprint (ceb_version_stamp.hpp) dann am Gate-Zustand der
/// CEB-eigenen TU statt an ihrer Mess-WAHL. Der Wert wird deshalb NUR am Makro-Expansionsort explizit
/// gereicht -- dort, wo genau eine TU gemeint ist. Praezedenz der Fehlerklasse: die
/// M-1/D-4-Verdrahtungs-Wache in ceb_version_stamp.hpp.
///
/// -- S-6b: DIE DREI ZEILEN REISEN AB HIER TYPISIERT ----------------------------------------------------
///
/// organ/system/measurement sind keine nackten string_view mehr, sondern OrganZeile/SystemZeile/MessZeile
/// (Begruendung ausfuehrlich bei den Traegern oben). Die Funktion rechnet unveraendert: sie packt
/// dieselben WERTE an dieselben Positionen. Was sich aendert, ist allein, dass eine Vertauschung nicht
/// mehr uebersetzt.
[[nodiscard]] constexpr std::array<std::string_view, kAnatomyFingerprintGliedCount>
anatomy_fingerprint_glieder(OrganZeile organ, SystemZeile system, MessZeile measurement,
                            ToolchainGlied toolchain  = ToolchainGlied{kToolchainStampGlied},
                            BvsetGlied     bvset      = BvsetGlied{kBuildVariantSetSignatureGlied},
                            OverlayHash    overlay    = OverlayHash{kOverlaySourceHash},
                            MessGatesGlied mess_gates = MessGatesGlied{""}) {
    require_injizierter_glied_wert("toolchain", toolchain.wert());
    require_injizierter_glied_wert("bvset", bvset.wert());
    require_injizierter_glied_wert("overlay", overlay.wert());
    require_injizierter_glied_wert("mess-gates", mess_gates.wert());
    return {kAnatomyFingerprintFormat, organ.wert(), system.wert(),  measurement.wert(), kSubAxisValuesetSegment,
            toolchain.wert(),          bvset.wert(), overlay.wert(), mess_gates.wert()};
}

/// S-6b ROH-SPERRE (Zwilling der K-1-Sperre unten, dieselbe Bauart und derselbe Zweck): der Alt-Aufruf mit
/// DREI nackten Zeichenketten ist unbaubar und meldet sich mit BENANNTEM Text statt mit einem blossen
/// "no matching function". Ohne sie waere die lauteste Stelle des Umbaus die unverstaendlichste.
template <class... Rest>
[[nodiscard]] constexpr std::array<std::string_view, kAnatomyFingerprintGliedCount>
anatomy_fingerprint_glieder(std::string_view, std::string_view, std::string_view, Rest...) noexcept {
    static_assert(detail::kFingerprintRohZeilenGesperrt<Rest...>,
                  "S-6b: die drei Stempel-ZEILEN sind benannte Traeger-Typen, keine string_view -- "
                  "OrganZeile{...}, SystemZeile{...}, MessZeile{...}, in dieser Reihenfolge. Roh gereicht "
                  "koennten sie beliebig gegeneinander verschoben werden: es uebersetzte, der Digest waere "
                  "falsch, und sichtbar wuerde es erst als 'der Cache greift nie' oder als FALSCHER SKIP. "
                  "Genau das verhindert diese Sperre. Die Aufrufstelle auf die Traeger-Typen ziehen.");
    return {};
}

/// W10-C3: die Positions-Konstante ist BEWIESEN, nicht behauptet -- wer die Glied-Ordnung oben umbaut,
/// bricht hier compile-time, statt den Zellwert still ins Organ-Glied zu schreiben.
static_assert(anatomy_fingerprint_glieder(OrganZeile{"ORGAN"}, SystemZeile{"SYSTEM"},
                                          MessZeile{"MESS"})[kAnatomyFingerprintSystemGlied] == "SYSTEM",
              "kAnatomyFingerprintSystemGlied zeigt nicht mehr auf die System-Zeile der Glied-Folge.");
static_assert(kAnatomyFingerprintSystemGlied < kAnatomyFingerprintGliedCount);

/// O-2/C-2: dieselbe Beweis-Form fuer die drei neuen/verschobenen Positionen. Sie sind KEINE Kommentare:
/// bestandslog_factory vervollstaendigt Glied [2] anhand seiner Konstante, und die Konsumenten der
/// Toolchain-/bvset-Naht adressieren ihre Glieder ebenso. Eine Umsortierung ohne Nachzug bricht hier.
static_assert(anatomy_fingerprint_glieder(OrganZeile{"O"}, SystemZeile{"S"}, MessZeile{"M"}, ToolchainGlied{"TC"},
                                          BvsetGlied{"BV"},
                                          OverlayHash{"OV"})[kAnatomyFingerprintToolchainGlied] == "TC",
              "kAnatomyFingerprintToolchainGlied zeigt nicht mehr auf das Toolchain-Glied.");
static_assert(anatomy_fingerprint_glieder(OrganZeile{"O"}, SystemZeile{"S"}, MessZeile{"M"}, ToolchainGlied{"TC"},
                                          BvsetGlied{"BV"}, OverlayHash{"OV"})[kAnatomyFingerprintBvsetGlied] == "BV",
              "kAnatomyFingerprintBvsetGlied zeigt nicht mehr auf das bvset-Glied.");
static_assert(anatomy_fingerprint_glieder(OrganZeile{"O"}, SystemZeile{"S"}, MessZeile{"M"}, ToolchainGlied{"TC"},
                                          BvsetGlied{"BV"}, OverlayHash{"OV"})[kAnatomyFingerprintOverlayGlied] == "OV",
              "kAnatomyFingerprintOverlayGlied zeigt nicht mehr auf das Overlay-Glied.");
static_assert(anatomy_fingerprint_glieder(OrganZeile{"O"}, SystemZeile{"S"}, MessZeile{"M"}, ToolchainGlied{"TC"},
                                          BvsetGlied{"BV"}, OverlayHash{"OV"},
                                          MessGatesGlied{"MG"})[kAnatomyFingerprintMessGatesGlied] == "MG",
              "kAnatomyFingerprintMessGatesGlied zeigt nicht mehr auf das Mess-Gates-Glied.");
/// R-3: das Overlay-Glied war bis Format 3 das SCHWANZ-Glied (O-2/C-2: "ein noch leeres Glied gehoert
/// an den Schwanz, damit die Positionen der GEFUELLTEN Glieder stabil bleiben"). Ab Format 4 steht das
/// Mess-Gates-Glied dahinter -- und zwar OHNE die alte Begruendung zu verletzen: die Bestands-Nummern
/// [0]..[7] sind unveraendert, das neue Glied haengt sich an. Das Overlay-Glied bleibt damit das letzte
/// NOCH LEERE Glied; wird es scharfgeschaltet (L14/Phase 6), verschiebt sich weiterhin keine Position.
static_assert(kAnatomyFingerprintOverlayGlied == kAnatomyFingerprintGliedCount - 2,
              "Das Overlay-Glied ist das letzte NOCH LEERE Glied (L14/Phase 6) und steht vor dem "
              "Mess-Gates-Glied.");
static_assert(kAnatomyFingerprintMessGatesGlied == kAnatomyFingerprintGliedCount - 1,
              "Das Mess-Gates-Glied ist per R-3 (Format 4) das SCHWANZ-Glied.");
static_assert(kAnatomyFingerprintSystemGlied < kAnatomyFingerprintToolchainGlied &&
                  kAnatomyFingerprintToolchainGlied < kAnatomyFingerprintBvsetGlied &&
                  kAnatomyFingerprintBvsetGlied < kAnatomyFingerprintOverlayGlied &&
                  kAnatomyFingerprintOverlayGlied < kAnatomyFingerprintMessGatesGlied,
              "Die benannten Positionen muessen aufsteigend und paarweise verschieden sein.");
static_assert(kAnatomyFingerprintGliedCount == 9,
              "R-3 (Format 4): die Glied-Folge hat NEUN Glieder. Wer sie aendert, aendert das "
              "Preimage-Layout -- also die Identitaet jeder Tier-Binary -- und MUSS die Format-Kennung "
              "kAnatomyFingerprintFormat mitbumpen (F7).");
static_assert(kAnatomyFingerprintFormat == "fingerprint_format=4",
              "R-3: die Format-Kennung und die Glied-Zahl sind auseinandergelaufen.");

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
anatomy_fingerprint_hex(OrganZeile organ, SystemZeile system, MessZeile measurement,
                        ToolchainGlied toolchain  = ToolchainGlied{kToolchainStampGlied},
                        BvsetGlied     bvset      = BvsetGlied{kBuildVariantSetSignatureGlied},
                        OverlayHash    overlay    = OverlayHash{kOverlaySourceHash},
                        MessGatesGlied mess_gates = MessGatesGlied{""}) {
    auto const glieder = anatomy_fingerprint_glieder(organ, system, measurement, toolchain, bvset, overlay, mess_gates);
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

/// K-1-SPERRE, ab S-6b auf DREI rohe Zeichenketten VERENGT: jeder Alt-Aufruf, der mit drei nackten
/// string_view beginnt, ist unbaubar und meldet sich mit BENANNTEM Text. Sie ist bewusst KEIN `= delete`
/// -- die Form `= delete("Grund")` ist C++26 und waere unter -std=c++23 eine Erweiterung; ein Template mit
/// static_assert liefert denselben Effekt standardkonform und mit derselben Lesbarkeit im Fehlertext.
///
/// WARUM DER PRAEFIX AUF DREI SCHRUMPFT (vorher: vier): bis S-6b griff sie erst am 4. Argument, weil die
/// ersten drei legitim string_view WAREN. Seit die Zeilen benannte Traeger sind, ist schon der 3-arg-Aufruf
/// mit Rohtext falsch -- und ein Praefix von DREI deckt den alten 4-arg-Fall (merge-Slot) mit ab. Zwei
/// getrennte Ueberladungen (3 und 4) waeren fuer den 4-arg-Aufruf mehrdeutig gewesen und haetten statt der
/// benannten Meldung einen Ambiguitaets-Fehler geliefert; EINE Ueberladung mit variadischem Schwanz deckt
/// beide Faelle und mehr.
///
/// Der gueltige typisierte Aufruf trifft sie nicht: OrganZeile/SystemZeile/MessZeile konvertieren nicht
/// nach string_view (durch die CT-Negativ-Probe oben bewiesen), also ist diese Ueberladung dort nicht
/// einmal viabel.
template <class... Rest>
[[nodiscard]] consteval std::array<char, 129> anatomy_fingerprint_hex(std::string_view, std::string_view,
                                                                      std::string_view, Rest...) noexcept {
    static_assert(detail::kFingerprintRohZeilenGesperrt<Rest...>,
                  "S-6b: die drei Stempel-ZEILEN sind benannte Traeger-Typen, keine string_view -- "
                  "OrganZeile{...}, SystemZeile{...}, MessZeile{...}, in dieser Reihenfolge. Roh gereicht "
                  "koennten sie beliebig gegeneinander verschoben werden: es uebersetzte, der Digest waere "
                  "falsch, und sichtbar wuerde es erst als 'der Cache greift nie' oder als FALSCHER SKIP. "
                  "A13-M3/K-1 (weiterhin gueltig): die merge-ZEILE existiert nicht mehr (Owner-E2 "
                  "02.08.2026) -- auch das 4. Argument ist ein BENANNTER Glied-TYP. Die Schwanz-Slots "
                  "heissen ToolchainGlied{...}, BvsetGlied{...}, OverlayHash{...}, MessGatesGlied{...}, in "
                  "dieser Reihenfolge. Die Aufrufstelle auf die Traeger-Typen ziehen.");
    return {};
}

} // namespace comdare::cache_engine::abi
