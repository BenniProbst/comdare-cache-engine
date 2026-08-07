#pragma once
// measurement/axis_error_traits.hpp -- FK-3 (A15 EBENE 2, ACHSEN-Ebene): die BINDUNG der bestehenden
// System-/Unter-/Komplex-/Mess-Achsen an die bestehende Taxonomie aus axis_error.hpp.
//
// OWNER-DIREKTIVE (17.07.2026, axis_error.hpp:4-5 verbatim): "Fehlerklassen und Behandlung sind fuer
// ALLE Achsen -> Unterachsen -> Algorithmen Pflicht." FK-0..FK-2 sind vollzogen, FK-5 (Algorithmen-Ebene)
// ist mit E-24 C9 gelandet. FK-3 ist die ACHSEN-Ebene und war laut A15-DESIGN "der KERN des
// Owner-Auftrags" -- am Ist zu 0 % umgesetzt (A15-fehlerklassen.md, Befund L1: "ACHSEN-EBENE KOMPLETT
// LEER ... alle 20 System-/Sub-/Komplex-/Meta-Meta-Achsen-Header 0 Treffer auf axis_error/ErrorClass/
// ErrorDomain"). Eigener grep vor dem Bau: `axis_error_traits` und `AxisErrorTraits` je 0 Treffer.
//
// WAS OHNE DIESE DATEI DIE STILLE VARIANTE WAERE (der Zweck, nicht die Vollstaendigkeit):
// eine Achse konnte bisher NICHT sagen, welche Fehler sie ueberhaupt hervorbringen kann. Die
// Klassifizierung geschah AUSSCHLIESSLICH an den Pipeline-Naehten (build_orchestrator-Exitcode-Kette,
// perm_runner-Catch, simd_build_gate) -- also DORT, wo der Fehler ankommt, nie dort, wo er entsteht.
// Folge: faellt eine Achse auf eine Art aus, die niemand aufgezaehlt hat, bekommt sie am Naht-Ende die
// naechstliegende Sammel-Klasse. Ein fehlender clang++-22 (ToolchainFehlt) und ein vom Compiler
// abgelehnter Achsen-Schnitt (CompileKombination) laufen dann unter demselben Etikett zusammen, und die
// Auswertung kann "das Werkzeug war nie da" nicht mehr von "der Code liess sich nicht uebersetzen"
// unterscheiden. Genau diese Klasse Defekt -- ein Zaehler, der nie "nicht erhoben" sagen konnte -- hat
// das Projekt mehrfach beschaeftigt (M-3a, PmcSystemAxis).
//
// -- WARUM EXTERNER TRAITS-HEADER UND KEIN REQUIREMENT IN topics::AxisConcept (OF-3, Empfehlung liegt) --
// Das A15-DESIGN nennt beide Wege und empfiehlt den externen: "DURCHSETZUNG als EXTERNE
// Vollstaendigkeits-Wache (static_assert-Walk ueber die Registry-Typlisten + Achsen-Aufzaehlung), NICHT
// als Requirement in topics::AxisConcept -- Letzteres braeche alle Achsen atomar". Diesem Weg wird
// gefolgt: KEIN Achsen-Header wird angefasst, alle bleiben byte-stabil. Der Traits-Header BINDET
// bestehende Typen an die bestehende Taxonomie und legt KEINE Parallelstruktur an -- er definiert
// insbesondere KEIN eigenes Enum, sondern fuehrt ausschliesslich Werte aus axis_error.hpp.
//
// -- TOTALITAET (K5-Muster, Praezedenz hardware_probe_factory.hpp:28-32) -----------------------------
// Das primaere Template ist DEKLARIERT, aber NICHT DEFINIERT. Eine Achse ohne bewusste Entscheidung ist
// damit ein unvollstaendiger Typ und bricht -- statt still auf irgendein Default-Verhalten zu fallen.
// DASS DAS SFINAE-FREUNDLICH IST (und der Concept unten sauber `false` liefert statt hart zu brechen),
// ist NICHT angenommen, sondern vor dem Bau auf BEIDEN Hausteilen empirisch geprobt: g++ und
// clang++-22 liefern fuer einen Typ ohne Spezialisierung uebereinstimmend `false`.
//
// -- KEIN SAMMEL-EINTRAG, UND WARUM DAS DIE TRAGENDE EIGENSCHAFT IST ---------------------------------
// Die Eintraege sind PARTIELLE SPEZIALISIERUNGEN auf exakte template-ids. C++ loest sie NICHT ueber die
// Vererbungskette auf: `AxisErrorTraits<ExternalUtilsFamilyAxis<D>>` faellt NICHT auf
// `AxisErrorTraits<SystemMetaMetaAxis<D>>` zurueck, obwohl die eine von der anderen erbt. Das ist hier
// ausdruecklich ERWUENSCHT -- ein vererbter Sammel-Eintrag waere exakt der stille Default, den FK-3
// abschaffen soll. Deshalb tragen die Wurzeln CebSystemAxis/CebSubAxis/SystemAxis/MeasurementMetaMetaAxis
// BEWUSST KEINEN Eintrag: an sie haengt sich keine konkrete Achse direkt, und ein Eintrag dort waere ein
// Auffangbecken, kein Urteil.
//
// -- AUF WELCHER EBENE EIN EINTRAG SITZT (eine Regel, kein Fall-zu-Fall) -----------------------------
// Der Eintrag sitzt auf der TIEFSTEN Ebene, auf der der Fehlerraum fuer alles darunter noch GLEICH ist.
//   * Konfig-Familien: die Familie entscheidet. JEDER Compiler kann fehlen -- GccCompilerAxis und
//     ClangCompilerAxis teilen ihren Fehlerraum, also traegt ihn CompilerSystemAxis<D>.
//   * Mess-Achsen: das BLATT entscheidet. WallClock/ObserverSnapshot/Pmc teilen ihn NICHT (am Code
//     abgelesen, s.u.) -- also tragen ihn die drei Blaetter einzeln und NICHT SystemAxis<D>.
//
// -- BOOST-HERMETIK: EINE WACHE, KEIN VORSORGLICHER SCHNITT (Korrektur am eigenen Zwischenstand) ------
// Das A15-DESIGN fuehrt die Boost-Hermetik als eigenes Risiko: "zieht der Traits-Header transitiv mp11
// in system_axis-/builder-Pfade, reisst die CI-Hermetik wie bei test_v41/test_m_contract
// (LEDGER:2014-2015) -- Meta-Meta-Wache strikt im Separat-Header". Beim Bau lagen die beiden
// Meta-Meta-Eintraege deshalb zunaechst in einem eigenen Header. DIESER SCHNITT IST ZURUECKGEBAUT,
// weil seine Begruendung am Ist NICHT TRUG:
//   * Ein erster grep nach "boost/" meldete hardware_meta_meta_axis.hpp als Boost-Zieher. Das waren
//     KOMMENTARZEILEN (:22 und :233), kein #include -- die Datei ist Boost-frei.
//   * Der EINZIGE echte Boost-Include unter measurement/ ist meta_meta_identity.hpp:29
//     (grep auf '^\s*#\s*include.*boost', also nur echte Include-Zeilen). Ihn zieht von den hier
//     beruehrten Achsen NIEMAND -- nur meta_meta_admission.hpp und eine Test-TU.
//   * Gegenprobe empirisch statt per Lesart: die Test-TU inkludiert diesen Header ALLEIN und prueft
//     BOOST_MP11_VERSION -- das Makro bleibt undefiniert. Dass es ueberhaupt anschlagen KANN, ist
//     ebenfalls belegt (ein nacktes <boost/mp11.hpp> setzt es auf 109100).
// GEBLIEBEN IST DIE WACHE, NICHT DIE STRUKTUR: der #error in der Test-TU deckt jetzt GANZ FK-3 ab
// statt nur einen abgetrennten Teil. Zieht kuenftig eine hier gebundene Achse mp11 herein, bricht der
// Bau -- und DANN wird der Separat-Header mit einem echten Grund gebaut, nicht auf Verdacht.

