#pragma once
// Rueckschrieb-Methoden-Mess-UNTER-Achse (Ledger Section 47 / Section 54-T2 / Section 55, 2026-07-20): WIE die
// Mess-Ergebnisse persistiert werden. {Csv, LatexTable, ComparisonMetrics} = die EHRLICHE Formalisierung des
// heutigen <output>-Trios (ExperimentOutput: csv_path, latex_path, comparison_metrics). KEIN "pdf" -- die
// PDF-Kompilation ist KEIN Rueckschrieb-Kanal der Mess-Maschine (honest-0; das PDF entsteht Thesis-seitig).
//
// ABGRENZUNG (Section 54-T2): eine Mess-Tooling-UNTER-Achse (Planer-gesteuert, delegiert, binary_id-NEUTRAL)
// -- NICHT die HAUPT-Auffaecherung (MeasurementTooling, measurement_tooling_registry.hpp). A9.1 traegt diese
// Achse PASSIV (Feld + Parse + XSD + validate-id-Check); der Fan-out/Vollzug gehoert S5.
//
// KEIN Runtime-Switch: reine constexpr-Tabelle + Metaprogrammierungs-Iteration (analog measurement_tooling_registry).
// header-only, C++23. GOLDEN/HOST-NEUTRAL: reine constexpr-Identitaet, keine Host-/Bau-/Mess-Semantik.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::measurement {

/// Die Rueckschrieb-Methoden-UNTER-Achse: WIE die Mess-Ergebnisse persistiert werden (Section 47/55).
enum class WritebackMethod : std::uint8_t {
    Csv,               ///< CSV -- die Roh-Messzeilen (<output><csv_path>); die maschinen-lesbare Wahrheit
    LatexTable,        ///< LaTeX-Tabelle -- die formatierte Ergebnis-Tabelle (<output><latex_path>)
    ComparisonMetrics, ///< Vergleichs-Metriken -- die abgeleiteten SOTA-Deltas (<output><comparison_metrics>)
};

// Single-Source: Drift einer 4. Methode bricht hier compile-time (statt still 3 zu bleiben).
inline constexpr std::size_t kWritebackMethodCount = 3;

struct WritebackMethodInfo {
    WritebackMethod  method;
    std::string_view id;   ///< kanonischer XML-/Legenden-Token ("csv"/"latex_table"/"comparison_metrics")
    std::string_view name; ///< exakt der Enum-Name (Doku/Reporting)
};

/// Die EINE Registry der Rueckschrieb-Methoden-UNTER-Achse -- Index == WritebackMethod-Wert (static_assert-gesichert).
inline constexpr std::array<WritebackMethodInfo, kWritebackMethodCount> kWritebackMethodRegistry{{
    {WritebackMethod::Csv, "csv", "Csv"},
    {WritebackMethod::LatexTable, "latex_table", "LatexTable"},
    {WritebackMethod::ComparisonMetrics, "comparison_metrics", "ComparisonMetrics"},
}};

namespace detail {
[[nodiscard]] consteval bool writeback_method_registry_is_complete() {
    for (std::size_t i = 0; i < kWritebackMethodCount; ++i) {
        if (static_cast<std::size_t>(kWritebackMethodRegistry[i].method) != i) return false;
        if (kWritebackMethodRegistry[i].id.empty()) return false;
        if (kWritebackMethodRegistry[i].name.empty()) return false;
    }
    return true;
}
} // namespace detail
static_assert(kWritebackMethodRegistry.size() == kWritebackMethodCount,
              "kWritebackMethodRegistry: Array-Groesse == kWritebackMethodCount (Anzahl-Anker).");
static_assert(detail::writeback_method_registry_is_complete(),
              "kWritebackMethodRegistry: 3 Eintraege, Index==WritebackMethod, id/name nie leer.");
// Namen-Anker: Drift eines id-Tokens (Umbenennung/Vertauschung, inkl. des honest-0-Ausschlusses von pdf)
// bricht hier compile-time.
static_assert(kWritebackMethodRegistry[static_cast<std::size_t>(WritebackMethod::Csv)].id == std::string_view{"csv"} &&
                  kWritebackMethodRegistry[static_cast<std::size_t>(WritebackMethod::LatexTable)].id ==
                      std::string_view{"latex_table"} &&
                  kWritebackMethodRegistry[static_cast<std::size_t>(WritebackMethod::ComparisonMetrics)].id ==
                      std::string_view{"comparison_metrics"},
              "kWritebackMethodRegistry: id-Tokens sind {csv,latex_table,comparison_metrics} (Namen-Anker).");

/// constexpr-Lookup (Index == WritebackMethod-Wert, durch static_assert garantiert).
[[nodiscard]] constexpr WritebackMethodInfo const& writeback_method_info(WritebackMethod m) noexcept {
    return kWritebackMethodRegistry[static_cast<std::size_t>(m)];
}

/// Compile-time-Iteration ueber die Rueckschrieb-Methoden-UNTER-Achse (Metaprogrammierungs-Interface).
template <class Visitor>
constexpr void for_each_writeback_method(Visitor&& visitor) {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (visitor(kWritebackMethodRegistry[I]), ...);
    }(std::make_index_sequence<kWritebackMethodCount>{});
}

} // namespace comdare::cache_engine::measurement
