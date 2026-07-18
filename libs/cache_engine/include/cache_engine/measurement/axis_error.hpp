#pragma once
// measurement/axis_error.hpp -- Fehlerklassifizierungs-Framework (Task #29, INC-29.0).
//
// User-Direktive 2026-07-17: "Fehlerklassen und Behandlung sind fuer ALLE Achsen -> Unterachsen ->
// Algorithmen Pflicht." Diese Datei traegt AUSSCHLIESSLICH die Taxonomie (2 disjunkte Enums + ihre
// stabilen Etiketten) -- KEIN Aufrufer, KEINE Verhaltensaenderung (rein additiv, golden==320 unberuehrt).
// Die Aufrufer-Verdrahtung folgt in den Increments INC-29.1 (D2 CSV-"failed") + INC-29.2 (D1 CMake/Planer).
//
// Zwei bindende Direktiven, zwei disjunkte compile-time-Taxonomien:
//   D1  HW-/Compile-Fehlen -> "Compiler-Compiler-Fehler"-Klasse, IM LOG deklariert, Experiment MISST WEITER.
//       (Planer-/Compile-Zeit; getragen spaeter via std::expected an CompileFn/BuildResult.)
//   D2  Algo-/Mess-Fehler -> CSV-Zelle "failed" (NICHT null) + Log, Harness misst weiter.
//       (Runtime/Harness; ersetzt spaeter den konflatierenden bool valid je (binary_id x setting)-Zelle.)
//       Memory feedback_measurement_failure_visibility_csv_failed_not_null_plus_log ("Messung NIE als Nullen").
//
// Benannter Pattern-Stapel (compile-time-only, CRTP/Concept-konform, keine vtable): Error-Category
// (constexpr Etikett-Abbildung) + Expected/Result (Alexandrescu, an der Carrier-Naht) + Policy-Based-
// Design (compile-time Strategy der Behandlung) + Chain-of-Responsibility (GoF, deckungsgleich zur
// rekursiven Dock-Kette). INC-29.0 legt nur die Error-Category-Basis; die Carrier folgen additiv.
//
// OD-2 (User-Weiche, Empfehlung befolgt): das Klassifikations-Enum lebt in ce (Werkzeug/Framework);
// die konkrete Log-/Weiter-mess-Politik je Permutation darf spaeter in super/Experiment-Definition sitzen.
// OD-5 (User-Weiche, Empfehlung befolgt): 4+4-Granularitaet; GPU/FPGA wird additiv INNERHALB
// HardwareErweiterungFehlt nachgeruestet (kein Enum-Bruch), sobald die Nicht-CPU-Detektions-Naht steht.

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <variant>

