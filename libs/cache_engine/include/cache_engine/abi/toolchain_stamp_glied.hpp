#pragma once
// abi/toolchain_stamp_glied.hpp -- O-2/C-2 (Achsen-Vollstaendigkeits-Neuanker, Owner-KERN abend-5/F1):
// der RENDERER des Toolchain-Glieds [5] des Anatomie-Fingerprint-Preimage (Format 3).
//
// WARUM DIESES GLIED EXISTIERT (Owner-KERN 05.08.2026, Ledger-Nachtrag abend-5, verbatim): "ALLE Laufzeit
// Hauptachsen wie Compiler auf der CEB [muessen] zwangsweise compile time Hauptachsen auf der entstehenden
// Tier-Binary sein ... Diesbezueglich muessen ALLE Achsen nach Plan vollstaendig sein und auch im Fingerprint
// mit ihrer Versionierung verankert sein ... die Flags des Compilers werden in der Tier-Binary statisch
// verbaut, sind also in der Compiler Haupt-Achse ein Teil der Haupt-Achsen Definition selbst zur Laufzeit der
// CEB und compile time einer Tier-Binary."
//
// DIE LUECKE, DIE ES SCHLIESST (A2-Eichungs-Nachreview C1 [HOCH, REAL]): bis Format 2 trug das Preimage
// AUSSCHLIESSLICH Format-Kennung, die drei Realm-Stempel-Zeilen und das Sub-Achsen-Werteset. cxx/opt/bt/gate/
// ceb reisten allein im build_version-SUFFIX (profile_facade/system_version_suffix.hpp) und damit in
// `.version`/CSV/Cache-Pfad -- NICHT in der Identitaet. Seit dem A2-SHA512-only-Skip-Gate entscheidet aber NUR
// noch der Fingerprint ueber den Skip: zwei Baue derselben Permutation mit anderem opt/bt hatten denselben
// Fingerprint und konnten sich im geteilten Ausgabe-Verzeichnis gegenseitig ueberspringen (O3 skippt auf die
// O2-DLL, Debug auf Release). Das Glied [5] macht die Toolchain-Wahl identitaetswirksam.
//
// STUFEN-KONFORMITAET (kanonische Stufen-Doktrin mittag-9/-10, Dual-Natur-Register V7.2): die compiler-Gruppe
// ist RT-UNTER-Achse an der CEB (dort permutiert der Planer sie) und CT-HAUPT-Achse an der Tier-Binary
// ("einkompiliert + gestempelt"). Genau diese zweite Natur steht ab hier im Preimage. binary_id bleibt
// UNBERUEHRT (Registry binary_id="never", golden-neutral) -- der Fingerprint ist NICHT die binary_id.
//
// KEINE ZWEITE WAHRHEIT NEBEN DEM SUFFIX: die Segment-ORDNUNG dieses Glieds ist byte-fuer-byte die Ordnung
// von profile_facade::kSuffixSegmentOrder (dort ohne fuehrendes '+' und ohne '='). Die Kopplung ist nicht
// dokumentiert, sondern BEWIESEN -- der static_assert steht in system_version_suffix.hpp, weil nur die
// hoehere Schicht beide Header sehen darf (abi/ zieht nie profile_facade/). atomic128 haengt hinten an: es
// ist eine Toolchain-Unter-Achse mit eigener Flag-Materialisierung, die im build_version-Suffix nie ein
// Segment hatte -- als LETZTES Feld bleibt die Praefix-Deckung mit der Suffix-Ordnung exakt.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare, keine Laufzeit-Erhebung (die WERTE kommen von der CEB
// herein; dieser Header rendert nur).
// [NACHGEFUEHRT 2026-08-06, NB/CX-3 -- der Satz "keine Laufzeit-Erhebung" bleibt wahr und wird PRAEZISIERT:
// die COMPILER-REAL-VERSION wird ab hier COMPILE-TIME erhoben (kDetectedCompilerRealVersion, unten). Das ist
// keine Laufzeit-Probe, sondern die Praeprozessor-Wahrheit der uebersetzenden Toolchain -- exakt der Weg, den
// G-C4 fuer den CT-Stempel verlangt. Alle uebrigen WERTE kommen weiter von aussen herein.]

