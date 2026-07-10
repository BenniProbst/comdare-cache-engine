#pragma once
// Phase-6-Vorbau (2026-07-10): compile-time Mess-Achsen-Registry ueber die 16 MeasurementCategory-
// System-Achsen ("Blut", Dossier 19 Querschnitt M). KEIN Runtime-Switch — reine constexpr-Tabelle +
// Metaprogrammierungs-Iteration; das Regime kommt SINGLE-SOURCE aus regime_of (system_axis.hpp),
// nichts wird dupliziert. Konsumenten (E4-Reporting, Pruef-Dock-Verdrahtung) = Folge-Increment.

#include <cache_engine/measurement/system_axis.hpp> // MeasurementCategory / MeasurementRegime / regime_of

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::measurement {

struct MeasurementAxisInfo {
    MeasurementCategory category;
    std::string_view    name;   ///< exakt der Enum-Name (E4-Reporting/CSV-Spalten-Vokabular)
    MeasurementRegime   regime; ///< aus regime_of — Zweiteilung TimeObserver/PmcCounter (M-Wurzel)
};

// Single-Source: Drift einer 17. Kategorie bricht hier compile-time (statt still 16 zu bleiben).
inline constexpr std::size_t kMeasurementAxisCount = kMeasurementCategoryCount;

namespace detail {
[[nodiscard]] constexpr MeasurementAxisInfo make_axis_info(MeasurementCategory c, std::string_view n) {
    return MeasurementAxisInfo{c, n, regime_of(c)};
}
} // namespace detail

/// Die EINE Registry aller System-Mess-Achsen — Index == Kategorie-Wert (static_assert-gesichert).
inline constexpr std::array<MeasurementAxisInfo, kMeasurementAxisCount> kMeasurementAxisRegistry{{
    detail::make_axis_info(MeasurementCategory::CLU, "CLU"),
    detail::make_axis_info(MeasurementCategory::CACHE_MISS_L1, "CACHE_MISS_L1"),
    detail::make_axis_info(MeasurementCategory::CACHE_MISS_L2, "CACHE_MISS_L2"),
    detail::make_axis_info(MeasurementCategory::CACHE_MISS_L3, "CACHE_MISS_L3"),
    detail::make_axis_info(MeasurementCategory::DTLB_MISS, "DTLB_MISS"),
    detail::make_axis_info(MeasurementCategory::MEMORY_FOOTPRINT, "MEMORY_FOOTPRINT"),
    detail::make_axis_info(MeasurementCategory::BRANCH_MISS, "BRANCH_MISS"),
    detail::make_axis_info(MeasurementCategory::IPC_CPI, "IPC_CPI"),
    detail::make_axis_info(MeasurementCategory::LATENCY_MEAN, "LATENCY_MEAN"),
    detail::make_axis_info(MeasurementCategory::LATENCY_P50, "LATENCY_P50"),
    detail::make_axis_info(MeasurementCategory::LATENCY_P95, "LATENCY_P95"),
    detail::make_axis_info(MeasurementCategory::LATENCY_P99, "LATENCY_P99"),
    detail::make_axis_info(MeasurementCategory::LATENCY_P999, "LATENCY_P999"),
    detail::make_axis_info(MeasurementCategory::THROUGHPUT, "THROUGHPUT"),
    detail::make_axis_info(MeasurementCategory::ENERGY_J, "ENERGY_J"),
    detail::make_axis_info(MeasurementCategory::FILL_BUFFER_OCCUPANCY, "FILL_BUFFER_OCCUPANCY"),
}};

namespace detail {
[[nodiscard]] consteval bool registry_is_complete() {
    for (std::size_t i = 0; i < kMeasurementAxisCount; ++i) {
        if (static_cast<std::size_t>(kMeasurementAxisRegistry[i].category) != i) return false;
        if (kMeasurementAxisRegistry[i].regime != regime_of(kMeasurementAxisRegistry[i].category)) return false;
        if (kMeasurementAxisRegistry[i].name.empty()) return false;
    }
    return true;
}
} // namespace detail
static_assert(detail::registry_is_complete(),
              "kMeasurementAxisRegistry: 16 Eintraege, Index==Kategorie, Regime==regime_of, Name nie leer");

/// constexpr-Lookup (Index == Kategorie-Wert, durch static_assert garantiert).
[[nodiscard]] constexpr MeasurementAxisInfo const& axis_info(MeasurementCategory c) noexcept {
    return kMeasurementAxisRegistry[static_cast<std::size_t>(c)];
}

/// Compile-time-Iteration ueber alle Mess-Achsen (Metaprogrammierungs-Interface fuer E4-Reporting).
template <class Visitor>
constexpr void for_each_measurement_axis(Visitor&& visitor) {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (visitor(kMeasurementAxisRegistry[I]), ...);
    }(std::make_index_sequence<kMeasurementAxisCount>{});
}

} // namespace comdare::cache_engine::measurement
