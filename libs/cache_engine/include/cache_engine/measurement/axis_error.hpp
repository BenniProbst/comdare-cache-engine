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
    // RF-3 (§70.3, 2026-07-26): das OS-ANALOGON zu HardwareErweiterungFehlt. Nicht die Hardware fehlt,
    // sondern eine Betriebssystem-Faehigkeit ist auf diesem Host nicht verfuegbar ODER nicht konfiguriert
    // (hugetlbfs nicht gemountet, Kernel-Feature nicht gebaut, OS-Schnittstelle fehlt). Belegtes Material
    // aus der OS-Flotten-Erhebung: musl-libc-Abweichung unter Alpine, BSD-/msys-grep-Portabilitaetsluecken
    // unter macOS/Windows, PowerShell-Executor-Semantik der Windows-Runner. Ein solcher Zustand ist eine
    // DEKLARIERTE Klasse, kein Absturz -- das Experiment misst die uebrigen Permutationen weiter.
    BetriebssystemFeatureFehlt = 4, // OS-Feature auf dem Host nicht verfuegbar/konfiguriert
};
/// Single-Source der Klassenzahl (bei JEDER neuen Klasse mit hochzaehlen; der Drift-Guard unten faengt Vergessen).
inline constexpr std::size_t kCompilerCompilerErrorClassCount = 5;

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

// -- RF-2 (§70.2): Zulassungs-Status je Permutation (D1-Zell-Vokabular) ---------------------------
// Owner-Begruendung verbatim (§70.2): "die Hardware-Erweiterung spezialisiert die CPU-Spezifikation,
// jede untergeordnete Gesamtrekombination ist eine eigene Permutation+Messevaluation". Eine Perm, die
// auf dieser Maschine NICHT ZUGELASSEN ist (z.B. avx512 ohne Freigabe), ist deshalb kein verschwiegener
// Leerraum, sondern ein eigener, sichtbarer Datensatz.
//
// WARUM EINE EIGENE TAXONOMIE statt eines weiteren SampleStatus-Werts (W-4-Doktrin, Bauplan TEIL II):
// eine ZULASSUNGS-Entscheidung ist D1 (Planer-/Compile-Zeit), ein Mess-Ergebnis ist D2 (Runtime).
// `sample_status_token(Failed)` hiesse "der Algorithmus/die Messung ist gescheitert" -- gesperrt heisst
// aber, dass GAR NICHT GEMESSEN WURDE. Die Domaenen-Trennung dieser Datei (s. Kopf, D1 vs D2) waere
// verletzt, wenn eine Zulassungs-Aussage durch das D2-Vokabular reiste. Die Disjunktheit der Tokens ist
// unten compile-time verwacht, damit die Doktrin nicht nur im Kommentar steht.
enum class AdmissionStatus : std::uint8_t {
    Zugelassen = 0, // Perm ist auf dieser Maschine freigegeben -> es wird real gemessen (Default)
    Gesperrt   = 1, // Perm ist D1-zulassungs-gesperrt -> Marker-Datensatz statt Messung, NIE eine Null
};
/// Single-Source der Zulassungs-Status-Zahl (beide Drift-Wachen unten).
inline constexpr std::size_t kAdmissionStatusCount = 2;

// -- A15/FK-1: BAU-Status je Permutations-Zeile (D1-Zell-Vokabular, DRITTE Nicht-Zahl-Semantik) -----
// Owner-Freigabe 02.08.2026 (Volles GO auf die Bauplan-Empfehlung Q4): eine Permutation, deren Binary
// GAR NICHT GEBAUT werden konnte, bekommt einen eigenen, sichtbaren Datensatz statt einer verschwiegenen
// Luecke -- exakt die Doktrin, die RF-2 fuer die Zulassung eingefuehrt hat. Bis dahin verschwand ein
// Bau-Fehler NUR ins Log (kein Mess-Eintrag), und die Auswerte-CSV konnte "nie gebaut" nicht von
// "nie geplant" unterscheiden.
//
// WARUM EINE DRITTE EIGENE TAXONOMIE (W-4-Doktrin, wie schon bei AdmissionStatus): es sind DREI
// Aussagen, die eine Auswertung auseinanderhalten koennen MUSS und die eine gemeinsame Vokabel
// unwiederbringlich verschmelzen wuerde --
//   failed       (D2, SampleStatus)     = GEMESSEN und dabei GESCHEITERT,
//   gesperrt     (D1, AdmissionStatus)  = gebaut/baubar, aber NICHT ZUGELASSEN, deshalb nicht gemessen,
//   nicht_gebaut (D1, BuildCellStatus)  = es gibt gar keine Binary, der Bau selbst ist gescheitert.
// Die Disjunktheit dieser drei Tokens ist unten compile-time verwacht (nicht bloss im Kommentar).
enum class BuildCellStatus : std::uint8_t {
    Gebaut      = 0, // Binary liegt vor -> die Zeile rendert wie bisher (Default, byte-identisch)
    NichtGebaut = 1, // Bau gescheitert (D1: Infra ODER Compiler-Compiler) -> Marker-Zeile statt Zeilen-Nichts
};
/// Single-Source der Bau-Status-Zahl (beide Drift-Wachen unten).
inline constexpr std::size_t kBuildCellStatusCount = 2;

