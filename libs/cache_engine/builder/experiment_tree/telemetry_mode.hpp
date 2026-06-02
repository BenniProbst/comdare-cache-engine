#pragma once
// KF-11 (2026-06-02) — Telemetrie: Default-AN + SILENT-Modus via Snapshot-Diff.
//
// User-Direktive 2026-06-02: „Das Telemetrie-Default bleibt host-seitig UND in den Tier-Binaries AN, aber wir
// müssen einen silent-Modus einstellen … Diff-Messung … durch die Messung NICHT beeinflusst." Idee: statt die
// Telemetrie während der gemessenen Region weiterlaufen/emittieren zu lassen (was die Messung perturbiert),
// nimmt der Silent-Modus EINEN Snapshot VOR und EINEN NACH der Region; das DELTA ist das Mess-Ergebnis. In der
// Region selbst wird NICHT emittiert/geloggt → die Region läuft unverfälscht.
//
// Builder-Seite (das WIE des Messens); die Tier-Binary-Telemetrie bleibt unverändert AN (nur ihre Emission wird
// in der Mess-Region stummgeschaltet). C++23, header-only.

#include <cstdint>

namespace comdare::cache_engine::builder::experiment {

/// Default = Active (Telemetrie AN, emittiert). Silent = stummgeschaltet während der Mess-Region (nur Diff).
enum class TelemetryMode : std::uint8_t { Active = 0, Silent = 1 };

/// Telemetrie-Konfiguration. Default-AN (Active) — host-seitig UND in den Tier-Binaries.
struct TelemetryConfig {
    TelemetryMode mode = TelemetryMode::Active;   // Default-AN
    [[nodiscard]] constexpr bool is_silent() const noexcept { return mode == TelemetryMode::Silent; }
    [[nodiscard]] constexpr bool operator==(TelemetryConfig const&) const noexcept = default;
};

/// Flacher Telemetrie-Snapshot: die gemessene Größe + ein Emissions-Zähler (Perturbations-Proxy).
struct TelemetrySnapshot {
    std::uint64_t value      = 0;  // die kumulierte Mess-Größe (Operationen / Bytes / Cache-Misses …)
    std::uint64_t emit_count = 0;  // wie oft die Telemetrie EMITTIERTE (Perturbations-Indikator)
    [[nodiscard]] constexpr bool operator==(TelemetrySnapshot const&) const noexcept = default;
};

/// Differenz zweier Snapshots (after − before) = das Diff-Mess-Ergebnis. NIE Mittelung — reine Subtraktion.
[[nodiscard]] constexpr TelemetrySnapshot telemetry_diff(TelemetrySnapshot const& before,
                                                         TelemetrySnapshot const& after) noexcept {
    return TelemetrySnapshot{
        (after.value      >= before.value)      ? after.value      - before.value      : 0,
        (after.emit_count >= before.emit_count) ? after.emit_count - before.emit_count : 0,
    };
}

/// SILENT-DIFF-Messung: Snapshot VOR → gemessene Region (im Silent-Modus, KEINE Emission) → Snapshot NACH → Diff.
/// `snap()` liefert einen TelemetrySnapshot; `region(TelemetryMode)` führt die zu messende Arbeit aus. Die Region
/// wird mit TelemetryMode::Silent gefahren → sie emittiert NICHT (emit_count-Delta == 0) → unverfälschte Messung.
template <class SnapshotFn, class RegionFn>
[[nodiscard]] TelemetrySnapshot measure_silent_diff(SnapshotFn&& snap, RegionFn&& region) {
    TelemetrySnapshot const before = snap();
    region(TelemetryMode::Silent);                 // Region läuft STUMM (kein Logging während der Messung)
    TelemetrySnapshot const after = snap();
    return telemetry_diff(before, after);
}

}  // namespace comdare::cache_engine::builder::experiment
