#pragma once
// BenchmarkRunner - No-deprecate Wrapper aller Testmethoden (REV 7 §8.2.1)
//
// API-Garantie: NIEMALS aendern (no-deprecate).
// Akkumuliert Messdaten in 2 separaten Custom-Allokationen.

#include "custom_allocation_1_measurements.hpp"
#include "custom_allocation_2_state_log.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits> // UINT64_MAX-Sentinel von CustomAllocation1::append (Ueberlauf-Erkennung)
#include <span>
#include <string_view>

namespace comdare::benchmark_suite {

enum class EventKind : std::uint32_t {
    BeginPhase   = 0,
    EndPhase     = 1,
    CacheMiss    = 2,
    Allocation   = 3,
    Deallocation = 4,
    NodeSplit    = 5,
    NodeMerge    = 6,
    Custom       = 7,
};

class BenchmarkRunner {
public:
    using id_t = std::uint64_t;

    explicit BenchmarkRunner(std::size_t measurements_capacity_bytes = 1ULL << 30,
                             std::size_t state_log_capacity_bytes    = 64ULL << 20)
        : measurements_{measurements_capacity_bytes}, state_log_{state_log_capacity_bytes} {}

    // -----------------------------------------------------------------------------------------
    // BEFUND (Warnungs-Review Runde 2b, clang ueber den ce-Test-Bau, 09.08.2026), woertlich:
    //   benchmark_runner.hpp:47:9: warning: ignoring return value of function declared with
    //                              'nodiscard' attribute [-Wunused-result]   (ebenso :57, :66, :70)
    //
    // WAS DAHINTER LAG -- die Fehlerklasse dieses Hauses, in Reinform:
    //   CustomAllocation1::append() gibt bei vollem Puffer UINT64_MAX zurueck ("Lock-free append:
    //   returns slot-index oder UINT64_MAX bei Ueberlauf"), CustomAllocation2::push_state() gibt
    //   false. BEIDE Rueckgaben wurden hier verworfen. Ein voller Puffer verlor damit JEDEN weiteren
    //   Messsatz -- lautlos. Und weil records_used() auf capacity_ klemmt, sah der Bestand danach
    //   aus wie ein sauber gefuellter Puffer: kein Wurf, kein Rueckgabewert, keine Luecke in der
    //   Zaehlung. Es gab nichts, was klappern konnte.
    //   Verschaerfend: append() erhoeht next_slot_ VOR dem Kapazitaetstest. Nach dem ersten Ueberlauf
    //   sind also alle folgenden Saetze verloren, nicht nur die ueberzaehligen.
    //
    // DIE HEILUNG IST ADDITIV -- der Kopf dieser Datei sagt "API-Garantie: NIEMALS aendern
    // (no-deprecate)". Keine bestehende Signatur, kein bestehendes Verhalten im NICHT-Ueberlauf-Fall
    // aendert sich. NEU ist ausschliesslich ein Zaehler samt Lesezugriff: der Verlust wird von
    // unsichtbar zu ZAEHLBAR. Wer eine Messreihe abnimmt, hat damit den Nenner
    // (records_collected() + dropped_records()) statt nur den Zaehler.
    // -----------------------------------------------------------------------------------------

    // Pflicht-API (no-deprecate Garantie)
    [[nodiscard]] id_t begin_measurement(std::string_view tag) noexcept {
        // tag wird NICHT gespeichert: MeasurementRecord32 hat kein Feld dafuer. Das ist ein
        // benannter offener Posten des Warnungs-Reviews (clang: unused parameter 'tag'), KEIN
        // Versehen dieser Zeile -- ein Feld anzuhaengen aendert das Mess-Satz-Format und damit
        // jeden bestehenden Blob. Der Parameter bleibt in der Signatur, weil der Kopf dieser Datei
        // die API einfriert; er wird hier ausdruecklich verworfen, damit die Absicht lesbar ist.
        (void)tag;
        id_t                handle = next_handle_.fetch_add(1, std::memory_order_relaxed);
        MeasurementRecord32 rec{};
        rec.timestamp_ns    = now_ns();
        rec.op_id           = handle;
        rec.op_kind         = static_cast<std::uint32_t>(EventKind::BeginPhase);
        rec.cycles_or_value = 0;
        append_zaehlend_(rec);
        return handle;
    }

