// abi/system_axis_order.hpp -- kSystemAxisOrder: die EINE Ordnungs-Quelle der System-Haupt-Achsen
// (Lane A, Paket A1, 26.07.2026).
//
// PROBLEM (Riss 2 der Kartierung): die kanonische System-Achsen-ORDNUNG existiert heute mehrfach --
// als Reihenfolge in kSystemAxisCodeVersions (system_axis_code_versions.hpp), als Block-Folge im
// Generator (tools/system_axis_registry_gen/main.cpp), als Kopf-Reihenfolge der generierten XML und
// als Segment-Folge im build_version-Suffix. Vier Kopien, keine Wache: die einzige bestehende Pruefung
// ist "5 Eintraege, axis/version nie leer" (system_axis_code_versions.hpp:49-50) -- die faengt eine
// UMSORTIERUNG nicht. Genau diese Luecke hat die drei konkurrierenden Suffix-Ordnungen ermoeglicht,
// die Lane F (W-13) einsammelt.
//
// A1 legte diesen Header mit dem damaligen IST-Stand an (5 Achsen: compiler, external_utils,
// target_isa, scheduling, load_framework) und aenderte bewusst nichts an der Ordnung.
//
// A3 (O-8 Schritt 4) SETZT die finale Ordnung des Owner-KERNs: GENAU DREI Haupt-Achsen --
// target_isa, operating_system, external_utils. Die drei Abgaenge sind UMZUEGE, keine Loeschungen:
//   * load_framework VERLAESST die System-Welt ERSATZLOS Richtung MESS-Realm (Planer-Meta-Meta-
//     Haupt-Achse; NACHZUG R-G, Ledger 69.1). Die aeltere Notiz "META-META unter external_utils"
//     ist SUPERSEDED -- der Hub traegt NUR System-Meta-Metas; "NICHT eine Haupt-Achse" bleibt richtig.
//   * compiler wird Unter-Achsen-GRUPPE der aeusseren System-Komplex-Achse (O-1r, Schritt 6).
//   * scheduling wird Unter-Achse (sub_axis) des target_isa-Komplex-Wrappers (Schritt 6).
// Die Abgaenge sind unten durch eigene static_asserts gesichert, damit keiner still zurueckkehrt.
// Die A3-Wache ueber Generator-Blockfolge und XML-Kopf haengt an denselben Netzen im Registry-
// Generator (kSystemAxisEmitters / kEmissionOrderFromTypes), die kSystemAxisOrder lesen.

#pragma once

#include "system_axis_code_versions.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::abi {

/// Anzahl der System-HAUPT-Achsen in der kanonischen Ordnung. A3 (O-8 Schritt 4): DREI.
inline constexpr std::size_t kSystemAxisOrderCount = kSystemAxisCodeCount;

/// DIE kanonische Reihenfolge der System-Haupt-Achsen. Jeder Konsument, der eine Ordnung braucht
/// (Stempel-Zeile, Generator-Blockfolge, XML-Kopf, Suffix-Segmente), MUSS sie hier lesen und nirgends
/// nachbilden. A3 setzt die finale Ordnung des Owner-KERNs (siehe Kopf-Kommentar).
inline constexpr std::array<std::string_view, kSystemAxisOrderCount> kSystemAxisOrder{{
    "target_isa",
    "operating_system",
    "external_utils",
}};

/// Index einer Achse in der kanonischen Ordnung; kSystemAxisOrderCount, falls unbekannt.
/// consteval-taugliche Suche -- damit koennen Konsumenten ihre eigene Reihenfolge compile-time
/// gegen diese Quelle pruefen, statt sie zu duplizieren.
[[nodiscard]] constexpr std::size_t system_axis_order_index(std::string_view axis) noexcept {
    for (std::size_t i = 0; i < kSystemAxisOrderCount; ++i)
        if (kSystemAxisOrder[i] == axis) return i;
    return kSystemAxisOrderCount;
}

[[nodiscard]] constexpr bool is_known_system_axis(std::string_view axis) noexcept {
    return system_axis_order_index(axis) < kSystemAxisOrderCount;
}

