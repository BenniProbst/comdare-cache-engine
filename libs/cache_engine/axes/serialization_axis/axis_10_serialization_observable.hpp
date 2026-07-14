#pragma once
// V42 L-74c Composition-Driver — ObservableSerialization<Strategy>: ObservableAxis-Huelle um eine
// serialization-Strategie (axis_10). Drittes Exemplar des Huellen-Musters (nach telemetry/memory_layout):
// die Strategie traegt serialize_scan() als static, hat aber kein statistics(). Die Huelle reicht die
// static-Methode durch (Drop-in fuer abi_adapter.hpp:218 `Serializer::serialize_scan`) + bietet observe_serialize()
// als Instanz-Driver mit Tracking.
//
// @topic serialization @achse 10 @saeule 2 (Per-Achsen-Observer) @task V42-L-74c
//
// **Achsen-Semantik (treu):** Die serialization-Achse misst Serialisierungs-Aufwand je Strategie (RawBinary =
// Baseline memcpy, andere = Transformations-Aufwand). Die Counter (serialize_count/records_serialized/
// bytes_serialized/last_checksum) machen die Serialisierungs-Aktivitaet observable; der Aufwands-Unterschied
// der Strategien bleibt der Wall-Clock-Messung vorbehalten (Pfad B, Hybrid-Messmodell).

#include "concepts/axis_10_serialization_concept.hpp"
#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::serialization_axis {

/// ABI-taugliches Serialization-Snapshot (standard_layout + trivially_copyable).
struct SerializationSnapshot {
    std::uint64_t serialize_count    = 0; ///< Anzahl observe_serialize-Aufrufe
    std::uint64_t records_serialized = 0; ///< kumulierte Datensatz-Zahl
    std::uint64_t bytes_serialized   = 0; ///< serialisierte Bytes (records * record_size)
    std::uint64_t last_checksum      = 0; ///< letztes serialize_scan-Ergebnis (Korrektheits-Anker)

    [[nodiscard]] bool operator==(SerializationSnapshot const&) const noexcept = default;
};

/// ObservableAxis-Huelle: serialization-Strategie + Per-Achsen-Mess-Mechanik (gegated). KEIN Aggregat.
template <class Strategy>
    requires concepts::SerializationStrategy<Strategy>
class ObservableSerialization {
public:
    using strategy_type = Strategy;
    using topic_tag     = typename Strategy::topic_tag; // erfuellt SerializationComponent/SerializationStrategy

    // statische Forwarding-/Instrumentierungs-Hülle (KEIN GoF-Decorator: hält keine Komponenten-Instanz, kein Voll-Interface): Strategie-Inspektion durchgereicht.
    [[nodiscard]] static constexpr bool supports_compression() noexcept { return Strategy::supports_compression(); }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return Strategy::name(); }
    // INC-A #6: per-Organ-Codegen-Lokation. Wrapper-Typ = ObservableSerialization-Huelle (Enabled-Eintrag =
    // ObservableSerialization<Strategy>); Header = diese Huellen-Datei.
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::serialization_axis::ObservableSerialization",
                                  "axes/serialization_axis/axis_10_serialization_observable.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept
        requires requires { Strategy::family_name(); }
    {
        return Strategy::family_name();
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept
        requires requires { Strategy::flag_suffix(); }
    {
        return Strategy::flag_suffix();
    }
    [[nodiscard]] static constexpr std::string_view get_compiler() noexcept
        requires requires { Strategy::get_compiler(); }
    {
        return Strategy::get_compiler();
    }

    /// STATIC Pass-Through (Drop-in-Kompatibilität): unveraendert durchgereicht fuer die bestehenden Aufrufer
    /// (abi_adapter.hpp:218 `Serializer::serialize_scan`). Trackt NICHT.
    [[nodiscard]] static std::uint64_t serialize_scan(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        return Strategy::serialize_scan(buf, n, record_size);
    }

    /// Mess-Kopplung (Instanz-Driver): delegiert + trackt. Der Observer-Treiber ruft dies.
    [[nodiscard]] std::uint64_t observe_serialize(unsigned char const* buf, std::size_t n,
                                                  std::size_t record_size) noexcept {
        std::uint64_t const checksum = Strategy::serialize_scan(buf, n, record_size);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.serialize_count;
        stats_.records_serialized += static_cast<std::uint64_t>(n);
        stats_.bytes_serialized += static_cast<std::uint64_t>(n) * static_cast<std::uint64_t>(record_size);
        stats_.last_checksum = checksum;
#endif
        return checksum;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = SerializationSnapshot;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void                     reset() noexcept { stats_ = {}; }

private:
    snapshot_t stats_{};
#endif
};

} // namespace comdare::cache_engine::serialization_axis
