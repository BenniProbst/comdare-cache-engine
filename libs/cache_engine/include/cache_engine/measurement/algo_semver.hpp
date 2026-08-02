// measurement/algo_semver.hpp -- X.Y.Z-Interpretation des algo_version-Strings (Bau W12-A, Section43.b).
//
// Section43.b (User-Direktive): Versionen sind X.Y.Z mit X.Y = Feature-Version, Z = Debug-Revision im selben
// Feature-Stand -- fuer jeden Achsen-Algorithmus (alle Typen) einzeln + den Planer statisch.
//
// KRITISCH (Byte-Wache, Section43): Dieser Helfer PARST nur den bereits existierenden algo_version-String
// (`W::algo_version`, heute ueberall "v1.0.0") in ein X.Y.Z-Tripel. Er wird AUSSCHLIESSLICH von den NEUEN
// Stempel-Zeilen / dem Planer-Stempel / der neuen Registry-Emission genutzt -- NIE vom bestehenden
// compose_algo_signature-/.algos-Serialisierungspfad (der bleibt byte-identisch: die rohe Version
// serialisiert weiter roh, nie als "1.0.0"). Kein gemeinsamer Formatter mit der Alt-Welt.
//
// Format (Architekt-Entscheid W12-A-2, ABGELOEST durch Owner-Q3 -- s. A13-M1b unten): frueher "vN" (== N.0.0)
// ODER "vN.N.N". Kurzformen wie "v1.2" sind VERBOTEN (Mehrdeutigkeit). Unparsbar/Sentinel -> {0,0,0}
// (deckungsgleich zum @v0-Sentinel des compose-Pfads; der Aufrufer raet nie).
//
// A13-M1 (Owner-Entscheid E2 vom 02.08.2026, verbatim): "Die Markierung experimenteller Achsen-Algorithmen aus
// einem Pruefling wird in der Versionsbezifferung eines jeden Achsen-Algorithmus mit einem angehaengten 'e' fuer
// 'experimental' markiert."
//
// A13-M1b (Owner-Antwort Q3 vom 02.08.2026, verbatim): "Die Kurzform ist verboten, Versionierungen sind
// einheitlich und immer 3-Stellig und beginnen mit 'v'. Das 'e' ist eine Flag und kann spaeter gegen andere
// Falgs wie 'g' fuer GPU, 'c' fuer CPU, 'f' fuer FPGA und 'n' fuer NPU code erweitert werden. Wir produzieren
// nur CPU code, daher muessen alle Versionen mit 'c' oder 'ce' enden."
//
// DARAUS DIE GRAMMATIK (ersetzt die A13-M1-Form):
//   version := 'v' UINT '.' UINT '.' UINT [ HWFLAG [ 'e' ] ]     (rohe Form,      parse_algo_semver)
//   dotted  :=     UINT '.' UINT '.' UINT [ HWFLAG [ 'e' ] ]     (gerenderte Form, parse_dotted_semver)
//   HWFLAG  := 'c' (CPU) | 'g' (GPU) | 'f' (FPGA) | 'n' (NPU)    -- GENAU EINES, klein, direkt angehaengt
// Reihenfolge FIX: erst das Hardware-Flag, dann optional 'e'. Ein 'e' OHNE Hardware-Flag ist ungueltig
// (Sentinel) -- "vNe" gab es nur in der A13-M1-Zwischenform und ist mit dem Kurzform-Rueckbau erledigt.
//
// KURZFORM-RUECKBAU (A13-M1b, bindend): "vN" und "vNe" sind SENTINEL. Belegt vor dem Umbau per grep: der
// Bestand traegt KEIN vN-Literal als echte Version -- alle 122 W::algo_version, die 3 System-Achsen-Code-
// Versionen, die 3 Mess-Tooling- und die 1 Mess-Framework-Version stehen bereits als "v1.0.0" da. Die
// einzigen "v0"-Kurzformen waren SENTINEL-Literale (Render "0.0.0" vor wie nach dem Rueckbau); sie sind in
// diesem Paket auf die dreistellige Sentinel-Form "v0.0.0" gezogen. Ausnahme mit Absicht: die ROHE
// .algos-Signatur (compose_algo_signature) ist eine SEPARATE, byte-eingefrorene Welt und behaelt ihr "@v0".
//
// UEBERGANGS-TOLERANZ (A13-M1b, bewusst befristet): das FLAGLOSE dreistellige "vX.Y.Z" bleibt VORERST
// parsbar, weil der 122x-Bestand erst im A13-M2/M3-Neuanker-Fenster auf "v1.0.0c" migriert (ein einziges
// Stempel-/SHA512-Byte-Ereignis, kein zweiter Neuanker). Die Owner-PFLICHT "alle Versionen enden auf 'c'
// oder 'ce'" ist als version_satisfies_cpu_only_policy() GEBAUT und in axis_variant_version_table.hpp
// verdrahtet, aber hinter COMDARE_VERSION_HW_FLAG_ENFORCE (Default OFF) scharfgeschaltet.
//
// SENTINEL-WACHE (A13-Review-Auflage K-5): das NULL-TRIPEL IST der Sentinel -- auch mit Flags. "v0.0.0c"/
// "v0.0.0ce"/"0.0.0ce" parsen exakt auf AlgoSemVer{} (hardware none, experimental FALSE), damit die
// bestehenden CT-Wachen (parse != Sentinel) nicht ueber die Flag-Formen ausgehebelt werden koennen.
// is_sentinel() prueft zusaetzlich NUR das x/y/z-Tripel (nicht den ganzen Struct) -- die Wachen bleiben auch
// dann dicht, wenn eine spaetere Aenderung ein Flag doch durch den Sentinel-Pfad durchreicht.
//
// Metaprog: reines constexpr, kein std::variant, keine vtable, keine schweren Includes.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