namespace comdare::cache_engine::measurement {

// ── D1: Compiler-Compiler-Fehlerklassen (Planer-/Compile-Zeit) ────────────────────────────────────
// Ein erkennbarer, klassifizierbarer Zustand (nicht Absturz): das Fehlen einer HW-Erweiterung
// (AVX512/GPU/FPGA), ein fehlendes Tool, eine ungueltige XML-Spec oder eine vom Compiler abgelehnte
// Achsen-Kombination. Wird IM LOG deklariert; das Experiment misst die uebrigen Permutationen weiter.
enum class CompilerCompilerErrorClass : std::uint8_t {
    KonfigXmlParse           = 0, // Experiment-/Registry-XML ungueltig oder unparsbar
    ToolchainFehlt           = 1, // Compiler/Tool nicht verfuegbar (z.B. clang++-22 fehlt)
    HardwareErweiterungFehlt = 2, // ISA-/Beschleuniger-Erweiterung auf dem Host nicht verfuegbar (AVX512, GPU, FPGA)
    CompileKombination       = 3, // gueltige Achsen-Kombination vom Compiler abgelehnt (ISA-/Dialekt-Inkompat)
};
/// Single-Source der Klassenzahl (bei JEDER neuen Klasse mit hochzaehlen; der Drift-Guard unten faengt Vergessen).
inline constexpr std::size_t kCompilerCompilerErrorClassCount = 4;

// ── D2: Sample-Status je (binary_id x setting)-Zelle (Runtime/Harness) ────────────────────────────
// Ersetzt (spaeter) den konflatierenden bool valid. Trennt die drei ehrlichen Zell-Bedeutungen sauber:
// eine Zahl (Ok) / ein ehrliches "n/a" (kein Fehler) / ein "failed" (echter Algo-/Mess-Fehler) -- NIE Null.
enum class SampleStatus : std::uint8_t {
    Ok                = 0, // gueltige Messung -> die Zahl steht in der Zelle
    NotApplicable     = 1, // Achse fuer diese Binary sinnlos -> ehrlich "n/a", KEIN Fehler
    SourceUnavailable = 2, // Mess-Quelle (PMC/DLL/Interface) nicht da -> "n/a"
    Failed            = 3, // Algo-/Mess-Fehler (OOM, Gate-Fail, Exception) -> Zelle "failed" + Log, NIE Null
};
/// Single-Source der Status-Zahl (Drift-Guard unten).
inline constexpr std::size_t kSampleStatusCount = 4;

// ── Error-Category: stabile Etiketten (Single-Source fuer Log + Serialisierung) ───────────────────
/// Log-Etikett je D1-Klasse (stabil; darf in Experiment-Logs zitiert werden).
[[nodiscard]] constexpr std::string_view error_class_label(CompilerCompilerErrorClass c) noexcept {
    switch (c) {
        case CompilerCompilerErrorClass::KonfigXmlParse: return "konfig_xml_parse";
        case CompilerCompilerErrorClass::ToolchainFehlt: return "toolchain_fehlt";
        case CompilerCompilerErrorClass::HardwareErweiterungFehlt: return "hardware_erweiterung_fehlt";
        case CompilerCompilerErrorClass::CompileKombination: return "compile_kombination";
    }
    return "unbekannt"; // out-of-range-Cast -> sicherer, sichtbarer Default (kein UB, kein stiller Skip)
}

/// CSV-Zell-Token je D2-Status (OD-1): Ok -> die Zahl (Aufrufer rendert), N-A/SourceUnavailable -> "n/a",
/// Failed -> "failed". Unbekannt -> "failed" (sicherer Default: NIE eine stille Null, "Messung nie als Nullen").
[[nodiscard]] constexpr std::string_view sample_status_token(SampleStatus s) noexcept {
    switch (s) {
        case SampleStatus::Ok: return "ok"; // Platzhalter-Etikett; bei Ok rendert der Aufrufer die Zahl
        case SampleStatus::NotApplicable: return "n/a";
        case SampleStatus::SourceUnavailable: return "n/a";
        case SampleStatus::Failed: return "failed";
    }
    return "failed";
}

// ── INC-29.2: Infra-Fehlerklassen (Prozess-/IO-Ebene) — DISJUNKT von D1 (Compiler-Compiler-Fehler). ──
// Direktiven-Trennung: ein Infra-Fehler (Compiler-Subprozess nicht startbar, .rsp nicht schreibbar,
// waitpid-Abbruch) ist KEIN Compiler-Compiler-Fehler und darf NICHT als HW-/Compile-Klasse fehletikettiert
// werden (Sweep-Befund). Eigene Domaene, eigenes Log-Praefix. Exit-Codes: 127=Start, 125=IO, 128+sig=Abbruch.
enum class InfraErrorClass : std::uint8_t {
    ProzessStart   = 0, // Compiler-/Tool-Subprozess nicht startbar (spawn/exec; exit 127)
    ProzessAbbruch = 1, // Subprozess signal-abgebrochen (128+WTERMSIG: 137=SIGKILL/OOM, 139=SIGSEGV) o. Sentinel <0
    ArtefaktIo     = 2, // .rsp/Quell-/Ziel-Datei-IO fehlgeschlagen (exit 125; kein Compiler-Urteil)
};
/// Single-Source der Infra-Klassenzahl (Drift-Guard unten).
inline constexpr std::size_t kInfraErrorClassCount = 3;

/// Log-Etikett je Infra-Klasse (stabil; getrennt von error_class_label).
[[nodiscard]] constexpr std::string_view infra_error_label(InfraErrorClass c) noexcept {
    switch (c) {
        case InfraErrorClass::ProzessStart: return "prozess_start";
        case InfraErrorClass::ProzessAbbruch: return "prozess_abbruch";
        case InfraErrorClass::ArtefaktIo: return "artefakt_io";
    }
    return "unbekannt";
}

// ── Chain-of-Responsibility: die Fehler-DOMAENE diskriminiert, welche Log-/Behandlungs-Kette greift. ──
// error_domain() ist je Fehler-Typ ueberladen (Tag-Dispatch, compile-time); die rekursive Dock-Kette
// (Planer->CEB->Tier) reicht einen Fehler an die zustaendige Domaene weiter OHNE Runtime-Typ-Pruefung.
enum class ErrorDomain : std::uint8_t {
    Infra            = 0, // Prozess/IO — kein Compiler-Urteil, kein Mess-Ergebnis
    CompilerCompiler = 1, // D1: HW-/Compile-Fehlen — Log, Experiment misst weiter
    Sample           = 2, // D2: Mess-Zell-Status (Ok/n-a/failed)
};
[[nodiscard]] constexpr ErrorDomain error_domain(InfraErrorClass) noexcept { return ErrorDomain::Infra; }
[[nodiscard]] constexpr ErrorDomain error_domain(CompilerCompilerErrorClass) noexcept {
    return ErrorDomain::CompilerCompiler;
}
[[nodiscard]] constexpr ErrorDomain error_domain(SampleStatus) noexcept { return ErrorDomain::Sample; }

/// Bau-Fehler-Traeger: EIN Wert, der ENTWEDER ein Infra- ODER ein Compiler-Compiler-Fehler ist (nie beides,
/// nie fehletikettiert). std::variant = typisierte Summe (Expected/Result-Naht an BuildResult.outcome).
using BuildError = std::variant<InfraErrorClass, CompilerCompilerErrorClass>;
[[nodiscard]] constexpr ErrorDomain error_domain(BuildError const& e) noexcept {
    return std::visit([](auto c) noexcept { return error_domain(c); }, e);
}
/// Log-Etikett je BuildError-Variante (Serialisierungs-Single-Source): das richtige Label je Domaene.
[[nodiscard]] constexpr std::string_view build_error_label(BuildError const& e) noexcept {
    return std::visit(
        [](auto c) noexcept -> std::string_view {
            if constexpr (std::is_same_v<decltype(c), InfraErrorClass>)
                return infra_error_label(c);
            else
                return error_class_label(c);
        },
        e);
}

// ── Policy-Based Design: compile-time Behandlungs-Politik je Domaene (benannt, keine vtable). ──────
// Jede Politik deklariert compile-time, ob sie die Pipeline ABBRICHT (nie: honest-weiter), welche Domaene
// sie fuehrt und welches Log-Praefix. Der Aufrufer waehlt die Politik per static dispatch (kein Runtime-Switch).
struct LogAndContinueInfraPolicy { // Infra: loggen, Permutation ueberspringen, Harness laeuft weiter
    [[nodiscard]] static constexpr bool             aborts() noexcept { return false; }
    [[nodiscard]] static constexpr ErrorDomain      domain() noexcept { return ErrorDomain::Infra; }
    [[nodiscard]] static constexpr std::string_view log_prefix() noexcept { return "Infra-Fehler"; }
};
struct LogAndContinueD1Policy { // D1: Compiler-Compiler-Fehler loggen, Experiment misst weiter
    [[nodiscard]] static constexpr bool             aborts() noexcept { return false; }
    [[nodiscard]] static constexpr ErrorDomain      domain() noexcept { return ErrorDomain::CompilerCompiler; }
    [[nodiscard]] static constexpr std::string_view log_prefix() noexcept { return "Compiler-Compiler-Fehler"; }
};
struct FailedCellD2Policy { // D2: Zelle "failed" + Log, Harness misst weiter (NIE Null)
    [[nodiscard]] static constexpr bool             aborts() noexcept { return false; }
    [[nodiscard]] static constexpr ErrorDomain      domain() noexcept { return ErrorDomain::Sample; }
    [[nodiscard]] static constexpr std::string_view log_prefix() noexcept { return "Mess-Fehler"; }
};
template <class P>
concept HandlingPolicyConcept = requires {
    { P::aborts() } -> std::same_as<bool>;
    { P::domain() } -> std::same_as<ErrorDomain>;
    { P::log_prefix() } -> std::same_as<std::string_view>;
};

// ── Trennungs-Garantien + POD-Eigenschaften + Drift-Guards (alles compile-time) ───────────────────
static_assert(std::is_same_v<std::underlying_type_t<CompilerCompilerErrorClass>, std::uint8_t>);
static_assert(std::is_same_v<std::underlying_type_t<SampleStatus>, std::uint8_t>);
static_assert(std::is_trivially_copyable_v<SampleStatus> && std::is_trivially_copyable_v<CompilerCompilerErrorClass>);
static_assert(static_cast<std::uint8_t>(SampleStatus::Ok) == 0, "Ok MUSS 0 sein (Default-Init = gueltig).");
static_assert(SampleStatus::Failed != SampleStatus::NotApplicable, "Failed und NotApplicable MUESSEN disjunkt sein.");
// Drift-Guards: neue Enum-Werte erzwingen ein Hochzaehlen der Count-Single-Source.
static_assert(kCompilerCompilerErrorClassCount ==
              static_cast<std::size_t>(CompilerCompilerErrorClass::CompileKombination) + 1);
static_assert(kSampleStatusCount == static_cast<std::size_t>(SampleStatus::Failed) + 1);
// Token-Kontrakt (D2/OD-1): die entscheidenden Zell-Vokabeln sind zementiert.
static_assert(sample_status_token(SampleStatus::Failed) == std::string_view{"failed"});
static_assert(sample_status_token(SampleStatus::NotApplicable) == std::string_view{"n/a"});
static_assert(sample_status_token(SampleStatus::Ok) != sample_status_token(SampleStatus::Failed));

// INC-29.2: Infra-Domaene disjunkt von D1 + Drift-Guards + Policy-Concept-Erfuellung (alles compile-time).
static_assert(std::is_same_v<std::underlying_type_t<InfraErrorClass>, std::uint8_t>);
static_assert(std::is_same_v<std::underlying_type_t<ErrorDomain>, std::uint8_t>);
static_assert(kInfraErrorClassCount == static_cast<std::size_t>(InfraErrorClass::ArtefaktIo) + 1);
static_assert(infra_error_label(InfraErrorClass::ProzessStart) == std::string_view{"prozess_start"});
// Die geruegte Fehletikettierung ist hier ZEMENTIERT ausgeschlossen: ein Infra-Fehler ist NIE ein D1-Fehler.
static_assert(error_domain(InfraErrorClass::ProzessStart) == ErrorDomain::Infra);
static_assert(error_domain(CompilerCompilerErrorClass::ToolchainFehlt) == ErrorDomain::CompilerCompiler);
static_assert(error_domain(InfraErrorClass::ProzessStart) != error_domain(CompilerCompilerErrorClass::ToolchainFehlt));
static_assert(error_domain(BuildError{InfraErrorClass::ProzessAbbruch}) == ErrorDomain::Infra);
static_assert(error_domain(BuildError{CompilerCompilerErrorClass::CompileKombination}) ==
              ErrorDomain::CompilerCompiler);
static_assert(build_error_label(BuildError{CompilerCompilerErrorClass::HardwareErweiterungFehlt}) ==
              std::string_view{"hardware_erweiterung_fehlt"});
static_assert(build_error_label(BuildError{InfraErrorClass::ProzessStart}) == std::string_view{"prozess_start"});
static_assert(HandlingPolicyConcept<LogAndContinueInfraPolicy> && HandlingPolicyConcept<LogAndContinueD1Policy> &&
              HandlingPolicyConcept<FailedCellD2Policy>);
static_assert(!LogAndContinueInfraPolicy::aborts() && !LogAndContinueD1Policy::aborts() &&
              !FailedCellD2Policy::aborts()); // honest-weiter, nie Abbruch (Pipeline reisst nicht)
static_assert(LogAndContinueInfraPolicy::domain() == ErrorDomain::Infra &&
              FailedCellD2Policy::domain() == ErrorDomain::Sample);

} // namespace comdare::cache_engine::measurement
