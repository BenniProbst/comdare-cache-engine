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
// A1-AUFTRAG, WORTGETREU: "NEUER Header kSystemAxisOrder als Single-Source (zunaechst mit dem
// IST-Inhalt)". Der Inhalt unten ist daher BIT-FUER-BIT die heutige Reihenfolge aus
// kSystemAxisCodeVersions -- compiler, external_utils, target_isa, scheduling, load_framework.
// A1 aendert KEINE Ordnung und KEINEN Namen; der Stempel bleibt byte-identisch (A1-GATE).
//
// A1 traegt AUSSCHLIESSLICH den IST-Stand (5 Achsen, Reihenfolge == kSystemAxisCodeVersions).
// Die finale System-Achsen-Ordnung legt Bauplan-v3 fest -- KEINE Vorwegnahme hier: nach dem
// Owner-KERN sind es GENAU DREI Haupt-Achsen (target_isa, operating_system, external_utils);
// load_framework VERLAESST die Ordnung Richtung MESS-Realm (Planer-Meta-Meta-Haupt-Achse) -- NACHZUG
// R-G, Ledger 69.1: die frueher hier notierte Zuordnung "META-META unter external_utils" ist
// SUPERSEDED, der Hub traegt nur SYSTEM-Meta-Metas; der Teil "NICHT eine Haupt-Achse" bleibt richtig.
// compiler wird Unter-Achsen-GRUPPE, scheduling Unter-Achse von target_isa. Erst A3 setzt die Ordnung
// + weitet die Wache auf "Suffix-Emitter == kSystemAxisOrder == XML-Kopf" aus.

#pragma once

#include "system_axis_code_versions.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::abi {

/// Anzahl der System-HAUPT-Achsen in der kanonischen Ordnung. A1: unveraendert 5 (IST-Stand).
inline constexpr std::size_t kSystemAxisOrderCount = kSystemAxisCodeCount;

/// DIE kanonische Reihenfolge der System-Haupt-Achsen. Jeder Konsument, der eine Ordnung braucht
/// (Stempel-Zeile, Generator-Blockfolge, XML-Kopf, Suffix-Segmente), MUSS sie hier lesen und nirgends
/// nachbilden. Reihenfolge == IST-Stand vor Lane A (siehe Kopf-Kommentar).
inline constexpr std::array<std::string_view, kSystemAxisOrderCount> kSystemAxisOrder{{
    "compiler",
    "external_utils",
    "target_isa",
    "scheduling",
    "load_framework",
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
static_assert(system_axis_order_index("target_isa") == 2);
static_assert(!is_known_system_axis("operating_system"),
              "operating_system tritt erst mit Paket A3 in die Ordnung ein -- A1 ist ordnungs-neutral.");

} // namespace comdare::cache_engine::abi
