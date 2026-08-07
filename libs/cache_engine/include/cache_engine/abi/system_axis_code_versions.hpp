#pragma once
// abi/system_axis_code_versions.hpp -- Single-Source der System-Achsen-CODE-Versionen (Bau W12 / G2-4 Schritt 3,
// Lager-Gate A2). Frueher waren diese Versionen in system_stamp_line() (anatomy_version_stamp.hpp) hartkodiert als
// 5x {"<achse>","code","v1"}. Jetzt: EINE Tabelle -> die Stempel-Funktion iteriert sie; ab jetzt ist jede System-
// Achsen-Code-Version einzeln BUMP-BAR (A10-X.Y.Z-Disziplin), ohne die Stempel-Funktion anzufassen.
//
// RENDER-NEUTRAL (A11, Stand bis A13-M3/C4): "v1.0.0" rendert ueber algo_semver_string identisch zu "1.0.0" wie
// zuvor "v1" -> der emittierte Quelltext + der gerenderte System-Stempel blieben byte-identisch. SEIT A13-M3/C4
// steht die Tabelle auf "1.0.0.c" und rendert "1.0.0c" -- das ist das DEKLARIERTE Stempel-/SHA512-Byte-Ereignis
// der Owner-Q3-Flag-Migration (EIN Neuanker im M3-Fenster), keine stille Verschiebung.
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
        version; ///< rohe Code-Version (seit A13-M3/C4 "1.0.0.c"); build_axis_version_stamp_line rendert X.Y.Z[c]
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
/// W12-A-1). Init war "v1.0.0" (render-neutral zum frueheren "v1"); seit A13-M3/C4 "1.0.0.c" (Owner-Q3).
/// Je Eintrag einzeln bump-bar.
///
/// BUMP-WACHE operating_system (A14 / Bump-Verbots-Wache (Design-3/RISIKEN; K5 selbst = probe_id-Versionierung, OS-U3), Owner-Entscheid E3 vom 02.08.2026) -- seit B6
/// (Codex-Review 02.08.2026) HART als static_assert je Eintrag unter der Tabelle, nicht mehr nur als
/// Kommentar (der einzige maschinelle Anker war zuvor der test_m_w12-Golden-String, also indirekt):
/// Die Einfuehrung der drei operating_system-UNTER-Achsen (os_version/kernel/build,
/// measurement/operating_system_sub_axes.hpp) darf den Eintrag "operating_system" NICHT bumpen. Die
/// Unter-Achsen sind RT-Unter-Achsen (A-15: nie im Binary-Stempel); der in die Binary kompilierte
/// Achsen-CODE aendert sich durch sie nicht. Ein Bump ginge in system_stamp_line ein, verschoebe den
/// SHA512 ALLER Neubauten gegen den Bestand und liesse das Lager-Skip-Gate an den Alt-Binaries
/// vorbeilaufen -- exakt die Folge, die Owner-E3 ausschliesst ("ansonsten muessen alle Binaries bei
/// Einfuehrung neu gebaut werden"). Wer hier dennoch bumpt, tut das aus einem ANDEREN Grund und muss
/// ihn benennen.
inline constexpr std::array<SystemAxisCodeVersion, kSystemAxisCodeCount> kSystemAxisCodeVersions{{
    {"target_isa", "1.0.0.c"},
    {"operating_system", "1.0.0.c"}, // A14/Bump-Verbot: NICHT wegen der Unter-Achsen bumpen (Wache oben)
    {"external_utils", "1.0.0.c"},
}};

// -- B6 (Codex-Review 02.08.2026): die BUMP-WACHE ist ab hier MASCHINELL -------------------------------
// Je Eintrag ein expliziter static_assert auf (Achse, Version) an SEINEM Index. Vorher stand die Auflage
// NUR im Kommentar oben; der einzige maschinelle Anker war der test_m_w12-Golden-String -- also indirekt
// (er faellt erst auf, wenn jemand den erwarteten Stempel MIT nachzieht). Der Golden-String bleibt als
// ZWEITER Anker bestehen; diese Asserts sind der erste.
//
// BEWUSSTER BUMP == ASSERT MIT-AENDERN (DOPPEL-ABSICHT). Wer eine dieser Code-Versionen hebt, muss im
// SELBEN Commit die Zeile hier nachziehen UND das BYTE-EREIGNIS DEKLARIEREN: die Version geht ueber
// system_stamp_line in den System-Stempel, verschiebt den SHA512 ALLER Neubauten gegen den Bestand und
// laesst damit das Lager-Skip-Gate an den Alt-Binaries vorbeilaufen. Zwei bewusste Handgriffe statt eines
// versehentlichen -- genau die Folge, die Owner-E3 vom 02.08.2026 ausschliesst ("ansonsten muessen alle
// Binaries bei Einfuehrung neu gebaut werden").
//
// Der INDEX ist mit gepinnt (nicht nur die Version): die Reihenfolge dieser Tabelle IST die kanonische
// System-Stempel-Ordnung (Section 43 / W12-A-1). Eine reine Umsortierung waere sonst ein Byte-Ereignis,
// das keine der Wachen sieht.
//
// MIGRATIONS-NAHT (VOLLZOGEN in A13-M3/C4): die drei Literale stehen auf "1.0.0.c" (Owner-Q3) -- DIESE DREI
// ASSERTS SIND IM SELBEN COMMIT MITGEGANGEN. Das war Absicht: die Migration ist ein deklariertes
// Byte-Ereignis, kein stiller Nachzug (Naht-Liste (a) in measurement/algo_semver.hpp).
static_assert(kSystemAxisCodeVersions[0].axis == std::string_view{"target_isa"} &&
                  kSystemAxisCodeVersions[0].version == std::string_view{"1.0.0.c"},
              "System-Achsen-Code-Version [0] target_isa != \"1.0.0.c\": ein Bump ist ein STEMPEL-/SHA512-"
              "BYTE-EREIGNIS (system_stamp_line -> Lager-/Skip-Identitaet). Wenn er GEWOLLT ist, diese Zeile "
              "im selben Commit mit-aendern und das Byte-Ereignis deklarieren (Doppel-Absicht).");