// -- WAS DIESER HEADER NICHT LEISTET (deklariert, nicht verschwiegen) --------------------------------
// Die Eintraege sitzen auf FAMILIEN-Ebene (bzw. am Blatt, wo die Familie sich teilt). Eine AUFLOESUNG
// vom konkreten Blatt-Typ zu seiner Familie -- also `AxisErrorTraits`-Zugriff mit GccCompilerAxis statt
// mit CompilerSystemAxis<D> -- gibt es hier NICHT. Sie braeuchte eine Familien-Typliste und damit
// mp11 in genau diesem Header, also den Hermetik-Bruch von oben. Heute hat sie keinen Konsumenten;
// sie ist deshalb eine bewusst offene Luecke und kein Versehen.
//
// ASCII-only.

#include <cache_engine/measurement/axis_error.hpp>

#include <cache_engine/measurement/compiler_atomic_sub_axis.hpp>
#include <cache_engine/measurement/compiler_system_axis.hpp>
#include <cache_engine/measurement/extension_hardware_system_axis.hpp>
#include <cache_engine/measurement/external_utils_family_axis.hpp>
#include <cache_engine/measurement/hardware_isa_system_axis.hpp>
#include <cache_engine/measurement/hardware_meta_meta_axis.hpp>
#include <cache_engine/measurement/load_framework_measurement_axis.hpp>
#include <cache_engine/measurement/operating_system_axis.hpp>
#include <cache_engine/measurement/operating_system_sub_axes.hpp>
#include <cache_engine/measurement/optimization_level_sub_axis.hpp>
#include <cache_engine/measurement/scheduling_system_axis.hpp>
#include <cache_engine/measurement/simd_sub_axis.hpp>
#include <cache_engine/measurement/system_axis.hpp>
#include <cache_engine/measurement/target_isa_complex_axis.hpp>
#include <cache_engine/measurement/target_isa_sub_axes.hpp>
#include <cache_engine/measurement/target_isa_system_axis.hpp>