    void record_event(id_t handle, EventKind kind, std::uint64_t aux = 0) noexcept {
        MeasurementRecord32 rec{};
        rec.timestamp_ns    = now_ns();
        rec.op_id           = handle;
        rec.op_kind         = static_cast<std::uint32_t>(kind);
        rec.cycles_or_value = aux;
        append_zaehlend_(rec);
    }

    void end_measurement(id_t handle, std::uint64_t observed_cycles) noexcept {
        MeasurementRecord32 rec{};
        rec.timestamp_ns    = now_ns();
        rec.op_id           = handle;
        rec.op_kind         = static_cast<std::uint32_t>(EventKind::EndPhase);
        rec.cycles_or_value = observed_cycles;
        append_zaehlend_(rec);
    }

    void log_sparse_state(std::uint8_t marker, std::span<std::byte const> delta) noexcept {
        if (!state_log_.push_state(marker, delta))
            dropped_states_.fetch_add(1, std::memory_order_relaxed);
    }

    // Conversion (Phase 8 — NUR Post-Experiment): binary blob -> handy formats
    void flush_to_binary_blob(std::filesystem::path const& output) const;

    [[nodiscard]] std::uint64_t records_collected() const noexcept { return measurements_.records_used(); }

    /// VERWORFENE Messsaetze (Puffer voll). ADDITIV zum eingefrorenen API-Satz. Ohne diese Zahl ist
    /// records_collected() ein Zaehler ohne Nenner: capacity_ Saetze koennen "alles" heissen oder
    /// "alles bis hierhin, der Rest ist weg". Erst records_collected() + dropped_records() sagt,
    /// wieviel angefallen IST -- und ein Wert > 0 ist der harte Beleg, dass die Reihe unvollstaendig
    /// ist und die Kapazitaet nicht reichte.
    [[nodiscard]] std::uint64_t dropped_records() const noexcept {
        return dropped_records_.load(std::memory_order_relaxed);
    }
    /// Dasselbe fuer den Zustands-Log (CustomAllocation2::push_state == false).
    [[nodiscard]] std::uint64_t dropped_states() const noexcept {
        return dropped_states_.load(std::memory_order_relaxed);
    }
    /// TRUE, sobald irgendetwas verloren ging -- die eine Frage, die eine Abnahme stellen muss.
    [[nodiscard]] bool messreihe_ist_vollstaendig() const noexcept {
        return dropped_records() == 0 && dropped_states() == 0;
    }
    [[nodiscard]] std::size_t   state_log_bytes() const noexcept { return state_log_.bytes_used(); }

    [[nodiscard]] CustomAllocation1 const& measurements() const noexcept { return measurements_; }
    [[nodiscard]] CustomAllocation2 const& state_log() const noexcept { return state_log_; }

private:
    /// Der EINE Ort, an dem der append-Rueckgabewert ausgewertet wird. Vorher stand an drei Stellen
    /// ein `measurements_.append(rec);` ohne Auswertung -- drei Kopien derselben Blindstelle.
    void append_zaehlend_(MeasurementRecord32 const& rec) noexcept {
        if (measurements_.append(rec) == (std::numeric_limits<std::uint64_t>::max)())
            dropped_records_.fetch_add(1, std::memory_order_relaxed);
    }

    [[nodiscard]] static std::uint64_t now_ns() noexcept {
        auto const t = std::chrono::steady_clock::now().time_since_epoch();
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t).count());
    }

    CustomAllocation1          measurements_;
    CustomAllocation2          state_log_;
    std::atomic<std::uint64_t> next_handle_{0};
    std::atomic<std::uint64_t> dropped_records_{0};
    std::atomic<std::uint64_t> dropped_states_{0};
};

} // namespace comdare::benchmark_suite
