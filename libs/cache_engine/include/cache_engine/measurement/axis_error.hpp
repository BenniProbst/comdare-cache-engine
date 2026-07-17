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

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

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

} // namespace comdare::cache_engine::measurement