#include <array>
#include <cstddef>

namespace comdare::cache_engine::measurement {

// -- Das primaere Template: DEKLARIERT, NICHT DEFINIERT (K5) -------------------------------------
/// AxisErrorTraits<Axis> -- der Fehlerraum EINER Achse. Jede Spezialisierung fuehrt drei Saetze:
///   domains     welche ErrorDomain(s) die Achse ueberhaupt beruehrt (Chain-of-Responsibility-Eingang),
///   d1_klassen  die Planer-/Compile-Zeit-Klassen, die sie hervorbringen kann,
///   d2_klassen  die Mess-Zell-Status, die sie hervorbringen kann.
/// Eine Achse ohne Spezialisierung ist ein UNVOLLSTAENDIGER Typ -- der beabsichtigte Compile-Bruch.
template <class Axis>
struct AxisErrorTraits;

namespace detail {

/// Ist ein Wert im Satz? (constexpr, kein <algorithm> -- der Header bleibt schlank.)
template <class Satz, class Wert>
[[nodiscard]] constexpr bool satz_enthaelt(Satz const& s, Wert w) noexcept {
    for (auto const e : s)
        if (e == w) return true;
    return false;
}

/// Ist der Satz dublettenfrei? Eine Dublette heisst: jemand hat angehaengt, ohne zu lesen, was schon
/// dastand -- und die Kardinalitaet des Satzes luegt ab da ueber die Zahl der wirklich verschiedenen
/// Faelle. Genau dieses Anhaengen-ohne-Hinsehen ist die RF-3-Lehre aus axis_error.hpp.
template <class Satz>
[[nodiscard]] constexpr bool satz_ist_dublettenfrei(Satz const& s) noexcept {
    for (std::size_t i = 0; i < s.size(); ++i)
        for (std::size_t j = i + 1; j < s.size(); ++j)
            if (s[i] == s[j]) return false;
    return true;
}

/// DIE VIER INVARIANTEN eines Fehlerraums. Jede hat eine benannte stille Variante -- ohne sie waere
/// die Deklaration Dekoration:
///  (I1) NICHT-LEER. Ein leerer Fehlerraum ist die Behauptung "diese Achse kann nicht scheitern".
///       Das ist fuer KEINE Achse wahr; es ist die Luecke, die aussieht wie eine Aussage.
///  (I2) DUBLETTENFREI (alle drei Saetze). s.o.
///  (I3) DOMAENE UND SATZ MUESSEN SICH DECKEN, in BEIDE Richtungen. Wer ErrorDomain::CompilerCompiler
///       fuehrt, aber keine einzige D1-Klasse aufzaehlt, hat einen Realm behauptet und ihn leer
///       gelassen -- die Naht bekaeme "irgendwas aus D1" und muesste raten. Wer umgekehrt D1-Klassen
///       fuehrt, ohne die Domaene zu nennen, laesst sie am Domaenen-Dispatch vorbeilaufen: die Klassen
///       stuenden da und wuerden nie gefragt. Dasselbe fuer Sample/d2_klassen.
///  (I4) KEIN Ok IM FEHLERRAUM. SampleStatus::Ok ist der Normalfall, kein Fehler. Eine Achse, die Ok
///       mitzaehlt, haette einen scheinbar nicht-leeren Fehlerraum und wuerde (I1) unterlaufen --
///       dieselbe Ueberlegung, mit der FK-5 "mess_ok" aus dem Organ-Fehlerraum haelt
///       (organ_axis_error_classes.hpp:43-45).
template <class A>
[[nodiscard]] constexpr bool traits_invarianten_erfuellt() noexcept {
    auto const& dom = AxisErrorTraits<A>::domains;
    auto const& d1  = AxisErrorTraits<A>::d1_klassen;
    auto const& d2  = AxisErrorTraits<A>::d2_klassen;

    if (dom.size() == 0) return false;                                                       // I1
    if (!satz_ist_dublettenfrei(dom)) return false;                                          // I2
    if (!satz_ist_dublettenfrei(d1)) return false;                                           // I2
    if (!satz_ist_dublettenfrei(d2)) return false;                                           // I2
    if (satz_enthaelt(dom, ErrorDomain::CompilerCompiler) != (d1.size() != 0)) return false; // I3
    if (satz_enthaelt(dom, ErrorDomain::Sample) != (d2.size() != 0)) return false;           // I3
    if (satz_enthaelt(d2, SampleStatus::Ok)) return false;                                   // I4
    return true;
}

} // namespace detail

/// Eine Achse hat einen deklarierten UND in sich stimmigen Fehlerraum.
template <class A>
concept AxisMitFehlerklassen = requires {
    { AxisErrorTraits<A>::domains };
    { AxisErrorTraits<A>::d1_klassen };
    { AxisErrorTraits<A>::d2_klassen };
} && detail::traits_invarianten_erfuellt<A>();

/// assert_axis_error_traits<A>() -- die fokussierte Wache mit SPRECHENDEN Meldungen. Sie leistet
/// dasselbe wie der Concept, sagt aber im Bruchfall, WELCHE der vier Invarianten gerissen ist. Was sie
/// NICHT leistet und was hier steht, statt verschwiegen zu werden: sie prueft nicht, ob der Satz fuer
/// DIESE Achse VOLLSTAENDIG ist -- ob also eine Achse eine Klasse hervorbringen kann, die sie nicht
/// deklariert hat. Diese Richtung ist in C++ nicht ausdrueckbar (sie waere eine Aussage ueber die
/// Rumpf-Anweisungen von collect()). Fuer die drei Mess-Achsen wird sie deshalb LAUFZEIT-seitig
/// verwacht: die Test-TU treibt jede Achse durch ihre Pfade und prueft, dass der entstandene Status im
/// deklarierten Satz liegt (tests/unit/test_a15_fk3_axis_error_traits.cpp, Block D).
template <class A>
constexpr void assert_axis_error_traits() noexcept {
    static_assert(
        requires { AxisErrorTraits<A>::domains; },
        "Achse ohne AxisErrorTraits-Spezialisierung (FK-3, Owner-Direktive 17.07.2026: "
        "Fehlerklassen sind fuer ALLE Achsen -> Unterachsen -> Algorithmen Pflicht).");
    static_assert(AxisErrorTraits<A>::domains.size() != 0,
                  "I1: LEERER Fehlerraum -- das ist die Behauptung 'diese Achse kann nicht scheitern'.");
    static_assert(detail::satz_ist_dublettenfrei(AxisErrorTraits<A>::domains) &&
                      detail::satz_ist_dublettenfrei(AxisErrorTraits<A>::d1_klassen) &&
                      detail::satz_ist_dublettenfrei(AxisErrorTraits<A>::d2_klassen),
                  "I2: DUBLETTE im Fehlerraum -- angehaengt, ohne zu lesen (RF-3-Lehre).");
    static_assert(detail::satz_enthaelt(AxisErrorTraits<A>::domains, ErrorDomain::CompilerCompiler) ==
                      (AxisErrorTraits<A>::d1_klassen.size() != 0),
                  "I3: Domaene CompilerCompiler und d1_klassen decken sich nicht (in einer der beiden "
                  "Richtungen) -- ein behaupteter leerer Realm oder Klassen am Dispatch vorbei.");
    static_assert(detail::satz_enthaelt(AxisErrorTraits<A>::domains, ErrorDomain::Sample) ==
                      (AxisErrorTraits<A>::d2_klassen.size() != 0),
                  "I3: Domaene Sample und d2_klassen decken sich nicht (in einer der beiden Richtungen).");
    static_assert(!detail::satz_enthaelt(AxisErrorTraits<A>::d2_klassen, SampleStatus::Ok),
                  "I4: SampleStatus::Ok im Fehlerraum -- Ok ist der Normalfall, kein Fehler.");
}

// ================================================================================================
//  DIE EINTRAEGE. Je Eintrag EIN Satz, WORAUS die Klassen erhoben sind -- nichts geraten.
//  Die Konfig-Achsen tragen alle ErrorDomain::CompilerCompiler; Infra kommt NUR dort dazu, wo die
//  Achse einen SUBPROZESS aufspannt (der Compiler-Aufruf), denn Infra ist ausdruecklich die
//  Prozess-/IO-Domaene und NIE ein Compiler-Urteil (axis_error.hpp:191-194).
// ================================================================================================

// -- Konfig-Realm (topics::AxisKind::system_config) -> D1 ----------------------------------------

/// compiler -- die Achse, die den Uebersetzer WAEHLT und ihn als Subprozess startet. ToolchainFehlt:
/// der gewaehlte Compiler ist auf dem Host nicht da (axis_error.hpp:41 nennt genau diesen Fall,
/// "z.B. clang++-22 fehlt"). CompileKombination: er ist da und lehnt die Kombination ab. Infra kommt
/// dazu, weil DIESE Achse den Subprozess aufspannt -- exit 127/125/128+sig klassifiziert
/// build_orchestrator bereits als InfraErrorClass, bisher aber ohne dass die Achse es deklariert haette.
template <class D>
struct AxisErrorTraits<CompilerSystemAxis<D>> {
    static constexpr auto domains = std::array{ErrorDomain::CompilerCompiler, ErrorDomain::Infra};
    static constexpr auto d1_klassen =
        std::array{CompilerCompilerErrorClass::ToolchainFehlt, CompilerCompilerErrorClass::CompileKombination};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

/// optimization_level -- -O0..-Ofast. Ein abgelehntes Optimierungs-Niveau ist ein KOMBINATIONS-Urteil
/// des Compilers, kein fehlendes Werkzeug: ToolchainFehlt gehoert hier ausdruecklich NICHT hin (die
/// Achse setzt einen vorhandenen Compiler voraus -- fehlt er, ist das die compiler-Achse, nicht diese).
template <class D>
struct AxisErrorTraits<OptimizationLevelSubAxis<D>> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::CompileKombination};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

/// simd -- die AVX2/AVX512-Wahl. HardwareErweiterungFehlt ist der belegte Hauptfall: simd_build_gate.hpp
/// bildet genau diesen Zustand ab, und isa_features.cmake:118-183 warnt woertlich mit
/// "[Compiler-Compiler-Fehler: hardware_erweiterung_fehlt|compile_kombination]" -- beide Klassen sind
/// an dieser Achse also bereits im Bestand, nur nie von der Achse selbst deklariert.
template <class D>
struct AxisErrorTraits<SimdSubAxis<D>> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::HardwareErweiterungFehlt,
                                                  CompilerCompilerErrorClass::CompileKombination};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