// ---------------------------------------------------------------------------------------------
// DRIFT-WACHE (der eigentliche Zweck des Headers, A1-Stufe)
// ---------------------------------------------------------------------------------------------
// Die bestehende Pruefung in system_axis_code_versions.hpp faengt nur LEERE Eintraege. Hier wird
// zusaetzlich die REIHENFOLGE gegen die Code-Versions-Tabelle zementiert: wer eine der beiden
// umsortiert, umbenennt oder verlaengert, ohne die andere nachzuziehen, bricht ab HIER compile-time.
// A3 weitet dieselbe Wache auf Generator-Blockfolge, XML-Kopf und Suffix-Emitter aus.
namespace detail {
[[nodiscard]] consteval bool system_axis_order_matches_code_versions() {
    if (kSystemAxisOrderCount != kSystemAxisCodeCount) return false;
    for (std::size_t i = 0; i < kSystemAxisOrderCount; ++i)
        if (kSystemAxisOrder[i] != kSystemAxisCodeVersions[i].axis) return false;
    return true;
}
[[nodiscard]] consteval bool system_axis_order_names_unique() {
    for (std::size_t i = 0; i < kSystemAxisOrderCount; ++i) {
        if (kSystemAxisOrder[i].empty()) return false;
        for (std::size_t j = i + 1; j < kSystemAxisOrderCount; ++j)
            if (kSystemAxisOrder[i] == kSystemAxisOrder[j]) return false;
    }
    return true;
}
} // namespace detail

static_assert(detail::system_axis_order_matches_code_versions(),
              "kSystemAxisOrder und kSystemAxisCodeVersions sind AUS DER REIHE gelaufen. Beide tragen "
              "dieselbe kanonische System-Achsen-Ordnung; eine Umsortierung/Umbenennung/Erweiterung muss "
              "in BEIDEN erfolgen (Lane A A1: Ordnungs-Single-Source, Riss 2).");
static_assert(detail::system_axis_order_names_unique(),
              "kSystemAxisOrder: Namen muessen nicht leer und paarweise verschieden sein.");
// Selbst-Test der Suche (haelt die Konsumenten-API ehrlich, kostet nichts zur Laufzeit).
static_assert(system_axis_order_index("target_isa") == 0);
static_assert(system_axis_order_index("operating_system") == 1);
static_assert(system_axis_order_index("external_utils") == 2);
static_assert(is_known_system_axis("operating_system"),
              "A3 (O-8 Schritt 4): operating_system IST mit diesem Paket in die Ordnung eingetreten. "
              "Die A1-Zusage 'tritt erst mit A3 ein' ist damit eingeloest, nicht gebrochen.");
// ABGANGS-WACHEN (A3): die drei Abgaenge duerfen NICHT still zurueckkehren. Sie sind nicht geloescht,
// sondern umgezogen -- compiler und scheduling in Schritt 6 (aeussere Komplex-Achse bzw. sub_axis am
// target_isa-Wrapper), load_framework ERSATZLOS in den Mess-Realm (Ledger 69.1). Wer einen von ihnen
// wieder als System-HAUPT-Achse eintraegt, bricht hier compile-time und muss den Umzug widerrufen.
static_assert(!is_known_system_axis("compiler"),
              "compiler ist mit A3 Unter-Achsen-GRUPPE der aeusseren System-Komplex-Achse (O-1r), "
              "keine System-Haupt-Achse mehr.");
static_assert(!is_known_system_axis("scheduling"),
              "scheduling ist mit A3 sub_axis am target_isa-Komplex-Wrapper, keine Haupt-Achse mehr.");
static_assert(!is_known_system_axis("load_framework"),
              "load_framework hat die System-Welt mit A3 ERSATZLOS verlassen (Ledger 69.1 / R-G): es ist "
              "Meta-Meta-HAUPT-Achse der MESS-Achsen im Planer. Es gehoert weder in diese Ordnung noch "
              "unter den external_utils-Hub.");

} // namespace comdare::cache_engine::abi
