#pragma once
// profile_facade/system_cell_values_naht.hpp -- W10-C4 (Bauplan-Dossier 20260803, Sektion 1/2): die EINE
// Stelle, an der die System-ZELLWERTE dieses Baus BESTIMMT und in die Compile-Define-Wertform gebracht
// werden.
//
// ARBEITSTEILUNG (WAS/WIE, Praezedenz compiler_tag/compile_for_perm): die FACADE loest die beiden
// lauf-konstanten Zellen auf (Ziel-ISA, OS-FAMILIE), die PERM-SCHLEIFE ergaenzt je Zelle ihre eigene
// Schleifen-Variable (simd_id) und ruft compose_system_cell_values. Es gibt damit GENAU EINE Ordnung und
// GENAU EINE Wertform; abi/system_cell_values.hpp (C1) prueft sie an der Define-Naht compile-hart nach.
//
// WERTE-HERKUNFT (je Wert EINE Quelle, keine Zweit-Ableitung -- Dossier Sektion 1):
//   target_isa        CT der BAU-PLATTFORM (platform_detection.cmake -> COMDARE_ARCH_*), ueberschrieben von
//                     einer im Profil DEKLARIERTEN Ziel-ISA (Cross-Bau). Nativer Bau => Host-ISA.
//   operating_system  CT-FAMILIEN-Zelle (platform_detection.cmake -> COMDARE_OS_*). Es gibt genau drei
//                     Familien (linux/windows/macos, OP-10); die <machine>-os-Attribute sind ERWARTUNG,
//                     nicht Werte-Quelle (Ledger-Nachtrag A14/OS-U2).
//   simd              DIESELBE Zell-Variable der optxsimd-Schleife, aus der auch march_flag,
//                     ZellKoordinate[f] und die .variant-simd-Signatur kommen -- EINE Quelle.
//   external_utils    KEIN Zellwert (E-2-Default (a)): der Hub hat keine eigene Zelle.
//
// n/a-STATT-NULL (bindend): ein nicht bestimmbarer oder nicht grammatik-konformer Wert wird zum SENTINEL
// `na` normalisiert -- NIE zu einem leeren Feld und NIE zu einer 0. Der Aufrufer prueft danach
// abi::system_cell_values_contain_na und schaltet FAIL-CLOSED (kein Lager-Rueckschrieb + klassifizierte
// Zeile). Das deckt die riscv64-Auflage mechanisch: eine ISA ohne Achsen-Glied ergibt `na`.
//
// UNVERDRAHTETER AUFRUFER == IDENTITAET: fehlt eine der beiden Facade-Zellen (leerer String), liefert
// compose_system_cell_values eine LEERE Wertform -- kein Define, byte-identischer Bau. Das ist der
// golden-neutrale Grundpfad fuer jeden Host, der die Naht (noch) nicht belegt.
//
// header-only, keine Bau-Abhaengigkeit; die Achsen-Ids kommen aus den Achsen-Single-Sources, nie als Literal.

#include <cache_engine/abi/system_cell_values.hpp> // C1: Token-Grammatik, na-Sentinel, Schluessel-Ordnung
#include <system_axes/operating_system_axis.hpp>   // os_family_id(): linux/windows/macos
#include <system_axes/target_isa_system_axis.hpp>  // target_isa_id(): x86_64/aarch64 + kAllTargetIsaIds

#include <string>
#include <string_view>