/// compiler_atomic -- cx16 / kein cx16. Die Ablehnung eines 128-Bit-CAS ist ein Kombinations-Urteil
/// (Flag gegen Ziel-ISA), kein fehlendes Geraet: die Erweiterungs-Klasse gehoert der ISA-/Extension-
/// Achse, nicht dieser.
template <class D>
struct AxisErrorTraits<CompilerAtomicSubAxis<D>> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::CompileKombination};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

/// target_isa -- die ZIEL-ISA des Codegen. Sie kann am Host-Koennen scheitern
/// (HardwareErweiterungFehlt) oder am Dialekt-Schnitt (CompileKombination, axis_error.hpp:43
/// "ISA-/Dialekt-Inkompat").
template <class D>
struct AxisErrorTraits<TargetIsaSystemAxis<D>> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::HardwareErweiterungFehlt,
                                                  CompilerCompilerErrorClass::CompileKombination};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

/// target_isa KOMPLEX (ISA x OS). Sie fuehrt die ISA-Klassen UND zusaetzlich BetriebssystemFeatureFehlt:
/// eine Komplex-Zelle spannt beide Achsen im TYP (hardware_probe_factory.hpp, Owner-Nachtrag 2/K2), also
/// gehoert ihr auch das OS-Urteil. DAS IST DIE ISA-x-OS-TOTALITAET DES A15-DESIGNS: neue Plattformen
/// (RISC-V, macOS M1, macOS x86 -- Owner-E4) treten als ZELLEN dazu und erben diesen Fehlerraum
/// automatisch, ohne dass hier eine Zeile nachgezogen werden muss.
template <class D, class IsaAxis>
struct AxisErrorTraits<TargetIsaComplexAxis<D, IsaAxis>> {
    static constexpr auto domains = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen =
        std::array{CompilerCompilerErrorClass::HardwareErweiterungFehlt, CompilerCompilerErrorClass::CompileKombination,
                   CompilerCompilerErrorClass::BetriebssystemFeatureFehlt};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

/// target_isa ACHSEN-ANKER. TargetIsaAxisTag ist keine Auspraegung, sondern der Typ, ueber den sich die
/// ISA-Unter-Achsen auf die ACHSE beziehen (target_isa_complex_axis.hpp:55-58). Er ist eine volle
/// CebSystemAxis und taucht als `parent_axis` in jeder ISA-Unter-Achse auf -- wer diesen Bezug
/// aufloest, um den Fehlerraum der Eltern-Achse zu erfragen, braeuchte ohne Eintrag hier einen
/// unvollstaendigen Typ. Er fuehrt deshalb den Fehlerraum der Achse, die er benennt.
template <>
struct AxisErrorTraits<TargetIsaAxisTag> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::HardwareErweiterungFehlt,
                                                  CompilerCompilerErrorClass::CompileKombination};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