/// A13-M1b SCHARFSCHALTUNG der Owner-Q3-PFLICHT ("alle Versionen muessen mit 'c' oder 'ce' enden").
/// DEFAULT OFF: solange der Bestand flaglos ist ("v1.0.0", 122x), wuerde die Wache jede Registry-Variante
/// compile-brechen. IM MIGRATIONS-COMMIT DES A13-M2/M3-NEUANKER-FENSTERS GEHT DIESES DEFINE AUF ON -- im
/// SELBEN Commit, der die Bestands-Strings auf "v1.0.0c" zieht (ein Byte-Ereignis, ein Neuanker). Wer das
/// Define frueher scharf schaltet, bricht den Bau; wer die Migration ohne das Define landet, laesst die
/// Pflicht undurchgesetzt.
/// MIGRATIONS-NAHT-LISTE (A13-M1b-Fixup, Review-BEFUND-2 -- der Migrations-Commit zieht ALLE Quellen,
/// nicht nur die 122 W::algo_version-Literale):
///   (a) 7 Nicht-Organ-Literale: abi/system_axis_code_versions.hpp (3x), measurement/
///       measurement_tooling_registry.hpp (3x), measurement/measurement_framework_registry.hpp (1x) --
///       alle tragen seit dem Fixup dieselbe gated ENFORCE-Wache und brechen beim Scharfschalten mit.
///   (b) builder/ceb_version_stamp.hpp: der konsteval-Zwilling rendert den Flag-Schwanz seit dem Fixup
///       symmetrisch mit (ceb_flag_len) -- der Zwillingstest bleibt ueber die Migration gruen.
///   (c) compose_algo_signature (axis_variant_version_table.hpp) serialisiert W::algo_version VERBATIM:
///       die 122er-Migration ist damit AUCH ein .algos-Sidecar-Byte-Ereignis (Skip-/Rebuild-Kaskade der
///       Sidecar-Welt) und ist im EINEN M2/M3-Byte-Ereignis-Fenster mit zu budgetieren.
#ifndef COMDARE_VERSION_HW_FLAG_ENFORCE
#define COMDARE_VERSION_HW_FLAG_ENFORCE 0
#endif

