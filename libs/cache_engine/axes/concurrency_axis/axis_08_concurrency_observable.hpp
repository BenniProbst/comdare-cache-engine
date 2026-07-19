#pragma once
// Per-Achsen-Vervollständigung Phase B (2026-06-04) — ObservableConcurrency<Strategy>: ObservableAxis-Hülle um
// eine concurrency-Strategie (axis_08, T8). Analog axis_11_telemetry_observable.hpp (ObservableTelemetry): die
// nackte concurrency-Strategie (None/Blocking/LockFree/Olc/…) trägt KEIN statistics() → ObservableAxis<...>=0.
// Die Per-Achsen-Mess-Mechanik gehört daher in DIESE Hülle.
//
// @topic concurrency @achse 08 @saeule 2 (Per-Achsen-Observer) @phase B
//
// **Achsen-Semantik (treu, NICHT erfunden):** Die concurrency-Achse beschreibt das Synchronisations-Primitiv
// (None=no-op / Blocking=mutex / LockFree=CAS / Olc=optimistic-validate / …). Die Strategien exponieren ihre Op
// als STATISCHE acquire()/release() (thread_local-static-Primitiv → der Wrapper-Typ bleibt leer/trivial). Die
// Hülle TREIBT das echte statische Primitiv (Strategy::acquire/release) instanz-gebunden und ZÄHLT dabei:
//   - acquire_count / release_count: je 1 pro getriebener Op (echte, gepaarte Lock/CAS/Validate-Bahn).
//   - contention_count: CAS-Retry-Zähler. Die Strategien kapseln ihre CAS-Schleife in der statischen acquire()
//     und kehren erst bei Erfolg zurück → von außen ist im EINZEL-THREAD-Mess-Pfad (Pfad B) KEINE Contention
//     beobachtbar (das CAS gelingt im 1. Versuch, OLC-Validate ist stets gültig). contention_count bleibt
//     daher im single-thread-Pfad EHRLICH 0 — KEIN erfundener Wert. (Mehr-Thread-Contention erfordert einen
//     nebenläufigen Treiber; bewusst NICHT Teil des deterministischen DRAM-Bench-Pfads.)
//   - validation_failure_count: analog 0 im single-thread-Pfad (OLC-Re-Read stets == acquire-Snapshot).
//   - pattern_id: das STATISCHE concurrency_pattern()-Enum (None=0..HazardPtr=7) — strategie-distinkt, real.
//
// Gating exakt nach ObservableTelemetry-Präzedenz: snapshot_t/statistics()/reset() unter
// COMDARE_CE_ENABLE_STATISTICS. Bei OFF: acquire()/release() = reine Delegation ohne Zähler (0 Footprint),
// ObservableAxis<...> = false → die Observer-Befüllung fällt auf 0 zurück (Release-Pfad, korrekt).
//
// @related [[feedback_zwei_dimensionen_messmodell]] [[reference_axis_gold_standard_checklist]]