/// target_isa-UNTER-Achsen (numa_node, page, core_class). Zwei verschiedene Nein-Sager: hugetlbfs nicht
/// gemountet / Kernel-Feature nicht gebaut ist BetriebssystemFeatureFehlt (axis_error.hpp:44-50 nennt
/// hugetlbfs woertlich als Beispiel); ein nicht vorhandener NUMA-Knoten oder eine nicht vorhandene
/// Kern-Klasse ist HardwareErweiterungFehlt. Beides zusammenzuwerfen hiesse, eine nicht gemountete
/// Datei als fehlende Hardware zu melden.
template <class D>
struct AxisErrorTraits<TargetIsaSubAxis<D>> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt,
                                                  CompilerCompilerErrorClass::HardwareErweiterungFehlt};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

/// operating_system -- Linux/Windows/macOS. Der belegte Producer-Platz von BetriebssystemFeatureFehlt:
/// die Klasse existiert seit RF-3, hatte aber laut A15-Befund L6 KEINEN Producer ("nur Kommentare
/// machine_identity.hpp:390/:526 + Tests"). Hier wird sie erstmals der Achse zugeschrieben, zu der sie
/// gehoert.
template <class D>
struct AxisErrorTraits<OperatingSystemAxis<D>> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

/// operating_system ACHSEN-ANKER (Muster TargetIsaAxisTag, s.o.).
template <>
struct AxisErrorTraits<OperatingSystemAxisTag> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

