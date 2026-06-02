#pragma once
// V42 L-74c Composition-Driver — ObservableMemoryLayout<Strategy>: ObservableAxis-Huelle um eine
// memory_layout-Strategie (axis_05). Analog axis_11_telemetry_observable.hpp + observable_composed_search.hpp:
// die Strategie selbst (CacheLineAlignedMemoryLayout etc.) traegt zwar die verhaltens-tragende Methode
// scan_field_sum(), hat aber KEIN statistics()/snapshot_t (kein ObservableAxis). Die Mess-Mechanik gehoert
// daher in diese Huelle, die scan_field_sum durchreicht UND dabei trackt.
//
// @topic memory_layout @achse 05 @saeule 2 (Per-Achsen-Observer) @task V42-L-74c
//
// **Achsen-Semantik (treu):** Die Layout-Achse misst Cache-Effekte des Zugriffs-Patterns (AoS strided vs
// SoA contiguous, axis_05_memory_layout_cache_line_aligned.hpp). Die Counter-Metriken (scan_count/
// records_scanned/field_bytes_read/cache_lines_touched) machen die Scan-Aktivitaet observable; der reine
// Latenz-Unterschied der Patterns bleibt der Wall-Clock-Messung vorbehalten (Pfad B, Hybrid-Messmodell).
// `cache_lines_touched` ist eine record_size-basierte AoS-Strided-Schaetzung (n * ceil(record_size/line)).
//
// Gating exakt nach Praezedenz: snapshot_t/statistics()/reset() unter COMDARE_CE_ENABLE_STATISTICS.
// Bei OFF: scan_field_sum = nackter Pass-Through (0 Footprint), ObservableAxis<...> = false.

#include "concepts/axis_05_memory_layout_concept.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::layout {

/// ABI-taugliches Memory-Layout-Snapshot (standard_layout + trivially_copyable).
struct MemoryLayoutSnapshot {
    std::uint64_t scan_count         = 0;   ///< Anzahl scan_field_sum-Aufrufe
    std::uint64_t records_scanned    = 0;   ///< kumulierte Datensatz-Zahl ueber alle Scans
    std::uint64_t field_bytes_read   = 0;   ///< gelesene Feld-Bytes (records * sizeof(uint32))
    std::uint64_t cache_lines_touched = 0;  ///< record_size-basierte AoS-Strided-Schaetzung
    std::uint64_t last_checksum      = 0;   ///< letztes scan_field_sum-Ergebnis (Korrektheits-Anker)

    [[nodiscard]] bool operator==(MemoryLayoutSnapshot const&) const noexcept = default;
};

/// ObservableAxis-Huelle: memory_layout-Strategie + Per-Achsen-Mess-Mechanik (gegated).
/// KEIN Aggregat (private member + Methoden) -> direkt als Anatomie-Member haltbar.
template <class Strategy>
    requires concepts::MemoryLayoutStrategy<Strategy>
class ObservableMemoryLayout {
public:
    using strategy_type = Strategy;

    // Transparenter Decorator: Strategie-Inspektion durchgereicht (composition_registry/axis_path_serialization
    // rufen C::memory_layout::name()).
    [[nodiscard]] static constexpr std::size_t      cache_line_size() noexcept { return Strategy::cache_line_size(); }
    [[nodiscard]] static constexpr std::string_view name()            noexcept { return Strategy::name(); }
    [[nodiscard]] static constexpr std::string_view family_name()
        noexcept requires requires { Strategy::family_name(); } { return Strategy::family_name(); }
    [[nodiscard]] static constexpr std::string_view flag_suffix()
        noexcept requires requires { Strategy::flag_suffix(); } { return Strategy::flag_suffix(); }

    /// STATIC Pass-Through (Drop-in-Kompatibilität): die Strategie-Methode wird unveraendert durchgereicht,
    /// damit die Huelle als memory_layout-Slot die bestehenden static-Aufrufer NICHT bricht
    /// (abi_adapter.hpp:215 `MemLayout::scan_field_sum`, axis_04_node_type_composed_store.hpp:90 `L::scan_field_sum`).
    /// Diese Variante trackt NICHT (static, kein Instanz-State).
    [[nodiscard]] static std::uint64_t scan_field_sum(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        return Strategy::scan_field_sum(buf, n, record_size);
    }

    /// Mess-Kopplung (der eigentliche „Driver", Instanz): delegiert an die Strategie + trackt. Der Observer-
    /// Treiber ruft dies → die Layout-Scan-Aktivitaet wird observable (statistics()). Getrennt von der static-
    /// Variante, weil die bestehenden Aufrufer static bleiben muessen.
    [[nodiscard]] std::uint64_t observe_scan(unsigned char const* buf, std::size_t n,
                                             std::size_t record_size) noexcept {
        std::uint64_t const checksum = Strategy::scan_field_sum(buf, n, record_size);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.scan_count;
        stats_.records_scanned  += static_cast<std::uint64_t>(n);
        stats_.field_bytes_read += static_cast<std::uint64_t>(n) * sizeof(std::uint32_t);
        std::size_t const line = Strategy::cache_line_size();
        std::size_t const lines_per_record = (line == 0) ? 1 : ((record_size + line - 1) / line);
        stats_.cache_lines_touched += static_cast<std::uint64_t>(n) * (lines_per_record == 0 ? 1 : lines_per_record);
        stats_.last_checksum = checksum;
#endif
        return checksum;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = MemoryLayoutSnapshot;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; }

private:
    snapshot_t stats_{};
#endif
};

}  // namespace comdare::cache_engine::layout