#include "concepts/axis_08_concurrency_concept.hpp"
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::concurrency_axis {

/// ABI-taugliches Concurrency-Snapshot (NUR uint64 → standard_layout + trivially_copyable; mappbar in den
/// generischen Cross-ABI-Observer-POD, axis_stats[8][...], observable_tier.hpp).
struct ConcurrencyStatistics {
    std::uint64_t acquire_count            = 0; ///< Anzahl getriebener acquire()-Ops (Lock/CAS/Validate-Eintritt)
    std::uint64_t release_count            = 0; ///< Anzahl getriebener release()-Ops (Austritt; gepaart)
    std::uint64_t contention_count         = 0; ///< CAS-Retries (single-thread-Pfad B: ehrlich 0, kein Konflikt)
    std::uint64_t validation_failure_count = 0; ///< fehlgeschlagene OLC-Validierungen (single-thread: ehrlich 0)
    std::uint64_t pattern_id = 0; ///< concurrency_pattern()-Enum (None=0..HazardPtr=7; strategie-distinkt)

    [[nodiscard]] bool operator==(ConcurrencyStatistics const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<ConcurrencyStatistics>);
static_assert(std::is_trivially_copyable_v<ConcurrencyStatistics>);

/// ObservableAxis-Hülle: concurrency-Strategie + Per-Achsen-Mess-Mechanik (gegated). KEIN Aggregat (private
/// member + Methoden) → direkt als Anatomie-/abi_adapter-Member `ObservableConcurrency<S> m;` haltbar.
template <class Strategy>
    requires concepts::ConcurrencyStrategy<Strategy>
class ObservableConcurrency {
public:
    using strategy_type = Strategy;

    // statische Forwarding-/Instrumentierungs-Hülle (KEIN GoF-Decorator: hält keine Komponenten-Instanz, kein Voll-Interface): die statische Strategie-Inspektion wird durchgereicht, damit die Hülle überall
    // als concurrency-Slot funktioniert (Composition-Inspektoren rufen C::concurrency::name()/pattern()).
    [[nodiscard]] static constexpr concepts::ConcurrencyPattern concurrency_pattern() noexcept {
        return Strategy::concurrency_pattern();
    }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return Strategy::name(); }
    static constexpr std::string_view               algo_version =
        Strategy::algo_version; // #50 Caching: algo_version-Weiterleitung (Organ-Provenienz, reflect_versions)
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

    // Observer-Strategy (M2, Dossier 18 A.3): set_runtime_thread_count ist die austauschbare
    // Wirkungs-Strategie der Achse; Min/Max-Semantik: Multicore-Durchsatz vs. Contention.
    void set_runtime_thread_count(std::uint64_t thread_count) noexcept {
        runtime_thread_count_ = thread_count > 64u ? 64u : static_cast<std::uint32_t>(thread_count);
    }
    [[nodiscard]] std::uint32_t runtime_thread_count() const noexcept {
        return runtime_thread_count_ == 0 ? 1u : runtime_thread_count_;
    }

    /// Mess-Kopplung (der eigentliche „Driver"): treibt das ECHTE statische Synchronisations-Primitiv der
    /// Strategie (acquire→Mini-Critical-Section-Eintritt) und zählt. Gepaart mit release() zu nutzen.
    void acquire() noexcept {
        bool contented = false;
        if constexpr (requires { Strategy::try_acquire(); }) {
            if (!Strategy::try_acquire()) {
                contented = true;
                Strategy::acquire();
            }
        } else if constexpr (requires { Strategy::acquire(); }) {
            Strategy::acquire();
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.acquire_count;
        if (contented) ++stats_.contention_count;
        stats_.pattern_id = static_cast<std::uint64_t>(static_cast<int>(Strategy::concurrency_pattern()));
#endif
    }

    /// Gegenstück zu acquire(): treibt Strategy::release() (Lock-Freigabe / OLC-Validierung) und zählt.
    void release() noexcept {
        if constexpr (requires { Strategy::release(); }) Strategy::release();
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.release_count;
#endif
    }

    /// Eine vollständige, korrekt gepaarte Mini-Critical-Section (acquire→release). Komfort-Driver für den
    /// abi_adapter (tier_insert/lookup): EIN Aufruf = EIN echtes Sync-Primitiv-Paar.
    void observe_critical_section() noexcept {
        acquire();
        release();
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = ConcurrencyStatistics;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    // reset() = Statistik-Reset (Memory-Regel). pattern_id wird beim nächsten acquire() neu gesetzt.
    void reset() noexcept { stats_ = {}; }
#endif

private:
    std::uint32_t runtime_thread_count_ = 0;

#ifdef COMDARE_CE_ENABLE_STATISTICS
    snapshot_t stats_{};
#endif
};

} // namespace comdare::cache_engine::concurrency_axis