/// operating_system-UNTER-Achsen (os_version, kernel, build). Sie verfeinern die OS-Aussage und teilen
/// deshalb deren Klasse: eine nicht verfuegbare Kernel-Version ist ein fehlendes OS-Merkmal.
template <class D>
struct AxisErrorTraits<OperatingSystemSubAxis<D>> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

/// scheduling -- die Ablauf-Politik. Sie steht und faellt mit OS-Schnittstellen (Affinitaet, Politik,
/// Prioritaet); ist die Faehigkeit nicht da oder nicht konfiguriert, ist das der OS-Fall.
template <class D>
struct AxisErrorTraits<SchedulingSystemAxis<D>> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::BetriebssystemFeatureFehlt};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

/// extension_hardware -- AVX2/AVX512, spaeter GPU/FPGA. Die kanonische Heimat von
/// HardwareErweiterungFehlt (axis_error.hpp:42 nennt "AVX512, GPU, FPGA" woertlich). Sie faellt
/// AUSSCHLIESSLICH so aus: eine Erweiterung ist da oder nicht.
template <class D>
struct AxisErrorTraits<ExtensionHardwareSystemAxis<D>> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::HardwareErweiterungFehlt};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

/// hardware_isa -- das ISA-KOENNEN DES HOSTS (nicht das Ziel des Codegen; das ist target_isa). Ihr
/// einziger Ausfall ist "der Host kann es nicht".
template <class D>
struct AxisErrorTraits<HardwareIsaSystemAxis<D>> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::HardwareErweiterungFehlt};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

