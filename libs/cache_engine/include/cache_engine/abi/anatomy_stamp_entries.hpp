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
// A13-M1 (Owner-Entscheid E2 vom 02.08.2026): das 'e'-Suffix der Versionsbezifferung ("achse=algo@X.Y.Ze") markiert
// einen EXPERIMENTELLEN Achsen-Algorithmus aus einem Pruefling. Im Entry-POD reist die Markierung im dafuer
// vorgesehenen reserved-Feld (anatomy_module_abi_v1_decl.hpp: "0 (Ausrichtung / kuenftige Flags)") als BIT 0 --
// kein sizeof-/Layout-Bruch am 48-Byte-Entry-POD, keine Entry-Layout-Versions-Folge. Ohne 'e' bleibt reserved == 0
// -> der gesamte Bestand ist byte-identisch (golden-neutral).
//
// NAMENS-TOLERANZ (Owner-Nachtrag Q2 vom 02.08.2026): erweiterte HIERARCHISCHE Algorithmus-/Achsen-Namen nach dem
// Muster "prt-art.memory.abc@1.0.0" sind zulaessig. Der Tokenizer teilt strikt an '=' (erstes) und '@' (erstes nach
// dem '='), danach liest parse_dotted_semver -- ein '.' im NAMENS-Anteil VOR dem '@' ist damit transparenter
// Namens-Bestandteil, ein '.' im VERSIONS-Anteil bleibt reiner Zahlen-Trenner (drei Zahlen, sonst Sentinel).

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // AnatomyStampEntryV1
#include <cache_engine/measurement/algo_semver.hpp>        // parse_dotted_semver (X.Y.Z-Ruecklesung)

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::abi {

/// A13-M1: Bit 0 des AnatomyStampEntryV1::reserved-Feldes == "experimenteller Achsen-Algorithmus" ('e'-Suffix der
/// Versionsbezifferung, Owner-E2). Die uebrigen 31 Bits bleiben 0/reserviert. EINZIGE Wahrheit dieser Belegung.
inline constexpr std::uint32_t kStampEntryFlagExperimental = 1u << 0;

/// Praedikat statt Bit-Fummelei am Aufruf-Ort (Konsumenten: Lager-Identitaet/Skip-Gate, spaeter G-E6/A2).
[[nodiscard]] constexpr bool stamp_entry_is_experimental(AnatomyStampEntryV1 const& e) noexcept {
    return (e.reserved & kStampEntryFlagExperimental) != 0u;
}

/// Zaehlt die ';'-getrennten Eintraege einer Stempel-Zeile. Leere Zeile -> 0 (kein Eintrag). Die gerenderten Zeilen
/// (build_axis_version_stamp_line) haben nie ein Trailing-';', daher: Eintraege == 1 + Anzahl der ';'.
[[nodiscard]] consteval std::size_t count_stamp_entries(std::string_view line) noexcept {
    if (line.empty()) return 0;
    std::size_t n = 1;
    for (char const c : line)
        if (c == ';') ++n;
    return n;
}

/// Parst die Stempel-Zeile aus dem Literal `lit` (nullterminiert, effektive Laenge M-1) in genau N Eintraege
/// (N == count_stamp_entries(lit)). Jedes ';'-Segment ist "achse=algorithmus@X.Y.Z": axis = vor '=', algorithm =
/// zwischen '=' und '@', Version = nach '@' (via parse_dotted_semver). axis/algorithm sind {ptr,len}-Sichten INS
/// Literal (das im Aufrufer static constexpr liegt -> Zeiger ueberleben; K7b-3-Praezedenz). Fehlt '='/'@' in einem
/// Segment, bleiben die jeweiligen Laengen/Version 0 (defensiv; die gerenderten Zeilen sind stets wohlgeformt).
template <std::size_t N, std::size_t M>
[[nodiscard]] consteval std::array<AnatomyStampEntryV1, N> parse_stamp_entries(char const (&lit)[M]) noexcept {
    std::array<AnatomyStampEntryV1, N> out{};
    std::size_t const                  len       = M - 1; // ohne '\0'
    std::size_t                        idx       = 0;
    std::size_t                        seg_start = 0;
    for (std::size_t pos = 0; pos <= len; ++pos) {
        if (pos != len && lit[pos] != ';') continue;
        std::size_t const seg_end = pos; // Segment [seg_start, seg_end)
        // '=' innerhalb des Segments finden.
        std::size_t eq = seg_start;
        while (eq < seg_end && lit[eq] != '=') ++eq;
        // '@' nach dem '=' finden.
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
            // A13-M1: das 'e'-Suffix wandert als Bit 0 ins reserved-Feld (ohne 'e' bleibt reserved == 0).
            if (sv.experimental) e.reserved |= kStampEntryFlagExperimental;
        }
        if (idx < N) out[idx] = e;
        ++idx;
        seg_start = pos + 1;
    }
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

// -- A13-M1: CT-Selbstbeweis des Parsers (Experimental-Bit + Owner-Q2-Namens-Toleranz) -------------------------
// Der Probe-Literal liegt bewusst NAMESPACE-SCOPE constexpr: nur so ueberleben die {ptr,len}-Sichten des
// consteval-Ergebnisses als Konstant-Ausdruck (dieselbe K7b-3-Praezedenz wie im Makro-Aufrufer).
namespace detail {
inline constexpr char kStampEntryProbeLine[] = "prt-art.memory.abc=prt_patricia@2.3.4e;filter=bloom@1.0.0";
inline constexpr auto kStampEntryProbe =
    parse_stamp_entries<count_stamp_entries(std::string_view{kStampEntryProbeLine})>(kStampEntryProbeLine);

static_assert(kStampEntryProbe.size() == 2, "Probe-Zeile hat genau zwei ';'-Segmente.");
// Owner-Q2: hierarchischer Name mit Punkten VOR dem '@' bleibt EIN Achsen-Name (kein Versions-Anteil).
static_assert(std::string_view(kStampEntryProbe[0].axis, kStampEntryProbe[0].axis_len) == "prt-art.memory.abc");
static_assert(std::string_view(kStampEntryProbe[0].algorithm, kStampEntryProbe[0].algo_len) == "prt_patricia");
static_assert(kStampEntryProbe[0].x == 2u && kStampEntryProbe[0].y == 3u && kStampEntryProbe[0].z == 4u);
// A13-M1: 'e' -> Bit 0; ohne 'e' bleibt reserved sauber 0 (Bestands-Byte-Gleichheit).
static_assert(stamp_entry_is_experimental(kStampEntryProbe[0]));
static_assert(kStampEntryProbe[0].reserved == kStampEntryFlagExperimental);
static_assert(!stamp_entry_is_experimental(kStampEntryProbe[1]));
static_assert(kStampEntryProbe[1].reserved == 0u);
static_assert(kStampEntryProbe[1].x == 1u && kStampEntryProbe[1].y == 0u && kStampEntryProbe[1].z == 0u);
} // namespace detail

} // namespace comdare::cache_engine::abi
