#pragma once
// Rueckschrieb-Methoden-Mess-UNTER-Achse (Ledger Section 47 / Section 54-T2 / Section 55, 2026-07-20): WIE die
// Mess-Ergebnisse persistiert werden. {Csv, LatexTable, ComparisonMetrics} = die EHRLICHE Formalisierung des
// heutigen <output>-Trios (ExperimentOutput: csv_path, latex_path, comparison_metrics). KEIN "pdf" -- die
// PDF-Kompilation ist KEIN Rueckschrieb-Kanal der Mess-Maschine (honest-0; das PDF entsteht Thesis-seitig).
//
// A9-S3 (2026-08-08): VIERTER Wert Xlsx. Owner-KERN 26.07. Section 6 woertlich: "xlsx = kuenftig DEFAULT,
// CSV einstellbar + Fallback" -- der xlsx-Ergebnis-Writer (builder/lager_ablage/) existiert jetzt, und ohne
// diesen vierten Wert bleibt der XML-Schalter der Rueckschrieb-Methode unwirksam (der Kommentar unten hat
// genau das angekuendigt: eine 4. Methode bricht hier compile-time -- das ist jetzt gewollt eingetreten).
// Xlsx bleibt wie die anderen drei GOLDEN-/HOST-NEUTRAL: die Registry-Zeile ist eine reine Identitaet, kein
// Fan-out/Vollzug (der liegt in der Lager-Ablage-Strecke, nicht hier).
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
    Xlsx,              ///< xlsx -- A9-S3 Lager-Ablage-Mappe (DEFAULT-Auswerteformat, CSV bleibt Fallback)
};

// Single-Source: Drift einer 5. Methode bricht hier compile-time (statt still 4 zu bleiben).
inline constexpr std::size_t kWritebackMethodCount = 4;

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
    {WritebackMethod::Xlsx, "xlsx", "Xlsx"},
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
              "kWritebackMethodRegistry: 4 Eintraege, Index==WritebackMethod, id/name nie leer.");
// Namen-Anker: Drift eines id-Tokens (Umbenennung/Vertauschung, inkl. des honest-0-Ausschlusses von pdf)
// bricht hier compile-time.
static_assert(kWritebackMethodRegistry[static_cast<std::size_t>(WritebackMethod::Csv)].id == std::string_view{"csv"} &&
                  kWritebackMethodRegistry[static_cast<std::size_t>(WritebackMethod::LatexTable)].id ==
                      std::string_view{"latex_table"} &&
                  kWritebackMethodRegistry[static_cast<std::size_t>(WritebackMethod::ComparisonMetrics)].id ==
                      std::string_view{"comparison_metrics"} &&
                  kWritebackMethodRegistry[static_cast<std::size_t>(WritebackMethod::Xlsx)].id ==
                      std::string_view{"xlsx"},
              "kWritebackMethodRegistry: id-Tokens sind {csv,latex_table,comparison_metrics,xlsx} (Namen-Anker).");
// Drift-Gegenprobe (RF-3-Muster): hinter dem Count darf KEIN etikettierter Eintrag mehr auftauchen. Ein
// Anhaengen ohne Hochzaehlen des Counts wuerde still schweigen -- diese Zeile bricht dann compile-time.
static_assert(kWritebackMethodCount == static_cast<std::size_t>(WritebackMethod::Xlsx) + 1,
              "kWritebackMethodCount muss genau hinter dem LETZTEN Enumerator stehen (Namens-Pin-Muster).");

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