// -- Mess-Realm (topics::AxisKind::system_measurement) -> D2 -------------------------------------
// DIE DREI SAETZE SIND AM CODE ABGELESEN, nicht aus dem Realm abgeleitet: massgeblich ist, welche
// mark_*()-Aufrufe im do_collect()-Rumpf der jeweiligen Achse tatsaechlich stehen. Genau deshalb
// unterscheiden sie sich -- und genau deshalb sitzen die Eintraege an den BLAETTERN und nicht an
// SystemAxis<D>. Ein gemeinsamer Wurzel-Eintrag haette allen dreien denselben Raum zugeschrieben und
// die Unterscheidung eingeebnet, um die es hier geht.

/// wall_clock (system_axis.hpp:247-303). ALLE DREI: mark_failed bei negativer Dauer (:266/:280 --
/// unplausibler Rohwert der Zeitquelle, ausdruecklich KEIN n/a), mark_source_unavailable bei op_count==0
/// bzw. fehlender Zeitspanne (:272/:284), mark_not_applicable fuer fremde Kategorien (:299/:303).
template <>
struct AxisErrorTraits<WallClockSystemAxis> {
    static constexpr auto domains    = std::array{ErrorDomain::Sample};
    static constexpr auto d1_klassen = std::array<CompilerCompilerErrorClass, 0>{};
    static constexpr auto d2_klassen =
        std::array{SampleStatus::NotApplicable, SampleStatus::SourceUnavailable, SampleStatus::Failed};
};

/// observer_snapshot (system_axis.hpp:308-377). KEIN Failed -- der Rumpf ruft mark_failed NICHT auf:
/// mark_source_unavailable bei fehlendem Snapshot bzw. fehlendem Nenner (:320/:355/:367),
/// mark_not_applicable fuer fremde Kategorien (:374/:377). Failed hier zu deklarieren waere bequem
/// (drei gleiche Zeilen), aber es waere eine Klasse, die diese Achse nicht hervorbringt -- und ein
/// Fehlerraum, der mehr behauptet als er haelt, ist so wenig wert wie einer, der schweigt.
template <>
struct AxisErrorTraits<ObserverSnapshotSystemAxis> {
    static constexpr auto domains    = std::array{ErrorDomain::Sample};
    static constexpr auto d1_klassen = std::array<CompilerCompilerErrorClass, 0>{};
    static constexpr auto d2_klassen = std::array{SampleStatus::NotApplicable, SampleStatus::SourceUnavailable};
};

/// pmc (system_axis.hpp:389-445). EBENFALLS KEIN Failed. SourceUnavailable ist hier der tragende Fall
/// und der Grund, warum FK-3 ueberhaupt gebraucht wird: der ZUGANG zur Quelle fehlt -- PMC nicht
/// angeheftet oder unprivilegiert (:404), und seit M-3a auch FEINKOERNIG je Kategorie, wenn der
/// EINZELNE Zaehler nichts geliefert hat (:421/:442). Genau das war der Defekt, den M-3a geheilt hat:
/// `available` sagte nur "mindestens ein Zaehler hat geliefert", und mark_ok(0) testierte eine 0, die
/// nie erhoben worden war. Die Achse fuehrt zusaetzlich ErrorDomain::HardwareProbe, weil ihr
/// Verfuegbarkeits-Urteil aus einer HOST-QUELLEN-Erhebung stammt (perf_event_open) und nicht aus der
/// Messung selbst -- das ist die Domaenen-Trennung aus axis_error.hpp:213-226 ("kein Urteil ueber die
/// Hardware, sondern ueber unseren ZUGANG zu ihr").
template <>
struct AxisErrorTraits<PmcSystemAxis> {
    static constexpr auto domains    = std::array{ErrorDomain::Sample, ErrorDomain::HardwareProbe};
    static constexpr auto d1_klassen = std::array<CompilerCompilerErrorClass, 0>{};
    static constexpr auto d2_klassen = std::array{SampleStatus::NotApplicable, SampleStatus::SourceUnavailable};
};

