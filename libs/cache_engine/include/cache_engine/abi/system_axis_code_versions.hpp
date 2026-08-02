#pragma once
// abi/system_axis_code_versions.hpp -- Single-Source der System-Achsen-CODE-Versionen (Bau W12 / G2-4 Schritt 3,
// Lager-Gate A2). Frueher waren diese Versionen in system_stamp_line() (anatomy_version_stamp.hpp) hartkodiert als
// 5x {"<achse>","code","v1"}. Jetzt: EINE Tabelle -> die Stempel-Funktion iteriert sie; ab jetzt ist jede System-
// Achsen-Code-Version einzeln BUMP-BAR (A10-X.Y.Z-Disziplin), ohne die Stempel-Funktion anzufassen.
//
// RENDER-NEUTRAL (A11): "v1.0.0" rendert ueber algo_semver_string identisch zu "1.0.0" wie zuvor "v1" -> der
// emittierte Quelltext + der gerenderte System-Stempel bleiben byte-identisch (Byte-Wache/CRC-Wache unberuehrt).
// "code" (der Algorithmus-Marker der System-Achse) lebt weiter in system_stamp_line, NICHT hier: diese Tabelle
// traegt NUR die pro-Achse bump-bare Code-Version, die Achse-zu-Marker-Zuordnung ist Sache der Stempel-Funktion.
//
// KEIN Runtime-Switch: reine constexpr-Tabelle (analog kMeasurementToolingRegistry). header-only, C++23.

#include <array>
#include <cstddef>
#include <string_view>

#include <cache_engine/measurement/algo_semver.hpp> // A13-M1b-Fixup: Flag-Grammatik-Wachen (Owner-Q3)

namespace comdare::cache_engine::abi {

/// Eine System-Haupt-Achse und ihre bump-bare Code-Version (die statische Code-Identitaet der Achse).
struct SystemAxisCodeVersion {
    std::string_view axis; ///< System-Haupt-Achsen-Name ("compiler"/"external_utils"/...)
    std::string_view
        version; ///< rohe Code-Version ("v1.0.0"); wird von build_axis_version_stamp_line zu X.Y.Z gerendert
};

// Single-Source: Drift einer 4. System-Haupt-Achse bricht hier compile-time (statt still 3 zu bleiben).
// A3 (O-8 Schritt 4): 5 -> 3. Der Owner-KERN kennt GENAU DREI System-Haupt-Achsen; die drei
// Abgaenge sind KEINE Loeschungen, sondern Umzuege in die ihnen zustehende Ebene:
//   * compiler   -> Unter-Achsen-GRUPPE der AEUSSEREN System-Komplex-Achse (O-1r; Schritt 6)
//   * scheduling -> sub_axis am target_isa-Komplex-Wrapper (Schritt 6)
//   * load_framework -> MESS-Realm, Meta-Meta-HAUPT-Achse des Planers (Ledger 69.1 / R-G;
//     verlaesst die System-Welt ERSATZLOS -- kein Platzhalter, keine Restzeile hier)
inline constexpr std::size_t kSystemAxisCodeCount = 3;

/// Die EINE Tabelle der System-Achsen-Code-Versionen -- Reihenfolge == kanonische System-Stempel-Ordnung (Section 43,
/// W12-A-1). Init "v1.0.0" (render-neutral zum frueheren "v1"); je Eintrag ab jetzt einzeln bump-bar.
///
/// BUMP-WACHE operating_system (A14 / Bump-Verbots-Wache (Design-3/RISIKEN; K5 selbst = probe_id-Versionierung, OS-U3), Owner-Entscheid E3 vom 02.08.2026) -- KOMMENTAR-WACHE,
/// bewusst kein static_assert, damit ein spaeterer, GEWOLLTER Bump nicht an dieser Datei haengenbleibt:
/// Die Einfuehrung der drei operating_system-UNTER-Achsen (os_version/kernel/build,
/// measurement/operating_system_sub_axes.hpp) darf den Eintrag "operating_system" NICHT bumpen. Die
/// Unter-Achsen sind RT-Unter-Achsen (A-15: nie im Binary-Stempel); der in die Binary kompilierte
/// Achsen-CODE aendert sich durch sie nicht. Ein Bump ginge in system_stamp_line ein, verschoebe den
/// SHA512 ALLER Neubauten gegen den Bestand und liesse das Lager-Skip-Gate an den Alt-Binaries
/// vorbeilaufen -- exakt die Folge, die Owner-E3 ausschliesst ("ansonsten muessen alle Binaries bei
/// Einfuehrung neu gebaut werden"). Wer hier dennoch bumpt, tut das aus einem ANDEREN Grund und muss
/// ihn benennen.
inline constexpr std::array<SystemAxisCodeVersion, kSystemAxisCodeCount> kSystemAxisCodeVersions{{
    {"target_isa", "v1.0.0"},
    {"operating_system", "v1.0.0"}, // A14/Bump-Verbot: NICHT wegen der Unter-Achsen bumpen (Wache oben)
    {"external_utils", "v1.0.0"},
}};

namespace detail {
// A13-M1b-Fixup (Review-BEFUND-1): Wachen-Doppelung wie an Organ-/Mess-Registries -- die System-Achsen-
// Code-Versionen stempeln in system_stamp_line und muessen der Owner-Q3-Flag-Grammatik folgen.
[[nodiscard]] consteval bool system_versionen_flag_konform() {
    for (auto const& e : kSystemAxisCodeVersions)
        if (::comdare::cache_engine::measurement::parse_algo_semver(e.version).has_hardware_flag() &&
            !::comdare::cache_engine::measurement::version_satisfies_cpu_only_policy(e.version))
            return false;
    return true;
}
[[nodiscard]] consteval bool system_versionen_cpu_pflicht() {
    for (auto const& e : kSystemAxisCodeVersions)
        if (!::comdare::cache_engine::measurement::version_satisfies_cpu_only_policy(e.version)) return false;
    return true;
}
} // namespace detail
static_assert(detail::system_versionen_flag_konform(),
              "System-Achsen-Code-Version mit FALSCHEM Hardware-Flag: im CPU-only-Scope ist GENAU 'c' (bzw. "
              "'ce') zulaessig (Owner-Q3 02.08.2026)");
#if COMDARE_VERSION_HW_FLAG_ENFORCE
static_assert(detail::system_versionen_cpu_pflicht(),
              "System-Achsen-Code-Version ohne CPU-Hardware-Flag: im CPU-only-Scope MUSS jede Version auf 'c' "
              "oder 'ce' enden (Owner-Q3 02.08.2026) -- COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
#endif

namespace detail {
[[nodiscard]] consteval bool system_axis_code_versions_complete() {
    for (std::size_t i = 0; i < kSystemAxisCodeCount; ++i) {
        if (kSystemAxisCodeVersions[i].axis.empty()) return false;
        if (kSystemAxisCodeVersions[i].version.empty()) return false;
    }
    return true;
}
} // namespace detail
static_assert(detail::system_axis_code_versions_complete(),
              "kSystemAxisCodeVersions: 3 Eintraege, axis/version nie leer");

} // namespace comdare::cache_engine::abi
