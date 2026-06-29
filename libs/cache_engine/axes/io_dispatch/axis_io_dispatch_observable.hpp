#pragma once
// Per-Achsen-Observer Phase B (2026-06-04) — ObservableIoDispatch<Strategy>: ObservableAxis-Huelle um eine
// io_dispatch-Strategie (T14, axis_io). EXAKT analog axis_05_memory_layout_observable.hpp / axis_11_telemetry_
// observable.hpp: die Strategie selbst (InMemoryOnly/Buffered/Direct/Mmap) traegt zwar die verhaltens-tragende
// static Methode io_dispatch_scan(), hat aber KEIN statistics()/snapshot_t (ist KEIN ObservableAxis). Die Mess-
// Mechanik gehoert daher in diese Huelle, die io_dispatch_scan static durchreicht (seg19-Aufrufer bleiben heil)
// UND einen Instanz-Driver observe_dispatch() ergaenzt, der beim Treiben trackt.
//
// @topic io @achse 14 @saeule 2 (Per-Achsen-Observer) @task Phase-B
//
// **Achsen-Semantik (EHRLICH deklariert):** Die io_dispatch-Achse modelliert das Dispatch-Profil der Persistence-
// Strategie OHNE echtes Disk-IO — bewusst, denn echte Platten-Wartezeit im DRAM-Benchmark verfaelschte die Algo-
// Charakteristik (Hauptagent-Entscheid: io = In-Memory, KEIN Disk-IO). Die Op ist eine reale, strategie-abhaengige
// IN-MEMORY-Dispatch-Simulation (so im Wrapper-Doc-Kommentar gekennzeichnet — keine erfundene Konstante). Die
// Huelle macht diese Aktivitaet observable: dispatch_rounds = Anzahl observe_dispatch-Aufrufe (Dispatch-Runden);
// bytes_dispatched = ueber alle Runden in den Index "dispatchte" Record-Bytes (n * record_size); alignment_adjusts
// = strategie-abhaengige Alignment-Anpassungen (Buffered/Direct/Mmap richten an die Block-Granularitaet aus, In-
// MemoryOnly nicht — der Zaehler folgt is_in_memory_only()); total_dispatch_count = kumulierte dispatchte Records
// (n je Runde). KEIN erfundener Wert — jeder Zaehler folgt der echten Op-Schleife.
//
// Gating exakt nach Praezedenz (axis_05): snapshot_t/statistics()/reset() unter COMDARE_CE_ENABLE_STATISTICS.
// Bei OFF: io_dispatch_scan = nackter Pass-Through (0 Footprint), ObservableAxis<...> = false → observe_all()
// faellt auf EmptyAxisSnapshot zurueck (Release-Pfad, korrekt).

#include "concepts/axis_io_concept.hpp"
#include "concepts/axis_io_cache_engine_permutation_concept.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::io_dispatch {

/// ABI-taugliches IoDispatch-Snapshot (NUR uint64 → standard_layout + trivially_copyable, Cross-ABI-POD-mappbar).
struct IoDispatchSnapshot {
    std::uint64_t dispatch_rounds   = 0; ///< Anzahl observe_dispatch-Aufrufe (Dispatch-Runden)
    std::uint64_t bytes_dispatched  = 0; ///< kumulierte "dispatchte" Record-Bytes (n * record_size) ueber alle Runden
    std::uint64_t alignment_adjusts = 0; ///< strategie-abhaengige Alignment-Anpassungen (0 fuer InMemoryOnly)
    std::uint64_t total_dispatch_count = 0; ///< kumulierte dispatchte Records (Σ n)
    std::uint64_t last_checksum        = 0; ///< letztes io_dispatch_scan-Ergebnis (Korrektheits-/Anti-Wegopt-Anker)

    [[nodiscard]] bool operator==(IoDispatchSnapshot const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<IoDispatchSnapshot>);
static_assert(std::is_trivially_copyable_v<IoDispatchSnapshot>);

/// ObservableAxis-Huelle: io_dispatch-Strategie + Per-Achsen-Mess-Mechanik (gegated).
/// KEIN Aggregat (private member + Methoden) → direkt als Anatomie-Member `ObservableIoDispatch<S> m;` haltbar
/// (umgeht das `{}`-Aggregat-Init-Problem nackter Strategie-Wrapper, test_d_v42_probe2).
template <class Strategy>
    requires concepts::IoStrategy<Strategy>
class ObservableIoDispatch {
public:
    using strategy_type = Strategy;
    // topic_tag durchgereicht → die Huelle erfuellt IoComponent/IoStrategy und ist als io_dispatch-Slot
    // einsetzbar (composition_registry / axis_path_serialization rufen C::io_dispatch::name()).
    using topic_tag = typename Strategy::topic_tag;

    // Transparenter Decorator: Strategie-Inspektion durchgereicht.
    [[nodiscard]] static constexpr bool is_in_memory_only() noexcept { return Strategy::is_in_memory_only(); }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return Strategy::name(); }
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

    /// STATIC Pass-Through (Drop-in-Kompatibilität): die Strategie-Methode wird unveraendert durchgereicht, damit
    /// die Huelle als io_dispatch-Slot die bestehenden seg19-Aufrufer NICHT bricht (abi_adapter.hpp T14
    /// `IoDispatch::io_dispatch_scan`). Diese Variante trackt NICHT (static, kein Instanz-State).
    [[nodiscard]] static std::uint64_t io_dispatch_scan(unsigned char const* buf, std::size_t n,
                                                        std::size_t record_size) noexcept {
        return Strategy::io_dispatch_scan(buf, n, record_size);
    }

    /// Mess-Kopplung (der eigentliche „Driver", Instanz): delegiert an die Strategie + trackt. Der Observer-Treiber
    /// (abi_adapter::fill_observer_v3 / tier_insert-Kopplung) ruft dies → die Dispatch-Aktivitaet wird observable.
    /// Getrennt von der static-Variante, weil die bestehenden seg19-Aufrufer static bleiben muessen.
    [[nodiscard]] std::uint64_t observe_dispatch(unsigned char const* buf, std::size_t n,
                                                 std::size_t record_size) noexcept {
        std::uint64_t const checksum = Strategy::io_dispatch_scan(buf, n, record_size);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.dispatch_rounds;
        stats_.total_dispatch_count += static_cast<std::uint64_t>(n);
        stats_.bytes_dispatched += static_cast<std::uint64_t>(n) * static_cast<std::uint64_t>(record_size);
        // Alignment-Anpassung: nur Persistence-Strategien mit Block-Granularitaet (Buffered/Direct/Mmap) richten
        // je Record an die Dispatch-Grenze aus; InMemoryOnly (RAM-only Baseline) tut das nicht → ehrlich 0.
        if (!Strategy::is_in_memory_only()) stats_.alignment_adjusts += static_cast<std::uint64_t>(n);
        stats_.last_checksum = checksum;
#endif
        return checksum;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = IoDispatchSnapshot;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void                     reset() noexcept { stats_ = {}; }

private:
    snapshot_t stats_{};
#endif
};

} // namespace comdare::cache_engine::io_dispatch
