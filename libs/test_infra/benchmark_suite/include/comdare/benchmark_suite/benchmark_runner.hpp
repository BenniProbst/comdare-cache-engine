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
    // (records_collected() + measurements_dropped()) statt nur den Zaehler.
    // -----------------------------------------------------------------------------------------

    // Pflicht-API (no-deprecate Garantie)
    //
    // 09.08.2026 (Warnungs-Runde 1, Klasse SPEICHER): die vier Methoden unten VERWARFEN bis heute den
    // Rueckgabewert ihrer Arena-Aufrufe. append() meldet den Ueberlauf mit kOverflow, push_state() mit
    // false -- beide Meldungen fielen auf den Boden. GCC nannte das viermal als -Wunused-result, und
    // die Warnung war KEIN Rauschen: ist die Arena voll, wird jeder weitere Satz still verworfen, und
    // records_collected() kappt auf die Kapazitaet, sieht also aus wie ein vollstaendiger Lauf. Eine
    // Messreihe konnte damit Loecher haben, ohne dass irgendwo etwas klapperte.
    //
    // Die Signaturen bleiben unangetastet (no-deprecate). Geheilt wird, indem der Verlust GEZAEHLT
    // und ueber measurements_dropped()/states_dropped() ABFRAGBAR wird -- der Aufrufer kann eine
    // Messung jetzt als unvollstaendig erkennen, statt sie fuer voll zu halten.
    [[nodiscard]] id_t begin_measurement([[maybe_unused]] std::string_view tag) noexcept {
        // tag ist bewusst ungenutzt und bleibt trotzdem im Kopf: MeasurementRecord32 ist auf 32 Byte
        // eingefroren und hat kein Feld fuer eine Phasen-Beschriftung. Der Parameter gehoert zur
        // no-deprecate-API und darf nicht entfallen; ein Umzug der Beschriftung in den State-Log
        // (CustomAllocation2) waere die saubere Erweiterung und ist als eigener Posten benannt.
        id_t                handle = next_handle_.fetch_add(1, std::memory_order_relaxed);
        MeasurementRecord32 rec{};
        rec.timestamp_ns    = now_ns();
        rec.op_id           = handle;
        rec.op_kind         = static_cast<std::uint32_t>(EventKind::BeginPhase);
        rec.cycles_or_value = 0;
        note_append_(measurements_.append(rec));
        return handle;
    }

    void record_event(id_t handle, EventKind kind, std::uint64_t aux = 0) noexcept {
        MeasurementRecord32 rec{};
        rec.timestamp_ns    = now_ns();
        rec.op_id           = handle;
        rec.op_kind         = static_cast<std::uint32_t>(kind);
        rec.cycles_or_value = aux;
        note_append_(measurements_.append(rec));
    }

    void end_measurement(id_t handle, std::uint64_t observed_cycles) noexcept {
        MeasurementRecord32 rec{};
        rec.timestamp_ns    = now_ns();
        rec.op_id           = handle;
        rec.op_kind         = static_cast<std::uint32_t>(EventKind::EndPhase);
        rec.cycles_or_value = observed_cycles;
        note_append_(measurements_.append(rec));
    }

    void log_sparse_state(std::uint8_t marker, std::span<std::byte const> delta) noexcept {
        if (!state_log_.push_state(marker, delta)) { states_dropped_.fetch_add(1, std::memory_order_relaxed); }
    }

    /// Wie viele Mess-Saetze diese Instanz VERLOREN hat (Arena voll). 0 = kein Verlust.
    /// ADDITIV zum eingefrorenen API-Satz. Ohne diese Zahl ist records_collected() ein Zaehler
    /// OHNE NENNER: capacity_ Saetze koennen "alles" heissen oder
    /// "alles bis hierhin, der Rest ist weg". Erst records_collected() + measurements_dropped() sagt,
    /// wieviel angefallen IST -- und ein Wert > 0 ist der harte Beleg, dass die Kapazitaet nicht
    /// reichte. Benannt wie CustomAllocation1::records_dropped(), damit es im selben Modul EINE
    /// Wortstellung fuer denselben Begriff gibt.
    [[nodiscard]] std::uint64_t measurements_dropped() const noexcept {
        return measurements_dropped_.load(std::memory_order_relaxed);
    }

    /// Wie viele State-Eintraege diese Instanz VERLOREN hat (State-Log voll). 0 = kein Verlust.
    [[nodiscard]] std::uint64_t states_dropped() const noexcept {
        return states_dropped_.load(std::memory_order_relaxed);
    }

    /// DIE EINE FRAGE, die ein Auswerter stellen muss, bevor er Zahlen glaubt: ist diese Messreihe
    /// vollstaendig? Solange sie false liefert, sind records_collected() und der State-Log deckend.
    [[nodiscard]] bool measurement_complete() const noexcept {
        return measurements_dropped() == 0 && states_dropped() == 0;
    }

    // Conversion (Phase 8 — NUR Post-Experiment): binary blob -> handy formats
    void flush_to_binary_blob(std::filesystem::path const& output) const;

    [[nodiscard]] std::uint64_t records_collected() const noexcept { return measurements_.records_used(); }

    [[nodiscard]] std::size_t state_log_bytes() const noexcept { return state_log_.bytes_used(); }

    [[nodiscard]] CustomAllocation1 const& measurements() const noexcept { return measurements_; }
    [[nodiscard]] CustomAllocation2 const& state_log() const noexcept { return state_log_; }

private:
    [[nodiscard]] static std::uint64_t now_ns() noexcept {
        auto const t = std::chrono::steady_clock::now().time_since_epoch();
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t).count());
    }

    /// EINE Stelle, an der der append()-Befund ausgewertet wird -- die drei Aufrufer sollen sich nicht
    /// je einzeln an den Vergleich mit kOverflow erinnern muessen. Im Gutfall kostet das einen
    /// vorhersagbaren Vergleich, der Zaehler wird nur im Verlustfall angefasst.
    void note_append_(std::uint64_t slot) noexcept {
        if (slot == CustomAllocation1::kOverflow) { measurements_dropped_.fetch_add(1, std::memory_order_relaxed); }
    }

    CustomAllocation1          measurements_;
    CustomAllocation2          state_log_;
    std::atomic<std::uint64_t> next_handle_{0};
    std::atomic<std::uint64_t> measurements_dropped_{0};
    std::atomic<std::uint64_t> states_dropped_{0};
};

} // namespace comdare::benchmark_suite
