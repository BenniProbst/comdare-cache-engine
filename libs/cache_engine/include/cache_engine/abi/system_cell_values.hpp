#pragma once
// abi/system_cell_values.hpp -- W10-C1 (Bauplan-Dossier 20260803, Sektion 2): die SINGLE-SOURCE der
// System-ZELLWERTE in der System-Stempel-Zeile.
//
// WOZU: bis W10 traegt die System-Zeile NUR die statischen Achsen-CODE-Versionen
// ("target_isa=code@1.0.0.c;..."), nicht die je Bau gewaehlte ZELLE. Zwei Baue derselben Permutation
// auf verschiedenen OS-Familien oder ISAs bekommen damit denselben SHA512 -- genau die Kollision, die
// Owner-E3 (Wiederverwendbarkeit/Zuordbarkeit) ausschliesst. W10 ergaenzt den Zellwert als
// hierarchische Namens-Erweiterung des Algorithmus-Markers ("code" -> "code.<token>"). Der Emitter
// bleibt dabei system-blind (W4-B-Invariante): die Zellwerte reisen ueber eine COMPILE-DEFINE-NAHT in
// die Makro-Expansion, nie in den emittierten Quelltext.
//
// PARSER-LEGAL OHNE GRAMMATIK-AENDERUNG (Owner-Q2-Namens-Toleranz, Ledger-Nachtrag A13-M1/A13-M2):
// "ein '.' VOR dem '@' ist transparenter Namens-Bestandteil (Achse UND Algorithmus)". Der Zellwert ist
// deshalb ein reiner NAMENS-Anteil; der Versionsteil (X.Y.Z + Hardware-Flag + optionales 'e') bleibt
// byte-unberuehrt, und die Achsen-NAMEN bleiben kanonisch (kSystemAxisOrder-Konsumenten, A14-Guards).
//
// ZIEL-FORM (E-1-Default (a), Beispiel prod1):
//   "target_isa=code.x86_64@1.0.0.c;operating_system=code.linux@1.0.0.c;external_utils=code@1.0.0.c;"
//   "[simd=code.avx512@1.0.0.c]"
// Der Meta-Meta-Anhang bleibt am Realm-Zeilen-ENDE (Owner-E2 / Stempel-KERN 02.08.): dieser
// Vervollstaendiger fuegt IN Entries ein und haengt NIE ein Segment an.
//
// A-15-GRENZE (bindend, Ledger-Nachtrag A14/OS-U2): NUR Haupt-Achsen-Zellwerte auf FAMILIEN-Ebene.
// RT-Unter-Achsen (os_version/kernel/build, numa_node/page, scheduling) stehen NIE im Binary-Stempel;
// die Instanz-Zuordnung einer Messung laeuft ueber Mess-Spalten/Dateinamen (OS-U4). Diese Grenze ist
// hier MECHANISCH: die zulaessigen Schluessel sind ABSCHLIESSEND aufgezaehlt, und der Fallen-Katalog
// unten gibt den RT-Schluesseln eine EIGENE, benannte Diagnose (Muster der 9-Fallen-Wache
// measurement/operating_system_sub_axes.hpp).
//
// IDENTITAET IST DER GRUNDPFAD: ein LEERES Werte-Set laesst die Zeile BYTE-IDENTISCH. Ohne gesetztes
// Define ist W10 damit golden-neutral -- der Fingerprint, das .fingerprint-Sidecar und der Lager-Key
// aller heutigen Baue bleiben unbewegt (der eingefrorene Testvektor kFrozenFingerprintV1 ist der Zeuge).
//
// MAPPING-REINHEIT (Paragraf 66-N3): CT->CT (Define -> consteval-Makro-Expansion) und RT->RT
// (Zell-Variable der CEB -> Laufzeit-Zwilling). Die EINZIGE Bruecke ist die Pre-Build-Define-Naht --
// exakt die sanktionierte COMDARE_GN_ALGO_SIG-/COMDARE_OVERLAY_SOURCE_HASH-Klasse. Kein Runtime-Switch,
// kein std::variant, keine zweite Ableitung.
//
// EINE ORDNUNG, ZWEI SENKEN: der Renderer detail::render_system_cell_values existiert GENAU EINMAL und
// wird von allen drei Wegen gefuettert (Laengen-Zaehlung, consteval-Array-Fuellung, Laufzeit-String).
// Die frueher an solchen Stellen entstandene Drift (Risiko R6, vier Kopien der Preimage-Ordnung) kann
// hier nicht entstehen.
//
// header-only, C++23, std-only. BEWUSST LEICHT: dieser Header wird von anatomy_module_abi_v1.hpp und
// damit von JEDER emittierten Modul-Quelle inkludiert. Er zieht deshalb nur, was dort ohnehin schon
// liegt (anatomy_stamp_entries.hpp) plus die Achsen-Ordnung -- KEINE measurement-Achsen-Typen (die
// simd-Namens-Drift-Wache gegen ExternalUtilsHub::meta_metas sitzt aus genau diesem Grund in der
// TU tests/unit/test_w10_system_cell_values.cpp, wo Boost/Achsen-Header erlaubt sind).

