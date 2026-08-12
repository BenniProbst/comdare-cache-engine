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
// NAMENS-TOLERANZ (Owner-Nachtrag Q2 vom 02.08.2026): erweiterte HIERARCHISCHE Algorithmus-/Achsen-Namen nach dem
// Muster "prt-art.memory.abc@1.0.0" sind zulaessig. Der Tokenizer teilt strikt an '=' (erstes) und '@' (erstes nach
// dem '='), danach liest parse_algo_semver -- ein '.' im NAMENS-Anteil VOR dem '@' ist damit transparenter
// Namens-Bestandteil, ein '.' im VERSIONS-Anteil gehoert der Versions-Grammatik (drei Zahlen, danach der
// Flag-Schwanz; sonst Sentinel).
//
// == FLAG-GRAMMATIK v2 (Owner-KERN 07.08.2026) -- WAS SICH HIER GEAENDERT HAT ======================
// Der Versions-Anteil eines entry traegt jetzt "X.Y.Z" plus null bis n punkt-getrennte Flags mit optionalen,
// rekursiven Klammer-Gruppen ("achse=algo@1.0.0.c{p.e}.x512{f.vl}.gfni"). Die Grammatik selbst lebt EINMAL,
// in measurement/algo_semver.hpp; dieser Header ruft sie und kodiert ihr Ergebnis in den POD.
//   * ENTFALLEN: das Experimental-Bit 0 (das Konzept "experimental" ist ersatzlos deprecated -- 'e' bedeutet
//     ab jetzt EFFICIENCY CORE) und der 4-Werte-Hardware-Code in den Bits 1-2 (die Hardware-Zielrichtung ist
//     kein Skalar mehr, sondern eine Liste). Die drei Bits stehen bewusst LEER (kStampEntryReservedFreeMask).
//   * NEU: die Flag-Liste reist als HASH in den Bits 6-31 (Owner-Entscheid F-3: "Hash der normalisierten
//     Flag-Zeichenkette, nicht als Bitmaske ueber einen festen Katalog"). Begruendung, Grenze und die
//     Frage, warum die Kollisions-Moeglichkeit hier tragbar ist, stehen bei kStampEntryFlagsHashShift.
//   * UNVERAENDERT: die Bits 3-5 (Meta-Meta-Ebene) und das 48-Byte-Layout des Entry-POD. Es gibt weiterhin
//     KEINEN sizeof-/Layout-Bruch und keine Entry-Layout-Versions-Folge.
// BYTE-WIRKUNG, ehrlich benannt: eine FLAGLOSE Version laesst reserved (ausser der Meta-Ebene) weiterhin
// exakt 0. Der migrierte Bestand traegt "@1.0.0.c" und damit einen Flag-Hash != 0 -- das POD-Feld bewegt
// sich also, gemeinsam mit der Stempel-ZEILE, im selben deklarierten Byte-Ereignis.
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
//   entry    := achse '=' algorithmus '@' <version>   -- <version> == Flag-Grammatik v2, s. algo_semver.hpp
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
// Im Entry-POD reist die Ebene in den reserved-BITS 3-5. Ebene 0 == Bit-Muster 0 -> der klammerlose
// Bestand bleibt exakt bei reserved == 0 (Byte-Gleichheit unveraendert; das 'c' des Bestands seit
// A13-M3/C4 kodiert ebenfalls auf 0, s. Bits 1-2 unten).
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
//     einen leeren Rahmen (meta_meta_stamp_suffix); die Grammatik zementiert diese Zusage,
//   * (A13-M3/C2b, Befund Z-02) die DIREKT AUFEINANDER FOLGENDEN GESCHWISTER-GRUPPEN
//     "a=b@1.0.0;[c=d@1.0.0];[e=f@1.0.0]" -- sie tragen dieselben Glieder auf derselben Ebene wie die
//     kanonische EIN-Gruppen-Form "a=b@1.0.0;[c=d@1.0.0;e=f@1.0.0]" und lieferten bis C2b dasselbe
//     (Text, Ebene)-Entry-Array. Damit galt die Verlustfreiheits-Zusage wieder nur fuer kanonische Zeilen.
//     Die Verschaerfung erzwingt die EIN-Gruppen-Form je Anhang-Position, und genau die erzeugt der
//     Renderer ohnehin: append_meta_meta_suffix haengt genau EINE Gruppe ans Realm-Zeilen-Ende, und
//     innerhalb einer Gruppe stehen zwischen zwei Untergruppen immer die entries ihrer Eltern-Glieder.
//     ABGEGRENZT: verboten ist NUR die unmittelbare Folge (Gruppe, Gruppe) auf einer Ebene. Zwei Gruppen
//     mit einem entry dazwischen ("a;[b];x;[c]") bleiben zulaessig -- sie sind NICHT mehrdeutig, weil die
//     Entry-REIHENFOLGE sie von jeder Zusammenfassung unterscheidet.
// Die Fehlformen sind unten als CT-NEGATIVPROBEN (stamp_line_is_parsable) festgenagelt -- die Wachen sind
// damit nicht bloss behauptet, sondern bewiesen.
//
// == A13-M3/C2b: STRENGE AUCH INNERHALB EINES SEGMENTS (Befund Z-09) ==============================
// Bis C2b war der Zeilen-Scanner streng und der ENTRY-Parser fail-open: ein Segment ohne '=' oder ohne
// '@' und ein ungueltiger Flag-Schwanz ("a=x@1.0.0cg") fielen still auf leere Felder bzw. den
// @0.0.0-Sentinel. Zwei byte-VERSCHIEDENE Defekt-Formen kollabierten damit im POD in denselben Wert --
// dieselbe Identitaets-Kollision wie F4/F6, nur eine Grammatik-Ebene tiefer. Ab C2b bricht auch das
// entry-seitig hart. Byte-neutral: jede kanonisch gerenderte Zeile ist wohlgeformt (flaglos wie mit 'c'),
// und das dokumentierte Sentinel-Rendering "@0.0.0" (axis_variant_version_table: versionslose Eintraege)
// bleibt ausdruecklich zulaessig -- verboten ist der UNPARSBARE Rest, nicht die Null-Version.

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // AnatomyStampEntryV1
#include <cache_engine/abi/stempel_basis.hpp>              // S-1: ist_stempel_baustein (Baustein-Anbindung)
#include <cache_engine/measurement/algo_semver.hpp>        // parse_algo_semver + render_flag_tail

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::abi {

/// FLAG-GRAMMATIK v2 (Owner-KERN 07.08.2026): BITS 0-2 SIND FREI. Bis zur v2 trug Bit 0 die Markierung
/// "experimenteller Achsen-Algorithmus" und die Bits 1-2 das EINE Hardware-Flag als 4-Werte-Code. Beides
/// gibt es nicht mehr: 'e' bedeutet EFFICIENCY CORE (das Konzept "experimental" ist ersatzlos deprecated),
/// und die Hardware-Zielrichtung ist kein Skalar mehr, sondern eine LISTE (Kardinalitaet 1 -> n). Die drei
/// Bits bleiben bewusst LEER stehen statt umgewidmet zu werden -- ein Leser, der einen Alt-Eintrag sieht,
/// soll die Stelle finden, an der die alte Bedeutung sass, statt sie mit einer neuen zu verwechseln.
inline constexpr std::uint32_t kStampEntryReservedFreeMask = 0x7u;

/// A13-M2 (Owner-Q1): BITS 3-5 == die META-META-EBENE == die KLAMMER-TIEFE des Eintrags. 0 == Haupt-Achse
/// (klammerlos); System- und Mess-Zeile tragen seit A13-M2 je EINE Ebene-1-Gruppe ("[simd=code@1.0.0.c]" bzw.
/// "[load_framework=ycsb@1.0.0.c]"), die Organ-Zeile bleibt klammerlos (OrganMetaMetas ist leer).
/// 1 == Meta-Meta, 2 == Meta-Meta-Meta, ... Shift + Maske sind die
/// EINZIGE Wahrheit der Lage, kStampEntryMaxMetaLevel die der Kapazitaet (tiefer bricht der Parser hart).
inline constexpr std::uint32_t kStampEntryMetaLevelShift = 3u;
inline constexpr std::uint32_t kStampEntryMetaLevelMask  = 0x7u << kStampEntryMetaLevelShift;
inline constexpr std::uint32_t kStampEntryMaxMetaLevel   = 7u;

/// FLAG-GRAMMATIK v2 -- BITS 6-31 == der HASH der Flag-Liste (Owner-Entscheid F-3 vom 07.08.2026 verbatim:
/// "Die Flag-Menge wird als Hash der normalisierten Flag-Zeichenkette kodiert, nicht als Bitmaske ueber
/// einen festen Katalog. Damit ist die Katalog-Groesse nicht gedeckelt.").
///
/// WARUM EIN HASH UND KEINE BITMASKE: die Flag-Menge ist ab der v2 offen (Kardinalitaet 1 -> n, rekursive
/// Sub-Token, ein Katalog, der erst noch recherchiert wird). Eine Bitmaske haette den Katalog auf 26
/// Eintraege gedeckelt und muesste bei jedem neuen SIMD-Subset das POD-Layout anfassen.
///
/// EHRLICHE GRENZE, weil sie zur Sache gehoert: 26 Bit sind KEINE kollisionsfreie Kodierung. Zwei
/// verschiedene Flag-Listen koennen denselben Hash tragen. Das ist tragbar, und zwar aus einem Grund, der
/// nachpruefbar ist und nicht aus Bequemlichkeit stammt: die IDENTITAET eines Stempels haengt NICHT an
/// diesem Feld, sondern an der ZEILE. Der Fingerprint (abi/anatomy_fingerprint.hpp) rechnet SHA-512 ueber
/// die vollstaendige Zeichenfolge der Realm-Zeilen -- dort reist der Flag-Schwanz Zeichen fuer Zeichen mit.
/// Dieses Feld ist ein SCHNELLER VERGLEICHS-/FILTER-Wert auf dem geparsten POD, kein Schluessel, an dem das
/// Skip-Gate haengt. Waere es einer, waere die Kollision fail-open und der Hash die falsche Wahl.
///
/// Shift und Maske sind die EINZIGE Wahrheit der Lage; kStampEntryFlagsHashNone die der Leer-Bedeutung.
inline constexpr std::uint32_t kStampEntryFlagsHashShift = 6u;
inline constexpr std::uint32_t kStampEntryFlagsHashBits  = 26u;
inline constexpr std::uint32_t kStampEntryFlagsHashSpan  = (1u << kStampEntryFlagsHashBits) - 1u;
inline constexpr std::uint32_t kStampEntryFlagsHashMask  = kStampEntryFlagsHashSpan << kStampEntryFlagsHashShift;

/// Der Hash-Wert einer LEEREN Flag-Liste. Er ist EXAKT 0 und wird von keiner nicht-leeren Liste erreicht
/// (s. stamp_entry_flags_hash_of): "keine Flags" und "irgendwelche Flags" sind damit im POD sicher
/// unterscheidbar, statt sich mit Wahrscheinlichkeit 2^-26 zu verwechseln. Folge, die das mitbringt: eine
/// flaglose Version laesst reserved (ausser der Meta-Ebene) exakt 0 -- so wie der gesamte Bestand vor der
/// v2 dastand.
inline constexpr std::uint32_t kStampEntryFlagsHashNone = 0u;

/// FNV-1a (32 Bit) ueber die KANONISCHE Flag-Zeichenkette, auf kStampEntryFlagsHashBits gefaltet und auf
/// [1, kStampEntryFlagsHashSpan] abgebildet.
///
/// DIE ZEICHENKETTE KOMMT AUS measurement::render_flag_tail -- also aus DEMSELBEN Renderer, der sie in die
/// Stempel-Zeile schreibt. Ein hier nachgebauter Zusammenbau waere eine zweite Wahrheit ueber die
/// kanonische Schreibweise; genau daran ist die Q3-Welt am CEB-Zwilling schon einmal entlanggeschrammt.
///
/// Die XOR-Faltung (h >> Bits) ^ h vor der Maske ist kein Zierrat: eine nackte Maske wuerfe die oberen
/// 6 Bit weg, in denen FNV-1a den groessten Teil seiner Streuung traegt.
/// Die Abbildung auf [1, Span] statt [0, Span] haelt die 0 fuer den Leer-Fall frei (s.o.).
[[nodiscard]] constexpr std::uint32_t
stamp_entry_flags_hash_of(::comdare::cache_engine::measurement::FlagList const& flags) noexcept {
    // Der Puffer MUSS benannt werden: .view() zeigt in ihn hinein, und ein Temporary waere am Ende des
    // vollen Ausdrucks tot. Genau diese Klasse beschreibt measurement::FlagToken in ihrer Begruendung,
    // warum die Token als KOPIE im Knoten liegen -- sie faellt hier ein zweites Mal an.
    auto const             puffer    = ::comdare::cache_engine::measurement::render_flag_tail(flags);
    std::string_view const kanonisch = puffer.view();
    if (kanonisch.empty()) return kStampEntryFlagsHashNone;
    std::uint32_t h = 2166136261u; // FNV-1a offset basis
    for (char const c : kanonisch) {
        h ^= static_cast<std::uint32_t>(static_cast<unsigned char>(c));
        h *= 16777619u; // FNV-1a prime
    }
    std::uint32_t const gefaltet = ((h >> kStampEntryFlagsHashBits) ^ h) & kStampEntryFlagsHashSpan;
    return 1u + (gefaltet % kStampEntryFlagsHashSpan);
}

/// Der Flag-Hash eines Eintrags -- Praedikat statt Bit-Fummelei am Aufruf-Ort (Konsumenten:
/// Lager-Identitaet/Skip-Gate, G-E6/A2).
[[nodiscard]] constexpr std::uint32_t stamp_entry_flags_hash(AnatomyStampEntryV1 const& e) noexcept {
    return (e.reserved & kStampEntryFlagsHashMask) >> kStampEntryFlagsHashShift;
}

/// Traegt der Eintrag ueberhaupt Flags? (Verlaesslich, nicht wahrscheinlich -- s. kStampEntryFlagsHashNone.)
[[nodiscard]] constexpr bool stamp_entry_has_flags(AnatomyStampEntryV1 const& e) noexcept {
    return stamp_entry_flags_hash(e) != kStampEntryFlagsHashNone;
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

// -- S-1 (P2): Baustein-Anbindung UNTER der Definition -- OHNE Basisklassen-Einbau (StampSegment ist ein
//    positional-init-Aggregat; eine leere Basis fraesse den ersten Initialisierer). Die Spezialisierung
//    gehoert in den abi-Namensraum, deshalb das kurze Anbindungs-Fenster.
} // namespace detail (S-1-Anbindungs-Fenster)
template <>
struct ist_stempel_baustein<detail::StampSegment> : StempelBausteinTag<StempelBausteinRolle::Segment> {};
static_assert(StempelBaustein<detail::StampSegment>);
namespace detail {

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
///   (F6) A13-M3/C2b-HAERTUNG (Befund Z-02): zwei DIREKT aufeinander folgende Geschwister-GRUPPEN auf
///        derselben Ebene ("a=b@1.0.0;[c=d@1.0.0];[e=f@1.0.0]"). Sie sind byte-verschieden zur
///        kanonischen EIN-Gruppen-Form "a=b@1.0.0;[c=d@1.0.0;e=f@1.0.0]", lieferten aber bis C2b
///        dasselbe (Text, Ebene)-Entry-Array -- dieselbe Verlust-Stelle wie F4/F4b, nur an der
///        Gruppen-GRENZE statt an der Klebe-Naht.
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
/// F6 schliesst die verbleibende Grenz-Mehrdeutigkeit: das VORHERGEHENDE Geschwister-Segment einer Gruppe
/// darf keine Gruppe sein. Der Renderer erzeugt genau diese Form -- append_meta_meta_suffix haengt EINE
/// Gruppe ans Realm-Zeilen-Ende, und innerhalb einer Gruppe trennt immer ein entry zwei Untergruppen
/// (render_meta_meta_entry haengt die Untergruppe an SEIN eigenes entry). Zwei Gruppen mit einem entry
/// dazwischen ("a;[b];x;[c]") bleiben zulaessig: die Entry-REIHENFOLGE unterscheidet sie eindeutig von
/// jeder Zusammenfassung, es gibt dort also nichts zu verlieren.
///
/// GA-06 (A13-M3/C2b): CONSTEVAL, nicht constexpr. Der Scanner meldet seine Fehlformen als nackten
/// `throw "..."` (char const*) -- das ist im consteval-Pfad ein sauberer Compile-Fehler mit lesbarem
/// Text, an einem LAUFZEIT-Aufrufer aber ein Wurf, der durch jedes `catch (std::exception const&)`
/// hindurchfiele. Beide Aufrufer (count_stamp_entries/parse_stamp_entries) sind heute consteval, die
/// Aenderung hat also null Byte-Wirkung -- sie schliesst die Tuer, bevor die POD-Konsumenten des
/// SHA512-Gates kommen.
template <class Visitor>
consteval void scan_stamp_segments(std::string_view line, Visitor&& visit) {
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
    // (F6) War das ZULETZT abgeschlossene Geschwister-Segment dieser Ebene eine GRUPPE? Nur dann ist eine
    // unmittelbar folgende Gruppe die mehrdeutige Form. Ein dazwischenliegendes entry setzt das Flag
    // zurueck -- "a;[b];x;[c]" bleibt damit zulaessig, "a;[b];[c]" nicht.
    std::array<bool, kStampEntryMaxMetaLevel + 1> voriges_geschwister_war_gruppe{};
    auto const                                    flush = [&](std::size_t end) {
        if (end > start) {
            // (F4b) Ein Segment darf nie direkt hinter einem ']' beginnen -- das waere die geklebte Form
            // spiegelverkehrt ("[a=b@1.0.0]c=d@1.0.0" statt "[a=b@1.0.0];c=d@1.0.0").
            if (last_delim == ']')
                throw "A13-M2 Klammer-Grammatik: Segment direkt hinter ']' geklebt -- nach einer Gruppe folgt "
                      "';', eine weitere schliessende Klammer oder das Zeilenende";
            visit(StampSegment{start, end, depth});
            ++seen[depth];
            voriges_geschwister_war_gruppe[depth] = false;
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
            // (F6) ... und nie DIREKT hinter einem Geschwister-group derselben Ebene: "[c];[e]" traegt
            // exakt dieselben Glieder auf derselben Ebene wie die kanonische Form "[c;e]".
            if (voriges_geschwister_war_gruppe[depth])
                throw "A13-M3 Klammer-Grammatik: zwei aufeinander folgende Geschwister-Gruppen auf derselben "
                      "Ebene -- die kanonische Form ist EINE Gruppe je Anhang-Position ('[c;e]' statt "
                      "'[c];[e]'); beide Formen ergaeben sonst dasselbe Entry-Array";
            flush(pos);
            ++depth;
            if (depth > kStampEntryMaxMetaLevel)
                throw "A13-M2 Klammer-Grammatik: Meta-Meta-Ebene > 7 passt nicht in die reserved-Bits 3-5";
            seen[depth]                           = 0;
            voriges_geschwister_war_gruppe[depth] = false; // die neue Ebene beginnt ohne Geschwister
            start                                 = pos + 1;
            last_delim                            = '[';
        } else if (c == ']') {
            flush(pos);
            if (depth == 0) throw "A13-M2 Klammer-Grammatik: schliessende Klammer ohne oeffnende";
            // (F5) Eine Gruppe ohne ein einziges Segment ist keine Ebene, sondern ein leerer Rahmen.
            if (seen[depth] == 0)
                throw "A13-M2 Klammer-Grammatik: leere Gruppe '[]' -- der Renderer liefert fuer ein Blatt einen "
                      "LEEREN Anhang, nie einen leeren Rahmen";
            --depth;
            ++seen[depth]; // das gerade geschlossene group ist ein Segment SEINER Eltern-Ebene.
            voriges_geschwister_war_gruppe[depth] = true; // ... und ist damit das vorige Geschwister (F6).
            start                                 = pos + 1;
            last_delim                            = ']';
        }
    }
    if (depth != 0) throw "A13-M2 Klammer-Grammatik: offene Klammer am Zeilenende";
    flush(line.size());
}

/// Das GERENDERTE Sentinel-Literal der Versions-Bezifferung. Es ist die EINZIGE Zeichenfolge, die auf den
/// {0,0,0}-Sentinel parsen DARF; jede andere, die dort landet, ist unparsbarer Rest.
///
/// FLAG-GRAMMATIK v2: hier stand bis zur v2 ein EIGENES Literal "0.0.0" neben dem rohen
/// kAlgoSemVerSentinelLiteral ("v0.0.0") -- zwei Wortlaute, weil rohe und gerenderte Form sich um das 'v'
/// unterschieden. Mit dem Wegfall des 'v'-Praefixes fallen beide zusammen; ein zweites Literal waere ab
/// jetzt eine zweite Wahrheit. Es steht deshalb nur noch der VERWEIS auf die Single-Source.
inline constexpr std::string_view kStampEntryRenderedSentinel =
    ::comdare::cache_engine::measurement::kAlgoSemVerSentinelLiteral;

/// A13-M3/C2b (Befund Z-09): der Versions-Teil ist wohlgeformt, wenn er entweder auf einen echten Wert
/// parst ODER exakt das dokumentierte Sentinel-Rendering ist. Ohne diese Unterscheidung koennte man den
/// Sentinel nicht von einer Fehlform wie "1.0.0.c{" trennen, denn beide parsen auf {0,0,0}.
///
/// FLAG-GRAMMATIK v2: das ist ab jetzt WORTGLEICH die B12-Doktrin
/// measurement::version_is_parsable_or_documented_sentinel -- die frueher getrennte "gerenderte Seite der
/// Grammatik" gibt es nicht mehr. Die Funktion bleibt als abi-seitiger NAME stehen (die Aufrufer unten
/// lesen sich damit an der Stempel-Grammatik entlang), delegiert aber, statt die Bedingung ein zweites
/// Mal zu formulieren.
[[nodiscard]] constexpr bool dotted_version_is_wellformed(std::string_view ver) noexcept {
    return ::comdare::cache_engine::measurement::version_is_parsable_or_documented_sentinel(ver);
}

/// Materialisiert EIN Segment [begin, end) des Literals in einen Entry-POD: axis = vor '=', algorithm =
/// zwischen '=' und '@', Version = nach '@' (via parse_dotted_semver). axis/algorithm sind {ptr,len}-Sichten
/// INS Literal (das im Aufrufer static constexpr liegt -> Zeiger ueberleben; K7b-3-Praezedenz).
///
/// A13-M3/C2b (Befund Z-09): HART statt fail-open. Bis C2b fielen ein fehlendes '=', ein fehlendes '@' und
/// ein unparsbarer Versions-Rest still auf leere Laengen bzw. den @0.0.0-Sentinel -- zwei byte-VERSCHIEDENE
/// Defekt-Formen ergaben denselben POD. Das ist exakt die Kollision, die der Scanner eine Ebene hoeher (F4/
/// F4b/F5/F6) verbietet, und sie wiegt hier schwerer: der Entry-POD IST ab A13-M3 die Lager-Identitaet. Der
/// Header sagt seine Strenge seit A13-M2/B2 selbst zu ("der Stempel ist Identitaet -- er darf nie still
/// falsch reisen"); C2b macht sie entry-seitig wahr. BYTE-NEUTRAL: jede kanonisch gerenderte Zeile traegt
/// '='/'@' und eine parsbare Version (flaglos wie mit 'c'), und "@0.0.0" bleibt als dokumentiertes
/// Sentinel-Rendering ausdruecklich zulaessig. CONSTEVAL wie der Scanner (GA-06): der Wurf ist ein
/// Compile-Fehler mit Text, kein Laufzeit-Wurf am catch(std::exception const&) vorbei.
[[nodiscard]] consteval AnatomyStampEntryV1 parse_one_stamp_segment(char const* lit, std::size_t seg_start,
                                                                    std::size_t seg_end) {
    std::size_t eq = seg_start;
    while (eq < seg_end && lit[eq] != '=') ++eq;
    if (eq >= seg_end)
        throw "A13-M3 Stempel-Eintrag: Segment ohne '=' -- die Form ist 'achse=algorithmus@X.Y.Z'; ein "
              "Eintrag ohne Achsen-Trenner darf nicht still als namenloser Eintrag reisen";
    std::size_t at = eq + 1;
    while (at < seg_end && lit[at] != '@') ++at;
    if (at >= seg_end)
        throw "A13-M3 Stempel-Eintrag: Segment ohne '@' -- die Versions-Bezifferung ist Pflicht; ein Eintrag "
              "ohne sie darf nicht still auf @0.0.0 kollabieren";
    std::string_view const ver{lit + at + 1, seg_end - (at + 1)};
    if (!dotted_version_is_wellformed(ver))
        throw "Stempel-Eintrag: unparsbarer Versions-Teil (Kurzform, 'v'-Praefix, fehlender Punkt vor "
              "einem Flag, fuehrender Punkt hinter '{', leere oder unbalancierte Klammer, Rest hinter dem "
              "Flag-Schwanz) -- er faellt nicht mehr still auf @0.0.0; zulaessig ist nur ein echtes "
              "X.Y.Z[.flag]* der Flag-Grammatik v2 oder exakt das Sentinel-Rendering '0.0.0'";
    AnatomyStampEntryV1 e{};
    e.axis      = lit + seg_start;
    e.axis_len  = eq - seg_start;
    e.algorithm = lit + eq + 1;
    e.algo_len  = at - (eq + 1);
    ::comdare::cache_engine::measurement::AlgoSemVer const sv =
        ::comdare::cache_engine::measurement::parse_algo_semver(ver);
    e.x = sv.x;
    e.y = sv.y;
    e.z = sv.z;
    // FLAG-GRAMMATIK v2 (Owner-F-3): die Flag-Liste wandert als HASH in die Bits 6-31. Eine flaglose
    // Version und das dokumentierte Sentinel-Rendering "0.0.0" lassen das Feld exakt 0 (die
    // Meta-Ebene-Bits setzt der Aufrufer parse_stamp_entries danach daneben).
    e.reserved |= (stamp_entry_flags_hash_of(sv.flags) << kStampEntryFlagsHashShift) & kStampEntryFlagsHashMask;
    return e;
}

/// A13-M3/C2b: der VOLLE Wohlgeformtheits-Weg einer Zeile -- Scanner UND Entry-Parser. Er ist die Grundlage
/// der CT-Negativproben unten und muss aus zwei Gruenden GENAU SO aussehen:
///   (1) Er darf nicht bloss count_stamp_entries rufen. Das ruft nur scan_stamp_segments; die entry-seitigen
///       Fehlformen (Z-09: fehlendes '='/'@', unparsbarer Versions-Rest) entstehen erst in
///       parse_one_stamp_segment -- jede Z-09-Negativprobe waere sonst blind gruen.
///   (2) Er ist eine NICHT-Template-Funktion (nimmt die Zeile als Argument statt als NTTP). Nur dann faellt
///       ein Fehlform-Wurf in das UNMITTELBARE Substitutions-Umfeld des Default-Template-Arguments unten und
///       wird zur Substitutions-Ablehnung. In einem Funktions-TEMPLATE laege der Wurf im Rumpf einer
///       Instanziierung -- ein harter Fehler, und die '...'-Ueberladung kaeme nie zum Zug.
[[nodiscard]] consteval bool stamp_line_is_wellformed(std::string_view line) {
    scan_stamp_segments(
        line, [line](StampSegment const& seg) { (void)parse_one_stamp_segment(line.data(), seg.begin, seg.end); });
    return true;
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

// -- S-1 (P2): Baustein-Anbindung UNTER der Definition (NTTP-Traeger; kein Basisklassen-Einbau, das
//    braeche die strukturelle NTTP-Form). Partielle Spezialisierung ueber alle Laengen N.
} // namespace detail (S-1-Anbindungs-Fenster)
template <std::size_t N>
struct ist_stempel_baustein<detail::StampLineLiteral<N>> : StempelBausteinTag<StempelBausteinRolle::ZeilenLiteral> {};
static_assert(StempelBaustein<detail::StampLineLiteral<1>>);
namespace detail {

template <StampLineLiteral L, bool = stamp_line_is_wellformed(L.view())>
consteval bool stamp_line_is_parsable_impl(int) {
    return true;
}
template <StampLineLiteral L>
consteval bool stamp_line_is_parsable_impl(...) {
    return false;
}

} // namespace detail

/// true genau dann, wenn die Zeile die A13-M2-Klammer-Grammatik UND (seit A13-M3/C2b) die entry-seitige
/// Strenge erfuellt -- also der volle consteval-Weg count -> scan -> parse_one_stamp_segment durchlaeuft.
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
static_assert(stamp_line_is_parsable<"a=b@1.0.0;[c=d@1.0.0;e=f@1.0.0]">,
              "die KANONISCHE Form zweier Glieder an derselben Anhang-Position: EINE Gruppe (A13-M3/C2b).");
static_assert(stamp_line_is_parsable<"a=b@1.0.0;[c=d@1.0.0];x=y@1.0.0;[e=f@1.0.0]">,
              "zwei Gruppen MIT entry dazwischen bleiben zulaessig -- die Entry-Reihenfolge macht sie "
              "eindeutig (F6 verbietet nur die UNMITTELBARE Folge).");
static_assert(stamp_line_is_parsable<"a=b@0.0.0">,
              "das dokumentierte Sentinel-Rendering '@0.0.0' (versionslose Registry-Eintraege, "
              "axis_variant_version_table) bleibt entry-seitig ausdruecklich zulaessig (A13-M3/C2b).");

// -- NEGATIV (F6, A13-M3/C2b): zwei DIREKT aufeinander folgende Geschwister-Gruppen --
static_assert(!stamp_line_is_parsable<"a=b@1.0.0;[c=d@1.0.0];[e=f@1.0.0]">,
              "Gruppen-GRENZEN-Kollision (Z-02): diese Zeile trug bis A13-M3/C2b dasselbe (Text, Ebene)-"
              "Entry-Array wie 'a=b@1.0.0;[c=d@1.0.0;e=f@1.0.0]' -- zwei byte-verschiedene Zeilen, ein POD. "
              "Kanonisch ist EINE Gruppe je Anhang-Position; genau die erzeugt der Renderer.");
static_assert(!stamp_line_is_parsable<"[[a=b@1.0.0];[c=d@1.0.0]]">, "dieselbe Fehlform eine Ebene tiefer.");

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

// -- NEGATIV (Z-09, A13-M3/C2b): die ENTRY-seitigen Fehlformen. Sie kollabierten bis C2b still auf leere
//    Felder bzw. @0.0.0 -- byte-verschiedene Defekte mit identischem POD, eine Grammatik-Ebene unter F4/F6.
static_assert(!stamp_line_is_parsable<"nur_ein_name">, "Segment ohne '=' faellt nicht mehr auf einen leeren "
                                                       "Algorithmus + @0.0.0 durch.");
static_assert(!stamp_line_is_parsable<"achse=algo">, "Segment ohne '@' faellt nicht mehr auf @0.0.0 durch.");
static_assert(!stamp_line_is_parsable<"achse=algo@1.0.c">, "Kurzform 'X.Y' mit Flag-Schwanz ist unparsbar.");
static_assert(!stamp_line_is_parsable<"a=b@1.0.0;achse=algo">, "auch als ZWEITES Segment einer sonst "
                                                               "wohlgeformten Zeile bricht die Fehlform.");
static_assert(!stamp_line_is_parsable<"a=b@1.0.0;[c=d]">, "und ebenso im Klammer-Anhang.");

// -- NEGATIV (FLAG-GRAMMATIK v2): die ALTE Schreibweise und die neuen Fehlformen ------------------------
// DIESE BATTERIE IST NEU GEDACHT, NICHT UMGESCHRIEBEN. Die alten Negativ-Literale ("a=x@1.0.0cg" = zweites
// Hardware-Flag, "@v1.0.0" = 'v'-Praefix in der gerenderten Form) testeten die Q3-Grammatik. Nach dem
// Umbau sind sie ENTWEDER trivial ("cg" ist heute ein Token wie jedes andere und faellt nur noch am
// FEHLENDEN PUNKT durch, nicht mehr am zweiten Flag) ODER sie haetten weiter gruen gemeldet, ohne die
// Regel zu treffen, fuer die sie einmal standen. Ersetzt sind sie durch je eine Probe auf DIE REGEL, die
// die v2 wirklich durchsetzt.
static_assert(!stamp_line_is_parsable<"a=x@1.0.0c">, "R2: das Flag OHNE fuehrenden Punkt ist die ALTE "
                                                     "Q3-Schreibweise -- es gibt keine Uebergangs-Toleranz.");
static_assert(!stamp_line_is_parsable<"a=x@v1.0.0.c">, "R1: kein 'v'-Praefix, in KEINER Form mehr.");
static_assert(!stamp_line_is_parsable<"a=x@1.0.0.c.{p}">, "R3: die Klammer haengt DIREKT an der Basis, "
                                                          "ohne Punkt dazwischen.");
static_assert(!stamp_line_is_parsable<"a=x@1.0.0.c{.p}">, "R4: hinter '{' steht nie ein fuehrender Punkt.");
static_assert(!stamp_line_is_parsable<"a=x@1.0.0.c{}">, "R4: eine leere Gruppe ist keine Gruppe.");
static_assert(!stamp_line_is_parsable<"a=x@1.0.0.c{p">, "unbalancierte Klammer im Versions-Teil.");
static_assert(!stamp_line_is_parsable<"a=x@1.0.0.{p}">, "Klammer ohne Basis.");
static_assert(!stamp_line_is_parsable<"a=x@1.0.0..c">, "doppelter Punkt vor dem Flag.");
static_assert(!stamp_line_is_parsable<"a=x@1.0.0.avx512_vbmi2">, "R5: der Unterstrich der Katalog-Token "
                                                                 "gehoert nicht in die Notation.");
// -- POSITIV (FLAG-GRAMMATIK v2): die Formen, die der Renderer wirklich erzeugt --
static_assert(stamp_line_is_parsable<"a=x@1.0.0.c">, "die migrierte Bestands-Form.");
static_assert(stamp_line_is_parsable<"a=x@1.0.0.c{p.e}">, "Basis mit Komposit-Klammer.");
static_assert(stamp_line_is_parsable<"memory_layout=Soa@1.0.0.c{p.e}.x512{f.vl.bw.dq}.gfni">,
              "das Owner-Beispiel in voller Laenge -- die '{'/'}' der Versions-Grammatik kollidieren NICHT "
              "mit den '['/']' der Zeilen-Grammatik (der Zeilen-Scanner kennt nur ';', '[' und ']').");
static_assert(stamp_line_is_parsable<"a=x@1.0.0.c;[b=y@1.0.0.c{p}]">,
              "Komposit-Flag INNERHALB eines Meta-Meta-Klammer-Anhangs -- beide Klammer-Ebenen zugleich.");

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

// -- CT-Selbstbeweis des Parsers (Flag-Hash + Owner-Q2-Namens-Toleranz) ----------------------------------
// Der Probe-Literal liegt bewusst NAMESPACE-SCOPE constexpr: nur so ueberleben die {ptr,len}-Sichten des
// consteval-Ergebnisses als Konstant-Ausdruck (dieselbe K7b-3-Praezedenz wie im Makro-Aufrufer).
namespace detail {
// Segment 0: Owner-Q2-Namens-Toleranz + die v2-Voll-Form mit Komposit-Flag. Segment 1: die FLAGLOSE Form
// (bis A13-M3/C4 der Bestand, seither eine reine Parser-Probe fuer Alt-/Fremd-Zeilen -- die Probe BLEIBT,
// weil der Parser sie weiter lesen koennen muss).
inline constexpr char kStampEntryProbeLine[] = "prt-art.memory.abc=prt_patricia@2.3.4.c{p.e};filter=bloom@1.0.0";
inline constexpr auto kStampEntryProbe =
    parse_stamp_entries<count_stamp_entries(std::string_view{kStampEntryProbeLine})>(kStampEntryProbeLine);

static_assert(kStampEntryProbe.size() == 2, "Probe-Zeile hat genau zwei ';'-Segmente.");
// Owner-Q2: hierarchischer Name mit Punkten VOR dem '@' bleibt EIN Achsen-Name (kein Versions-Anteil).
// Das gilt ab der v2 GEGEN eine Grammatik, die den Punkt auch RECHTS vom '@' als Trenner kennt -- die
// Trennung an '@' ist damit die Stelle, an der die beiden Punkt-Bedeutungen auseinandergehalten werden.
static_assert(std::string_view(kStampEntryProbe[0].axis, kStampEntryProbe[0].axis_len) == "prt-art.memory.abc");
static_assert(std::string_view(kStampEntryProbe[0].algorithm, kStampEntryProbe[0].algo_len) == "prt_patricia");
static_assert(kStampEntryProbe[0].x == 2u && kStampEntryProbe[0].y == 3u && kStampEntryProbe[0].z == 4u);
// Die Flag-Liste reist als Hash in den Bits 6-31; die Bits 0-2 bleiben frei, die Bits 3-5 sind die Ebene.
static_assert(stamp_entry_has_flags(kStampEntryProbe[0]));
static_assert(stamp_entry_flags_hash(kStampEntryProbe[0]) ==
              stamp_entry_flags_hash_of(::comdare::cache_engine::measurement::parse_algo_semver("2.3.4.c{p.e}").flags));
static_assert((kStampEntryProbe[0].reserved & kStampEntryReservedFreeMask) == 0u);
static_assert((kStampEntryProbe[0].reserved & kStampEntryMetaLevelMask) == 0u);
// Die flaglose Form: KEIN Flag-Hash -> reserved bleibt exakt 0 (so wie der Bestand vor der v2 dastand).
static_assert(!stamp_entry_has_flags(kStampEntryProbe[1]));
static_assert(kStampEntryProbe[1].reserved == 0u);
static_assert(kStampEntryProbe[1].x == 1u && kStampEntryProbe[1].y == 0u && kStampEntryProbe[1].z == 0u);

// VERSCHIEDENE Flag-Listen belegen das reserved-Feld VERSCHIEDEN -- sonst waeren zwei Hardware-Staende im
// POD ununterscheidbar (Lager-Key-Kollision). Die Probe-Zeile deckt die vier Basen, die Komposit-Form und
// ein Companion-Flag ab; die Aussage ist eine ECHTE Ungleichheits-Pruefung auf den erhobenen Werten, keine
// Nachrechnung der Kodierungs-Formel.
inline constexpr char kStampEntryFlagProbeLine[] =
    "a=x@1.0.0.c;b=y@1.0.0.g;c=z@1.0.0.f;d=w@1.0.0.n;e=v@1.0.0.c{p.e};f=u@1.0.0.c{e};g=t@1.0.0.gfni";
inline constexpr auto kStampEntryFlagProbe =
    parse_stamp_entries<count_stamp_entries(std::string_view{kStampEntryFlagProbeLine})>(kStampEntryFlagProbeLine);

static_assert(kStampEntryFlagProbe.size() == 7);
static_assert(stamp_entry_has_flags(kStampEntryFlagProbe[0]));
static_assert(stamp_entry_has_flags(kStampEntryFlagProbe[6]));
// Paarweise verschieden -- ALLE 21 Paare, nicht nur die Nachbarn (die Nachbar-Pruefung der Q3-Batterie
// haette eine Kollision zwischen 'c' und 'gfni' nicht gesehen).
static_assert(
    [] {
        for (std::size_t i = 0; i < kStampEntryFlagProbe.size(); ++i)
            for (std::size_t j = i + 1; j < kStampEntryFlagProbe.size(); ++j)
                if (kStampEntryFlagProbe[i].reserved == kStampEntryFlagProbe[j].reserved) return false;
        return true;
    }(),
    "zwei verschiedene Flag-Listen duerfen nie dasselbe reserved-Feld belegen.");
// ... und alle verschieden von der FLAGLOSEN Form (deren Hash ist per Konstruktion die 0).
static_assert(
    [] {
        for (auto const& e : kStampEntryFlagProbe)
            if (e.reserved == kStampEntryProbe[1].reserved) return false;
        return true;
    }(),
    "eine Flag-tragende Version darf nie wie die flaglose Form aussehen.");
// 'c{p.e}' und 'c{e}' unterscheiden sich -- der Hash sieht die GANZE Liste, nicht nur die Basis.
static_assert(kStampEntryFlagProbe[4].reserved != kStampEntryFlagProbe[5].reserved);
// 'gfni' ist NICHT 'g' -- der mehrzeichige Token bleibt auch im POD ein eigener Stand.
static_assert(kStampEntryFlagProbe[6].reserved != kStampEntryFlagProbe[1].reserved);

// Der Parser raet NIE ein Flag herbei: das dokumentierte Sentinel-Rendering bleibt zulaessig UND flaglos.
inline constexpr char kStampEntrySentinelLine[] = "a=x@0.0.0";
inline constexpr auto kStampEntrySentinelProbe =
    parse_stamp_entries<count_stamp_entries(std::string_view{kStampEntrySentinelLine})>(kStampEntrySentinelLine);
static_assert(kStampEntrySentinelProbe[0].x == 0u && kStampEntrySentinelProbe[0].y == 0u &&
              kStampEntrySentinelProbe[0].z == 0u);
static_assert(kStampEntrySentinelProbe[0].reserved == 0u);
static_assert(!stamp_entry_has_flags(kStampEntrySentinelProbe[0]));

// -- A13-M2: CT-Selbstbeweis der KLAMMER-Grammatik (Owner-Q1) -------------------------------------------
// (1) Der klammerlose Bestand bleibt Ebene 0 und reserved == 0 -- die Erweiterung ist byte-neutral.
static_assert(stamp_entry_meta_level(kStampEntryProbe[1]) == 0u);
static_assert(!stamp_entry_is_meta_meta(kStampEntryProbe[1]));
static_assert(!stamp_entry_is_meta_meta(kStampEntryProbe[0])); // Flags setzen den Hash, aber KEINE Ebene

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
// Ebene und Flag-Hash ueberlappen NICHT: eine geklammerte Version mit Komposit-Flag traegt beide Gruppen
// zugleich, ohne dass eine die andere ueberschreibt.
inline constexpr char kStampEntryLevelFlagLine[] = "[a=x@1.0.0.c{p.e}]";
inline constexpr auto kStampEntryLevelFlagProbe =
    parse_stamp_entries<count_stamp_entries(std::string_view{kStampEntryLevelFlagLine})>(kStampEntryLevelFlagLine);
static_assert(kStampEntryLevelFlagProbe.size() == 1);
static_assert(stamp_entry_has_flags(kStampEntryLevelFlagProbe[0]));
static_assert(stamp_entry_meta_level(kStampEntryLevelFlagProbe[0]) == 1u);
// Derselbe Flag-Stand wie in der KLAMMERLOSEN Probe oben -- die Ebene aendert den Hash nicht, und der Hash
// nicht die Ebene.
static_assert(stamp_entry_flags_hash(kStampEntryLevelFlagProbe[0]) == stamp_entry_flags_hash(kStampEntryProbe[0]));
static_assert(kStampEntryLevelFlagProbe[0].reserved !=
              kStampEntryProbe[0].reserved); // ... verschieden bleiben sie trotzdem (die Ebene)
static_assert((kStampEntryLevelFlagProbe[0].reserved & kStampEntryReservedFreeMask) == 0u);
// Die MASKEN der drei Bit-Gruppen sind disjunkt und decken zusammen das ganze Wort ab (Bits 0-2 frei /
// Bits 3-5 Ebene / Bits 6-31 Flag-Hash). Die Vollstaendigkeits-Zeile ist neu: sie faengt eine kuenftige
// Verschiebung, bei der zwischen zwei Gruppen ein unbenanntes Loch entstuende.
static_assert((kStampEntryReservedFreeMask & kStampEntryMetaLevelMask) == 0u);
static_assert((kStampEntryReservedFreeMask & kStampEntryFlagsHashMask) == 0u);
static_assert((kStampEntryMetaLevelMask & kStampEntryFlagsHashMask) == 0u);
static_assert((kStampEntryReservedFreeMask | kStampEntryMetaLevelMask | kStampEntryFlagsHashMask) == 0xFFFFFFFFu);
} // namespace detail

} // namespace comdare::cache_engine::abi
