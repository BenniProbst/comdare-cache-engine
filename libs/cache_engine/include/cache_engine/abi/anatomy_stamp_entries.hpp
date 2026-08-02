#pragma once
// abi/anatomy_stamp_entries.hpp -- G2-1a (Lager-Gate A3): consteval-Tokenizer der gerenderten Stempel-Zeilen
// "achse=algorithmus@X.Y.Z;..." in ein Array von AnatomyStampEntryV1. Die 4 String-Literale (organ/system/
// measurement/merge) bleiben die Single-Source; die Array-Form wird INNEN im COMDARE_DEFINE_ANATOMY_MODULE-Makro per
// consteval aus denselben Literalen materialisiert (K7b-3-Praezedenz, anatomy_module_abi_v1.hpp) -- der emittierte
// Quelltext bleibt byte-identisch (golden-neutral). Organ- und System-Array bleiben ZWEI getrennte Felder, NIE
// fusioniert (Layer-Doktrin).
//
// A3 liefert NUR den Parser + den Sentinel; die Verdrahtung ans AnatomyVersionLines-POD (drei Zeiger+Count-Paare)
// folgt in A4 (POD 88->136, Layout 4->5). header-only, C++23, rein consteval/constexpr (kein Runtime-Switch).
//
// A13-M1 (Owner-Entscheid E2 vom 02.08.2026): das 'e'-Suffix der Versionsbezifferung (seit A13-M1b in der Form
// "achse=algo@X.Y.Zce") markiert einen EXPERIMENTELLEN Achsen-Algorithmus aus einem Pruefling. Im Entry-POD reist
// die Markierung im dafuer
// vorgesehenen reserved-Feld (anatomy_module_abi_v1_decl.hpp: "0 (Ausrichtung / kuenftige Flags)") als BIT 0 --
// kein sizeof-/Layout-Bruch am 48-Byte-Entry-POD, keine Entry-Layout-Versions-Folge. Ohne 'e' bleibt reserved == 0
// -> der gesamte Bestand ist byte-identisch (golden-neutral).
//
// NAMENS-TOLERANZ (Owner-Nachtrag Q2 vom 02.08.2026): erweiterte HIERARCHISCHE Algorithmus-/Achsen-Namen nach dem
// Muster "prt-art.memory.abc@1.0.0" sind zulaessig. Der Tokenizer teilt strikt an '=' (erstes) und '@' (erstes nach
// dem '='), danach liest parse_dotted_semver -- ein '.' im NAMENS-Anteil VOR dem '@' ist damit transparenter
// Namens-Bestandteil, ein '.' im VERSIONS-Anteil bleibt reiner Zahlen-Trenner (drei Zahlen, sonst Sentinel).
//
// A13-M1b (Owner-Antwort Q3 vom 02.08.2026): die Versionsbezifferung traegt zusaetzlich GENAU EIN
// HARDWARE-FLAG ('c'=CPU / 'g'=GPU / 'f'=FPGA / 'n'=NPU), und zwar VOR dem optionalen 'e'
// ("achse=algo@X.Y.Zc", "achse=algo@X.Y.Zce"). Im Entry-POD belegt es die BITS 1-2 des reserved-Feldes --
// direkt neben dem A13-M1-Experimental-Bit 0, weiterhin ohne sizeof-/Layout-Bruch am 48-Byte-Entry-POD.
// KODIERUNG 00=c, 01=g, 10=f, 11=n. Der POD kennt bewusst KEIN "kein Flag": wir produzieren nur CPU-Code,
// 'c' IST der Default -- eine flaglose Uebergangs-Zeile ("@X.Y.Z", der heutige Bestand) landet damit auf 00
// und laesst reserved bei 0 (Bestands-Byte-Gleichheit bleibt erhalten).
//
// == A13-M2: DIE KLAMMER-GRAMMATIK (Owner-Antwort Q1 vom 02.08.2026) ==============================
// Owner-Q1 verbatim: "Q1 - Wie empfohlen nach Klammern (derzeit auch so geplant, bitte nachlesen)."
// Owner-E2 verbatim: "Da eine Meta-Meta-Achse immer zu den Mess-Achsen, System-Achsen oder Organ-Achsen
// gehoert, wird sie auch einfach dynamisch ans Ende der Kette in den bestehenden Zeilen angehaengt."
// Damit ist die im Code bereits kodifizierte Q-A-Auflage (hardware_meta_meta_axis.hpp Kopf: "Ein Glied,
// das selbst Hub ist, behaelt seine eigene Klammer. Das ist die stempel-taugliche Form (Klammer-ANZAHL
// kodiert die Ebene)") zur ZEILEN-Grammatik erhoben; die Punkt-Pfad-Empfehlung des A13-Designs ist
// VERWORFEN.
//
// ZEILEN-GRAMMATIK (die EINE Wahrheit; detail::scan_stamp_segments setzt sie durch):
//   line     := [ segment ( ';' segment )* ]
//   segment  := entry | group
//   group    := '[' segment ( ';' segment )* ']'
//   entry    := achse '=' algorithmus '@' X '.' Y '.' Z [ HWFLAG [ 'e' ] ]
//   EBENE    := die KLAMMER-TIEFE, in der das entry steht: 0 == Haupt-Achse (der gesamte heutige
//               Bestand, byte-unveraendert), 1 == Meta-Meta, 2 == Meta-Meta-Meta, ... Die Rekursion ist
//               OFFEN (Layer-Modell D4 / Owner Q-D: kein festes drittes Level).
// EIN group ist IMMER ein regulaeres ';'-GESCHWISTER-Segment (nie ans vorige entry geklebt) -- die
// Grammatik ist damit auf JEDER Ebene dieselbe, und die Zeile ist aus dem Entry-Array samt Ebenen
// VERLUSTFREI rekonstruierbar. ZUORDNUNG: ein group traegt die Meta-Metas der Ebene, in der es steht --
// auf ZEILEN-Ebene die der Realm-Zeile (Owner-E2: "ans Ende der Kette"), INNERHALB eines group die
// Glieder des unmittelbar vorangehenden Geschwister-entry (Q-A: "ein Glied, das selbst Hub ist, behaelt
// seine eigene Klammer"). Kein Namens-Pfad noetig.
// ZWEI NAMENSRAEUME, keine Kollision (Owner-Nachtrag ~12:1x): die EBENE lebt in den KLAMMERN (hier), die
// hierarchischen ALGORITHMUS-/Achsen-Namen ("prt-art.memory.abc", Owner-Q2) leben in den PUNKTEN VOR
// dem '@'. Der Tokenizer unten fasst beides nie an derselben Stelle an.
// Im Entry-POD reist die Ebene in den reserved-BITS 3-5. Ebene 0 == Bit-Muster 0 -> der flaglose,
// klammerlose Bestand bleibt exakt bei reserved == 0 (Byte-Gleichheit unveraendert).
// STRENGE (A13-M2/B2 vervollstaendigt): KEINE dieser Fehlformen wird toleriert -- jede bricht die
// consteval-Auswertung hart (der Stempel ist Identitaet -- er darf nie still falsch reisen):
//   * unbalancierte Klammern ( ']' ohne '[' / '[' ohne ']' ),
//   * Tiefe > 7 (passt nicht in die reserved-Bits 3-5),
//   * die GEKLEBTE Gruppe "a=b@1.0.0[c=d@1.0.0]" und die aneinandergereihte Gruppe "[a=b@1.0.0][c=d@1.0.0]"
//     -- ein '[' ist NUR am Segment-Anfang zulaessig (Zeilenanfang, nach ';' oder nach '['). Genau das ist
//     der bindende Owner-Q1-Satz "ein group ist IMMER ein regulaeres ';'-Geschwister-Segment"; ohne diese
//     Wache ergaeben zwei byte-VERSCHIEDENE Zeilen dasselbe Entry-Array und die Zusage "Zeile aus dem
//     Entry-Array VERLUSTFREI rekonstruierbar" gaelte nur noch fuer kanonische Zeilen,
//   * die LEERE Gruppe "[]" (und "[;]") -- der Renderer liefert fuer ein Blatt einen LEEREN Anhang, nie
//     einen leeren Rahmen (meta_meta_stamp_suffix); die Grammatik zementiert diese Zusage.
// Die Fehlformen sind unten als CT-NEGATIVPROBEN (stamp_line_is_parsable) festgenagelt -- die Wachen sind
// damit nicht bloss behauptet, sondern bewiesen.

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // AnatomyStampEntryV1
#include <cache_engine/measurement/algo_semver.hpp>        // parse_dotted_semver (X.Y.Z-Ruecklesung)

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::abi {

/// A13-M1: Bit 0 des AnatomyStampEntryV1::reserved-Feldes == "experimenteller Achsen-Algorithmus" ('e'-Suffix der
/// Versionsbezifferung, Owner-E2). EINZIGE Wahrheit dieser Belegung -- zusammen mit den Hardware-Flag-Bits unten.
inline constexpr std::uint32_t kStampEntryFlagExperimental = 1u << 0;

/// A13-M1b (Owner-Q3): BITS 1-2 == das Hardware-Flag der Versionsbezifferung. Die uebrigen 29 Bits bleiben
/// 0/reserviert. Shift + Maske sind die EINZIGE Wahrheit der Lage; die Codes darunter die der Belegung.
inline constexpr std::uint32_t kStampEntryHwFlagShift = 1u;
inline constexpr std::uint32_t kStampEntryHwFlagMask  = 0x3u << kStampEntryHwFlagShift;

/// A13-M2 (Owner-Q1): BITS 3-5 == die META-META-EBENE == die KLAMMER-TIEFE des Eintrags. 0 == Haupt-Achse
/// (klammerlos, der gesamte heutige Bestand), 1 == Meta-Meta, 2 == Meta-Meta-Meta, ... Shift + Maske sind die
/// EINZIGE Wahrheit der Lage, kStampEntryMaxMetaLevel die der Kapazitaet (tiefer bricht der Parser hart).
inline constexpr std::uint32_t kStampEntryMetaLevelShift = 3u;
inline constexpr std::uint32_t kStampEntryMetaLevelMask  = 0x7u << kStampEntryMetaLevelShift;
inline constexpr std::uint32_t kStampEntryMaxMetaLevel   = 7u;

/// Die Code-Punkte der Bits 1-2. c == 0 ist der DEFAULT (Owner-Q3: "Wir produzieren nur CPU code") -- deshalb
/// bleibt reserved fuer den gesamten flaglosen Bestand exakt 0 und der Bestand byte-identisch.
inline constexpr std::uint32_t kStampEntryHwCodeCpu  = 0u;
inline constexpr std::uint32_t kStampEntryHwCodeGpu  = 1u;
inline constexpr std::uint32_t kStampEntryHwCodeFpga = 2u;
inline constexpr std::uint32_t kStampEntryHwCodeNpu  = 3u;

/// Der Flag-Typ der Versions-Grammatik, unter abi-Namen sichtbar (keine zweite Aufzaehlung -- die Grammatik in
/// measurement/algo_semver.hpp bleibt die Single-Source).
using StampEntryHardwareFlag = ::comdare::cache_engine::measurement::HardwareFlag;

/// Grammatik-Flag -> Bit-Code. `none` (flaglose Uebergangs-Form) UND `cpu` fallen beide auf den c-Default 0 --
/// genau so ist "c == Default" gemeint; der POD unterscheidet "kein Flag" nicht von "CPU".
[[nodiscard]] constexpr std::uint32_t stamp_entry_hw_code(StampEntryHardwareFlag f) noexcept {
    switch (f) {
        case StampEntryHardwareFlag::gpu: return kStampEntryHwCodeGpu;
        case StampEntryHardwareFlag::fpga: return kStampEntryHwCodeFpga;
        case StampEntryHardwareFlag::npu: return kStampEntryHwCodeNpu;
        case StampEntryHardwareFlag::cpu:
        case StampEntryHardwareFlag::none: break;
    }
    return kStampEntryHwCodeCpu;
}

/// Praedikat statt Bit-Fummelei am Aufruf-Ort (Konsumenten: Lager-Identitaet/Skip-Gate, spaeter G-E6/A2).
[[nodiscard]] constexpr bool stamp_entry_is_experimental(AnatomyStampEntryV1 const& e) noexcept {
    return (e.reserved & kStampEntryFlagExperimental) != 0u;
}

/// Bit-Code -> Grammatik-Flag. Liefert NIE `none`: der POD-Default 00 IST 'c' (s. Kopf-Kommentar), ein
/// Alt-Eintrag mit reserved == 0 liest sich damit korrekt als CPU-Stand.
[[nodiscard]] constexpr StampEntryHardwareFlag stamp_entry_hardware_flag(AnatomyStampEntryV1 const& e) noexcept {
    switch ((e.reserved & kStampEntryHwFlagMask) >> kStampEntryHwFlagShift) {
        case kStampEntryHwCodeGpu: return StampEntryHardwareFlag::gpu;
        case kStampEntryHwCodeFpga: return StampEntryHardwareFlag::fpga;
        case kStampEntryHwCodeNpu: return StampEntryHardwareFlag::npu;
        default: break;
    }
    return StampEntryHardwareFlag::cpu;
}

/// A13-M2: die META-META-EBENE des Eintrags == seine Klammer-Tiefe (0 == Haupt-Achse). Praedikat statt
/// Bit-Fummelei am Aufruf-Ort; Konsumenten (Lager-Identitaet/Skip-Gate, G-E6/A2) unterscheiden damit
/// Haupt-Achsen von Meta-Metas OHNE die Zeile erneut zu tokenisieren.
[[nodiscard]] constexpr std::uint32_t stamp_entry_meta_level(AnatomyStampEntryV1 const& e) noexcept {
    return (e.reserved & kStampEntryMetaLevelMask) >> kStampEntryMetaLevelShift;
}

[[nodiscard]] constexpr bool stamp_entry_is_meta_meta(AnatomyStampEntryV1 const& e) noexcept {
    return stamp_entry_meta_level(e) != 0u;
}

namespace detail {

/// Ein Segment der Zeile: die Zeichen-Spanne [begin, end) PLUS die Klammer-TIEFE, in der es steht
/// (A13-M2: level == Meta-Meta-Ebene, 0 == Haupt-Achse).
struct StampSegment {
    std::size_t   begin = 0;
    std::size_t   end   = 0;
    std::uint32_t level = 0;
};

/// Der EINE Zeilen-Scanner der A13-M2-Klammer-Grammatik. Beide Konsumenten (count_stamp_entries UND
/// parse_stamp_entries) teilen ihn sich -- die Grammatik existiert genau EINMAL und kann zwischen Zaehlung
/// und Materialisierung nicht driften (genau diese Drift waere der stille Array-Ueberlauf).
///
/// Trenner sind ';' (Segment-Ende auf gleicher Ebene), '[' (eine Ebene tiefer) und ']' (eine Ebene zurueck);
/// LEERE Spannen zwischen zwei Trennern liefern KEIN Segment (so ist ein reiner Klammer-Anhang
/// "...;[a=b@1.0.0]" genau ein Eintrag und nicht drei). Die gerenderten Zeilen sind stets wohlgeformt.
///
/// HARTE FEHLFORMEN (der Stempel IST Identitaet -- er darf nie still falsch reisen). In den einzigen
/// Aufrufern (consteval) ist jede davon ein COMPILE-Fehler, kein Laufzeit-Wurf:
///   (F1) ']' ohne offene Klammer,
///   (F2) am Zeilenende offen gebliebene Klammer,
///   (F3) Tiefe > kStampEntryMaxMetaLevel (passt nicht in die reserved-Bits 3-5),
///   (F4) A13-M2/B2-HAERTUNG: ein '[' AUSSERHALB eines Segment-ANFANGS -- also die GEKLEBTE Form
///        "a=b@1.0.0[c=d@1.0.0]" (group direkt ans vorige entry gehaengt) und die aneinandergereihte
///        Form "[a=b@1.0.0][c=d@1.0.0]" (group direkt hinter einem ']'),
///  (F4b) dieselbe Klebe-Fehlform SPIEGELVERKEHRT: ein entry direkt hinter einem ']' ohne ';'-Trenner
///        ("[a=b@1.0.0]c=d@1.0.0"). Nach einem ']' darf NUR ';', ']' oder das Zeilenende folgen.
///   (F5) A13-M2/B2-HAERTUNG: die LEERE Gruppe "[]" (auch "[;]" und jede Gruppe ohne ein einziges
///        Segment -- eine Gruppe, die nichts traegt, ist keine Ebene).
///
/// WARUM F4/F5 hart brechen muessen (Owner-Q1-Ledger-Satz, bindend): "Ein group ist IMMER ein regulaeres
/// ';'-Geschwister-Segment (nie ans vorige entry geklebt) -- dadurch ist die Zeile aus dem Entry-Array samt
/// Ebenen VERLUSTFREI rekonstruierbar." Die geklebte Form parste bis A13-M2/B2 KOMMENTARLOS in dasselbe
/// Entry-Array wie die kanonische Geschwister-Form. Zwei byte-VERSCHIEDENE Zeilen (verschiedener SHA512,
/// verschiedene Lager-Identitaet) haetten damit denselben POD ergeben -- die Rekonstruktions-Zusage waere
/// nur noch fuer kanonische Zeilen wahr gewesen, und der Stempel darf genau das nie. Die leere Gruppe
/// lieferte still 0 Eintraege und damit einen Rahmen ohne Inhalt; der kanonische Renderer erzeugt statt
/// dessen einen LEEREN Anhang (meta_meta_stamp_suffix eines Blattes ist "", nie "[]") -- die Haertung
/// zementiert genau diese Renderer-Zusage in der Grammatik.
/// ZULAESSIGE VORGAENGER eines '[' sind daher AUSSCHLIESSLICH: Zeilenanfang, ';' und '['. Ein '[' nach ']'
/// oder nach einem Namens-/Versions-Zeichen bricht. Und spiegelbildlich (F4b): auf ein ']' darf nur ';',
/// ']' oder das Zeilenende folgen -- sonst waeren "[a=b@1.0.0]c=d@1.0.0" und "[a=b@1.0.0];c=d@1.0.0" wieder
/// zwei byte-verschiedene Zeilen mit identischem Entry-Array (dieselbe Verlust-Stelle, nur andersherum).
template <class Visitor>
constexpr void scan_stamp_segments(std::string_view line, Visitor&& visit) {
    std::uint32_t depth = 0;
    std::size_t   start = 0;
    // Der zuletzt gesehene Trenner; '\0' == Zeilenanfang. Er allein entscheidet ueber F4 -- "pos == start"
    // genuegt NICHT, weil auch die Position direkt hinter einem ']' ein Segment-Anfang waere.
    char last_delim = '\0';
    // Anzahl der Segmente, die in der auf dieser Tiefe aktuell OFFENEN Gruppe schon gesehen wurden. Ein
    // verschachteltes group zaehlt in seiner ELTERN-Ebene als ein Segment (Grammatik: segment := entry|group)
    // -- deshalb ist "[[a=b@1.0.0]]" wohlgeformt, "[]" und "[;]" dagegen nicht. Index 0 == Zeilen-Ebene
    // (nie geprueft, eine leere ZEILE ist zulaessig).
    std::array<std::size_t, kStampEntryMaxMetaLevel + 1> seen{};
    auto const                                           flush = [&](std::size_t end) {
        if (end > start) {
            // (F4b) Ein Segment darf nie direkt hinter einem ']' beginnen -- das waere die geklebte Form
            // spiegelverkehrt ("[a=b@1.0.0]c=d@1.0.0" statt "[a=b@1.0.0];c=d@1.0.0").
            if (last_delim == ']')
                throw "A13-M2 Klammer-Grammatik: Segment direkt hinter ']' geklebt -- nach einer Gruppe folgt "
                      "';', eine weitere schliessende Klammer oder das Zeilenende";
            visit(StampSegment{start, end, depth});
            ++seen[depth];
        }
    };
    for (std::size_t pos = 0; pos < line.size(); ++pos) {
        char const c = line[pos];
        if (c == ';') {
            flush(pos);
            start      = pos + 1;
            last_delim = ';';
        } else if (c == '[') {
            // (F4) NUR am Segment-ANFANG: Zeilenanfang, direkt nach ';' oder direkt nach '['.
            if (pos != start || (last_delim != '\0' && last_delim != ';' && last_delim != '['))
                throw "A13-M2 Klammer-Grammatik: '[' steht nicht am Segment-Anfang -- ein group ist IMMER ein "
                      "regulaeres ';'-Geschwister-Segment (nie ans vorige entry oder an ein ']' geklebt)";
            flush(pos);
            ++depth;
            if (depth > kStampEntryMaxMetaLevel)
                throw "A13-M2 Klammer-Grammatik: Meta-Meta-Ebene > 7 passt nicht in die reserved-Bits 3-5";
            seen[depth] = 0;
            start       = pos + 1;
            last_delim  = '[';
        } else if (c == ']') {
            flush(pos);
            if (depth == 0) throw "A13-M2 Klammer-Grammatik: schliessende Klammer ohne oeffnende";
            // (F5) Eine Gruppe ohne ein einziges Segment ist keine Ebene, sondern ein leerer Rahmen.
            if (seen[depth] == 0)
                throw "A13-M2 Klammer-Grammatik: leere Gruppe '[]' -- der Renderer liefert fuer ein Blatt einen "
                      "LEEREN Anhang, nie einen leeren Rahmen";
            --depth;
            ++seen[depth]; // das gerade geschlossene group ist ein Segment SEINER Eltern-Ebene.
            start      = pos + 1;
            last_delim = ']';
        }
    }
    if (depth != 0) throw "A13-M2 Klammer-Grammatik: offene Klammer am Zeilenende";
    flush(line.size());
}

/// Materialisiert EIN Segment [begin, end) des Literals in einen Entry-POD: axis = vor '=', algorithm =
/// zwischen '=' und '@', Version = nach '@' (via parse_dotted_semver). axis/algorithm sind {ptr,len}-Sichten
/// INS Literal (das im Aufrufer static constexpr liegt -> Zeiger ueberleben; K7b-3-Praezedenz). Fehlt '='/'@',
/// bleiben die jeweiligen Laengen/Version 0 (defensiv; die gerenderten Zeilen sind stets wohlgeformt).
[[nodiscard]] constexpr AnatomyStampEntryV1 parse_one_stamp_segment(char const* lit, std::size_t seg_start,
                                                                    std::size_t seg_end) noexcept {
    std::size_t eq = seg_start;
    while (eq < seg_end && lit[eq] != '=') ++eq;
    std::size_t at = (eq < seg_end) ? eq + 1 : seg_end;
    while (at < seg_end && lit[at] != '@') ++at;
    AnatomyStampEntryV1 e{};
    e.axis     = lit + seg_start;
    e.axis_len = (eq < seg_end) ? (eq - seg_start) : (seg_end - seg_start);
    if (eq < seg_end) {
        e.algorithm = lit + eq + 1;
        e.algo_len  = at - (eq + 1);
    } else {
        e.algorithm = lit + seg_end; // kein '=' -> leerer Algorithmus
        e.algo_len  = 0;
    }
    if (at < seg_end) { // '@X.Y.Z' vorhanden
        std::string_view const                                 ver{lit + at + 1, seg_end - (at + 1)};
        ::comdare::cache_engine::measurement::AlgoSemVer const sv =
            ::comdare::cache_engine::measurement::parse_dotted_semver(ver);
        e.x = sv.x;
        e.y = sv.y;
        e.z = sv.z;
        // A13-M1: das 'e'-Suffix wandert als Bit 0 ins reserved-Feld (ohne 'e' bleibt das Bit 0).
        if (sv.experimental) e.reserved |= kStampEntryFlagExperimental;
        // A13-M1b: das Hardware-Flag wandert als Bits 1-2 daneben. Ohne Flag (und bei 'c') ist der Code 0
        // -> reserved bleibt fuer den gesamten flaglosen Bestand exakt 0 (Byte-Gleichheit). Der Sentinel-Zweig
        // (kein '@'-Teil) laeuft hier gar nicht durch und bleibt damit ebenfalls bei reserved == 0.
        e.reserved |= (stamp_entry_hw_code(sv.hardware) << kStampEntryHwFlagShift);
    }
    return e;
}

} // namespace detail

/// Zaehlt die Eintraege einer Stempel-Zeile: die ';'-getrennten Haupt-Achsen-Segmente PLUS die Eintraege in
/// den A13-M2-Klammer-Anhaengen (auf jeder Ebene). Leere Zeile -> 0 (kein Eintrag). Fuer den KLAMMERLOSEN
/// Bestand ist das Ergebnis unveraendert (1 + Anzahl der ';') -- die gerenderten Zeilen tragen weder ein
/// Trailing-';' noch leere Segmente.
[[nodiscard]] consteval std::size_t count_stamp_entries(std::string_view line) {
    std::size_t n = 0;
    detail::scan_stamp_segments(line, [&n](detail::StampSegment const&) { ++n; });
    return n;
}

// == A13-M2/B2: DIE CT-NEGATIVPROBEN ==============================================================
// Eine harte Fehlform ist im consteval-Pfad ein COMPILE-Fehler -- man kann sie also nicht einfach in einem
// static_assert aufrufen (das braeche die Uebersetzung genau dieser Datei). Das Praedikat unten dreht das
// um: es fragt, OB die Zeile ueberhaupt konstant auswertbar ist, und macht damit die Nicht-Parsbarkeit
// selbst zu einer beweisbaren, positiven Aussage. Mechanik: schlaegt die Auswertung im Default-Template-
// Argument fehl, ist das eine Substitutions-Ablehnung (kein Fehler) und die '...'-Ueberladung gewinnt.

namespace detail {

/// Nicht-Typ-Template-Parameter-Traeger fuer ein Zeilen-Literal (std::string_view ist kein strukturaler Typ
/// und als NTTP unzulaessig; ein public char-Array ist es).
template <std::size_t N>
struct StampLineLiteral {
    char data[N]{};
    consteval StampLineLiteral(char const (&s)[N]) {
        for (std::size_t i = 0; i < N; ++i) data[i] = s[i];
    }
    [[nodiscard]] constexpr std::string_view view() const noexcept { return std::string_view{data, N - 1}; }
};
template <std::size_t N>
StampLineLiteral(char const (&)[N]) -> StampLineLiteral<N>;

template <StampLineLiteral L, std::size_t = count_stamp_entries(L.view())>
consteval bool stamp_line_is_parsable_impl(int) {
    return true;
}
template <StampLineLiteral L>
consteval bool stamp_line_is_parsable_impl(...) {
    return false;
}

} // namespace detail

/// true genau dann, wenn die Zeile die A13-M2-Klammer-Grammatik erfuellt (also consteval durchlaeuft).
/// Nutzung: `static_assert(!stamp_line_is_parsable<"a=b@1.0.0[c=d@1.0.0]">);`
template <detail::StampLineLiteral L>
inline constexpr bool stamp_line_is_parsable = detail::stamp_line_is_parsable_impl<L>(0);

// -- POSITIV: die kanonischen Formen, die der Renderer wirklich erzeugt --
static_assert(stamp_line_is_parsable<"">, "die leere Realm-Zeile ist zulaessig (0 Eintraege).");
static_assert(stamp_line_is_parsable<"search_algo=k_ary@1.0.0;filter=bloom@2.3.4">, "klammerloser Bestand.");
static_assert(stamp_line_is_parsable<"external_utils=code@1.0.0;[simd=code@1.0.0]">, "Geschwister-Anhang (System).");
static_assert(stamp_line_is_parsable<"[load_framework=ycsb@1.0.0]">, "Anhang als EINZIGES Segment der Zeile.");
static_assert(stamp_line_is_parsable<"external_utils=code@1.0.0;[gpu=code@2.0.0;[nvlink=code@3.0.0]]">,
              "offene Rekursion: ein group als Geschwister INNERHALB eines group.");
static_assert(stamp_line_is_parsable<"[[a=b@1.0.0]]">, "ein group als ERSTES Segment eines group ('[' nach '[').");
static_assert(stamp_line_is_parsable<"a=b@1.0.0;[c=d@1.0.0];[e=f@1.0.0]">, "zwei Anhaenge, sauber durch ';' getrennt.");

// -- NEGATIV (F4): '[' ausserhalb eines Segment-Anfangs --
static_assert(!stamp_line_is_parsable<"a=b@1.0.0[c=d@1.0.0]">,
              "GEKLEBTE Gruppe: ein group ist IMMER ein regulaeres ';'-Geschwister-Segment (Owner-Q1). Ohne "
              "diese Wache haette diese Zeile dasselbe Entry-Array wie 'a=b@1.0.0;[c=d@1.0.0]' -- zwei "
              "byte-verschiedene Zeilen mit identischem POD.");
static_assert(!stamp_line_is_parsable<"[a=b@1.0.0][c=d@1.0.0]">, "aneinandergereihte Gruppen ohne ';'-Trenner.");

// -- NEGATIV (F4b): dieselbe Klebe-Fehlform spiegelverkehrt -- entry direkt hinter ']' --
static_assert(!stamp_line_is_parsable<"[a=b@1.0.0]c=d@1.0.0">,
              "Segment direkt hinter ']' geklebt -- haette dasselbe Entry-Array wie '[a=b@1.0.0];c=d@1.0.0'.");
static_assert(stamp_line_is_parsable<"[a=b@1.0.0];c=d@1.0.0">, "... die kanonische Form derselben Zeile.");

// -- NEGATIV (F5): die leere Gruppe --
static_assert(!stamp_line_is_parsable<"[]">, "leerer Rahmen ohne Inhalt: der Renderer liefert stattdessen "
                                             "einen LEEREN Anhang (meta_meta_stamp_suffix eines Blattes).");
static_assert(!stamp_line_is_parsable<"a=b@1.0.0;[]">, "leere Gruppe auch als Anhang einer belegten Zeile.");
static_assert(!stamp_line_is_parsable<"[;]">, "eine Gruppe aus lauter leeren Spannen ist ebenfalls leer.");

// -- NEGATIV (F1/F2/F3): die schon vor B2 deklarierten harten Fehlformen, jetzt ebenfalls BEWIESEN --
static_assert(!stamp_line_is_parsable<"a=b@1.0.0]">, "schliessende Klammer ohne oeffnende.");
static_assert(!stamp_line_is_parsable<"a=b@1.0.0;[c=d@1.0.0">, "offene Klammer am Zeilenende.");
static_assert(!stamp_line_is_parsable<"[[[[[[[[a=b@1.0.0]]]]]]]]">, "Tiefe 8 > kStampEntryMaxMetaLevel (7).");
static_assert(stamp_line_is_parsable<"[[[[[[[a=b@1.0.0]]]]]]]">, "Tiefe 7 ist die letzte zulaessige Ebene.");

/// Parst die Stempel-Zeile aus dem Literal `lit` (nullterminiert, effektive Laenge M-1) in genau N Eintraege
/// (N == count_stamp_entries(lit)), in TEXT-Reihenfolge -- die Klammer-Anhaenge stehen also hinter den
/// Haupt-Achsen-Eintraegen ihrer Realm-Zeile (Owner-E2 "ans Ende der Kette"). Jeder Eintrag traegt seine
/// Klammer-TIEFE in den reserved-Bits 3-5 (A13-M2); Ebene 0 laesst reserved unveraendert, der klammerlose
/// Bestand bleibt damit byte-identisch.
template <std::size_t N, std::size_t M>
[[nodiscard]] consteval std::array<AnatomyStampEntryV1, N> parse_stamp_entries(char const (&lit)[M]) {
    std::array<AnatomyStampEntryV1, N> out{};
    std::size_t                        idx = 0;
    detail::scan_stamp_segments(std::string_view{lit, M - 1}, [&](detail::StampSegment const& seg) {
        AnatomyStampEntryV1 e = detail::parse_one_stamp_segment(lit, seg.begin, seg.end);
        e.reserved |= (seg.level << kStampEntryMetaLevelShift) & kStampEntryMetaLevelMask;
        if (idx < N) out[idx] = e;
        ++idx;
    });
    return out;
}

/// Gemeinsamer Sentinel fuer count==0: die POD-Zeiger (A4) zeigen NIE auf nullptr, sondern hierauf (""-Doktrin-
/// konsistent -- leere Felder, kein nullptr). Ein Eintrag genuegt (wird nie dereferenziert, da count==0).
inline constexpr AnatomyStampEntryV1 kAnatomyStampNoEntries[1] = {{"", 0, "", 0, 0, 0, 0, 0}};

/// Liefert den POD-Zeiger fuer ein Eintrags-Array (A4-Verdrahtung): fuer N==0 den Sentinel (NIE nullptr, ""-Doktrin),
/// sonst arr.data(). So traegt das AnatomyVersionLines-POD auch bei leerem Mess-Array (kein Tooling) einen gueltigen,
/// nicht dereferenzierten Zeiger + count==0.
template <std::size_t N>
[[nodiscard]] constexpr AnatomyStampEntryV1 const*
stamp_entries_ptr(std::array<AnatomyStampEntryV1, N> const& arr) noexcept {
    if constexpr (N == 0)
        return kAnatomyStampNoEntries;
    else
        return arr.data();
}

// -- A13-M1/M1b: CT-Selbstbeweis des Parsers (Flag-Bits + Owner-Q2-Namens-Toleranz) ----------------------------
// Der Probe-Literal liegt bewusst NAMESPACE-SCOPE constexpr: nur so ueberleben die {ptr,len}-Sichten des
// consteval-Ergebnisses als Konstant-Ausdruck (dieselbe K7b-3-Praezedenz wie im Makro-Aufrufer).
namespace detail {
// Segment 0: Owner-Q2-Namens-Toleranz + Owner-Q3-Voll-Form "ce". Segment 1: der heutige flaglose Bestand.
inline constexpr char kStampEntryProbeLine[] = "prt-art.memory.abc=prt_patricia@2.3.4ce;filter=bloom@1.0.0";
inline constexpr auto kStampEntryProbe =
    parse_stamp_entries<count_stamp_entries(std::string_view{kStampEntryProbeLine})>(kStampEntryProbeLine);

static_assert(kStampEntryProbe.size() == 2, "Probe-Zeile hat genau zwei ';'-Segmente.");
// Owner-Q2: hierarchischer Name mit Punkten VOR dem '@' bleibt EIN Achsen-Name (kein Versions-Anteil).
static_assert(std::string_view(kStampEntryProbe[0].axis, kStampEntryProbe[0].axis_len) == "prt-art.memory.abc");
static_assert(std::string_view(kStampEntryProbe[0].algorithm, kStampEntryProbe[0].algo_len) == "prt_patricia");
static_assert(kStampEntryProbe[0].x == 2u && kStampEntryProbe[0].y == 3u && kStampEntryProbe[0].z == 4u);
// A13-M1: 'e' -> Bit 0. A13-M1b: 'c' -> Bits 1-2 == 0 (c ist der Default) -> reserved == genau das e-Bit.
static_assert(stamp_entry_is_experimental(kStampEntryProbe[0]));
static_assert(stamp_entry_hardware_flag(kStampEntryProbe[0]) == StampEntryHardwareFlag::cpu);
static_assert(kStampEntryProbe[0].reserved == kStampEntryFlagExperimental);
// Der flaglose Bestand: KEIN Flag-Bit gesetzt, und er liest sich per Default als CPU-Stand.
static_assert(!stamp_entry_is_experimental(kStampEntryProbe[1]));
static_assert(stamp_entry_hardware_flag(kStampEntryProbe[1]) == StampEntryHardwareFlag::cpu);
static_assert(kStampEntryProbe[1].reserved == 0u);
static_assert(kStampEntryProbe[1].x == 1u && kStampEntryProbe[1].y == 0u && kStampEntryProbe[1].z == 0u);

// A13-M1b: die NICHT-CPU-Flags belegen die Bits 1-2 verschieden -- sonst waeren zwei Hardware-Staende im POD
// ununterscheidbar (Lager-Key-Kollision). Zweite Probe-Zeile, damit die Bit-Belegung nicht nur behauptet ist.
inline constexpr char kStampEntryHwProbeLine[] = "a=x@1.0.0g;b=y@1.0.0f;c=z@1.0.0n;d=w@1.0.0ne";
inline constexpr auto kStampEntryHwProbe =
    parse_stamp_entries<count_stamp_entries(std::string_view{kStampEntryHwProbeLine})>(kStampEntryHwProbeLine);

static_assert(kStampEntryHwProbe.size() == 4);
static_assert(stamp_entry_hardware_flag(kStampEntryHwProbe[0]) == StampEntryHardwareFlag::gpu);
static_assert(stamp_entry_hardware_flag(kStampEntryHwProbe[1]) == StampEntryHardwareFlag::fpga);
static_assert(stamp_entry_hardware_flag(kStampEntryHwProbe[2]) == StampEntryHardwareFlag::npu);
static_assert(stamp_entry_hardware_flag(kStampEntryHwProbe[3]) == StampEntryHardwareFlag::npu);
static_assert(kStampEntryHwProbe[0].reserved == (kStampEntryHwCodeGpu << kStampEntryHwFlagShift));
static_assert(kStampEntryHwProbe[1].reserved == (kStampEntryHwCodeFpga << kStampEntryHwFlagShift));
static_assert(kStampEntryHwProbe[2].reserved == (kStampEntryHwCodeNpu << kStampEntryHwFlagShift));
// 'ne' == NPU + experimentell: BEIDE Bit-Gruppen zugleich, ohne einander zu ueberschreiben.
static_assert(stamp_entry_is_experimental(kStampEntryHwProbe[3]));
static_assert(kStampEntryHwProbe[3].reserved ==
              (kStampEntryFlagExperimental | (kStampEntryHwCodeNpu << kStampEntryHwFlagShift)));
static_assert(!stamp_entry_is_experimental(kStampEntryHwProbe[2]));
// Die vier Flag-Staende sind im reserved-Feld paarweise verschieden (keine stille Kollision).
static_assert(kStampEntryHwProbe[0].reserved != kStampEntryHwProbe[1].reserved);
static_assert(kStampEntryHwProbe[1].reserved != kStampEntryHwProbe[2].reserved);
static_assert(kStampEntryHwProbe[2].reserved != kStampEntryHwProbe[3].reserved);
static_assert(kStampEntryHwProbe[0].reserved != kStampEntryProbe[1].reserved); // g != flagloser Bestand

// Fehlform-Wache: ein zweites Hardware-Flag ist grammatisch Sentinel -> KEIN Bit wandert ins reserved-Feld
// (der Parser raet nie ein Flag herbei).
inline constexpr char kStampEntryBadFlagLine[] = "a=x@1.0.0cg";
inline constexpr auto kStampEntryBadFlagProbe =
    parse_stamp_entries<count_stamp_entries(std::string_view{kStampEntryBadFlagLine})>(kStampEntryBadFlagLine);
static_assert(kStampEntryBadFlagProbe[0].x == 0u && kStampEntryBadFlagProbe[0].y == 0u &&
              kStampEntryBadFlagProbe[0].z == 0u);
static_assert(kStampEntryBadFlagProbe[0].reserved == 0u);

// -- A13-M2: CT-Selbstbeweis der KLAMMER-Grammatik (Owner-Q1) -------------------------------------------
// (1) Der klammerlose Bestand bleibt Ebene 0 und reserved == 0 -- die Erweiterung ist byte-neutral.
static_assert(stamp_entry_meta_level(kStampEntryProbe[1]) == 0u);
static_assert(!stamp_entry_is_meta_meta(kStampEntryProbe[1]));
static_assert(!stamp_entry_is_meta_meta(kStampEntryProbe[0])); // 'ce' setzt Flag-Bits, aber KEINE Ebene

// (2) Die reale A13-M2-System-Zeilen-Form: DREI Haupt-Achsen + EIN Klammer-Anhang == VIER Eintraege.
inline constexpr char kStampEntryBracketLine[] =
    "target_isa=code@1.0.0;operating_system=code@1.0.0;external_utils=code@1.0.0;[simd=code@1.0.0]";
inline constexpr auto kStampEntryBracketProbe =
    parse_stamp_entries<count_stamp_entries(std::string_view{kStampEntryBracketLine})>(kStampEntryBracketLine);

static_assert(kStampEntryBracketProbe.size() == 4, "3 Haupt-Achsen + 1 geklammerte Meta-Meta == 4 Eintraege.");
static_assert(stamp_entry_meta_level(kStampEntryBracketProbe[0]) == 0u);
static_assert(stamp_entry_meta_level(kStampEntryBracketProbe[2]) == 0u);
// Der Klammer-Anhang steht ANS ENDE der Kette (Owner-E2) und traegt Ebene 1.
static_assert(stamp_entry_meta_level(kStampEntryBracketProbe[3]) == 1u);
static_assert(stamp_entry_is_meta_meta(kStampEntryBracketProbe[3]));
static_assert(std::string_view(kStampEntryBracketProbe[3].axis, kStampEntryBracketProbe[3].axis_len) == "simd");
static_assert(std::string_view(kStampEntryBracketProbe[3].algorithm, kStampEntryBracketProbe[3].algo_len) == "code");
static_assert(kStampEntryBracketProbe[3].x == 1u && kStampEntryBracketProbe[3].y == 0u &&
              kStampEntryBracketProbe[3].z == 0u);
// Die Klammer-Zeichen selbst gehoeren NIE in einen Namen -- der Anhang ist genau EIN Eintrag, kein Rest.
static_assert(std::string_view(kStampEntryBracketProbe[2].axis, kStampEntryBracketProbe[2].axis_len) ==
              "external_utils");

// (3) OFFENE REKURSION (Layer-Modell D4 / Owner Q-D): die Klammer-ANZAHL kodiert die Ebene, ohne dass ein
//     drittes Level irgendwo deklariert waere. Ein Glied, das selbst Hub ist, behaelt seine eigene Klammer.
inline constexpr char kStampEntryNestedLine[] = "external_utils=code@1.0.0;[gpu=code@2.0.0;[nvlink=code@3.0.0]]";
inline constexpr auto kStampEntryNestedProbe =
    parse_stamp_entries<count_stamp_entries(std::string_view{kStampEntryNestedLine})>(kStampEntryNestedLine);

static_assert(kStampEntryNestedProbe.size() == 3);
static_assert(stamp_entry_meta_level(kStampEntryNestedProbe[0]) == 0u); // external_utils (Haupt-Achse)
static_assert(stamp_entry_meta_level(kStampEntryNestedProbe[1]) == 1u); // gpu (Meta-Meta)
static_assert(stamp_entry_meta_level(kStampEntryNestedProbe[2]) == 2u); // nvlink (Meta-Meta-Meta)
static_assert(std::string_view(kStampEntryNestedProbe[1].axis, kStampEntryNestedProbe[1].axis_len) == "gpu");
static_assert(std::string_view(kStampEntryNestedProbe[2].axis, kStampEntryNestedProbe[2].axis_len) == "nvlink");
// Die Ebenen sind im reserved-Feld paarweise verschieden -- sonst waeren zwei Ebenen im POD ununterscheidbar.
static_assert(kStampEntryNestedProbe[0].reserved != kStampEntryNestedProbe[1].reserved);
static_assert(kStampEntryNestedProbe[1].reserved != kStampEntryNestedProbe[2].reserved);
// Ebene und Flag-Bits ueberlappen NICHT: eine geklammerte 'ce'-Version traegt beide Gruppen zugleich.
inline constexpr char kStampEntryLevelFlagLine[] = "[a=x@1.0.0ce]";
inline constexpr auto kStampEntryLevelFlagProbe =
    parse_stamp_entries<count_stamp_entries(std::string_view{kStampEntryLevelFlagLine})>(kStampEntryLevelFlagLine);
static_assert(kStampEntryLevelFlagProbe.size() == 1);
static_assert(stamp_entry_is_experimental(kStampEntryLevelFlagProbe[0]));
static_assert(stamp_entry_meta_level(kStampEntryLevelFlagProbe[0]) == 1u);
static_assert(kStampEntryLevelFlagProbe[0].reserved ==
              (kStampEntryFlagExperimental | (1u << kStampEntryMetaLevelShift)));
// Die MASKEN der drei Bit-Gruppen sind disjunkt (Bit 0 / Bits 1-2 / Bits 3-5).
static_assert((kStampEntryFlagExperimental & kStampEntryHwFlagMask) == 0u);
static_assert((kStampEntryFlagExperimental & kStampEntryMetaLevelMask) == 0u);
static_assert((kStampEntryHwFlagMask & kStampEntryMetaLevelMask) == 0u);
} // namespace detail

} // namespace comdare::cache_engine::abi
