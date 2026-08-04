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

// Ordnungs-Anker: die bindende Form beginnt mit cxx, opt, ext -- in dieser Folge. Wer sie umsortiert,
// bricht hier compile-time und muss die Aenderung als das ausweisen, was sie ist: ein Byte-Ereignis
// fuer jede build_version, jedes .version-Sidecar und jeden Cache-Key.
static_assert(kSuffixSegmentOrder[0] == std::string_view{"+cxx="});
static_assert(kSuffixSegmentOrder[1] == std::string_view{"+opt="});
static_assert(kSuffixSegmentOrder[2] == std::string_view{"+ext="});
// OP-7: gate steht am ENDE, nach +bt.
static_assert(kSuffixSegmentOrder[kSuffixSegmentOrder.size() - 1] == std::string_view{"+gate="});
static_assert(kSuffixSegmentOrder[kSuffixSegmentOrder.size() - 2] == std::string_view{"+bt="});

} // namespace comdare::cache_engine::profile_facade
