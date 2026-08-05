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

#include <cache_engine/measurement/algo_semver.hpp> // algo_semver_string (X.Y.Z[Flag]-Voll-Form, Owner-Q10)

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace comdare::cache_engine::abi {

/// Format-Version des Toolchain-Glieds, unabhaengig von kAnatomyFingerprintFormat. Bumpen, sobald sich
/// Trennzeichen, Feldauswahl oder Feld-Reihenfolge aendern. Ein Bump ist per Konstruktion ein
/// Fingerprint-Bruch (das Glied steht im Preimage) und erzwingt damit Neubau statt stiller Fehl-Vergleiche.
inline constexpr std::string_view kToolchainStampGliedFormat = "1";

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
[[nodiscard]] inline std::string render_toolchain_stamp_glied(ToolchainStampParts const& p) {
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