static_assert(kSystemAxisCodeVersions[1].axis == std::string_view{"operating_system"} &&
                  kSystemAxisCodeVersions[1].version == std::string_view{"1.0.0.c"},
              "System-Achsen-Code-Version [1] operating_system != \"1.0.0.c\": die drei OS-UNTER-Achsen "
              "(os_version/kernel/build) sind RT-Unter-Achsen (A-15) und duerfen diesen Eintrag NICHT bumpen "
              "(Owner-E3 02.08.2026). Ein Bump aus einem ANDEREN Grund ist ein STEMPEL-/SHA512-BYTE-EREIGNIS: "
              "diese Zeile im selben Commit mit-aendern und den Grund benennen (Doppel-Absicht).");
static_assert(kSystemAxisCodeVersions[2].axis == std::string_view{"external_utils"} &&
                  kSystemAxisCodeVersions[2].version == std::string_view{"1.0.0.c"},
              "System-Achsen-Code-Version [2] external_utils != \"1.0.0.c\": ein Bump ist ein STEMPEL-/SHA512-"
              "BYTE-EREIGNIS (system_stamp_line -> Lager-/Skip-Identitaet). Wenn er GEWOLLT ist, diese Zeile "
              "im selben Commit mit-aendern und das Byte-Ereignis deklarieren (Doppel-Absicht).");

namespace detail {
// A13-M1b-Fixup (Review-BEFUND-1) + B12 (Codex-Review 02.08.2026): Wachen-Batterie wie an Organ-/Mess-
// Registries -- die System-Achsen-Code-Versionen stempeln in system_stamp_line. Die Politik lebt EINMAL
// (measurement/algo_semver.hpp): parsbar (oder exakt der dokumentierte Sentinel) + nie experimentell +
// Flag-konform. Vorher prueften diese Wachen WEDER Parsbarkeit (junk fiel still auf @0.0.0) NOCH das 'e'.
[[nodiscard]] consteval bool system_versionen_wohlgeformt() {
    for (auto const& e : kSystemAxisCodeVersions)
        if (!::comdare::cache_engine::measurement::ce_owned_version_is_wellformed(e.version)) return false;
    return true;
}
[[nodiscard]] consteval bool system_versionen_cpu_pflicht() {
    for (auto const& e : kSystemAxisCodeVersions)
        if (!::comdare::cache_engine::measurement::ce_owned_version_satisfies_cpu_enforce(e.version)) return false;
    return true;
}
} // namespace detail
static_assert(detail::system_versionen_wohlgeformt(),
              "System-Achsen-Code-Version verletzt die ce-Registry-Politik: (a) UNPARSBAR (und nicht der "
              "dokumentierte Sentinel \"0.0.0\") -- ein junk-Literal wuerde still als @0.0.0 in "
              "system_stamp_line stempeln; oder (b) experimentelles 'e' (AUSSCHLIESSLICH die Pruefling-"
              "Markierung, Owner-E2 02.08.2026); oder (c) FALSCHES Hardware-Flag (im CPU-only-Scope GENAU "
              "'c' bzw. 'ce', Owner-Q3 02.08.2026)");
#if COMDARE_VERSION_HW_FLAG_ENFORCE
static_assert(detail::system_versionen_cpu_pflicht(),
              "System-Achsen-Code-Version ohne CPU-Hardware-Flag (oder mit 'e'): im CPU-only-Scope MUSS jede "
              "Version auf 'c' enden und darf NIE experimentell sein (Owner-Q3/E2 02.08.2026) -- "
              "COMDARE_VERSION_HW_FLAG_ENFORCE ist scharf");
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