#include <cache_engine/measurement/algo_semver.hpp>         // algo_semver_string (X.Y.Z[Flag]-Voll-Form, Owner-Q10)
#include <cache_engine/measurement/compiler_system_axis.hpp> // NB/CX-3: die Dialekt-Ids als Single-Source

#include <array>
#include <cstddef>
#include <stdexcept> // NB/CX-2: die Injektivitaets-Wache des Renderers ist FAIL-LOUD, nicht still
#include <string>
#include <string_view>

namespace comdare::cache_engine::abi {

/// Format-Version des Toolchain-Glieds, unabhaengig von kAnatomyFingerprintFormat. Bumpen, sobald sich
/// Trennzeichen, Feldauswahl oder Feld-Reihenfolge aendern. Ein Bump ist per Konstruktion ein
/// Fingerprint-Bruch (das Glied steht im Preimage) und erzwingt damit Neubau statt stiller Fehl-Vergleiche.
inline constexpr std::string_view kToolchainStampGliedFormat = "1";

// -- NB/CX-3: DIE COMPILER-REAL-VERSIONS-ERHEBUNG, COMPILE-TIME ----------------------------------------
//
// WAS G-C4/OE-C VERLANGT: nicht den TREIBER-NAMEN ("g++-16" sagt nichts darueber, ob 16.0.1 oder 16.2.0
// wirklich uebersetzt hat), sondern die REAL erkannte Version. Bis zum NB-Fenster war das ein reines
// Datenfeld (ToolchainStampParts.cxx_realversion) OHNE Erhebung -- ein Slot, den niemand fuellen konnte.
//
// WARUM COMPILE-TIME UND NICHT ALS LAUFZEIT-PROBE: der Wert, der den TIER-CT-STEMPEL traegt, ist die
// Version DERJENIGEN Uebersetzung, in der das Stempel-Makro expandiert. Genau diese Version steht dem
// Praeprozessor als __GNUC__/__clang_major__ zur Verfuegung -- eine externe Probe (`--version` starten und
// parsen) waere eine zweite, schwaechere Wahrheit mit eigener Fehlerflaeche (Dialekt-Parser je Distro) und
// koennte die uebersetzende Toolchain gar nicht beweisen, nur behaupten.
//
// EHRLICHE GRENZE (nicht verschwiegen): dieser Wert beschreibt IMMER die Toolchain DERJENIGEN TU, in der er
// ausgewertet wird. Wird das Glied von der CEB komponiert und per Compile-Define in die Tier-Uebersetzung
// gereicht, dann ist es die CEB-Toolchain -- und die ist nur dann auch die Tier-Toolchain, wenn beide
// dieselbe ist. Die Naht, die das Glied komponiert, MUSS diese Deckung deshalb pruefen und fail-closed
// degradieren, statt eine ungedeckte Version zu behaupten (profile_facade/toolchain_stamp_naht.hpp).
//
// DIALEKT-VORRANG __clang__: clang definiert __GNUC__ MIT (GCC-Kompatibilitaets-Emulation) und meldet dort
// die emulierte GCC-Version, nicht seine eigene. Die Reihenfolge der Praeprozessor-Zweige ist deshalb kein
// Stil, sondern die Korrektheitsbedingung: __clang__ zuerst, sonst wuerde ein clang-Bau als "gcc-4.2.1"
// gestempelt.

namespace detail {

/// Die drei Roh-Zahlen der uebersetzenden Toolchain. -1 == UNBEKANNT (weder clang noch GCC-kompatibel).
#if defined(__clang__)
inline constexpr int kCtCompilerMajorRaw = __clang_major__;
inline constexpr int kCtCompilerMinorRaw = __clang_minor__;
inline constexpr int kCtCompilerPatchRaw = __clang_patchlevel__;
#elif defined(__GNUC__)
inline constexpr int kCtCompilerMajorRaw = __GNUC__;
inline constexpr int kCtCompilerMinorRaw = __GNUC_MINOR__;
inline constexpr int kCtCompilerPatchRaw = __GNUC_PATCHLEVEL__;
#else
inline constexpr int kCtCompilerMajorRaw = -1;
inline constexpr int kCtCompilerMinorRaw = -1;
inline constexpr int kCtCompilerPatchRaw = -1;
#endif

[[nodiscard]] consteval std::size_t ct_dezimal_stellen(unsigned v) noexcept {
    std::size_t n = 1;
    while (v >= 10) {
        v /= 10;
        ++n;
    }
    return n;
}

/// Laenge der Form "<major>.<minor>.<patch>"; 0, wenn die Toolchain unbekannt ist.
[[nodiscard]] consteval std::size_t ct_realversion_laenge() noexcept {
    if (kCtCompilerMajorRaw < 0) return 0;
    return ct_dezimal_stellen(static_cast<unsigned>(kCtCompilerMajorRaw)) + 1 +
           ct_dezimal_stellen(static_cast<unsigned>(kCtCompilerMinorRaw)) + 1 +
           ct_dezimal_stellen(static_cast<unsigned>(kCtCompilerPatchRaw));
}

/// Der Zeichen-Speicher. N ist bewusst um EINS groesser als die Laenge: eine std::array<char,0> haette kein
/// gueltiges data(), und ein string_view auf nullptr waere in einem konstanten Ausdruck nicht bildbar.
template <std::size_t N>
[[nodiscard]] consteval std::array<char, N> ct_realversion_zeichen() noexcept {
    std::array<char, N> out{};
    if (kCtCompilerMajorRaw < 0) return out;
    std::size_t n       = 0;
    auto const  put_uint = [&out, &n](unsigned v) {
        std::size_t const stellen = ct_dezimal_stellen(v);
        for (std::size_t i = 0; i < stellen; ++i) {
            std::size_t teiler = 1;
            for (std::size_t k = 1; k + i < stellen; ++k) teiler *= 10;
            out[n++] = static_cast<char>('0' + ((v / teiler) % 10));
        }
    };
    put_uint(static_cast<unsigned>(kCtCompilerMajorRaw));
    out[n++] = '.';
    put_uint(static_cast<unsigned>(kCtCompilerMinorRaw));
    out[n++] = '.';
    put_uint(static_cast<unsigned>(kCtCompilerPatchRaw));
    return out;
}

inline constexpr auto kCtRealversionZeichen = ct_realversion_zeichen<ct_realversion_laenge() + 1>();

} // namespace detail

/// Die REAL erkannte Version der uebersetzenden Toolchain, gerendert als "<major>.<minor>.<patch>"
/// (z.B. "16.2.0"). LEER == unbekannte Toolchain (weder clang noch GCC-kompatibel) -- dann wird auch
/// NICHTS behauptet (fail-closed statt geraten).
inline constexpr std::string_view kDetectedCompilerRealVersion{detail::kCtRealversionZeichen.data(),
                                                               detail::ct_realversion_laenge()};

/// Der DIALEKT der uebersetzenden Toolchain, Single-Source aus der Compiler-System-Achse (nie ein Literal).
/// LEER == unbekannt.
inline constexpr std::string_view kDetectedCompilerDialect =
#if defined(__clang__)
    ::comdare::cache_engine::measurement::ClangCompilerAxis::compiler_id();
#elif defined(__GNUC__)
    ::comdare::cache_engine::measurement::GccCompilerAxis::compiler_id();
#else
    std::string_view{};
#endif

/// TRUE, wenn Dialekt UND Realversion erhoben werden konnten.
inline constexpr bool kDetectedCompilerIsKnown =
    !kDetectedCompilerDialect.empty() && !kDetectedCompilerRealVersion.empty();

// Erhebungs-Wachen. Sie beissen genau dann, wenn die Erhebung strukturell kaputt waere -- eine leere oder
// ungrammatische Version darf NIE in ein Preimage-Glied wandern (sie waere dort nicht mehr korrigierbar).
static_assert(kDetectedCompilerDialect.empty() ||
                  kDetectedCompilerDialect == ::comdare::cache_engine::measurement::GccCompilerAxis::compiler_id() ||
                  kDetectedCompilerDialect == ::comdare::cache_engine::measurement::ClangCompilerAxis::compiler_id(),
              "NB/CX-3: der erhobene Compiler-Dialekt ist keine Option der Compiler-System-Achse.");
static_assert(kDetectedCompilerRealVersion.empty() || kDetectedCompilerRealVersion.size() >= 5,
              "NB/CX-3: die Realversion traegt drei Zahlen und zwei Punkte, ist also mindestens 5 Zeichen lang.");
static_assert(
    [] {
        if (kDetectedCompilerRealVersion.empty()) return true;
        std::size_t punkte = 0;
        std::size_t ziffern_im_glied = 0;
        for (char const c : kDetectedCompilerRealVersion) {
            if (c == '.') {
                if (ziffern_im_glied == 0) return false; // leeres Zahlen-Glied
                ++punkte;
                ziffern_im_glied = 0;
                continue;
            }
            if (c < '0' || c > '9') return false; // NUR Ziffern und Punkte
            ++ziffern_im_glied;
        }
        return punkte == 2 && ziffern_im_glied > 0;
    }(),
    "NB/CX-3: die erhobene Compiler-Realversion ist nicht die Form <major>.<minor>.<patch>. Ein anderer "
    "Zeichenvorrat wuerde die Injektivitaet des Glieds [5] gefaehrden (';', '=', '{', '}', '@' sind dort "
    "STRUKTUR) -- deshalb bricht die Erhebung hier, statt still ein unzerlegbares Glied zu bauen.");

/// Ein Versions-Anker je TOOLCHAIN-Achse. Owner-KERN abend-5: "ALLE Achsen ... auch im Fingerprint MIT IHRER
/// VERSIONIERUNG verankert". Bis heute hatte die compiler-Gruppe KEINE Version: system_axis_code_versions.hpp
/// fuehrt compiler/scheduling/load_framework nur als UMZUEGE (A3/O-8 Schritt 4), also ohne eigenen Eintrag.
struct ToolchainAxisVersion {
    std::string_view axis;    ///< Toolchain-Achsen-Name ("compiler"/"opt_level"/"atomic128")
    std::string_view version; ///< rohe Code-Version in Q3-Grammatik ("v1.0.0c"); gerendert praefixfrei (Q10)
};

/// Genau DREI Toolchain-Achsen -- die untrennbare Unter-Achsen-GRUPPE der aeusseren System-Komplex-Achse
/// (V7.2, Ledger 2229: "compiler+opt_level+atomic128 = untrennbare Unter-Achsen-GRUPPE"), gespiegelt aus der
/// Registry-Gruppe build_toolchain (measurement/system_axis_registry.xml, sub_axis_group "build_toolchain").
inline constexpr std::size_t kToolchainAxisCount = 3;

/// Die EINE Tabelle der Toolchain-Achsen-Code-Versionen.
///
/// WARUM SIE NICHT IN kSystemAxisCodeVersions GEHOERT (A3/O-8, bewusst getrennt gehalten): die drei
/// System-HAUPT-Achsen sind target_isa/operating_system/external_utils, ABSCHLIESSEND. compiler ist seit
/// O-8 Schritt 4 KEINE System-Haupt-Achse mehr, sondern Unter-Achsen-Gruppe der Komplex-Achse. Ein vierter
/// Eintrag dort waere eine Regression gegen genau diesen Entscheid und wuerde ausserdem die System-ZEILE
/// (Glied [2]) veraendern -- also Glieder-[1]-[3]-Literale bewegen, was in diesem Fenster TABU ist. Der
/// Anker wohnt deshalb hier, neben dem Glied, in dem er wirkt.
///
/// BUMP-WACHE: wer hier bumpt, verschiebt den Fingerprint ALLER Neubauten (das Glied steht im Preimage) und
/// muss das als deklariertes Byte-Ereignis begruenden -- dieselbe Doppel-Absicht wie bei
/// kSystemAxisCodeVersions (dort static_assert je Eintrag, hier ebenso unten).
inline constexpr std::array<ToolchainAxisVersion, kToolchainAxisCount> kToolchainAxisVersions{{
    {"compiler", "v1.0.0c"},
    {"opt_level", "v1.0.0c"},
    {"atomic128", "v1.0.0c"},
}};

// Bump-Wache, maschinell (Muster system_axis_code_versions.hpp:B6): Achse UND Version je Index.
static_assert(kToolchainAxisVersions[0].axis == std::string_view{"compiler"});
static_assert(kToolchainAxisVersions[0].version == std::string_view{"v1.0.0c"});
static_assert(kToolchainAxisVersions[1].axis == std::string_view{"opt_level"});
static_assert(kToolchainAxisVersions[1].version == std::string_view{"v1.0.0c"});
static_assert(kToolchainAxisVersions[2].axis == std::string_view{"atomic128"});
static_assert(kToolchainAxisVersions[2].version == std::string_view{"v1.0.0c"});

// Q3-GRAMMATIK-WACHE (Owner 02.08.2026: "Versionierungen sind einheitlich und immer 3-Stellig und beginnen
// mit 'v' ... alle Versionen [muessen] mit 'c' oder 'ce' enden"): jede der drei Versionen ist wohlgeformt
// UND CPU-flag-konform. Ohne diese Wache koennte hier eine Kurzform oder eine flaglose Version einziehen und
// den Fingerprint mit einer Grammatik-Verletzung zementieren.
static_assert(::comdare::cache_engine::measurement::ce_owned_version_is_wellformed(kToolchainAxisVersions[0].version));
static_assert(::comdare::cache_engine::measurement::ce_owned_version_is_wellformed(kToolchainAxisVersions[1].version));
static_assert(::comdare::cache_engine::measurement::ce_owned_version_is_wellformed(kToolchainAxisVersions[2].version));
static_assert(
    ::comdare::cache_engine::measurement::ce_owned_version_satisfies_cpu_enforce(kToolchainAxisVersions[0].version));
static_assert(
    ::comdare::cache_engine::measurement::ce_owned_version_satisfies_cpu_enforce(kToolchainAxisVersions[1].version));
static_assert(
    ::comdare::cache_engine::measurement::ce_owned_version_satisfies_cpu_enforce(kToolchainAxisVersions[2].version));

/// Die Feld-SCHLUESSEL des Glieds, in Renderer-Reihenfolge. Die ersten acht sind byte-gleich
/// profile_facade::kSuffixSegmentOrder ohne '+' und '=' -- bewiesen dort per static_assert (die Wache kann
/// nur in der hoeheren Schicht stehen, abi/ darf profile_facade/ nicht sehen).
inline constexpr std::size_t                                           kToolchainGliedKeyCount = 9;
inline constexpr std::array<std::string_view, kToolchainGliedKeyCount> kToolchainGliedKeys     = {
    "cxx", "opt", "ext", "ceb", "target", "tel", "bt", "gate", "atomic128"};

/// Die Glieder des Toolchain-Stempels. LEER heisst IMMER "kein Segment" -- exakt die Regel von
/// SystemVersionSuffixParts (system_version_suffix.hpp:59-70). Der Aufrufer fuellt nur, was er wirklich weiss;
/// sind ALLE Felder leer, ist das gerenderte Glied leer (die IDENTITAET, byte-neutral zum Vor-Neuanker-Stand
/// dieses Glieds).
///
/// cxx ist ZWEIGETEILT und nicht ein fertiger String: G-C4/OE-C verlangt die REAL ERKANNTE Compiler-Version
/// statt des Treiber-NAMENS ("g++-16" sagt nichts ueber die tatsaechlich benutzte 16.1.0 vs 16.2.0). Ein
/// zusammengesetztes Feld liesse offen, ob der Aufrufer das getan hat; zwei Felder machen es zur Pflicht,
/// die der Renderer an EINER Stelle zusammensetzt.
struct ToolchainStampParts {
    std::string_view cxx_dialect{};       ///< Compiler-Haupt-Achsen-Option-id ("gcc"/"clang")
    std::string_view cxx_realversion{};   ///< REAL erkannte Version ("16.1.0"), NIE der Treiber-Name (G-C4)
    std::string_view opt{};               ///< opt_level-id ("O3")
    std::string_view opt_flags{};         ///< die konkreten Flags dieses Dialekts ("-O3") -- Teil der Definition
    std::string_view simd{};              ///< simd-id (+ext=); no_extension wird vom Aufrufer als leer gereicht
    std::string_view ceb{};               ///< "<abi_major>.<codegen_minor>" (+ceb=, Perm-Pfad; heilt Fall C)
    std::string_view target_isa{};        ///< NUR bei Ziel != Host (+target=)
    std::string_view telemetry{};         ///< NUR bei abweichendem Regime (+tel=)
    std::string_view build_type{};        ///< "Debug" (+bt=); Release/Default = leer
    std::string_view gate_contribution{}; ///< die Gate-Beitraege (+gate=)
    std::string_view atomic128{};         ///< atomic128-id ("cx16"/"no_cx16")
    std::string_view atomic128_flags{};   ///< die konkreten Flags dieser Option ("-mcx16")
};

// -- NB/CX-2: DIE INTERNE INJEKTIVITAET DES RENDERERS --------------------------------------------------
//
// DER BEFUND, DEN DAS HEILT (Codex-Nachreview [MAJOR]): der Renderer klebte key/value roh mit ';', '=',
// '{', '}' und '@' aneinander -- ohne Escaping, ohne Laengenpraefix, ohne Zeichenvorrats-Pruefung. Damit war
// er NICHT injektiv, und zwar nicht theoretisch, sondern mit drei konkreten Kollisionen:
//     {simd="avx2;ceb=8.0", ceb=""}            und {simd="avx2", ceb="8.0"}       -> beide "..;ext=avx2;ceb=8.0"
//     {opt="O3{-funroll}", opt_flags=""}       und {opt="O3", opt_flags="-funroll"}
//     {cxx_dialect="gcc-13.2.0", realversion=""} und {cxx_dialect="gcc", realversion="13.2.0"}
// Zwei VERSCHIEDENE Toolchain-Belegungen mit demselben Glied heissen: derselbe Fingerprint, also ein
// falscher Skip -- exakt die Klasse Fehler, die das Glied [5] gerade beseitigen soll.
//
// WARUM ZEICHENVORRAT STATT ESCAPING: Escaping macht das Glied laenger, braucht einen Unescaper (den es
// nirgends gibt, weil niemand das Glied zerlegt) und verschiebt das Problem auf das Escape-Zeichen. Der
// Zeichenvorrat ist die schaerfere und billigere Zusage: die fuenf STRUKTUR-Zeichen kommen in keinem WERT
// vor, also ist die Zerlegung des Glieds eindeutig -- und ein Verstoss ist FAIL-LOUD, nicht still.
// Der reale Zeichenvorrat der Werte (Achsen-Ids, Compiler-Flags, Versionen) enthaelt keines der fuenf.

/// Die Zeichen, die im Toolchain-Glied STRUKTUR sind und deshalb in keinem WERT vorkommen duerfen.
/// '\n' ist zusaetzlich verboten -- es ist der Domain-Separator der Glied-Folge (OF-M3-1).
inline constexpr std::string_view kToolchainGliedStrukturZeichen = ";={}@";

/// Ein WERT des Glieds ist wohlgeformt, wenn er kein Struktur- und kein Steuerzeichen traegt.
[[nodiscard]] constexpr bool toolchain_wert_ist_wohlgeformt(std::string_view v) noexcept {
    for (char const c : v) {
        if (c == '\n' || c == '\r') return false;
        if (kToolchainGliedStrukturZeichen.find(c) != std::string_view::npos) return false;
    }
    return true;
}

/// Der DIALEKT traegt zusaetzlich kein '-': der Renderer klebt ihn mit '-' an die Realversion. Ohne diese
/// Zusatz-Regel blieben {"gcc-13.2.0", ""} und {"gcc", "13.2.0"} ununterscheidbar (dritte Kollision oben).
[[nodiscard]] constexpr bool toolchain_dialekt_ist_wohlgeformt(std::string_view v) noexcept {
    return toolchain_wert_ist_wohlgeformt(v) && v.find('-') == std::string_view::npos;
}

/// Nennt das ERSTE verletzende Feld beim Namen (leer == alles wohlgeformt). Ein benannter Befund ist der
/// Unterschied zwischen einer brauchbaren Fehlerzeile und "irgendwas am Stempel ist kaputt".
[[nodiscard]] constexpr std::string_view toolchain_stamp_parts_diagnose(ToolchainStampParts const& p) noexcept {
    if (!toolchain_dialekt_ist_wohlgeformt(p.cxx_dialect)) return "cxx_dialect";
    if (!toolchain_wert_ist_wohlgeformt(p.cxx_realversion)) return "cxx_realversion";
    if (!toolchain_wert_ist_wohlgeformt(p.opt)) return "opt";
    if (!toolchain_wert_ist_wohlgeformt(p.opt_flags)) return "opt_flags";
    if (!toolchain_wert_ist_wohlgeformt(p.simd)) return "simd";
    if (!toolchain_wert_ist_wohlgeformt(p.ceb)) return "ceb";
    if (!toolchain_wert_ist_wohlgeformt(p.target_isa)) return "target_isa";
    if (!toolchain_wert_ist_wohlgeformt(p.telemetry)) return "telemetry";
    if (!toolchain_wert_ist_wohlgeformt(p.build_type)) return "build_type";
    if (!toolchain_wert_ist_wohlgeformt(p.gate_contribution)) return "gate_contribution";
    if (!toolchain_wert_ist_wohlgeformt(p.atomic128)) return "atomic128";
    if (!toolchain_wert_ist_wohlgeformt(p.atomic128_flags)) return "atomic128_flags";
    return {};
}

// Die CT-Erhebung selbst muss den Zeichenvorrat einhalten -- sonst koennte ausgerechnet der einzige Wert,
// den dieser Header SELBST beisteuert, das Glied unzerlegbar machen.
static_assert(toolchain_dialekt_ist_wohlgeformt(kDetectedCompilerDialect));
static_assert(toolchain_wert_ist_wohlgeformt(kDetectedCompilerRealVersion));

namespace detail {

/// Ein Segment `key=value` (leerer Wert => KEIN Segment, Regel wie im Suffix).
inline void toolchain_append(std::string& out, std::string_view key, std::string_view value) {
    if (value.empty()) return;
    if (!out.empty()) out += ';';
    out += key;
    out += '=';
    out += value;
}

/// Ein Achsen-Segment `key=<id>{<flags>}@X.Y.Zc`: id + (optional) die materialisierten Flags + der
/// Versions-Anker der Achse. Die Flags stehen IM Glied, weil sie per Owner-KERN Teil der Haupt-Achsen-
/// DEFINITION sind -- zwei opt_level-Optionen mit gleicher id, aber anderen Flags waeren sonst identisch.
inline void toolchain_append_axis(std::string& out, std::string_view key, std::string_view id, std::string_view flags,
                                  std::string_view raw_version) {
    if (id.empty()) return;
    if (!out.empty()) out += ';';
    out += key;
    out += '=';
    out += id;
    if (!flags.empty()) {
        out += '{';
        out += flags;
        out += '}';
    }
    out += '@';
    out += ::comdare::cache_engine::measurement::algo_semver_string(raw_version);
}

} // namespace detail

/// render_toolchain_stamp_glied(parts) -- DER EINE Renderer des Preimage-Glieds [5].
///
/// Form (Beispiel, voll belegt):
///   tc=1;cxx=gcc-16.1.0@1.0.0c;opt=O3{-O3}@1.0.0c;ext=avx512;ceb=8.0;bt=Debug;gate=avx512;atomic128=cx16{-mcx16}@1.0.0c
///
/// ALLE Felder leer => "" (die Identitaet). Das ist kein Sonderfall, sondern dieselbe Zusage wie beim
/// Overlay-Glied und beim Zellwert-Set: ein nicht injizierter Wert veraendert das Preimage NICHT.
///
/// NB/CX-2: FAIL-LOUD statt still. Ein Feld mit Struktur-Zeichen wuerde ein Glied erzeugen, das zwei
/// verschiedene Belegungen gleich rendert -- der Renderer wirft dann eine BENANNTE Fehlerklasse, statt eine
/// Kollision in den Fingerprint zu schreiben. Der Wurf ist der einzige richtige Ausgang: ein degradierter
/// Ersatzwert waere wieder eine stille Identitaets-Aussage.
[[nodiscard]] inline std::string render_toolchain_stamp_glied(ToolchainStampParts const& p) {
    if (std::string_view const feld = toolchain_stamp_parts_diagnose(p); !feld.empty())
        throw std::invalid_argument(
            std::string{"fehlerklasse=stempel_injektivitaet: das Toolchain-Glied [5] kann nicht injektiv "
                        "gerendert werden -- das Feld '"} +
            std::string{feld} +
            "' traegt ein STRUKTUR-Zeichen (';', '=', '{', '}', '@'), '-' im Dialekt oder einen "
            "Zeilenumbruch. Zwei verschiedene Toolchain-Belegungen wuerden dann dasselbe Glied ergeben, "
            "also denselben Fingerprint -- und damit einen falschen Skip. Den WERT korrigieren, nicht die "
            "Wache.");
    std::string felder;
    // Reihenfolge == kToolchainGliedKeys == kSuffixSegmentOrder (+ atomic128 am Ende).
    if (!p.cxx_dialect.empty()) {
        std::string cxx{p.cxx_dialect};
        if (!p.cxx_realversion.empty()) {
            cxx += '-';
            cxx += p.cxx_realversion;
        }
        detail::toolchain_append_axis(felder, kToolchainGliedKeys[0], cxx, {}, kToolchainAxisVersions[0].version);
    }
    detail::toolchain_append_axis(felder, kToolchainGliedKeys[1], p.opt, p.opt_flags,
                                  kToolchainAxisVersions[1].version);
    detail::toolchain_append(felder, kToolchainGliedKeys[2], p.simd);
    detail::toolchain_append(felder, kToolchainGliedKeys[3], p.ceb);
    detail::toolchain_append(felder, kToolchainGliedKeys[4], p.target_isa);
    detail::toolchain_append(felder, kToolchainGliedKeys[5], p.telemetry);
    detail::toolchain_append(felder, kToolchainGliedKeys[6], p.build_type);
    detail::toolchain_append(felder, kToolchainGliedKeys[7], p.gate_contribution);
    detail::toolchain_append_axis(felder, kToolchainGliedKeys[8], p.atomic128, p.atomic128_flags,
                                  kToolchainAxisVersions[2].version);
    if (felder.empty()) return {}; // IDENTITAET: kein injizierter Wert => kein Byte im Preimage
    std::string out{"tc="};
    out += kToolchainStampGliedFormat;
    out += ';';
    out += felder;
    return out;
}

} // namespace comdare::cache_engine::abi