#include "anatomy_stamp_entries.hpp" // die EINE Zeilen-Grammatik (count_stamp_entries / stamp_line_is_wellformed)
#include "stempel_basis.hpp"         // S-1: ist_stempel_baustein (Baustein-Anbindung)
#include "system_axis_order.hpp"     // kSystemAxisOrder: die EINE Ordnung der System-Haupt-Achsen

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace comdare::cache_engine::abi {

// ---------------------------------------------------------------------------------------------
// (a) TOKEN-GRAMMATIK + na-SENTINEL
// ---------------------------------------------------------------------------------------------

/// Der Zeichenvorrat eines Zellwert-Tokens: `[a-z0-9_]+`. Er ist eine ECHTE TEILMENGE des
/// Stempel-Zeichenvorrats (alnum + "=@;.+_[]") -- ein Token kann damit weder einen Trenner noch eine
/// Klammer noch ein '@' oder '.' einschleusen und die Zeilen-Grammatik nicht verlassen.
/// KLEINSCHREIBUNG ist Pflicht: Grossbuchstaben waeren eine zweite Schreibweise derselben Zelle und
/// damit zwei verschiedene SHA512 fuer denselben Bau.
[[nodiscard]] constexpr bool system_cell_token_char_is_allowed(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

/// Ein wohlgeformtes Token: NIE leer, ausschliesslich aus dem Zeichenvorrat oben.
[[nodiscard]] constexpr bool system_cell_token_is_wellformed(std::string_view token) noexcept {
    if (token.empty()) return false;
    for (char const c : token)
        if (!system_cell_token_char_is_allowed(c)) return false;
    return true;
}

/// n/a-STATT-NULL (bindend, Dossier Sektion 1): ein nicht bestimmbarer Zellwert reist als SENTINEL-Token
/// `na` -- NIE als leerer String und NIE als 0. Das Anti-Muster ist real (Prod1-RAM
/// ram_frequency_mhz==0/Unbekannt, target_isa_complex_axis.hpp): eine 0 ist ein WERT und laesst sich
/// nicht mehr von "unbekannt" unterscheiden, ein leeres Feld verschwindet spurlos aus der Zeile.
/// Geschrieben `na` und nicht "n/a", weil '/' nicht im Stempel-Zeichenvorrat liegt.
/// FAIL-CLOSED-ZUSAGE (scharfgeschaltet in W10-C4): ein `na` in einem lager-faehigen Bau bedeutet KEIN
/// Lager-Rueckschrieb, sondern eine klassifizierte Fehler-Zeile. Das deckt zugleich die riscv64-Auflage
/// mechanisch (ISA ohne Achsen-Glied => na => kein Rueckschrieb).
inline constexpr std::string_view kSystemCellValueNa = "na";

static_assert(system_cell_token_is_wellformed(kSystemCellValueNa),
              "der na-Sentinel muss selbst der Token-Grammatik genuegen (sonst braeche die Zeile, "
              "die ihn traegt) -- genau deshalb 'na' und nicht 'n/a'.");
// ENFORCE-WACHE (Dossier Sektion 5, Punkt 1): ein Zellwert-Token ist NIE eine Version. '.' und '@' sind
// keine Token-Zeichen, Grossbuchstaben ebenso wenig -- W10 fuehrt damit beweisbar KEIN neues
// Versions-Literal ein und braucht keinen Naht-Listen-Eintrag.
static_assert(!system_cell_token_is_wellformed("1.0.0c"), "ein Token kann keine Version sein ('.' fehlt im Vorrat)");
static_assert(!system_cell_token_is_wellformed("code@1.0.0.c"), "ein Token kann kein '@' tragen");
static_assert(!system_cell_token_is_wellformed("X86_64"), "Grossbuchstaben sind eine zweite Schreibweise -- verboten");
static_assert(!system_cell_token_is_wellformed(""), "ein leeres Token ist kein Token (n/a reist als 'na')");

[[nodiscard]] constexpr bool system_cell_value_is_na(std::string_view token) noexcept {
    return token == kSystemCellValueNa;
}

// ---------------------------------------------------------------------------------------------
// (b) DIE KANONISCHE WERT-ORDNUNG + DRIFT-WACHEN
// ---------------------------------------------------------------------------------------------

/// Die ABSCHLIESSENDE Liste der Achsen, die einen Zellwert tragen duerfen -- in kanonischer Ordnung:
/// erst die Haupt-Achsen aus kSystemAxisOrder, die eine eigene Zelle haben, dann das Meta-Meta-Glied.
///
/// external_utils fehlt ABSICHTLICH (E-2-Default (a)): der Hub hat keine eigene Zelle, sein "Wert" IST
/// die Glieder-Menge. Ein Kunstwert dort waere eine zweite, redundante Identitaets-Quelle.
inline constexpr std::size_t kSystemCellValueKeyCount = 3;

inline constexpr std::array<std::string_view, kSystemCellValueKeyCount> kSystemCellValueKeys{{
    "target_isa",
    "operating_system",
    "simd",
}};

/// Der Achsen-Name des HUBS, der bewusst wertfrei bleibt. Steht hier benannt, damit die Wache unten
/// nicht auf ein nacktes Literal zeigt.
inline constexpr std::string_view kSystemCellValueHubAxis = "external_utils";

[[nodiscard]] constexpr bool is_system_cell_value_key(std::string_view achse) noexcept {
    for (auto const& k : kSystemCellValueKeys)
        if (k == achse) return true;
    return false;
}

namespace detail {

/// DRIFT-WACHE gegen abi/system_axis_order.hpp: die beiden Haupt-Achsen-Schluessel stehen in DERSELBEN
/// Reihenfolge und unter DENSELBEN Namen wie in der kanonischen Ordnung. Wer dort umsortiert, umbenennt
/// oder eine vierte System-Haupt-Achse eintraegt, bricht hier compile-time und muss entscheiden, ob die
/// neue Achse eine Zelle hat.
[[nodiscard]] consteval bool system_cell_value_keys_match_axis_order() {
    if (kSystemAxisOrderCount != 3) return false;
    if (kSystemCellValueKeys[0] != kSystemAxisOrder[0]) return false;
    if (kSystemCellValueKeys[1] != kSystemAxisOrder[1]) return false;
    if (kSystemAxisOrder[2] != kSystemCellValueHubAxis) return false;
    return !is_system_cell_value_key(kSystemCellValueHubAxis);
}

[[nodiscard]] consteval bool system_cell_value_keys_are_distinct() {
    for (std::size_t i = 0; i < kSystemCellValueKeyCount; ++i) {
        if (!system_cell_token_is_wellformed(kSystemCellValueKeys[i])) return false;
        for (std::size_t j = i + 1; j < kSystemCellValueKeyCount; ++j)
            if (kSystemCellValueKeys[i] == kSystemCellValueKeys[j]) return false;
    }
    return true;
}

/// (f) A-15-VERBOTS-KATALOG: Schluessel, die hier NIE einen Zellwert tragen duerfen. Jeder Eintrag steht
/// fuer eine REALE Verwechslung, nicht fuer eine hypothetische (Muster der 9-Fallen-Wache in
/// measurement/operating_system_sub_axes.hpp):
///   os_version/kernel/build -- die FINAL DREI RT-Unter-Achsen von operating_system (A14/OS-U1). Sie
///       werden je Lauf ERHOBEN (OS-U3) und gehoeren in Mess-Spalten/Dateinamen, nie in den Stempel;
///   os_family -- die Familie heisst im Stempel operating_system; ein zweiter Name waere eine zweite
///       Wahrheit derselben Zelle;
///   numa_node/page/scheduling -- die RT-Unter-Achsen am target_isa-Komplex (OD-10-RT bzw. A3-Umzug).
///       Sie sind Instanz-Eigenschaften der Maschine, nicht Familien-Eigenschaften des Baus;
///   core_class -- die dritte RT-Unter-Achse am target_isa-Komplex (OD-11-RT, Owner-KERN 06.08.2026):
///       die AUSFUEHRUNGS-Lokalitaet (welche Kern-Klassen fuehrt DIESE Maschine). Sie steht hier aus
///       demselben Grund wie numa_node und aus einem zweiten, der den Owner-KERN traegt: stuende sie im
///       Stempel, waere die Kern-Klasse Teil der Binary-Identitaet -- und die CEB koennte NICHT
///       "DIESELBE Tier-Binary einmal auf einen E-Core und einmal auf einen P-Core gepinnt" starten,
///       weil die beiden Laeufe dann verschiedene Binaries braeuchten. Genau das schliesst der KERN aus
///       ("reine Wiederverwendung, kein zweiter Bau").
inline constexpr std::array<std::string_view, 8> kSystemCellValueForbiddenKeys = {
    "os_version", "kernel", "build", "os_family", "numa_node", "page", "scheduling", "core_class"};

[[nodiscard]] consteval bool system_cell_value_keys_avoid_traps() {
    for (auto const& k : kSystemCellValueKeys)
        for (auto const& verboten : kSystemCellValueForbiddenKeys)
            if (k == verboten) return false;
    return true;
}

[[nodiscard]] constexpr bool is_forbidden_system_cell_value_key(std::string_view achse) noexcept {
    for (auto const& verboten : kSystemCellValueForbiddenKeys)
        if (achse == verboten) return true;
    return false;
}

} // namespace detail

static_assert(detail::system_cell_value_keys_match_axis_order(),
              "W10-C1: kSystemCellValueKeys ist aus der kanonischen System-Achsen-Ordnung gelaufen "
              "(abi/system_axis_order.hpp). Die ersten beiden Schluessel MUESSEN target_isa und "
              "operating_system in dieser Reihenfolge sein, external_utils MUSS der wertfreie Hub "
              "bleiben (E-2-Default (a)). Wer eine vierte System-Haupt-Achse einfuehrt, entscheidet "
              "HIER, ob sie eine Zelle traegt -- still weglassen geht nicht.");
static_assert(detail::system_cell_value_keys_are_distinct(),
              "W10-C1: die Zellwert-Schluessel muessen paarweise verschieden und selbst token-grammatik-"
              "konform sein (zwei gleiche Schluessel waeren zwei Werte fuer dieselbe Achse).");
static_assert(detail::system_cell_value_keys_avoid_traps(),
              "W10-C1/A-15: ein Zellwert-Schluessel steht im RT-Unter-Achsen-Verbots-Katalog. "
              "RT-Unter-Achsen stehen NIE im Binary-Stempel -- die Instanz-Zuordnung einer Messung "
              "laeuft ueber Mess-Spalten/Dateinamen (OS-U4).");

// ---------------------------------------------------------------------------------------------
// (d) DIE DEFINE-NAHT + der benannte Traeger-Typ
// ---------------------------------------------------------------------------------------------

/// Die Compile-Define-Naht -- Muster COMDARE_OVERLAY_SOURCE_HASH (anatomy_fingerprint.hpp) bzw.
/// COMDARE_GN_ALGO_SIG. WERTFORM (achsen-genannt, Vollstaendigkeits-Wache):
///   "target_isa=<token>;operating_system=<token>;simd=<token>"
/// Gesetzt wird sie AUSSCHLIESSLICH von der CEB-Bau-Naht je System-Zelle (perm_compile, W10-C4) -- der
/// Emitter bleibt system-blind, der emittierte Quelltext byte-identisch.
///
/// DEFAULT LEER == IDENTITAET: ohne Define aendert der Vervollstaendiger kein einziges Byte. Das ist der
/// golden-neutrale Grundpfad, an dem W10a (C1-C3) haengt.
#ifndef COMDARE_SYSTEM_CELL_VALUES
#define COMDARE_SYSTEM_CELL_VALUES ""
#endif
inline constexpr std::string_view kSystemCellValues = COMDARE_SYSTEM_CELL_VALUES;

/// K-1-MUSTER (Praezedenz OverlayHash, anatomy_fingerprint.hpp): die Zellwerte reisen als BENANNTER TYP,
/// nicht als weiterer nackter string_view.
///
/// DIE FALLE, DIE ER SCHLIESST: die Rechen-Stellen des Fingerprints tragen bereits drei string_view
/// (organ/system/measurement). Ein VIERTER string_view koennte an jeder dieser Stellen still auf den
/// falschen Slot rutschen -- es kompiliert, die Semantik ist verschoben, niemand merkt es. Ein
/// string_view konvertiert NICHT implizit nach SystemCellValues (explicit) -> jeder Fehl-Aufruf bricht
/// compile-hart. Der DEFAULT-konstruierte Wert ist leer und damit die Identitaet.
struct SystemCellValues {
    std::string_view value{};

    constexpr SystemCellValues() noexcept = default;
    constexpr explicit SystemCellValues(std::string_view v) noexcept : value{v} {}
};

/// Der Wert der Define-Naht als Traeger-Typ -- die EINE Stelle, an der das Makro ihn abholt.
inline constexpr SystemCellValues kSystemCellValuesFromDefine{kSystemCellValues};

// ---------------------------------------------------------------------------------------------
// DIAGNOSE der Werte-Zeichenkette (KEIN throw -- CT und RT teilen sie sich)
// ---------------------------------------------------------------------------------------------

/// Die benannte Diagnose einer Zellwert-Zeichenkette. Bewusst ein PRAEDIKAT statt eines Wurfs: derselbe
/// Weg taugt damit fuer den consteval-Pfad (static_assert mit benanntem Text) UND fuer den Laufzeit-Pfad
/// (fail-closed PRUEFEN, bevor gerendert wird). Die Wuerfe der Renderer unten sind die harte Kante
/// DAHINTER, nicht die einzige Wahrheit.
enum class SystemCellValuesDiagnose {
    ok,                        ///< vollstaendig, alle Schluessel bekannt, alle Tokens wohlgeformt
    leer,                      ///< kein Define / kein Werte-Set -> IDENTITAET (kein Fehler)
    segment_ohne_gleich,       ///< ein Segment ohne '=' (auch das leere Segment ";;")
    unbekannter_schluessel,    ///< ein Schluessel ausserhalb der ABSCHLIESSENDEN Liste
    verbotener_rt_schluessel,  ///< A-15: eine RT-Unter-Achse als Zellwert-Schluessel
    leerer_token,              ///< "achse=" -- n/a reist als 'na', nie als Leere
    ungueltiges_token_zeichen, ///< Grossbuchstabe, '.', '@', '-' ... ausserhalb [a-z0-9_]
    doppelter_schluessel,      ///< derselbe Schluessel zweimal -> zwei Werte fuer eine Achse
    unvollstaendig,            ///< nicht-leeres Set, das nicht ALLE drei Achsen nennt
};

/// diagnose_system_cell_values(werte) -- die Vollstaendigkeits-/Grammatik-Pruefung der Define-Wertform.
/// Reine Funktion, kein Wurf, in CT und RT identisch auswertbar.
[[nodiscard]] constexpr SystemCellValuesDiagnose diagnose_system_cell_values(std::string_view werte) noexcept {
    if (werte.empty()) return SystemCellValuesDiagnose::leer;
    std::array<bool, kSystemCellValueKeyCount> gesehen{};
    std::size_t                                start = 0;
    for (std::size_t pos = 0; pos <= werte.size(); ++pos) {
        if (pos != werte.size() && werte[pos] != ';') continue;
        std::string_view const segment = werte.substr(start, pos - start);
        start                          = pos + 1;
        std::size_t const eq           = segment.find('=');
        if (eq == std::string_view::npos) return SystemCellValuesDiagnose::segment_ohne_gleich;
        std::string_view const achse = segment.substr(0, eq);
        std::string_view const token = segment.substr(eq + 1);
        // A-15 ZUERST: eine RT-Unter-Achse bekommt ihre EIGENE Diagnose, damit die Fehlermeldung die
        // Grenz-Verletzung benennt statt bloss "unbekannt" zu sagen.
        if (detail::is_forbidden_system_cell_value_key(achse))
            return SystemCellValuesDiagnose::verbotener_rt_schluessel;
        if (!is_system_cell_value_key(achse)) return SystemCellValuesDiagnose::unbekannter_schluessel;
        if (token.empty()) return SystemCellValuesDiagnose::leerer_token;
        if (!system_cell_token_is_wellformed(token)) return SystemCellValuesDiagnose::ungueltiges_token_zeichen;
        for (std::size_t i = 0; i < kSystemCellValueKeyCount; ++i)
            if (kSystemCellValueKeys[i] == achse) {
                if (gesehen[i]) return SystemCellValuesDiagnose::doppelter_schluessel;
                gesehen[i] = true;
            }
    }
    for (bool const g : gesehen)
        if (!g) return SystemCellValuesDiagnose::unvollstaendig;
    return SystemCellValuesDiagnose::ok;
}

/// true, wenn das Werte-Set gerendert werden DARF (vollstaendig oder leer). Das ist die Bedingung, die
/// ein Laufzeit-Aufrufer fail-closed prueft, BEVOR er den Vervollstaendiger ruft.
[[nodiscard]] constexpr bool system_cell_values_are_usable(std::string_view werte) noexcept {
    auto const d = diagnose_system_cell_values(werte);
    return d == SystemCellValuesDiagnose::ok || d == SystemCellValuesDiagnose::leer;
}

/// true, wenn irgendein Zellwert der Sentinel `na` ist -- die FAIL-CLOSED-Bedingung des Lager-Rueckschriebs
/// (scharfgeschaltet in W10-C4). Ein `na` ist grammatisch voellig in Ordnung; es ist eine AUSSAGE
/// ("nicht bestimmbar"), kein Formfehler -- deshalb ein eigenes Praedikat neben der Diagnose.
[[nodiscard]] constexpr bool system_cell_values_contain_na(std::string_view werte) noexcept {
    std::size_t start = 0;
    for (std::size_t pos = 0; pos <= werte.size(); ++pos) {
        if (pos != werte.size() && werte[pos] != ';') continue;
        std::string_view const segment = werte.substr(start, pos - start);
        start                          = pos + 1;
        std::size_t const eq           = segment.find('=');
        if (eq != std::string_view::npos && system_cell_value_is_na(segment.substr(eq + 1))) return true;
    }
    return false;
}

/// Der Zellwert einer Achse aus der Werte-Zeichenkette; LEER, wenn die Achse nicht genannt ist.
[[nodiscard]] constexpr std::string_view system_cell_value_for(std::string_view werte,
                                                               std::string_view achse) noexcept {
    std::size_t start = 0;
    for (std::size_t pos = 0; pos <= werte.size(); ++pos) {
        if (pos != werte.size() && werte[pos] != ';') continue;
        std::string_view const segment = werte.substr(start, pos - start);
        start                          = pos + 1;
        std::size_t const eq           = segment.find('=');
        if (eq != std::string_view::npos && segment.substr(0, eq) == achse) return segment.substr(eq + 1);
    }
    return {};
}

// -- DIE DEFINE-NAHT SELBST STEHT UNTER DEN WACHEN (compile-hart, je Fehlform ein benannter Text) -----
static_assert(diagnose_system_cell_values(kSystemCellValues) != SystemCellValuesDiagnose::verbotener_rt_schluessel,
              "W10/A-15: COMDARE_SYSTEM_CELL_VALUES nennt eine RT-UNTER-ACHSE (os_version/kernel/build/"
              "os_family/numa_node/page/scheduling/core_class). RT-Unter-Achsen stehen NIE im Binary-Stempel -- sie "
              "sind Instanz-, nicht Familien-Eigenschaften; ihre Zuordnung laeuft ueber Mess-Spalten/"
              "Dateinamen (OS-U4).");
static_assert(diagnose_system_cell_values(kSystemCellValues) != SystemCellValuesDiagnose::unbekannter_schluessel,
              "W10-C1: COMDARE_SYSTEM_CELL_VALUES nennt einen Schluessel ausserhalb der ABSCHLIESSENDEN "
              "Liste kSystemCellValueKeys (target_isa/operating_system/simd). external_utils ist der "
              "wertfreie Hub (E-2-Default (a)).");
static_assert(diagnose_system_cell_values(kSystemCellValues) != SystemCellValuesDiagnose::leerer_token,
              "W10-C1: leerer Zellwert. Ein nicht bestimmbarer Wert reist als Sentinel 'na', NIE als "
              "leerer String und NIE als 0 (n/a-statt-NULL, bindend).");
static_assert(diagnose_system_cell_values(kSystemCellValues) != SystemCellValuesDiagnose::ungueltiges_token_zeichen,
              "W10-C1: ein Zellwert-Token verlaesst den Zeichenvorrat [a-z0-9_] (Grossbuchstabe, '.', "
              "'@', '-' ...). Der Zellwert ist ein NAMENS-Anteil vor dem '@'; er darf die Zeilen-"
              "Grammatik nicht verlassen und nie wie eine Version aussehen.");
static_assert(diagnose_system_cell_values(kSystemCellValues) != SystemCellValuesDiagnose::doppelter_schluessel,
              "W10-C1: derselbe Zellwert-Schluessel zweimal -- zwei Werte fuer eine Achse ergaeben zwei "
              "Wahrheiten in einer Zeile.");
static_assert(diagnose_system_cell_values(kSystemCellValues) != SystemCellValuesDiagnose::unvollstaendig,
              "W10-C1: COMDARE_SYSTEM_CELL_VALUES ist gesetzt, nennt aber nicht ALLE drei Achsen "
              "(target_isa/operating_system/simd). Eine Teil-Belegung liesse eine Stempel-BLINDSTELLE "
              "zurueck, in der eine Zell-Drift unsichtbar bliebe.");
static_assert(system_cell_values_are_usable(kSystemCellValues),
              "W10-C1: COMDARE_SYSTEM_CELL_VALUES ist nicht wohlgeformt. Die Wertform ist "
              "\"target_isa=<token>;operating_system=<token>;simd=<token>\" mit Tokens aus [a-z0-9_].");

// ---------------------------------------------------------------------------------------------
// (c)/(e) DER VERVOLLSTAENDIGER -- EINE Ordnung, drei Senken
// ---------------------------------------------------------------------------------------------

/// Die Traegerform der consteval-vervollstaendigten Zeile: ein nullterminiertes char-Array, damit die
/// {ptr,len}-Sichten des Stempel-PODs und der consteval-Parser (parse_stamp_entries nimmt eine
/// char-Array-Referenz) es unveraendert konsumieren koennen. N == Laenge + 1.
template <std::size_t N>
struct CompletedSystemStampLine {
    char chars[N]{};

    [[nodiscard]] constexpr std::string_view   view() const noexcept { return std::string_view{chars, N - 1}; }
    [[nodiscard]] static constexpr std::size_t size() noexcept { return N - 1; }
};

// -- S-1 (P2): Baustein-Anbindung UNTER der Definition (Trait, kein Basisklassen-Einbau -- der Traeger
//    wird u.a. aggregat-/array-artig befuellt und ist zugleich das Muster des S-1-Kompositums).
template <std::size_t N>
struct ist_stempel_baustein<CompletedSystemStampLine<N>>
    : StempelBausteinTag<StempelBausteinRolle::VervollstaendigteZeile> {};
static_assert(StempelBaustein<CompletedSystemStampLine<1>>);

namespace detail {

/// DER RENDERER -- die EINE Stelle, an der ein Zellwert in eine Stempel-Zeile eingefuegt wird.
///
/// Er laeuft die Zeile zeichenweise ab und trennt an DENSELBEN Zeichen wie der Grammatik-Scanner in
/// anatomy_stamp_entries.hpp (';' Geschwister, '[' eine Ebene tiefer, ']' eine Ebene zurueck). Die
/// Trenner werden UNVERAENDERT durchgereicht -- deshalb bleibt der Meta-Meta-Klammer-Anhang exakt dort,
/// wo er stand (am Realm-Zeilen-ENDE, Owner-E2 / Stempel-KERN), und es entsteht weder ein zusaetzliches
/// Segment noch eine merge-Zeile.
///
/// INNERHALB eines Segments wird der Zellwert hinter den ALGORITHMUS-Namen gehaengt, also unmittelbar
/// VOR das erste '@' nach dem '=' ("code" -> "code.<token>"). Das ist der Owner-Q2-Namensraum; der
/// Versionsteil hinter dem '@' bleibt byte-unberuehrt.
///
/// Ein Segment ohne '=' oder ohne '@' wird VERBATIM durchgereicht -- die harte Grammatik-Wache dafuer
/// ist und bleibt der consteval-Parser (parse_one_stamp_segment); dieser Renderer baut keine zweite.
///
/// RUECKGABE: die Zahl der tatsaechlich ANGEWANDTEN Zellwerte. Nur so kann der Aufrufer beweisen, dass
/// ein gesetztes Werte-Set die Zeile auch wirklich erreicht hat, statt still ins Leere zu laufen.
template <class Emit>
constexpr std::size_t render_system_cell_values(std::string_view zeile, std::string_view werte, Emit&& emit) {
    std::size_t angewandt = 0;
    std::size_t start     = 0;
    for (std::size_t pos = 0; pos <= zeile.size(); ++pos) {
        bool const grenze = pos == zeile.size() || zeile[pos] == ';' || zeile[pos] == '[' || zeile[pos] == ']';
        if (!grenze) continue;
        if (pos > start) {
            std::size_t eq = start;
            while (eq < pos && zeile[eq] != '=') ++eq;
            std::size_t at = eq < pos ? eq + 1 : pos;
            while (at < pos && zeile[at] != '@') ++at;
            std::string_view const token = (eq < pos && at < pos)
                                               ? system_cell_value_for(werte, zeile.substr(start, eq - start))
                                               : std::string_view{};
            for (std::size_t i = start; i < at; ++i) emit(zeile[i]);
            if (!token.empty()) {
                emit('.');
                for (char const c : token) emit(c);
                ++angewandt;
            }
            for (std::size_t i = at; i < pos; ++i) emit(zeile[i]);
        }
        if (pos < zeile.size()) emit(zeile[pos]);
        start = pos + 1;
    }
    return angewandt;
}

/// Die gemeinsame Vor-Pruefung beider Vervollstaendiger-Formen. Sie wirft mit BENANNTEM Text; im
/// consteval-Pfad (Makro-Expansion) ist das ein Compile-Fehler mit lesbarer Meldung -- exakt die
/// GA-06-Praezedenz aus anatomy_stamp_entries.hpp. Ein Laufzeit-Aufrufer prueft vorher
/// system_cell_values_are_usable und kommt hier nie an.
constexpr void system_cell_values_gate(std::string_view werte) {
    switch (diagnose_system_cell_values(werte)) {
        case SystemCellValuesDiagnose::ok:
        case SystemCellValuesDiagnose::leer: return;
        case SystemCellValuesDiagnose::segment_ohne_gleich:
            throw "W10-C1 Zellwerte: ein Segment ohne '=' -- die Wertform ist "
                  "'target_isa=<token>;operating_system=<token>;simd=<token>'";
        case SystemCellValuesDiagnose::unbekannter_schluessel:
            throw "W10-C1 Zellwerte: unbekannter Schluessel -- zulaessig sind AUSSCHLIESSLICH target_isa, "
                  "operating_system und simd (external_utils ist der wertfreie Hub)";
        case SystemCellValuesDiagnose::verbotener_rt_schluessel:
            throw "W10/A-15 Zellwerte: eine RT-UNTER-ACHSE steht nie im Binary-Stempel (os_version/kernel/"
                  "build/os_family/numa_node/page/scheduling/core_class) -- Instanz-Zuordnung laeuft ueber "
                  "Mess-Spalten/Dateinamen (OS-U4)";
        case SystemCellValuesDiagnose::leerer_token:
            throw "W10-C1 Zellwerte: leerer Zellwert -- ein nicht bestimmbarer Wert reist als Sentinel 'na', "
                  "nie als leerer String und nie als 0";
        case SystemCellValuesDiagnose::ungueltiges_token_zeichen:
            throw "W10-C1 Zellwerte: ein Token verlaesst den Zeichenvorrat [a-z0-9_] -- der Zellwert ist ein "
                  "Namens-Anteil vor dem '@' und darf die Zeilen-Grammatik nicht verlassen";
        case SystemCellValuesDiagnose::doppelter_schluessel:
            throw "W10-C1 Zellwerte: derselbe Schluessel zweimal -- zwei Werte fuer eine Achse";
        case SystemCellValuesDiagnose::unvollstaendig:
            throw "W10-C1 Zellwerte: ein gesetztes Werte-Set MUSS alle drei Achsen nennen (target_isa/"
                  "operating_system/simd) -- eine Teil-Belegung liesse eine Stempel-Blindstelle zurueck";
    }
}

/// Die Nach-Pruefung: ein gesetztes Werte-Set muss die Zeile auch WIRKLICH erreicht haben. Ohne sie
/// liefe eine System-Zeile, der (etwa nach einem Achsen-Umbau) das simd-Glied fehlt, still ohne
/// Zellwert durch -- ein Bau ohne Zell-Diskriminierung, der wie einer mit aussieht.
constexpr void system_cell_values_applied_gate(std::string_view werte, std::size_t angewandt) {
    if (werte.empty()) return;
    if (angewandt != kSystemCellValueKeyCount)
        throw "W10-C1 Zellwerte: ein Schluessel des Werte-Sets kommt in der System-Zeile nicht als Eintrag "
              "vor -- das Werte-Set liefe still ins Leere und die Binary traege eine Zell-Blindstelle";
}

} // namespace detail

/// complete_system_stamp_line_size(zeile, werte) -- die Laenge der vervollstaendigten Zeile PLUS 1 (fuer
/// das '\0'). Sie ist das Groessen-Argument des consteval-Vervollstaendigers unten und laeuft ueber
/// GENAU denselben Renderer -- Zaehlung und Fuellung koennen nicht auseinanderlaufen.
[[nodiscard]] constexpr std::size_t complete_system_stamp_line_size(std::string_view zeile, SystemCellValues werte) {
    detail::system_cell_values_gate(werte.value);
    std::size_t       n   = 0;
    std::size_t const ang = detail::render_system_cell_values(zeile, werte.value, [&n](char) { ++n; });
    detail::system_cell_values_applied_gate(werte.value, ang);
    return n + 1;
}

/// complete_system_stamp_line_array<N>(zeile, werte) -- der CONSTEVAL-Vervollstaendiger fuer die
/// Makro-Naht. N kommt aus complete_system_stamp_line_size (derselbe Renderer, keine zweite Zaehlung).
/// EIGENER NAME statt einer Ueberladung der Laufzeit-Form: eine Ueberladung, die sich nur ueber ein
/// nicht ableitbares Template-Argument unterscheidet, waere genau die Art stiller Aufruf-Verschiebung,
/// gegen die das K-1-Muster antritt.
///
/// Er prueft die vervollstaendigte Zeile mit den BESTEHENDEN Werkzeugen aus anatomy_stamp_entries.hpp
/// nach, statt eine zweite Grammatik zu behaupten:
///   (1) detail::stamp_line_is_wellformed -- der VOLLE Weg Scanner + Entry-Parser (F1-F6 der
///       Klammer-Grammatik UND die entry-seitige Strenge Z-09). Er WIRFT mit benanntem Text; eine
///       Vervollstaendigung, die die Zeile unparsbar machte, bricht damit compile-hart, statt still in
///       den Stempel zu reisen;
///   (2) count_stamp_entries -- die Eintrags-ZAHL bleibt gleich. Das ist die mechanische Form der
///       Stempel-KERN-Zusage "der Vervollstaendiger fuegt IN Entries ein": ein zusaetzliches Segment
///       (oder gar eine wieder eingebaute merge-Zeile) waere hier sofort sichtbar.
template <std::size_t N>
[[nodiscard]] consteval CompletedSystemStampLine<N> complete_system_stamp_line_array(std::string_view zeile,
                                                                                     SystemCellValues werte) {
    detail::system_cell_values_gate(werte.value);
    CompletedSystemStampLine<N> out{};
    std::size_t                 n   = 0;
    std::size_t const           ang = detail::render_system_cell_values(zeile, werte.value, [&out, &n](char c) {
        if (n + 1 >= N) throw "W10-C1: Puffer der vervollstaendigten System-Zeile zu klein (Groessen-Drift)";
        out.chars[n++] = c;
    });
    detail::system_cell_values_applied_gate(werte.value, ang);
    if (n + 1 != N) throw "W10-C1: Laengen-Zaehlung und Fuellung der vervollstaendigten System-Zeile weichen ab";
    out.chars[n] = '\0';
    (void)detail::stamp_line_is_wellformed(out.view());
    if (count_stamp_entries(out.view()) != count_stamp_entries(zeile))
        throw "W10-C1: die Vervollstaendigung hat die Eintrags-ZAHL veraendert -- der Zellwert gehoert IN "
              "einen Eintrag (hinter den Algorithmus-Namen), nie als zusaetzliches Segment";
    return out;
}

/// complete_system_stamp_line(zeile, werte) -- die LAUFZEIT-Form (der Zwilling des consteval-Wegs oben).
/// Sie ruft DENSELBEN Renderer mit derselben Ordnung; nur die Senke ist ein std::string statt eines
/// char-Arrays (ein reinterpret_cast auf std::string::data() waere in einem konstanten Ausdruck
/// verboten -- dieselbe Zwei-Senken-Begruendung wie bei anatomy_fingerprint_preimage).
[[nodiscard]] inline std::string complete_system_stamp_line(std::string_view zeile, SystemCellValues werte) {
    detail::system_cell_values_gate(werte.value);
    std::string out;
    out.reserve(zeile.size() + werte.value.size());
    std::size_t const ang = detail::render_system_cell_values(zeile, werte.value, [&out](char c) { out += c; });
    detail::system_cell_values_applied_gate(werte.value, ang);
    return out;
}

} // namespace comdare::cache_engine::abi