namespace comdare::cache_engine::measurement {

/// A13-M1b (Owner-Q3): die FLAG-FAMILIE der Hardware-Zielrichtung eines Achsen-Algorithmus. Erweiterbar
/// gehalten (der Owner nennt g/c/f/n explizit als Familie); `none` == KEIN Flag und existiert NUR fuer die
/// befristete Uebergangs-Toleranz des flaglosen Bestands, nie als Zielzustand.
enum class HardwareFlag : std::uint8_t { none = 0, cpu = 1, gpu = 2, fpga = 3, npu = 4 };

/// Das Flag-ZEICHEN der rohen/gerenderten Form. `none` -> '\0' (rendert nichts). EINZIGE Wahrheit der
/// Zeichen-Zuordnung; der Parser liest ueber hardware_flag_from_char() dieselbe Tabelle rueckwaerts.
[[nodiscard]] constexpr char hardware_flag_char(HardwareFlag f) noexcept {
    switch (f) {
        case HardwareFlag::cpu: return 'c';
        case HardwareFlag::gpu: return 'g';
        case HardwareFlag::fpga: return 'f';
        case HardwareFlag::npu: return 'n';
        case HardwareFlag::none: break;
    }
    return '\0';
}

/// Umkehrung von hardware_flag_char(): unbekanntes/grosses Zeichen -> none (der Aufrufer prueft danach auf
/// Zeilen-Ende und faellt so auf den Sentinel).
[[nodiscard]] constexpr HardwareFlag hardware_flag_from_char(char c) noexcept {
    switch (c) {
        case 'c': return HardwareFlag::cpu;
        case 'g': return HardwareFlag::gpu;
        case 'f': return HardwareFlag::fpga;
        case 'n': return HardwareFlag::npu;
        default: break;
    }
    return HardwareFlag::none;
}

/// X.Y.Z-Tripel (X.Y = Feature, Z = Debug-Revision) + die A13-M1b-Flag-Position: GENAU EIN Hardware-Flag
/// (Owner-Q3) und optional das A13-M1-Experimental-Flag (Owner-E2). Feld-Reihenfolge == Grammatik-Reihenfolge
/// (erst Hardware, dann 'e') -- eine Alt-Aggregat-Initialisierung `AlgoSemVer{x,y,z,true}` bricht dadurch
/// compile-time statt still an das Hardware-Feld zu binden (Lehre "Gruene Tests zementieren alte Ordnung").
struct AlgoSemVer {
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t z = 0;
    /// A13-M1b: die Hardware-Zielrichtung aus der Flag-Position ('c'/'g'/'f'/'n'). `none` == flagloser
    /// Uebergangs-Bestand (Owner-Q3-widrig, aber bis zum M2/M3-Migrations-Commit toleriert).
    HardwareFlag hardware = HardwareFlag::none;
    /// true == der Algorithmus-Stand ist EXPERIMENTELL (Trailing-'e' aus einem Pruefling, Owner-E2 02.08.2026).
    /// ce-EIGENE Registry-Varianten duerfen das NIE tragen (CT-Wache in axis_variant_version_table.hpp).
    bool experimental = false;

    /// K-5-Sentinel-Praedikat: der Sentinel ist das NULL-TRIPEL -- unabhaengig von den Flags. CT-Wachen pruefen
    /// HIERMIT, nicht per Struct-Vergleich gegen AlgoSemVer{}, damit eine Flag-Fehlform sie nie umgeht.
    [[nodiscard]] constexpr bool is_sentinel() const noexcept { return x == 0u && y == 0u && z == 0u; }

