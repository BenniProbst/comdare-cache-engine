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
    std::uint64_t field_bytes_read   = 0;   ///< P-MD1: REAL belegte Nutzbytes je Layout (n * useful, LAYOUT-ABHAENGIG)
    std::uint64_t cache_lines_touched = 0;  ///< P-MD1: REAL beruehrte 64-B-Lines je Layout (ceil(n*span/64), LAYOUT-ABHAENGIG)
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
    // topic_tag durchgereicht → die Huelle erfuellt MemoryLayoutComponent/MemoryLayoutStrategy und ist damit
    // als L in ComposedStore<N,L,A> einsetzbar (node_type-Achse, anatomy_execution_context.hpp:46 / abi_adapter.hpp:393).
    using topic_tag = typename Strategy::topic_tag;

    // Transparenter Decorator: Strategie-Inspektion durchgereicht (composition_registry/axis_path_serialization
    // rufen C::memory_layout::name()).
    [[nodiscard]] static constexpr std::size_t      cache_line_size() noexcept { return Strategy::cache_line_size(); }
    [[nodiscard]] static constexpr std::string_view name()            noexcept { return Strategy::name(); }
    // P-MD1-ERDUNG (#167): die REALE Repraesentation der gewrappten Strategie transparent durchreichen, damit die
    // Huelle als L im LayoutAwareChunkedStore (z.B. via ArtComposition/observable_composed_search) denselben
    // physischen Store-Footprint erzeugt wie die nackte Strategie.
    [[nodiscard]] static constexpr ::comdare::cache_engine::layout::RepresentationKind representation_kind() noexcept {
        return Strategy::representation_kind();
    }
    [[nodiscard]] static constexpr std::size_t block_width()
        noexcept requires requires { Strategy::block_width(); } { return Strategy::block_width(); }
    [[nodiscard]] static constexpr std::string_view family_name()
        noexcept requires requires { Strategy::family_name(); } { return Strategy::family_name(); }
    [[nodiscard]] static constexpr std::string_view flag_suffix()
        noexcept requires requires { Strategy::flag_suffix(); } { return Strategy::flag_suffix(); }
    // AxisBase-Eigenschaft (Default "original") transparent durchgereicht: test_v41_compositions ruft
    // C::memory_layout::get_compiler() auf der gewrappten Achse. SFINAE-sicher (Methode existiert nur,
    // wenn die Strategie sie traegt).
    [[nodiscard]] static constexpr std::string_view get_compiler()
        noexcept requires requires { Strategy::get_compiler(); } { return Strategy::get_compiler(); }

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
        // GENERISCHER Footprint (Raw-Buffer-Pfad ohne realen Store, z.B. abi_adapter/Anatomie-Test): voller
        // Record-Stride. Die LAYOUT-DISTINKTE, REALE CLU kommt aus observe_real_footprint() (P-MD1-ERDUNG #167),
        // das vom LayoutAwareChunkedStore mit dem ECHTEN, representation-spezifischen Key-Scan-Footprint gefuettert
        // wird — NICHT mehr aus einem entkoppelten record_useful_bytes/record_line_span-Deskriptor.
        constexpr std::size_t kKeyBytes  = sizeof(std::uint64_t);
        constexpr std::size_t kLineBytes = 64u;
        std::size_t const rs = (record_size == 0) ? (2u * kKeyBytes) : record_size;
        std::uint64_t const touched_bytes = static_cast<std::uint64_t>(n) * static_cast<std::uint64_t>(rs);
        stats_.field_bytes_read    += static_cast<std::uint64_t>(n) * static_cast<std::uint64_t>(kKeyBytes);
        stats_.cache_lines_touched += (touched_bytes + (kLineBytes - 1u)) / kLineBytes;
        stats_.last_checksum = checksum;
#endif
        return checksum;
    }

    /// observe_real_footprint (P-MD1-ERDUNG #167): der REALE, vom LayoutAwareChunkedStore representation-genau
    /// vermessene Key-Scan-Footprint EINES Chunks. `field_bytes` = real beruehrte NUTZ-Key-Bytes, `cache_lines` =
    /// real beruehrte DISTINKTE 64-B-Linien aus dem echten Byte-Layout der Repraesentation. CLU =
    /// field_bytes/(cache_lines*64) folgt damit aus dem ECHTEN Store-Footprint, nicht aus einem Modell. Der
    /// Store ruft dies pro Chunk; die Werte werden akkumuliert (`records` = Chunk-Record-Zahl als Anker).
    [[nodiscard]] std::uint64_t observe_real_footprint(std::uint64_t checksum, std::size_t records,
                                                       std::uint64_t field_bytes, std::uint64_t cache_lines) noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.scan_count;
        stats_.records_scanned     += static_cast<std::uint64_t>(records);
        stats_.field_bytes_read    += field_bytes;
        stats_.cache_lines_touched += cache_lines;
        stats_.last_checksum        = checksum;
#else
        (void)records; (void)field_bytes; (void)cache_lines;
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
