// profile_facade/system_version_suffix.hpp -- die EINZIGE Quelle des System-Achsen-build_version-Suffix
// (Lane F R3, O-8 Schritt 10; Traeger-Datei aus der T-c-Wachen-Luecke des Registry-Generators).
//
// WAS HIER GELOEST WIRD: der Suffix wurde an DREI Orten imperativ zusammengeklebt, in DREI verschiedenen
// Reihenfolgen -- die Facade-Naht mit "+ext+cxx+opt", die Perm-Schleife mit "+cxx+opt+ext" und der
// Cache-Key-Builder noch einmal fuer sich. Drei Konkatenations-Ketten heissen: es gibt keine Ordnung, die
// man PRUEFEN koennte, nur drei, die man LESEN muss. Genau deshalb konnte die Divergenz W-6/W-13
// entstehen und unbemerkt bleiben, und genau deshalb konnte die vierte Ordnungs-Wache des
// Registry-Generators (T-c) nicht gebaut werden: es gab nichts Deklaratives, wogegen sie haette
// vergleichen koennen.
//
// AUFLOESUNG: die Ordnung ist ab hier ein DATUM (kSuffixSegmentOrder) und die Zusammensetzung EINE
// Funktion. Die bindende Form ist die der Perm-Schleife -- "+cxx+opt+ext" --; die beiden anderen Orte
// fallen darauf. Das ist eine bewusste Suffix-AENDERUNG an der Facade-Naht (golden-veraendernd, gedeckt
// durch das O-8-GO), keine Umformatierung.
//
// D2.8(ii): no_extension emittiert KEIN "+ext="-Segment. Das ist Teil der bindenden Form, nicht ein
// Sonderfall der Perm-Schleife -- ein leeres Glied erzeugt nie ein leeres Segment.
//
// Ledger 70.6: der build_version-Suffix IST die Stempel-Variable-Version der System-Achsen. Er traegt
// also die BUILD-Version an der Stelle, an der die Achsen-Zeile die Algorithmus-Version traegt; beide
// Wege beschreiben dieselbe System-Achsen-Belegung, nur aus verschiedener Richtung.
//
// OP-7: das gate=[...]-Segment steht am ENDE der Ordnung. Grund ist Praefix-Stabilitaet -- alles, was
// heute Suffixe vergleicht oder zerlegt, sieht den unveraenderten Anfang und ein Anhaengsel, statt einen
// Einschub mitten in der Kette.

#pragma once

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // W10-C4: COMDARE_ANATOMY_ABI_MAJOR + kCebContractCodegenMinor
#include <cache_engine/abi/toolchain_stamp_glied.hpp>      // O-2/C-2: kToolchainGliedKeys (Doppel-Wahrheits-Wache)

#include <array>
#include <string>
#include <string_view>