    /// A13-M1b: traegt die Version ueberhaupt ein Hardware-Flag? (Die Owner-PFLICHT verlangt zusaetzlich, dass
    /// es im CPU-Scope 'c' IST -- dafuer version_satisfies_cpu_only_policy().)
    [[nodiscard]] constexpr bool has_hardware_flag() const noexcept { return hardware != HardwareFlag::none; }
};

[[nodiscard]] constexpr bool operator==(AlgoSemVer const& a, AlgoSemVer const& b) noexcept {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.hardware == b.hardware && a.experimental == b.experimental;
}

namespace detail {
/// Konsumiert eine Dezimalzahl am Anfang von s (modifiziert s). Rueckgabe {wert, gefunden}.
[[nodiscard]] constexpr std::pair<std::uint32_t, bool> take_uint(std::string_view& s) noexcept {
    if (s.empty() || s.front() < '0' || s.front() > '9') return {0u, false};
    std::uint32_t n = 0;
    while (!s.empty() && s.front() >= '0' && s.front() <= '9') {
        n = n * 10u + static_cast<std::uint32_t>(s.front() - '0');
        s.remove_prefix(1);
    }
    return {n, true};
}

/// Konsumiert ein optionales Trailing-'e' (A13-M1). NUR klein-'e'; der Aufrufer prueft danach auf Zeilen-Ende
/// (ein Rest wie "e5"/"ee" bleibt damit unparsbar -> Sentinel).
[[nodiscard]] constexpr bool take_experimental(std::string_view& s) noexcept {
    if (s.empty() || s.front() != 'e') return false;
    s.remove_prefix(1);
    return true;
}

/// Konsumiert ein optionales Hardware-Flag (A13-M1b). Genau EIN Zeichen aus {c,g,f,n}; alles andere laesst s
/// unberuehrt und meldet none (der Aufrufer faellt dann ueber die Rest-Pruefung auf den Sentinel).
[[nodiscard]] constexpr HardwareFlag take_hardware_flag(std::string_view& s) noexcept {
    if (s.empty()) return HardwareFlag::none;
    HardwareFlag const f = hardware_flag_from_char(s.front());
    if (f != HardwareFlag::none) s.remove_prefix(1);
    return f;
}

/// K-5: baut das Ergebnis-Tripel und faltet JEDES Null-Tripel auf den reinen Sentinel AlgoSemVer{} zurueck --
/// "v0.0.0c"/"0.0.0ce" sind der Sentinel, nicht ein "experimenteller CPU-Sentinel".
[[nodiscard]] constexpr AlgoSemVer make_semver(std::uint32_t x, std::uint32_t y, std::uint32_t z, HardwareFlag hw,
                                               bool experimental) noexcept {
    if (x == 0u && y == 0u && z == 0u) return AlgoSemVer{};
    return AlgoSemVer{x, y, z, hw, experimental};
}

/// A13-M1b: der FLAG-SCHWANZ hinter dem X.Y.Z-Tripel -- die EINE Stelle, an der die Owner-Q3-Reihenfolge
/// durchgesetzt wird (erst GENAU EIN Hardware-Flag, dann optional 'e'). Beide Parser (rohe und gerenderte Form)
/// teilen sie sich, damit die Grammatik nicht zweimal existiert und nicht driften kann.
/// Sentinel-Faelle: zweites Hardware-Flag ("...cc"/"...cg"), 'e' vor dem Hardware-Flag ("...ec"), 'e' OHNE
/// Hardware-Flag ("...e"), Gross-Buchstaben ("...C"/"...E"), jeder sonstige Rest.
[[nodiscard]] constexpr AlgoSemVer take_flag_tail(std::string_view& s, std::uint32_t x, std::uint32_t y,
                                                  std::uint32_t z) noexcept {
    HardwareFlag const hw  = take_hardware_flag(s);
    bool               exp = false;
    if (hw != HardwareFlag::none) exp = take_experimental(s);
    if (!s.empty()) return {};
    return make_semver(x, y, z, hw, exp);
}
} // namespace detail

/// Parst die ROHE Form "vX.Y.Z" (Uebergangs-Toleranz, flaglos) sowie die A13-M1b-Flag-Formen "vX.Y.Zc",
/// "vX.Y.Zce", "vX.Y.Zg"/"f"/"n"[e]. Alles andere -> {0,0,0}-Sentinel: leer, ohne 'v', die
/// RUECKGEBAUTE Kurzform "vN"/"vNe"/"vNc", "vX.Y", Gross-'E'/'C', zwei Hardware-Flags, 'e' ohne
/// Hardware-Flag, Zusatz-Rest, nicht-numerisch. Das Null-Tripel bleibt IMMER der reine Sentinel (K-5).
/// Rein constexpr.
[[nodiscard]] constexpr AlgoSemVer parse_algo_semver(std::string_view v) noexcept {
    if (v.empty() || v.front() != 'v') return {};
    v.remove_prefix(1);
    auto const [x, okx] = detail::take_uint(v);
    if (!okx) return {};
    // A13-M1b KURZFORM-RUECKBAU (Owner-Q3 "Die Kurzform ist verboten ... immer 3-Stellig"): nach der ersten
    // Zahl MUSS ein '.' folgen. "v1"/"v1e"/"v1c" enden damit im Sentinel statt in {1,0,0}.
    if (v.empty() || v.front() != '.') return {};
    v.remove_prefix(1);
    auto const [y, oky] = detail::take_uint(v);
    if (!oky || v.empty() || v.front() != '.') return {}; // Kurzform "vX.Y" verboten (auch "vX.Yc")
    v.remove_prefix(1);
    auto const [z, okz] = detail::take_uint(v);
    if (!okz) return {};
    return detail::take_flag_tail(v, x, y, z);
}

/// Die X.Y.Z-VOLL-Form als String -- NUR fuer neue Stempel-Zeilen/Planer/Registry (NIE die .algos-Sig).
/// A13-M1b: hinter das Tripel wandert erst das Hardware-Flag, dann das 'e' ("2.3.4c", "2.3.4ce") -- dieselbe
/// Reihenfolge wie in der rohen Form. Der Sentinel rendert immer nackt "0.0.0" (er traegt nie Flags, K-5).
/// GOLDEN-NEUTRAL bis zur M2/M3-Migration: der flaglose Bestand ("v1.0.0") rendert unveraendert "1.0.0".
[[nodiscard]] inline std::string algo_semver_string(std::string_view algo_version) {
    AlgoSemVer const v   = parse_algo_semver(algo_version);
    std::string      out = std::to_string(v.x) + '.' + std::to_string(v.y) + '.' + std::to_string(v.z);
    if (char const hw = hardware_flag_char(v.hardware); hw != '\0') out += hw;
    if (v.experimental) out += 'e';
    return out;
}

/// A13-M1b PFLICHT-WACHE (Owner-Q3: "Wir produzieren nur CPU code, daher muessen alle Versionen mit 'c' oder
/// 'ce' enden"). Praedikat GETRENNT vom Parser: der Parser toleriert die flaglose Uebergangs-Form weiter
/// (sonst waeren die static_assert-Batterien dieses Headers define-abhaengig und ein Header haette zwei
/// Bedeutungen); die POLITIK lebt hier und wird in axis_variant_version_table.hpp hinter
/// COMDARE_VERSION_HW_FLAG_ENFORCE scharf geschaltet. IMMER kompiliert und CT-bewiesen (Batterie unten) --
/// das Define schaltet nur die ANWENDUNG auf jede Registry-Variante, nie die Existenz der Wache.
[[nodiscard]] constexpr bool version_satisfies_cpu_only_policy(std::string_view raw) noexcept {
    AlgoSemVer const v = parse_algo_semver(raw);
    return !v.is_sentinel() && v.hardware == HardwareFlag::cpu;
}

/// parse_dotted_semver (G2-1a / A3): parst die bereits GERENDERTE Voll-Form "X.Y.Z" (dotted, OHNE 'v') zurueck in
/// ein Tripel -- die Umkehrung von algo_semver_string. Getrennt von parse_algo_semver (das die ROHE "vN"-Form parst):
/// der consteval-Stempel-Parser (anatomy_stamp_entries.hpp) liest die "achse=algo@X.Y.Z"-Zeilen, deren Versions-Teil
/// bereits die gerenderte Form traegt. STRENG: exakt drei dotted uints + der A13-M1b-Flag-Schwanz (optional
/// GENAU EIN Hardware-Flag, danach optional 'e'); alles andere (leer, mit 'v', Kurzform "X.Y", Zusatz-Rest,
/// nicht-numerisch) -> {0,0,0}-Sentinel. Rein constexpr.
/// Der PUNKT ist damit ausschliesslich TRENNER der drei Zahlen -- hierarchische Namen (Owner-Q2-Muster
/// "prt-art.memory.abc@1.0.0") leben im NAMENS-Anteil VOR dem '@' und beruehren diesen Parser nie.
[[nodiscard]] constexpr AlgoSemVer parse_dotted_semver(std::string_view v) noexcept {
    auto const [x, okx] = detail::take_uint(v);
    if (!okx || v.empty() || v.front() != '.') return {};
    v.remove_prefix(1);
    auto const [y, oky] = detail::take_uint(v);
    if (!oky || v.empty() || v.front() != '.') return {}; // Kurzform "X.Y" verboten
    v.remove_prefix(1);
    auto const [z, okz] = detail::take_uint(v);
    if (!okz) return {};
    return detail::take_flag_tail(v, x, y, z);
}

// -- Wohlgeformtheit (alles compile-time) --------------------------------------------------------
static_assert(parse_algo_semver("v1.0.0") == AlgoSemVer{1, 0, 0}); // heutiger Stand ALLER Algos (flaglos, tolerant)
static_assert(parse_algo_semver("v0.0.0") == AlgoSemVer{0, 0, 0}); // Sentinel
static_assert(parse_algo_semver("v2.3.4") == AlgoSemVer{2, 3, 4});
static_assert(parse_algo_semver("v10.0.1") == AlgoSemVer{10, 0, 1});
static_assert(parse_algo_semver("v1.2") == AlgoSemVer{0, 0, 0});     // Kurzform verboten -> Sentinel
static_assert(parse_algo_semver("1.0.0") == AlgoSemVer{0, 0, 0});    // ohne 'v' -> Sentinel
static_assert(parse_algo_semver("") == AlgoSemVer{0, 0, 0});         // leer -> Sentinel
static_assert(parse_algo_semver("vx") == AlgoSemVer{0, 0, 0});       // nicht-numerisch -> Sentinel
static_assert(parse_algo_semver("v1.2.3.4") == AlgoSemVer{0, 0, 0}); // Zusatz-Rest -> Sentinel

// parse_dotted_semver (A3): die gerenderte "X.Y.Z"-Form; Umkehrung von algo_semver_string.
static_assert(parse_dotted_semver("1.0.0") == AlgoSemVer{1, 0, 0}); // heutiger gerenderter Stand
static_assert(parse_dotted_semver("2.3.4") == AlgoSemVer{2, 3, 4});
static_assert(parse_dotted_semver("10.0.1") == AlgoSemVer{10, 0, 1});
static_assert(parse_dotted_semver("v1.0.0") == AlgoSemVer{0, 0, 0});  // mit 'v' (rohe Form) -> Sentinel
static_assert(parse_dotted_semver("1.0") == AlgoSemVer{0, 0, 0});     // Kurzform -> Sentinel
static_assert(parse_dotted_semver("1.0.0.1") == AlgoSemVer{0, 0, 0}); // Zusatz-Rest -> Sentinel
static_assert(parse_dotted_semver("") == AlgoSemVer{0, 0, 0});        // leer -> Sentinel
static_assert(parse_dotted_semver("x.y.z") == AlgoSemVer{0, 0, 0});   // nicht-numerisch -> Sentinel
// Round-Trip: algo_semver_string rendert "vX.Y.Z" -> "X.Y.Z", parse_dotted_semver liest es exakt zurueck.
static_assert(parse_dotted_semver("1.0.0") == parse_algo_semver("v1.0.0"));

// -- A13-M1b: KURZFORM-RUECKBAU (Owner-Q3 "Die Kurzform ist verboten ... immer 3-Stellig") -----------
// Die A13-M1-Zwischenform "vN"/"vNe" ist ZURUECKGEBAUT: beide sind ab jetzt Sentinel. Der Bestand ist davon
// nicht betroffen (grep-belegt: alle echten Versions-Literale stehen dreistellig als "v1.0.0" da).
static_assert(parse_algo_semver("v1") == AlgoSemVer{});  // frueher {1,0,0} -- jetzt Sentinel
static_assert(parse_algo_semver("v0") == AlgoSemVer{});  // frueher Sentinel -- bleibt Sentinel
static_assert(parse_algo_semver("v1e") == AlgoSemVer{}); // A13-M1-Kurzform mit 'e' -- zurueckgebaut
static_assert(parse_algo_semver("v1c") == AlgoSemVer{}); // auch mit Hardware-Flag bleibt die Kurzform verboten
static_assert(parse_algo_semver("v1ce") == AlgoSemVer{});
static_assert(parse_algo_semver("v12") == AlgoSemVer{}); // mehrstellige Kurzform ebenso

// -- A13-M1b: FLAG-GRAMMATIK (Owner-Q3 02.08.2026) -- vollstaendige CT-Batterie ---------------------
// (a) Uebergangs-Toleranz: der flaglose dreistellige Bestand parst weiter und traegt KEINE Flags.
static_assert(!parse_algo_semver("v1.0.0").has_hardware_flag());
static_assert(!parse_algo_semver("v1.0.0").experimental);
static_assert(!parse_algo_semver("v2.3.4").experimental);
static_assert(!parse_dotted_semver("1.0.0").has_hardware_flag());
static_assert(!parse_dotted_semver("1.0.0").experimental);
// (b) Die volle Flag-Familie c/g/f/n an der ROHEN Form.
static_assert(parse_algo_semver("v1.0.0c") == AlgoSemVer{1, 0, 0, HardwareFlag::cpu});
static_assert(parse_algo_semver("v1.0.0g") == AlgoSemVer{1, 0, 0, HardwareFlag::gpu});
static_assert(parse_algo_semver("v1.0.0f") == AlgoSemVer{1, 0, 0, HardwareFlag::fpga});
static_assert(parse_algo_semver("v1.0.0n") == AlgoSemVer{1, 0, 0, HardwareFlag::npu});
static_assert(parse_algo_semver("v10.0.1c").has_hardware_flag());
// (c) Reihenfolge FIX: Hardware-Flag, dann optional 'e' ("v2.3.4ce").
static_assert(parse_algo_semver("v2.3.4ce") == AlgoSemVer{2, 3, 4, HardwareFlag::cpu, true});
static_assert(parse_algo_semver("v2.3.4ge") == AlgoSemVer{2, 3, 4, HardwareFlag::gpu, true});
static_assert(parse_algo_semver("v2.3.4ce").experimental);
// (d) Gerenderte Form MIT Flags (die Stempel-Zeilen-Ruecklesung).
static_assert(parse_dotted_semver("1.0.0c") == AlgoSemVer{1, 0, 0, HardwareFlag::cpu});
static_assert(parse_dotted_semver("2.3.4ce") == AlgoSemVer{2, 3, 4, HardwareFlag::cpu, true});
static_assert(parse_dotted_semver("2.3.4n") == AlgoSemVer{2, 3, 4, HardwareFlag::npu});
// (e) GENAU EIN Hardware-Flag; 'e' NUR NACH einem Hardware-Flag; nur Kleinbuchstaben; kein Rest.
static_assert(parse_algo_semver("v1.0.0cc") == AlgoSemVer{});   // zweites Flag
static_assert(parse_algo_semver("v1.0.0cg") == AlgoSemVer{});   // zwei verschiedene Flags
static_assert(parse_algo_semver("v1.0.0ec") == AlgoSemVer{});   // Reihenfolge verdreht
static_assert(parse_algo_semver("v1.0.0e") == AlgoSemVer{});    // 'e' OHNE Hardware-Flag
static_assert(parse_algo_semver("v2.3.4e") == AlgoSemVer{});    // dito (die A13-M1-Form ohne HW-Flag)
static_assert(parse_algo_semver("v1.0.0C") == AlgoSemVer{});    // Gross-'C'
static_assert(parse_algo_semver("v1.0.0cE") == AlgoSemVer{});   // Gross-'E'
static_assert(parse_algo_semver("v1.0.0cee") == AlgoSemVer{});  // doppeltes 'e'
static_assert(parse_algo_semver("v1.0.0ce5") == AlgoSemVer{});  // Rest nach dem 'e'
static_assert(parse_algo_semver("v1.0.0x") == AlgoSemVer{});    // unbekanntes Flag-Zeichen
static_assert(parse_algo_semver("v1.2ce") == AlgoSemVer{});     // Kurzform bleibt verboten
static_assert(parse_algo_semver("1.0.0c") == AlgoSemVer{});     // gerenderte Form in der rohen Wache
static_assert(parse_dotted_semver("1.0c") == AlgoSemVer{});     // Kurzform
static_assert(parse_dotted_semver("v2.3.4ce") == AlgoSemVer{}); // rohe Form in der gerenderten Wache
static_assert(parse_dotted_semver("2.3.4E") == AlgoSemVer{});
static_assert(parse_dotted_semver("2.3.4e") == AlgoSemVer{}); // 'e' ohne Hardware-Flag
static_assert(parse_dotted_semver("2.3.4cc") == AlgoSemVer{});
// (f) K-5-SENTINEL-BATTERIE: das Null-Tripel ist der REINE Sentinel -- auch mit Flags. Die bestehenden
//     CT-Wachen (parse != AlgoSemVer{} bzw. !is_sentinel()) bleiben damit gegen jede Flag-Form dicht.
static_assert(parse_algo_semver("v0c") == AlgoSemVer{}); // Kurzform UND Null-Tripel
static_assert(parse_algo_semver("v0.0.0c") == AlgoSemVer{});
static_assert(parse_algo_semver("v0.0.0ce") == AlgoSemVer{});
static_assert(parse_dotted_semver("0.0.0c") == AlgoSemVer{});
static_assert(parse_dotted_semver("0.0.0ce") == AlgoSemVer{});
static_assert(!parse_algo_semver("v0.0.0ce").experimental);        // KEIN "experimenteller Sentinel"
static_assert(!parse_algo_semver("v0.0.0ce").has_hardware_flag()); // und KEIN "CPU-Sentinel"
static_assert(!parse_dotted_semver("0.0.0ce").experimental);
static_assert(parse_algo_semver("v0.0.0c").is_sentinel()); // x/y/z-Praedikat (statt Struct-Vergleich)
static_assert(parse_dotted_semver("0.0.0ce").is_sentinel());
static_assert(!parse_algo_semver("v1.0.0c").is_sentinel()); // ein echter Stand ist KEIN Sentinel
static_assert(!parse_algo_semver("v2.3.4ce").is_sentinel());
// (g) UNTERSCHEIDBARKEIT (Fingerprint-/Lager-Wirkung): jedes Flag erzeugt ein anderes Stempel-Segment.
static_assert(!(parse_algo_semver("v1.0.0c") == parse_algo_semver("v1.0.0")));
static_assert(!(parse_algo_semver("v1.0.0c") == parse_algo_semver("v1.0.0g")));
static_assert(!(parse_algo_semver("v1.0.0c") == parse_algo_semver("v1.0.0ce")));
static_assert(!(parse_dotted_semver("2.3.4ce") == parse_dotted_semver("2.3.4c")));
// (h) Round-Trip rohe <-> gerenderte Form INKLUSIVE beider Flags.
static_assert(parse_dotted_semver("1.0.0") == parse_algo_semver("v1.0.0"));
static_assert(parse_dotted_semver("1.0.0c") == parse_algo_semver("v1.0.0c"));
static_assert(parse_dotted_semver("2.3.4ce") == parse_algo_semver("v2.3.4ce"));
// (i) Zeichen-Tabelle als Einheit (hardware_flag_char <-> hardware_flag_from_char).
static_assert(hardware_flag_char(HardwareFlag::cpu) == 'c');
static_assert(hardware_flag_char(HardwareFlag::gpu) == 'g');
static_assert(hardware_flag_char(HardwareFlag::fpga) == 'f');
static_assert(hardware_flag_char(HardwareFlag::npu) == 'n');
static_assert(hardware_flag_char(HardwareFlag::none) == '\0');
static_assert(hardware_flag_from_char(hardware_flag_char(HardwareFlag::cpu)) == HardwareFlag::cpu);
static_assert(hardware_flag_from_char(hardware_flag_char(HardwareFlag::npu)) == HardwareFlag::npu);
static_assert(hardware_flag_from_char('C') == HardwareFlag::none);
static_assert(hardware_flag_from_char('e') == HardwareFlag::none); // 'e' ist KEIN Hardware-Flag
// (j) Die Owner-PFLICHT-Wache selbst (immer gebaut, unabhaengig von COMDARE_VERSION_HW_FLAG_ENFORCE):
//     nur "...c"/"...ce" erfuellen den CPU-only-Scope.
static_assert(version_satisfies_cpu_only_policy("v1.0.0c"));
static_assert(version_satisfies_cpu_only_policy("v2.3.4ce"));
static_assert(!version_satisfies_cpu_only_policy("v1.0.0"));  // flagloser Uebergangs-Bestand (M2/M3-Migration)
static_assert(!version_satisfies_cpu_only_policy("v1.0.0g")); // GPU ist im CPU-only-Scope unzulaessig
static_assert(!version_satisfies_cpu_only_policy("v1.0.0f"));
static_assert(!version_satisfies_cpu_only_policy("v1.0.0n"));
static_assert(!version_satisfies_cpu_only_policy("v0.0.0c")); // Sentinel erfuellt nie eine Politik
static_assert(!version_satisfies_cpu_only_policy("v1c"));     // Kurzform erfuellt nie eine Politik
static_assert(!version_satisfies_cpu_only_policy(""));

} // namespace comdare::cache_engine::measurement