// -- Mess-Realm-Meta-Meta (topics::AxisKind::measurement_meta_meta) -> D1 ------------------------

/// load_framework (YCSB). Sie liegt im PLANER (topics/axis.hpp: "Mess-Achsen leben im PLANER") und hat
/// KEIN collect() -- ihr Ausfall ist deshalb kein Zell-Status, sondern ein KONFIGURATIONS-Urteil vor der
/// Messung: das angeforderte Framework bzw. die angeforderte workload-Unter-Achse steht nicht in der
/// Registry. Das ist am Bestand belegt und nicht hergeleitet -- validate_profile.hpp:805 etikettiert
/// genau diesen Fall mit error_class_label(KonfigXmlParse) und prueft dabei gegen
/// kMeasurementFrameworkRegistry (validate_profile.hpp:63). SIE LIEGT DESHALB IN DIESEM Boost-FREIEN
/// Header und nicht bei den Meta-Metas nebenan: das Trennkriterium dort ist ausschliesslich Boost, nicht
/// die Achsen-Art -- und diese Familie zieht kein Boost.
template <class D>
struct AxisErrorTraits<LoadFrameworkMeasurementAxis<D>> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::KonfigXmlParse};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

// -- System-Meta-Meta-Realm (topics::AxisKind::system_meta_meta) -> D1 ---------------------------

/// SYSTEM-Meta-Meta-Haupt-Achse (hardware_meta_meta_axis.hpp:93). Sie ist eine VOLLE Konfig-System-
/// Haupt-Achse, an die sich konkrete Achsen DIREKT haengen (belegt: Avx512MetaMeta/GpuMetaMeta/
/// GpuClusterMetaMeta, tests/unit/test_meta_meta_halbordnung.cpp:63/:70/:78) -- sie ist also eine
/// Deklarations-Ebene und keine blosse Wurzel, und ein Eintrag hier ist kein Auffangbecken.
/// Der Fehlerraum ist am Bestand belegt: meta_meta_admission.hpp bildet die Meta-Meta-ZULASSUNG genau
/// auf HardwareErweiterungFehlt ab (:29-30). Der Hub traegt SIMD/AVX, externe Hardware, spaeter
/// GPU/FPGA/NPU (topics/axis.hpp, Owner-KERN 26.07.) -- eine Meta-Meta faellt aus, weil das Geraet
/// nicht da ist.
template <class D>
struct AxisErrorTraits<SystemMetaMetaAxis<D>> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::HardwareErweiterungFehlt};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

/// external_utils_family (external_utils_family_axis.hpp:71) -- die Familie UNTER dem Hub, heute mit
/// SimdExternalUtilsFamily als einzigem Glied (:96). Sie fuehrt NEBEN der Geraete-Klasse zusaetzlich
/// CompileKombination, und das ist der Unterschied zur Wurzel: eine externe Utils-FAMILIE wird
/// mituebersetzt (sie haengt die SimdSubAxis-Unter-Achse an, :53), kann also auch am Dialekt-Schnitt
/// scheitern und nicht nur am fehlenden Geraet.
/// SIE ERBT DIESEN RAUM NICHT VON DER WURZEL: partielle Spezialisierungen loesen nicht ueber die
/// Vererbungskette auf (s. Kopf). Dass die Familie hier einen EIGENEN, GROESSEREN Satz fuehrt, ist
/// genau deshalb eine Aussage und kein Duplikat.
template <class D>
struct AxisErrorTraits<ExternalUtilsFamilyAxis<D>> {
    static constexpr auto domains    = std::array{ErrorDomain::CompilerCompiler};
    static constexpr auto d1_klassen = std::array{CompilerCompilerErrorClass::HardwareErweiterungFehlt,
                                                  CompilerCompilerErrorClass::CompileKombination};
    static constexpr auto d2_klassen = std::array<SampleStatus, 0>{};
};

} // namespace comdare::cache_engine::measurement