namespace comdare::cache_engine::profile_facade {

/// ceb_contract_version_text() -- der WERT des +ceb=-Glieds: "<ABI-Major>.<codegen-Minor>".
///
/// W10-C4 (Bauplan-Dossier 20260803, Sektion 1/2; Manager-Entscheid W10-M2): dieser Text wurde an VIER
/// Orten getrennt zusammengesetzt (Einzel-Pfad-Suffix, --version-Block, Cache-Key-Naht, CI-Key-Druck) --
/// und ab C4 braucht ihn zusaetzlich JEDE der beiden Perm-Schleifen. Vier Kopien einer Konkatenation sind
/// exakt der Zustand, aus dem die W-6/W-13-Divergenz dieses Headers entstanden ist. Deshalb steht die
/// Zusammensetzung ab hier EINMAL, direkt neben der Ordnung, in die ihr Glied gehoert.
///
/// Die WERTE bleiben unveraendert Single-Source: der Major kommt AUTOMATISCH aus COMDARE_ANATOMY_ABI_MAJOR
/// (jeder echte ABI-Bruch bumpt ihn), der Minor von Hand aus abi::kCebContractCodegenMinor (die EINE
/// decl-Quelle). Immer non-empty, enthaelt immer genau einen '.'.
[[nodiscard]] inline std::string ceb_contract_version_text() {
    return std::to_string(COMDARE_ANATOMY_ABI_MAJOR) + "." +
           std::to_string(::comdare::cache_engine::abi::kCebContractCodegenMinor);
}

/// Die DEKLARATIVE Segment-Ordnung. Sie ist die Wahrheit, gegen die Wachen vergleichen duerfen
/// (T-c/Schritt 11); compose_system_version_suffix unten folgt ihr Zeile fuer Zeile.
inline constexpr std::array<std::string_view, 8> kSuffixSegmentOrder = {
    "+cxx=", "+opt=", "+ext=", "+ceb=", "+target=", "+tel=", "+bt=", "+gate="};

/// Die Glieder des Suffix. LEER heisst IMMER "kein Segment" -- es gibt keinen Zweig, der ein leeres
/// Glied als leeres Segment schreibt. Der Aufrufer fuellt nur, was er wirklich weiss.
struct SystemVersionSuffixParts {
    std::string_view cxx{};               ///< Compiler-Tag (+cxx=)
    std::string_view opt{};               ///< opt_level-id (+opt=)
    std::string_view simd{};              ///< simd-id (+ext=); no_extension wird vom Aufrufer als leer uebergeben
    std::string_view ceb{};               ///< "<abi_major>.<codegen_minor>" (+ceb=)
    std::string_view target_isa{};        ///< NUR bei Ziel != Host (+target=)
    std::string_view telemetry{};         ///< NUR bei abweichendem Regime, z.B. "silent" (+tel=)
    std::string_view build_type{};        ///< "Debug" (+bt=); Release/Default = leer
    std::string_view gate_contribution{}; ///< OP-7 / Ledger 70.9: die Gate-Beitraege (+gate=), leer = kein Segment
};

/// Setzt den Suffix in der EINEN Ordnung zusammen. Reine Funktion: gleiche Glieder, gleicher String.
[[nodiscard]] inline std::string compose_system_version_suffix(SystemVersionSuffixParts const& p) {
    std::string out;
    auto const  append = [&out](std::string_view key, std::string_view value) {
        if (value.empty()) return; // leeres Glied => KEIN Segment (D2.8(ii), Ledger 70.9)
        out += key;
        out += value;
    };
    append(kSuffixSegmentOrder[0], p.cxx);
    append(kSuffixSegmentOrder[1], p.opt);
    append(kSuffixSegmentOrder[2], p.simd);
    append(kSuffixSegmentOrder[3], p.ceb);
    append(kSuffixSegmentOrder[4], p.target_isa);
    append(kSuffixSegmentOrder[5], p.telemetry);
    append(kSuffixSegmentOrder[6], p.build_type);
    append(kSuffixSegmentOrder[7], p.gate_contribution);
    return out;
}

/// toolchain_stamp_parts_from_suffix_parts(...) -- die EINE Quelle fuer beide Wege (O-2/C-2).
///
/// Die sieben Felder, die es in BEIDEN Welten gibt (opt, ext, ceb, target, tel, bt, gate), werden hier
/// VERBATIM durchgereicht -- nicht neu erhoben, nicht umformatiert. Damit kann das Preimage-Glied [5] gar
/// nicht anderes behaupten als der build_version-Suffix; die Drift-Frage stellt sich strukturell nicht mehr.
///
/// DER cxx-FALL IST BEWUSST NICHT SYMMETRISCH und darf es nicht sein: der Suffix traegt seit jeher den
/// TREIBER-Tag ("g++-16") -- er ist Transport-/Cache-Pfad-Bestandteil und muss byte-stabil bleiben. Das
/// Glied verlangt per G-C4/OE-C zusaetzlich die REAL ERKANNTE Compiler-Version, weil "g++-16" nicht
/// zwischen 16.1.0 und 16.2.0 unterscheidet und zwei so gebaute Binaries verschieden sind. Deshalb kommen
/// Dialekt und Realversion als eigene Argumente herein; sie sind KEINE Ableitung aus dem Treiber-Tag (das
/// waere die verbotene Zweitwahrheit), sondern die Erhebung der bauenden Stufe.
///
/// NB2-1: der TREIBER-TAG steht ab jetzt in BEIDEN Welten -- im Suffix als Transport, im Glied als
/// Identitaets-Diskriminator (abi/toolchain_stamp_glied.hpp, Regel R1). Er wird hier VERBATIM aus p.cxx
/// durchgereicht, also aus derselben EINEN Quelle wie das Suffix-Segment "+cxx=" -- damit kann das Glied
/// gar keinen anderen Treiber nennen als der Pfad, unter dem gebaut wird. Die Asymmetrie schrumpft damit
/// auf das, was sie inhaltlich ist: das Glied traegt ZUSAETZLICH die (nur wenn gedeckt behauptete)
/// Realversion, der Suffix nicht.
///
/// Die glied-eigenen Felder (opt_flags, atomic128 + dessen Flags) haben im Suffix kein Gegenstueck: die
/// Flags sind per Owner-KERN abend-5 Teil der Haupt-Achsen-DEFINITION und gehoeren damit in die
/// IDENTITAET, waehrend der Suffix nur die id transportiert.
[[nodiscard]] inline ::comdare::cache_engine::abi::ToolchainStampParts
toolchain_stamp_parts_from_suffix_parts(SystemVersionSuffixParts const& p, std::string_view cxx_dialect,
                                        std::string_view cxx_realversion, std::string_view opt_flags = {},
                                        std::string_view atomic128 = {}, std::string_view atomic128_flags = {}) {
    ::comdare::cache_engine::abi::ToolchainStampParts t{};
    t.cxx_dialect       = cxx_dialect;
    t.cxx_realversion   = cxx_realversion;
    t.cxx_driver        = p.cxx;               // NB2-1: VERBATIM -- dieselbe Quelle wie das "+cxx="-Segment
    t.opt               = p.opt;               // VERBATIM aus der Suffix-Quelle
    t.opt_flags         = opt_flags;           // glied-eigen (Teil der Achsen-DEFINITION)
    t.simd              = p.simd;              // VERBATIM
    t.ceb               = p.ceb;               // VERBATIM (Perm-Pfad, G-C2 heilt Fall C)
    t.target_isa        = p.target_isa;        // VERBATIM
    t.telemetry         = p.telemetry;         // VERBATIM
    t.build_type        = p.build_type;        // VERBATIM
    t.gate_contribution = p.gate_contribution; // VERBATIM
    t.atomic128         = atomic128;           // glied-eigen
    t.atomic128_flags   = atomic128_flags;     // glied-eigen
    return t;
}

// Ordnungs-Anker: die bindende Form beginnt mit cxx, opt, ext -- in dieser Folge. Wer sie umsortiert,
// bricht hier compile-time und muss die Aenderung als das ausweisen, was sie ist: ein Byte-Ereignis
// fuer jede build_version, jedes .version-Sidecar und jeden Cache-Key.
static_assert(kSuffixSegmentOrder[0] == std::string_view{"+cxx="});
static_assert(kSuffixSegmentOrder[1] == std::string_view{"+opt="});
static_assert(kSuffixSegmentOrder[2] == std::string_view{"+ext="});
// OP-7: gate steht am ENDE, nach +bt.
static_assert(kSuffixSegmentOrder[kSuffixSegmentOrder.size() - 1] == std::string_view{"+gate="});
static_assert(kSuffixSegmentOrder[kSuffixSegmentOrder.size() - 2] == std::string_view{"+bt="});

// -- O-2/C-2: DIE DOPPEL-WAHRHEITS-WACHE (Suffix-Ordnung == Toolchain-Glied-Ordnung) -------------------
//
// Seit Format 3 stehen dieselben Toolchain-Glieder an ZWEI Orten: hier als build_version-SUFFIX (Transport
// und Provenienz -- .version, CSV, Cache-Pfad) und im Fingerprint-Preimage als Glied [5] (IDENTITAET). Das
// ist genau die Konstellation, aus der die W-6/W-13-Divergenz dieses Headers entstanden ist: zwei Ketten,
// die dasselbe behaupten und getrennt driften koennen. Diesmal ist die Kopplung BEWIESEN statt beschrieben.
//
// WARUM DIE WACHE HIER STEHT UND NICHT IM abi-HEADER: die Schichtung laeuft nur in EINE Richtung --
// profile_facade darf abi/ sehen, abi/ nie profile_facade/. Der einzige Ort, an dem beide Ordnungen
// gleichzeitig sichtbar sind, ist deshalb dieser.
//
// WAS SIE PRUEFT: die ersten acht Glied-Schluessel sind byte-gleich den acht Suffix-Segmenten ohne ihr
// fuehrendes '+' und ihr abschliessendes '='. Das neunte (atomic128) haengt im Glied HINTEN an und hat im
// Suffix bewusst kein Gegenstueck -- deshalb Praefix-Deckung statt Gleichheit der Laengen.
static_assert(kSuffixSegmentOrder.size() + 1 == ::comdare::cache_engine::abi::kToolchainGliedKeyCount,
              "O-2/C-2: das Toolchain-Glied traegt genau die acht Suffix-Segmente plus atomic128. Wer hier "
              "ein Segment ergaenzt oder streicht, muss kToolchainGliedKeys im selben Commit nachziehen -- "
              "sonst behaupten Suffix und Preimage verschiedene Toolchain-Identitaeten.");
static_assert(
    [] {
        for (std::size_t i = 0; i < kSuffixSegmentOrder.size(); ++i) {
            std::string_view const seg = kSuffixSegmentOrder[i];
            if (seg.size() < 3 || seg.front() != '+' || seg.back() != '=') return false;
            if (seg.substr(1, seg.size() - 2) != ::comdare::cache_engine::abi::kToolchainGliedKeys[i]) return false;
        }
        return true;
    }(),
    "O-2/C-2 DOPPEL-WAHRHEIT: die Suffix-Segment-Ordnung und die Feld-Ordnung des Toolchain-Glieds "
    "sind auseinandergelaufen. Beide beschreiben DIESELBE Toolchain-Wahl -- einmal als Transport, "
    "einmal als Identitaet; eine Drift hiesse, dass eine Binary anders gestempelt als gekeyt ist.");
static_assert(
    ::comdare::cache_engine::abi::kToolchainGliedKeys[::comdare::cache_engine::abi::kToolchainGliedKeyCount - 1] ==
        std::string_view{"atomic128"},
    "atomic128 ist das GLIED-EIGENE Feld und steht am Ende (kein Suffix-Gegenstueck).");

} // namespace comdare::cache_engine::profile_facade