// ── Error-Category: stabile Etiketten (Single-Source fuer Log + Serialisierung) ───────────────────
/// Log-Etikett je D1-Klasse (stabil; darf in Experiment-Logs zitiert werden).
[[nodiscard]] constexpr std::string_view error_class_label(CompilerCompilerErrorClass c) noexcept {
    switch (c) {
        case CompilerCompilerErrorClass::KonfigXmlParse: return "konfig_xml_parse";
        case CompilerCompilerErrorClass::ToolchainFehlt: return "toolchain_fehlt";
        case CompilerCompilerErrorClass::HardwareErweiterungFehlt: return "hardware_erweiterung_fehlt";
        case CompilerCompilerErrorClass::CompileKombination: return "compile_kombination";
        case CompilerCompilerErrorClass::BetriebssystemFeatureFehlt: return "betriebssystem_feature_fehlt";
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

/// A15/FK-1: LOG-Etikett je D2-Status -- das Gegenstueck zu error_class_label/infra_error_label/
/// hardware_probe_label fuer die Mess-Domaene. Es ist bewusst NICHT der Zell-Token: der Zell-Token
/// bildet NotApplicable und SourceUnavailable BEIDE auf "n/a" ab (so soll die CSV lesen), im Log muss
/// aber unterscheidbar bleiben, ob eine Achse fuer diese Binary sinnlos war oder ob die Mess-Quelle
/// fehlte -- sonst klassifiziert das Log nichts, sondern wiederholt nur die Zelle. Unbekannt ->
/// "sample_status_unbekannt" (eigener Fallback; "unbekannt" ist bereits doppelt vergeben).
[[nodiscard]] constexpr std::string_view sample_status_label(SampleStatus s) noexcept {
    switch (s) {
        case SampleStatus::Ok: return "mess_ok";
        case SampleStatus::NotApplicable: return "nicht_anwendbar";
        case SampleStatus::SourceUnavailable: return "quelle_nicht_verfuegbar";
        case SampleStatus::Failed: return "mess_fehler";
    }
    return "sample_status_unbekannt";
}

/// CSV-Zell-Token je Zulassungs-Status (RF-2). "gesperrt" = die Perm wurde NICHT gemessen, weil sie auf
/// dieser Maschine nicht zugelassen ist -- eine D1-Aussage. Unbekannt -> "gesperrt" (sicherer Default in
/// dieselbe Richtung wie das D2-Pendant: lieber sichtbar-nicht-gemessen als eine stille Zahl).
[[nodiscard]] constexpr std::string_view admission_status_token(AdmissionStatus a) noexcept {
    switch (a) {
        case AdmissionStatus::Zugelassen: return "zugelassen"; // Platzhalter-Etikett; hier rendert der Aufrufer Zahlen
        case AdmissionStatus::Gesperrt: return "gesperrt";
    }
    return "gesperrt";
}

/// CSV-Zell-Token je Bau-Status (A15/FK-1). "nicht_gebaut" = fuer diese Permutation existiert keine
/// Binary; es wurde weder zugelassen noch gemessen. Unbekannt -> "nicht_gebaut" (sicherer Default in
/// dieselbe Richtung wie die beiden Nachbarn: lieber sichtbar-nicht-vorhanden als eine stille Zahl).
[[nodiscard]] constexpr std::string_view build_cell_status_token(BuildCellStatus s) noexcept {
    switch (s) {
        case BuildCellStatus::Gebaut: return "gebaut"; // Platzhalter-Etikett; hier rendert der Aufrufer Zahlen
        case BuildCellStatus::NichtGebaut: return "nicht_gebaut";
    }
    return "nicht_gebaut";
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

// -- Task #7 / P1: Hardware-ERHEBUNGS-Fehlerklassen: EIGENE Domaene, disjunkt von D1 und Infra. -----
// Warum eine eigene Domaene und nicht HardwareErweiterungFehlt (D1): D1 sagt "die Hardware KANN etwas
// nicht" (AVX512 fehlt) und ist ein Urteil ueber die MASCHINE. Hier geht es um die ERHEBUNG selbst --
// die Quelle war nicht da, nicht lesbar, kaputt oder in einem unbekannten Format. Das ist kein Urteil
// ueber die Hardware, sondern ueber unseren ZUGANG zu ihr; beides zu vermischen wuerde eine fehlende
// Datei als fehlende CPU-Faehigkeit ausweisen. Die D1-Count-Wache (kCompilerCompilerErrorClassCount)
// bleibt damit unberuehrt (RF-3-Kollisionslehre: eine neue Klasse in einem fremden Enum zieht dessen
// Count-Wachen und bricht sie still).
//
// QuelleFehlt vs. QuelleKorrupt ist die tragende Unterscheidung (Auflage A4): eine FEHLENDE Quelle ist
// der ERWARTETE Normalfall (kein SPD-Knoten, keine Boot-Cache-Datei) -- die Erhebungs-Kette faellt
// ehrlich auf die naechste Stufe durch. Eine VORHANDENE, aber kaputte Quelle ist ein BEFUND: dort
// stimmt etwas nicht, und ein stilles Durchfallen wuerde genau das verbergen. Beide muessen im
// Degradations-Pfad unterscheidbar bleiben.
enum class HardwareProbeErrorClass : std::uint8_t {
    QuelleFehlt     = 0, // Knoten/Datei existiert nicht (erwarteter Zustand -> naechste Stufe, kein Alarm)
    QuelleUnlesbar  = 1, // existiert, aber kein Zugriff (gemessen: /sys/firmware/dmi/tables/DMI ist 0400 root)
    QuelleKorrupt   = 2, // vorhanden+lesbar, aber inhaltlich unbrauchbar (zu kurz, Nullwerte) -> BEFUND, nie still
    FormatUnbekannt = 3, // Kennung passt zu keinem unterstuetzten Format -> NIE interpretieren, nie raten
};
/// Single-Source der Erhebungs-Klassenzahl (Drift-Guards unten, beide Richtungen).
inline constexpr std::size_t kHardwareProbeErrorClassCount = 4;

/// Log-Etikett je Erhebungs-Klasse. Der Fallback heisst BEWUSST nicht "unbekannt": dieses Wort tragen
/// bereits error_class_label und infra_error_label als Fallback, und die Disjunktheits-Wachen unten
/// sollen eine echte Aussage machen statt an einem geteilten Sammelbegriff zu scheitern.
[[nodiscard]] constexpr std::string_view hardware_probe_label(HardwareProbeErrorClass c) noexcept {
    switch (c) {
        case HardwareProbeErrorClass::QuelleFehlt: return "quelle_fehlt";
        case HardwareProbeErrorClass::QuelleUnlesbar: return "quelle_unlesbar";
        case HardwareProbeErrorClass::QuelleKorrupt: return "quelle_korrupt";
        case HardwareProbeErrorClass::FormatUnbekannt: return "format_unbekannt";
    }
    return "hardware_probe_unbekannt";
}

// -- E-24 C5 / FK-7: DOCK-FEHLERPFAD-KLASSEN -- die Klassifizierung des MESS-EINTRITTS (D2-Seite). -----
// Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 6.2 (FK-7): "die rohen
// dock_status_*-Ints (pruef_dock.hpp:36-48) bleiben ABI-nahe TRANSPORTFORM (kein V5-Freeze-Bruch); die
// KLASSIFIZIERUNG geschieht CEB-seitig ... CSV-Zelle 'failed' + klassifiziertes Log mit stabilem Etikett,
// NIE stille 0/null."
//
// WARUM EINE EIGENE TAXONOMIE NEBEN SampleStatus (W-4-Doktrin, wie bei AdmissionStatus/BuildCellStatus):
// SampleStatus hat fuer den GESAMTEN Mess-Fehlerraum genau EINEN Wert (Failed) und genau EIN Log-Etikett
// ("mess_fehler"). Der Dock-/Loader-Eintritt kennt aber sechs unterscheidbare Gruende, und sie verlangen
// verschiedene Konsequenzen: eine FREMDE GATTUNG am Dock ist ein Registry-/Auswahl-Fehler, ein FEHLENDES
// ANTRIEBS-SUB-INTERFACE ist eine zu alte DLL, ein GESCHEITERTES ORAKEL ist ein Befund am Pruefling selbst.
// Alle drei als "mess_fehler" zu loggen hiesse, das Log klassifiziert nichts, sondern wiederholt die Zelle
// (exakt die Begruendung, mit der A15/FK-1 sample_status_label neben den Zell-Token gestellt hat).
//
// DIE DOMAENE BLEIBT D2 (Sample): der Dock-Fehlerpfad endet in einer MESS-Zelle, nicht in einem Bau-Urteil.
// error_domain(DockErrorClass) == ErrorDomain::Sample ist daher kein Zufall, sondern die Aussage "die Zelle
// liest 'failed'". Die Bau-Seite (D1) ist FK-8 und liegt in einem eigenen, disjunkten Vokabular.
//
// KEIN INCLUDE VON pruef_dock.hpp (Layer-Schnitt): diese Datei ist die measurement-Taxonomie und darf die
// builder-Seite nicht kennen. Die Abbildung Transport-Int -> Klasse steht am Dock-Aufrufer
// (builder/pruef_dock/dock_error_classification.hpp) und pinnt die Int-Werte dort compile-hart.
enum class DockErrorClass : std::uint8_t {
    ModulOhneAnatomie       = 0, // geladenes Modul ohne IAnatomyBase (dock_status_no_anatomy, Modul-Seite)
    KeinDockFuerGattung     = 1, // Registry-Seite: kein Dock fuer die im Modul deklarierte Gattung
    FremdeGattung           = 2, // Modul-Gattung != Dock-Gattung (dock_status_wrong_genus)
    AntriebsInterfaceFehlt  = 3, // gattungs-eigenes Sub-Interface fehlt -- zu alte DLL (subinterface_missing)
    KonformitaetGescheitert = 4, // Gattungs-Orakel VOR der Messung gerissen (conformance_failed); NICHT gemessen
    ModulLadeFehler         = 5, // der Loader kam gar nicht bis zum Dock (anatomy_module_loader status != ok)
    UnbekannterDockStatus   = 6, // Transport-Int jenseits des bekannten Vokabulars -> sichtbar, nie still
};
/// Single-Source der Dock-Klassenzahl (beide Drift-Wachen unten).
inline constexpr std::size_t kDockErrorClassCount = 7;

/// Log-Etikett je Dock-Fehlerklasse (stabil; darf in Experiment-Logs zitiert werden). Der Fallback heisst
/// bewusst NICHT "unbekannt" -- das Wort tragen error_class_label und infra_error_label bereits, und die
/// Disjunktheits-Wachen unten sollen eine echte Aussage machen statt an einem Sammelbegriff zu scheitern.
[[nodiscard]] constexpr std::string_view dock_error_label(DockErrorClass c) noexcept {
    switch (c) {
        case DockErrorClass::ModulOhneAnatomie: return "modul_ohne_anatomie";
        case DockErrorClass::KeinDockFuerGattung: return "kein_dock_fuer_gattung";
        case DockErrorClass::FremdeGattung: return "fremde_gattung";
        case DockErrorClass::AntriebsInterfaceFehlt: return "antriebs_interface_fehlt";
        case DockErrorClass::KonformitaetGescheitert: return "konformitaet_gescheitert";
        case DockErrorClass::ModulLadeFehler: return "modul_lade_fehler";
        case DockErrorClass::UnbekannterDockStatus: return "unbekannter_dock_status";
    }
    return "dock_fehler_unbekannt";
}

/// Die D2-ZELL-Wirkung einer Dock-Fehlerklasse. ALLE Klassen liefern Failed -- und das ist eine Entscheidung,
/// keine Verlegenheit: ein Dock-Fehlerpfad heisst, dass eine Binary GEBAUT und ZUM MESSEN VORGELEGT wurde und
/// die Messung dann nicht zustande kam. Das ist ein Defekt, kein planmaessiges "n/a" (NotApplicable heisst
/// "die Achse ist fuer diese Binary sinnlos", SourceUnavailable "die Quelle existiert hier planmaessig nicht").
/// Ein n/a wuerde den Defekt in der Auswertung unsichtbar machen -- genau die stille Luecke, die FK-7
/// schliesst. Die Unterscheidung WARUM traegt das Log (dock_error_label), nicht die Zelle.
/// Der Parameter bleibt bewusst stehen: sollte je eine Klasse anders abbilden, ist DIES die eine Stelle.
[[nodiscard]] constexpr SampleStatus dock_error_sample_status(DockErrorClass) noexcept { return SampleStatus::Failed; }

/// Ist ein Etikett gegen ALLE bestehenden Zell-/Fehler-Vokabeln disjunkt? Die Schleifen laufen ueber die
/// Count-Single-Sources statt ueber handgepflegte Listen: waechst eine fremde Taxonomie, waechst diese
/// Pruefung automatisch mit. Eine Liste haette genau beim naechsten Zuwachs geschwiegen -- dieselbe
/// Blindheit, die die zweite Drift-Richtung unten verhindert.
[[nodiscard]] constexpr bool probe_label_ist_disjunkt(std::string_view t) noexcept {
    for (std::size_t i = 0; i < kCompilerCompilerErrorClassCount; ++i)
        if (t == error_class_label(static_cast<CompilerCompilerErrorClass>(i))) return false;
    for (std::size_t i = 0; i < kSampleStatusCount; ++i)
        if (t == sample_status_token(static_cast<SampleStatus>(i))) return false;
    // A15/FK-1: die D2-LOG-Etiketten sind ein eigenes Vokabular neben den Zell-Tokens und muessen
    // ebenso gegen alles andere disjunkt bleiben (sonst liest ein Log-Grep eine fremde Domaene mit).
    for (std::size_t i = 0; i < kSampleStatusCount; ++i)
        if (t == sample_status_label(static_cast<SampleStatus>(i))) return false;
    for (std::size_t i = 0; i < kAdmissionStatusCount; ++i)
        if (t == admission_status_token(static_cast<AdmissionStatus>(i))) return false;
    // A15/FK-1: die Bau-Status-Tokens laufen aus demselben Grund mit wie die Zulassungs-Tokens -- sie
    // sind Zell-Vokabeln derselben CSV und muessen gegen JEDE andere Vokabel disjunkt bleiben.
    for (std::size_t i = 0; i < kBuildCellStatusCount; ++i)
        if (t == build_cell_status_token(static_cast<BuildCellStatus>(i))) return false;
    for (std::size_t i = 0; i < kInfraErrorClassCount; ++i)
        if (t == infra_error_label(static_cast<InfraErrorClass>(i))) return false;
    // E-24 C5 / FK-7: die Dock-Etiketten laufen aus demselben Grund mit wie die Bau-/Zulassungs-Tokens --
    // sie reisen in DIESELBEN Experiment-Logs, und ein Log-Grep darf keine fremde Domaene mittreffen.
    for (std::size_t i = 0; i < kDockErrorClassCount; ++i)
        if (t == dock_error_label(static_cast<DockErrorClass>(i))) return false;
    return true;
}

// ── Chain-of-Responsibility: die Fehler-DOMAENE diskriminiert, welche Log-/Behandlungs-Kette greift. ──
// error_domain() ist je Fehler-Typ ueberladen (Tag-Dispatch, compile-time); die rekursive Dock-Kette
// (Planer->CEB->Tier) reicht einen Fehler an die zustaendige Domaene weiter OHNE Runtime-Typ-Pruefung.
enum class ErrorDomain : std::uint8_t {
    Infra            = 0, // Prozess/IO — kein Compiler-Urteil, kein Mess-Ergebnis
    CompilerCompiler = 1, // D1: HW-/Compile-Fehlen — Log, Experiment misst weiter
    Sample           = 2, // D2: Mess-Zell-Status (Ok/n-a/failed)
    HardwareProbe    = 3, // Task #7: ZUGANG zur Hardware-Quelle, kein Urteil ueber die Hardware selbst
};
[[nodiscard]] constexpr ErrorDomain error_domain(InfraErrorClass) noexcept { return ErrorDomain::Infra; }
[[nodiscard]] constexpr ErrorDomain error_domain(HardwareProbeErrorClass) noexcept {
    return ErrorDomain::HardwareProbe;
}
[[nodiscard]] constexpr ErrorDomain error_domain(CompilerCompilerErrorClass) noexcept {
    return ErrorDomain::CompilerCompiler;
}
[[nodiscard]] constexpr ErrorDomain error_domain(SampleStatus) noexcept { return ErrorDomain::Sample; }
/// E-24 C5 / FK-7: der Dock-Fehlerpfad endet in einer MESS-Zelle -- er ist deshalb D2 (Sample) und
/// ausdruecklich KEINE eigene ErrorDomain. Die Domaenen-Liste bleibt damit unveraendert (kein Enum-Zuwachs
/// an ErrorDomain), waehrend die Dock-KLASSE die Log-Aufloesung liefert, die SampleStatus allein nicht hat.
[[nodiscard]] constexpr ErrorDomain error_domain(DockErrorClass) noexcept { return ErrorDomain::Sample; }

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
// FK-2/K1 -- GELTUNGSBEREICH dieser Zusicherung, praezisiert: Ok==0 ist eine WIRE-/POD-Aussage ueber
// PermResult.sample_status (harness/perm_runner.hpp:129-133). Dort ist der Default Ok richtig, weil die
// Zelle erst entsteht, NACHDEM die Messung gelaufen ist -- und weil ein von 0 verschiedener Default den
// Wire-/golden-Pfad byte-verschieben wuerde. Sie ist AUSDRUECKLICH KEINE Aussage ueber host-seitige
// Traeger: SystemAxisSample (measurement/system_axis.hpp) haelt bewusst einen FAIL-SAFE-Default
// (SourceUnavailable), damit eine nie-collectete Sample NIE als gueltige Messung liest. Wer diesen
// Kommentar liest, um einen weiteren Ok-Default zu rechtfertigen, liest ihn falsch.
static_assert(static_cast<std::uint8_t>(SampleStatus::Ok) == 0,
              "Ok MUSS 0 sein (PermResult-Wire-POD: Default-Init = gueltig; gilt NICHT host-seitig).");
static_assert(SampleStatus::Failed != SampleStatus::NotApplicable, "Failed und NotApplicable MUESSEN disjunkt sein.");
// Drift-Guards: neue Enum-Werte erzwingen ein Hochzaehlen der Count-Single-Source.
static_assert(kCompilerCompilerErrorClassCount ==
              static_cast<std::size_t>(CompilerCompilerErrorClass::BetriebssystemFeatureFehlt) + 1);
// Die Namens-Pin-Form oben faengt ANHAENGEN+Count-vergessen nicht (die Gleichung bleibt wahr, RF-3
// literal bewiesen). Diese zweite Richtung tut es: liegt hinter dem Count eine ETIKETTIERTE Klasse,
// ist der Count zu klein.
static_assert(error_class_label(static_cast<CompilerCompilerErrorClass>(kCompilerCompilerErrorClassCount)) ==
                  std::string_view{"unbekannt"},
              "Drift: hinter dem Count liegt eine etikettierte Klasse");
static_assert(kSampleStatusCount == static_cast<std::size_t>(SampleStatus::Failed) + 1);
static_assert(sample_status_token(static_cast<SampleStatus>(kSampleStatusCount)) == std::string_view{"failed"},
              "Drift: hinter dem Count liegt ein etikettierter SampleStatus");

// -- RF-2: die Zulassungs-Taxonomie, beide Drift-Richtungen (RF-3-Lehre) + die W-4-DISJUNKTHEIT ----
static_assert(std::is_same_v<std::underlying_type_t<AdmissionStatus>, std::uint8_t>);
static_assert(std::is_trivially_copyable_v<AdmissionStatus>);
static_assert(static_cast<std::uint8_t>(AdmissionStatus::Zugelassen) == 0,
              "Zugelassen MUSS 0 sein (Default-Init = gemessen wie bisher, byte-identisch).");
// (1) Namens-Pin und (2) Etikett-hinter-Count -- beide, weil (1) allein ein ANHAENGEN nicht faengt (RF-3).
static_assert(kAdmissionStatusCount == static_cast<std::size_t>(AdmissionStatus::Gesperrt) + 1);
static_assert(admission_status_token(static_cast<AdmissionStatus>(kAdmissionStatusCount)) ==
                  std::string_view{"gesperrt"},
              "Drift: hinter dem Count liegt ein etikettierter AdmissionStatus");
// W-4 ALS WACHE, nicht als Kommentar: kein Zulassungs-Token darf je mit einem Mess-Token zusammenfallen.
// Faellt hier etwas um, hat jemand die Domaenen-Trennung D1/D2 aufgeweicht -- und ein Auswerte-Reader
// koennte "nicht gemessen" nicht mehr von "Messung gescheitert" unterscheiden.
static_assert(admission_status_token(AdmissionStatus::Gesperrt) != sample_status_token(SampleStatus::Failed));
static_assert(admission_status_token(AdmissionStatus::Gesperrt) != sample_status_token(SampleStatus::Ok));
static_assert(admission_status_token(AdmissionStatus::Gesperrt) != sample_status_token(SampleStatus::NotApplicable));
static_assert(admission_status_token(AdmissionStatus::Gesperrt) !=
              sample_status_token(SampleStatus::SourceUnavailable));
static_assert(admission_status_token(AdmissionStatus::Zugelassen) != sample_status_token(SampleStatus::Failed));
static_assert(admission_status_token(AdmissionStatus::Zugelassen) != sample_status_token(SampleStatus::Ok));
static_assert(admission_status_token(AdmissionStatus::Zugelassen) != sample_status_token(SampleStatus::NotApplicable));
static_assert(admission_status_token(AdmissionStatus::Zugelassen) !=
              sample_status_token(SampleStatus::SourceUnavailable));
// Das Token ist zementiert: der Auswerte-Reader fuehrt es in seiner Verwerf-Liste (measurement_curve_loader).
static_assert(admission_status_token(AdmissionStatus::Gesperrt) == std::string_view{"gesperrt"});
// Token-Kontrakt (D2/OD-1): die entscheidenden Zell-Vokabeln sind zementiert.
static_assert(sample_status_token(SampleStatus::Failed) == std::string_view{"failed"});
static_assert(sample_status_token(SampleStatus::NotApplicable) == std::string_view{"n/a"});
// E-6/K-10-QW (2026-07-19): der seg_*-n/a-Zell-Renderer (cache_engine_builder_iterator.hpp) rendert ueber
// SourceUnavailable -- das Token ist hier zementiert, damit der Renderer-Umbau CSV-byte-neutral BLEIBT.
static_assert(sample_status_token(SampleStatus::SourceUnavailable) == std::string_view{"n/a"});
static_assert(sample_status_token(SampleStatus::Ok) != sample_status_token(SampleStatus::Failed));

// -- A15/FK-1: die D2-LOG-Etiketten -- Drift-Wache + die Unterscheidung, die der Zell-Token NICHT hat -
static_assert(sample_status_label(static_cast<SampleStatus>(kSampleStatusCount)) ==
                  std::string_view{"sample_status_unbekannt"},
              "Drift: hinter dem Count liegt ein etikettierter SampleStatus (Log-Seite)");
// Der Kern dieses Vokabulars: n/a-Zelle ja, n/a-LOG nein. Waeren diese beiden Etiketten gleich, koennte
// die Lade-/Dock-Naht nicht mehr melden, WARUM eine Zelle leer blieb.
static_assert(sample_status_label(SampleStatus::NotApplicable) != sample_status_label(SampleStatus::SourceUnavailable));
static_assert(sample_status_token(SampleStatus::NotApplicable) == sample_status_token(SampleStatus::SourceUnavailable),
              "Die Zell-Sicht bleibt bewusst zusammengefasst -- nur das Log differenziert.");
static_assert(sample_status_label(SampleStatus::Ok) != sample_status_label(SampleStatus::Failed));
// Log-Etikett und Zell-Token duerfen nie verwechselbar sein (ein Log-Grep darf keine Zelle treffen).
static_assert(sample_status_label(SampleStatus::Failed) != sample_status_token(SampleStatus::Failed));
static_assert(sample_status_label(SampleStatus::SourceUnavailable) !=
              sample_status_token(SampleStatus::SourceUnavailable));
static_assert(sample_status_label(SampleStatus::SourceUnavailable) !=
              hardware_probe_label(HardwareProbeErrorClass::QuelleFehlt));
static_assert(sample_status_label(SampleStatus::SourceUnavailable) !=
              hardware_probe_label(HardwareProbeErrorClass::QuelleUnlesbar));
static_assert(sample_status_label(SampleStatus::SourceUnavailable) ==
              std::string_view{"quelle_nicht_verfuegbar"}); // zementiert: die Lade-/Dock-Naht loggt es woertlich

// -- A15/FK-1: die Bau-Status-Taxonomie, BEIDE Drift-Richtungen + die DREIFACH-Disjunktheit ---------
static_assert(std::is_same_v<std::underlying_type_t<BuildCellStatus>, std::uint8_t>);
static_assert(std::is_trivially_copyable_v<BuildCellStatus>);
static_assert(static_cast<std::uint8_t>(BuildCellStatus::Gebaut) == 0,
              "Gebaut MUSS 0 sein (Default-Init = gebaut wie bisher, byte-identisch).");
// (1) Namens-Pin und (2) Etikett-hinter-Count -- beide, weil (1) allein ein ANHAENGEN nicht faengt (RF-3).
static_assert(kBuildCellStatusCount == static_cast<std::size_t>(BuildCellStatus::NichtGebaut) + 1);
static_assert(build_cell_status_token(static_cast<BuildCellStatus>(kBuildCellStatusCount)) ==
                  std::string_view{"nicht_gebaut"},
              "Drift: hinter dem Count liegt ein etikettierter BuildCellStatus");
// DIE DREI NICHT-ZAHL-SEMANTIKEN, paarweise verwacht. Faellt hier etwas um, kann ein Auswerte-Reader
// "nie gebaut" nicht mehr von "nicht zugelassen" und beides nicht mehr von "gemessen und gescheitert"
// unterscheiden -- genau die Verschmelzung, gegen die W-4 gebaut wurde.
static_assert(build_cell_status_token(BuildCellStatus::NichtGebaut) != sample_status_token(SampleStatus::Failed));
static_assert(build_cell_status_token(BuildCellStatus::NichtGebaut) !=
              admission_status_token(AdmissionStatus::Gesperrt));
static_assert(build_cell_status_token(BuildCellStatus::NichtGebaut) != sample_status_token(SampleStatus::Ok));
static_assert(build_cell_status_token(BuildCellStatus::NichtGebaut) !=
              sample_status_token(SampleStatus::NotApplicable));
static_assert(build_cell_status_token(BuildCellStatus::NichtGebaut) !=
              sample_status_token(SampleStatus::SourceUnavailable));
static_assert(build_cell_status_token(BuildCellStatus::Gebaut) != sample_status_token(SampleStatus::Failed));
static_assert(build_cell_status_token(BuildCellStatus::Gebaut) != admission_status_token(AdmissionStatus::Gesperrt));
static_assert(build_cell_status_token(BuildCellStatus::Gebaut) != admission_status_token(AdmissionStatus::Zugelassen));
static_assert(build_cell_status_token(BuildCellStatus::Gebaut) != sample_status_token(SampleStatus::NotApplicable));
// Das Token ist zementiert: der Auswerte-Reader fuehrt es woertlich in seiner Verwerf-Liste
// (measurement_curve_loader), und die xlsx-/Lager-Strecke uebernimmt es ueber dieselbe Vokabel.
static_assert(build_cell_status_token(BuildCellStatus::NichtGebaut) == std::string_view{"nicht_gebaut"});

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

// -- Task #7 / P1: die Erhebungs-Domaene -- POD-Form, BEIDE Drift-Richtungen, volle Disjunktheit ----
static_assert(std::is_same_v<std::underlying_type_t<HardwareProbeErrorClass>, std::uint8_t>);
static_assert(std::is_trivially_copyable_v<HardwareProbeErrorClass>);
static_assert(static_cast<std::uint8_t>(HardwareProbeErrorClass::QuelleFehlt) == 0,
              "QuelleFehlt MUSS 0 sein: der erwartete Normalfall ist der Default-Zustand.");
// (1) Namens-Pin und (2) Etikett-hinter-Count -- beide, weil (1) allein ein ANHAENGEN nicht faengt (RF-3).
static_assert(kHardwareProbeErrorClassCount == static_cast<std::size_t>(HardwareProbeErrorClass::FormatUnbekannt) + 1);
static_assert(hardware_probe_label(static_cast<HardwareProbeErrorClass>(kHardwareProbeErrorClassCount)) ==
                  std::string_view{"hardware_probe_unbekannt"},
              "Drift: hinter dem Count liegt eine etikettierte Erhebungs-Klasse");
// Die Domaenen-Trennung ist zementiert: eine nicht lesbare Quelle ist NIE ein Urteil ueber die Hardware
// (D1) und NIE ein Prozess-/IO-Fehler des Bau-Kanals (Infra). Wer das aufweicht, meldet eine fehlende
// Datei als fehlende CPU-Faehigkeit.
static_assert(error_domain(HardwareProbeErrorClass::QuelleFehlt) == ErrorDomain::HardwareProbe);
static_assert(error_domain(HardwareProbeErrorClass::QuelleFehlt) !=
              error_domain(CompilerCompilerErrorClass::HardwareErweiterungFehlt));
static_assert(error_domain(HardwareProbeErrorClass::QuelleFehlt) != error_domain(InfraErrorClass::ArtefaktIo));
static_assert(error_domain(HardwareProbeErrorClass::QuelleFehlt) != error_domain(SampleStatus::Failed));
// A4-KERN: fehlend und korrupt duerfen NIE zusammenfallen -- sonst verschwindet der Befund "Quelle
// vorhanden, aber kaputt" im erwarteten Normalfall "Quelle nicht da" und degradiert still.
static_assert(HardwareProbeErrorClass::QuelleFehlt != HardwareProbeErrorClass::QuelleKorrupt);
static_assert(hardware_probe_label(HardwareProbeErrorClass::QuelleFehlt) !=
              hardware_probe_label(HardwareProbeErrorClass::QuelleKorrupt));
// Token-Disjunktheit gegen ALLE bestehenden Vokabeln, jede Klasse einzeln + der Fallback.
static_assert(probe_label_ist_disjunkt(hardware_probe_label(HardwareProbeErrorClass::QuelleFehlt)));
static_assert(probe_label_ist_disjunkt(hardware_probe_label(HardwareProbeErrorClass::QuelleUnlesbar)));
static_assert(probe_label_ist_disjunkt(hardware_probe_label(HardwareProbeErrorClass::QuelleKorrupt)));
static_assert(probe_label_ist_disjunkt(hardware_probe_label(HardwareProbeErrorClass::FormatUnbekannt)));
static_assert(
    probe_label_ist_disjunkt(hardware_probe_label(static_cast<HardwareProbeErrorClass>(kHardwareProbeErrorClassCount))),
    "Auch der Fallback muss disjunkt sein -- 'unbekannt' ist bereits doppelt vergeben.");
// Gegenprobe der Wache selbst: sie muss ein bekannt KOLLIDIERENDES Etikett auch wirklich ablehnen,
// sonst waere die Disjunktheits-Aussage oben eine leere Zusicherung (Negativfall = der eigentliche Test).
static_assert(!probe_label_ist_disjunkt(sample_status_token(SampleStatus::Failed)));
static_assert(!probe_label_ist_disjunkt(infra_error_label(InfraErrorClass::ArtefaktIo)));
// A15/FK-1: dieselbe Gegenprobe fuer die neu eingehaengte Schleife -- ohne sie waere der Zuwachs oben
// eine leere Zusicherung (die Wache muss das neue Vokabular auch wirklich ablehnen).
static_assert(!probe_label_ist_disjunkt(build_cell_status_token(BuildCellStatus::NichtGebaut)));
static_assert(!probe_label_ist_disjunkt(build_cell_status_token(BuildCellStatus::Gebaut)));
static_assert(!probe_label_ist_disjunkt(sample_status_label(SampleStatus::SourceUnavailable)));
static_assert(!probe_label_ist_disjunkt(sample_status_label(SampleStatus::Failed)));

// -- E-24 C5 / FK-7: die Dock-Taxonomie -- POD-Form, BEIDE Drift-Richtungen, volle Disjunktheit ------
static_assert(std::is_same_v<std::underlying_type_t<DockErrorClass>, std::uint8_t>);
static_assert(std::is_trivially_copyable_v<DockErrorClass>);
// (1) Namens-Pin und (2) Etikett-hinter-Count -- beide, weil (1) allein ein ANHAENGEN nicht faengt (RF-3).
static_assert(kDockErrorClassCount == static_cast<std::size_t>(DockErrorClass::UnbekannterDockStatus) + 1);
static_assert(dock_error_label(static_cast<DockErrorClass>(kDockErrorClassCount)) ==
                  std::string_view{"dock_fehler_unbekannt"},
              "Drift: hinter dem Count liegt eine etikettierte Dock-Fehlerklasse");
// Die Domaenen-Zuordnung ist zementiert: ein Dock-Fehler ist eine MESS-Aussage (D2) und NIE ein Bau-Urteil
// (D1), nie ein Prozess-/IO-Fehler (Infra), nie ein Hardware-Erhebungs-Befund. Wer das aufweicht, meldet
// eine nicht messbare Binary als Bau- oder Maschinen-Problem.
static_assert(error_domain(DockErrorClass::FremdeGattung) == ErrorDomain::Sample);
static_assert(error_domain(DockErrorClass::FremdeGattung) != error_domain(InfraErrorClass::ArtefaktIo));
static_assert(error_domain(DockErrorClass::FremdeGattung) !=
              error_domain(CompilerCompilerErrorClass::CompileKombination));
static_assert(error_domain(DockErrorClass::FremdeGattung) != error_domain(HardwareProbeErrorClass::QuelleFehlt));
// DER KERN VON FK-7: jede Dock-Fehlerklasse rendert die Zelle als "failed" -- NIE als Null, NIE als "n/a".
static_assert(sample_status_token(dock_error_sample_status(DockErrorClass::ModulOhneAnatomie)) ==
              std::string_view{"failed"});
static_assert(sample_status_token(dock_error_sample_status(DockErrorClass::KeinDockFuerGattung)) ==
              std::string_view{"failed"});
static_assert(sample_status_token(dock_error_sample_status(DockErrorClass::FremdeGattung)) ==
              std::string_view{"failed"});
static_assert(sample_status_token(dock_error_sample_status(DockErrorClass::AntriebsInterfaceFehlt)) ==
              std::string_view{"failed"});
static_assert(sample_status_token(dock_error_sample_status(DockErrorClass::KonformitaetGescheitert)) ==
              std::string_view{"failed"});
static_assert(sample_status_token(dock_error_sample_status(DockErrorClass::ModulLadeFehler)) ==
              std::string_view{"failed"});
static_assert(sample_status_token(dock_error_sample_status(DockErrorClass::UnbekannterDockStatus)) ==
              std::string_view{"failed"});
static_assert(sample_status_token(dock_error_sample_status(DockErrorClass::ModulOhneAnatomie)) !=
                  sample_status_token(SampleStatus::NotApplicable),
              "Ein Dock-Fehler darf NIE als 'n/a' lesen -- das wuerde einen Defekt als Planmaessigkeit tarnen.");
// Das Log-Etikett ist NICHT der Zell-Token (ein Log-Grep darf keine Zelle treffen) und nicht das
// D2-Sammel-Etikett: genau diese Aufloesung ist der Zweck der eigenen Taxonomie.
static_assert(dock_error_label(DockErrorClass::FremdeGattung) != sample_status_token(SampleStatus::Failed));
static_assert(dock_error_label(DockErrorClass::FremdeGattung) != sample_status_label(SampleStatus::Failed));
static_assert(dock_error_label(DockErrorClass::AntriebsInterfaceFehlt) !=
              dock_error_label(DockErrorClass::KonformitaetGescheitert));
static_assert(dock_error_label(DockErrorClass::ModulOhneAnatomie) !=
                  dock_error_label(DockErrorClass::KeinDockFuerGattung),
              "Modul-Seite und Registry-Seite MUESSEN unterscheidbar bleiben -- der Transport-Int "
              "konflatiert beide (pruef_dock_sequencer.hpp), und genau das repariert FK-7.");
// Token-Disjunktheit gegen ALLE bestehenden Vokabeln, jede Klasse einzeln + der Fallback.
static_assert(probe_label_ist_disjunkt(dock_error_label(DockErrorClass::ModulOhneAnatomie)) == false);
static_assert(!probe_label_ist_disjunkt(dock_error_label(DockErrorClass::KeinDockFuerGattung)));
static_assert(!probe_label_ist_disjunkt(dock_error_label(DockErrorClass::UnbekannterDockStatus)));
// Der Fallback muss ebenfalls disjunkt SEIN (er liegt jenseits des Counts, die Schleife sieht ihn nicht --
// deshalb hier positiv geprueft statt ueber die Gegenprobe).
static_assert(probe_label_ist_disjunkt(dock_error_label(static_cast<DockErrorClass>(kDockErrorClassCount))),
              "Auch der Dock-Fallback muss disjunkt sein -- 'unbekannt' ist bereits doppelt vergeben.");

} // namespace comdare::cache_engine::measurement