namespace comdare::cache_engine::profile_facade {

namespace detail {

namespace cm  = ::comdare::cache_engine::measurement;
namespace cea = ::comdare::cache_engine::abi;

/// Die ISA-Zelle der BAU-PLATTFORM, rein compile-time. Die Praeprozessor-Schalter sind exakt die, die
/// platform_detection.cmake setzt und die perm_mess_defines() bereits an jede Tier-Quelle weiterreicht
/// (-DCOMDARE_ARCH_X86_64=1 / -DCOMDARE_ARCH_ARM64=1) -- der Zellwert und die einkompilierte Arch-Definition
/// koennen deshalb nicht auseinanderlaufen.
///
/// riscv64 hat HEUTE kein Achsen-Glied (kAllTargetIsaIds kennt es nicht, E-3-Default (a)) -- ein riscv64-Bau
/// faellt bewusst auf den na-Sentinel und damit in die FAIL-CLOSED-Regel (Ledger-Auflage: opt-in-CI ohne
/// Lager-Rueckschrieb, bis die Achse traegt).
[[nodiscard]] consteval std::string_view build_platform_isa_cell() noexcept {
#if defined(COMDARE_ARCH_X86_64)
    return cm::X86_64TargetIsa::target_isa_id();
#elif defined(COMDARE_ARCH_ARM64)
    return cm::Aarch64TargetIsa::target_isa_id();
#else
    return cea::kSystemCellValueNa;
#endif
}

/// Die OS-FAMILIEN-Zelle der Bau-Plattform, rein compile-time. FAMILIE, nicht Instanz: os_version/kernel/build
/// sind RT-Unter-Achsen und stehen NIE im Stempel (A-15); ihre Zuordnung laeuft ueber Mess-Spalten/Dateinamen.
[[nodiscard]] consteval std::string_view build_platform_os_family_cell() noexcept {
#if defined(COMDARE_OS_LINUX)
    return cm::LinuxOperatingSystem::os_family_id();
#elif defined(COMDARE_OS_WINDOWS)
    return cm::WindowsOperatingSystem::os_family_id();
#elif defined(COMDARE_OS_MACOS)
    return cm::MacosOperatingSystem::os_family_id();
#else
    return cea::kSystemCellValueNa;
#endif
}

/// Ein Zellwert-Token, das die C1-Grammatik nicht erfuellt, ist NICHT BESTIMMBAR -- es reist als `na`, statt
/// eine ungrammatische Define-Wertform zu erzeugen (die jeden Tier-Bau compile-hart braeche und den Fehler
/// als Compiler-Rauschen statt als klassifizierte Zeile zeigte). Damit gibt es GENAU EINEN Fehl-Pfad.
[[nodiscard]] inline std::string_view normalize_system_cell_token(std::string_view token) noexcept {
    return cea::system_cell_token_is_wellformed(token) ? token : cea::kSystemCellValueNa;
}

} // namespace detail

/// Die ISA-Zelle der Bau-Plattform (CT). `na`, wenn die Plattform kein Achsen-Glied hat.
inline constexpr std::string_view kSystemCellBuildIsa = detail::build_platform_isa_cell();

/// Die OS-Familien-Zelle der Bau-Plattform (CT). `na`, wenn die Familie kein Achsen-Glied hat.
inline constexpr std::string_view kSystemCellBuildOsFamily = detail::build_platform_os_family_cell();

static_assert(::comdare::cache_engine::abi::system_cell_token_is_wellformed(kSystemCellBuildIsa),
              "W10-C4: die ISA-Zelle der Bau-Plattform muss der Token-Grammatik [a-z0-9_] genuegen "
              "(der na-Sentinel tut das ebenfalls) -- sonst koennte sie die Stempel-Zeile verlassen.");
static_assert(::comdare::cache_engine::abi::system_cell_token_is_wellformed(kSystemCellBuildOsFamily),
              "W10-C4: die OS-Familien-Zelle der Bau-Plattform muss der Token-Grammatik [a-z0-9_] genuegen.");

/// resolve_system_cell_target_isa(deklariert) -- die EINE Aufloesung der ISA-Zelle.
///   deklariert LEER      -> die CT-Zelle der Bau-Plattform (nativer Bau; kSystemCellBuildIsa)
///   deklariert BEKANNT   -> genau dieser Wert (Cross-Bau; die Deklaration ist das Ziel, nicht der Host)
///   deklariert UNBEKANNT -> `na` (die Achse kennt das Ziel nicht -> nicht zuordbar -> FAIL-CLOSED)
[[nodiscard]] inline std::string_view resolve_system_cell_target_isa(std::string_view deklariert) noexcept {
    if (deklariert.empty()) return kSystemCellBuildIsa;
    for (auto const& bekannt : ::comdare::cache_engine::measurement::kAllTargetIsaIds)
        if (bekannt == deklariert) return deklariert;
    return ::comdare::cache_engine::abi::kSystemCellValueNa;
}

/// compose_system_cell_values(isa, os_family, simd) -- die Define-WERTFORM dieser Zelle, in der kanonischen
/// Ordnung von abi::kSystemCellValueKeys.
///
/// LEER (== Identitaet), wenn eine der drei Zellen unbelegt ist: ein Host, der die Naht nicht verdrahtet,
/// baut byte-identisch weiter. Belegte, aber ungrammatische Tokens werden zu `na` normalisiert (s.o.).
[[nodiscard]] inline std::string compose_system_cell_values(std::string_view target_isa,
                                                            std::string_view operating_system, std::string_view simd) {
    if (target_isa.empty() || operating_system.empty() || simd.empty()) return {};
    std::string out;
    out += ::comdare::cache_engine::abi::kSystemCellValueKeys[0];
    out += '=';
    out += detail::normalize_system_cell_token(target_isa);
    out += ';';
    out += ::comdare::cache_engine::abi::kSystemCellValueKeys[1];
    out += '=';
    out += detail::normalize_system_cell_token(operating_system);
    out += ';';
    out += ::comdare::cache_engine::abi::kSystemCellValueKeys[2];
    out += '=';
    out += detail::normalize_system_cell_token(simd);
    return out;
}

/// Das Compiler-Argument, das die Wertform als C-STRING-LITERAL in die Tier-Uebersetzung traegt --
/// -DCOMDARE_SYSTEM_CELL_VALUES=\"...\".
///
/// WARUM DIE BACKSLASHES: die Argumente reisen ueber eine gcc-Response-Datei (@rsp, build_orchestrator
/// make_gpp_compile_fn). Deren Grammatik ist dokumentiert: Optionen trennt Whitespace, und "any character
/// (including a backslash) may be included by prefixing the character with a backslash". Die Wertform selbst
/// enthaelt weder Whitespace noch Backslash (Zeichenvorrat [a-z0-9_] + '=' + ';'), also reicht das Escapen der
/// beiden Anfuehrungszeichen -- sie muessen im MAKRO-WERT landen, damit COMDARE_SYSTEM_CELL_VALUES zu einem
/// String-Literal expandiert (Muster COMDARE_GN_ALGO_SIG / COMDARE_OVERLAY_SOURCE_HASH).
///
/// PLATTFORM-VERMERK (ehrlich, nicht verschwiegen): geprueft und gefahren wird diese Form am POSIX-Weg
/// (make_gpp_compile_fn, @rsp). Der MSVC-Weg (make_msvc_compile_fn, std::system-Kommandozeile) braucht eine
/// EIGENE Escaping-Entscheidung; er baut heute keine Tier-Binaries der Flotte und wird deshalb hier NICHT
/// still mitbehauptet.
///
/// LEERE Wertform => LEERES Argument => kein Define => Identitaet.
[[nodiscard]] inline std::string system_cell_values_define_arg(std::string_view werte) {
    if (werte.empty()) return {};
    std::string out = "-DCOMDARE_SYSTEM_CELL_VALUES=\\\"";
    out += werte;
    out += "\\\"";
    return out;
}

namespace detail {

/// ORDNUNGS-WACHE: die drei Schluessel dieser Wertform sind die drei Schluessel der C1-Single-Source, in
/// DIESER Reihenfolge. Wer dort umsortiert oder umbenennt, bricht hier compile-time -- statt eine Wertform zu
/// erzeugen, die der Vervollstaendiger still an die falsche Achse haengte.
[[nodiscard]] consteval bool cell_value_order_matches_single_source() {
    return cea::kSystemCellValueKeyCount == 3 && cea::kSystemCellValueKeys[0] == std::string_view{"target_isa"} &&
           cea::kSystemCellValueKeys[1] == std::string_view{"operating_system"} &&
           cea::kSystemCellValueKeys[2] == std::string_view{"simd"};
}

} // namespace detail

static_assert(detail::cell_value_order_matches_single_source(),
              "W10-C4: die Zellwert-Wertform dieser Naht ist aus abi::kSystemCellValueKeys gelaufen. Die "
              "Ordnung (target_isa, operating_system, simd) ist die EINE Ordnung -- eine zweite waere eine "
              "zweite Wahrheit.");

} // namespace comdare::cache_engine::profile_facade
