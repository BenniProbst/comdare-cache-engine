#pragma once
// Per-Achsen-Vervollständigung Phase B (2026-06-04) — ObservablePrefetch<Strategy>: ObservableAxis-Hülle um eine
// prefetch-Strategie (axis_07, T7). Analog axis_11_telemetry_observable.hpp (ObservableTelemetry): die nackte
// prefetch-Strategie (NonePrefetch/HardwarePrefetch/PathOrientedPrefetch) ist ein ObservableAxis-loser Marker
// (kein statistics() → ObservableAxis<...>=0). Die Per-Achsen-Mess-Mechanik gehört daher in DIESE Hülle.
//
// @topic prefetch @achse 07 @saeule 2 (Per-Achsen-Observer) @phase B
//
// **Achsen-Semantik (treu, NICHT erfunden):** Die prefetch-Achse beschreibt das vorausschauende Laden von
// Knoten/Adressen entlang des erwarteten Such-Pfads. Die ECHTE getriebene Op:
//   - PathOrientedPrefetch (PRT-ART, axis_07_prefetch_path_oriented_impl.hpp) ist instanz-STATEFUL und trägt
//     bereits reale Zähler (total_enqueued()/total_hot_path_hints()/queue_depth()/suggest_next()). Die Hülle
//     TREIBT diesen Tracker (enqueue + note_hot_path_bytes + suggest_next) und SPIEGELT seine echten Zähler in
//     PrefetchStatistics — KEINE erfundenen Werte, jeder Zähler folgt einer echten Tracker-Op.
//   - NonePrefetch / HardwarePrefetch sind static-only (is_active() + Hardware-Hint via _mm_prefetch im seg19-
//     Pfad, KEIN Software-Tracking-Organ). Sie tragen KEINEN enqueue()-Pfad → die Hülle treibt nichts → die
//     PrefetchStatistics bleiben 0. Das ist die EHRLICHE Baseline (None=0-Overhead, Hardware=CPU-managed,
//     KEIN Software-State): 0 ist hier korrekt, nicht „n/a" und nicht erfunden.
//
// Gating exakt nach ObservableTelemetry/ObservableComposedSearch-Präzedenz: snapshot_t/statistics()/reset()
// unter COMDARE_CE_ENABLE_STATISTICS. Bei OFF: observe_*() = no-op (0 Footprint), ObservableAxis<...> = false
// → observe_all()/fill_observer_v3 fällt auf EmptyAxisSnapshot/0 zurück (Release-Pfad, korrekt).
//
// @related [[feedback_zwei_dimensionen_messmodell]] [[reference_axis_gold_standard_checklist]]

#include "concepts/axis_07_prefetch_concept.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::prefetch_axis {

/// ABI-taugliches Prefetch-Snapshot (NUR uint64 → standard_layout + trivially_copyable; mappbar in den
/// generischen Cross-ABI-POD ComdareTierObserverSnapshotV3, axis_stats[7][...], observable_tier.hpp).
struct PrefetchStatistics {
    std::uint64_t trigger_count             = 0;  ///< Anzahl Prefetch-Trigger (observe_prefetch-Aufrufe, die treiben)
    std::uint64_t suggestions_made          = 0;  ///< Anzahl erzeugter Next-Adress-Empfehlungen (suggest_next-Aufrufe)
    std::uint64_t hot_path_hints            = 0;  ///< V11.1 Hot-Path-Hints aus rohen Schlüssel-Bytes (Tracker-real)
    std::uint64_t max_queue_depth           = 0;  ///< Höchststand der Pfad-Trajektorien-Tiefe (Tracker-real)
    std::uint64_t total_addresses_enqueued  = 0;  ///< gesamt eingereihte Pfad-Adressen (Tracker total_enqueued())

    [[nodiscard]] bool operator==(PrefetchStatistics const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<PrefetchStatistics>);
static_assert(std::is_trivially_copyable_v<PrefetchStatistics>);

namespace detail {
/// Leerer Tracker für nicht-stateful Strategien (None/Hardware): default-konstruierbar, keine Op → 0-Baseline.
/// (Die Strategie-Klasse SELBST kann NICHT als Member gehalten werden — ihr CRTP-Base hat einen protected
/// Constructor, sie ist eine rein statische API. Daher hält die Hülle das Tracker-ORGAN, nicht die Strategie.)
struct NoPrefetchTracker {};

/// Erkennt die instanz-stateful PathOriented-Tracker-API (PathOrientedPrefetch trägt `impl_type` = der echte
/// PathOrientedImpl-Tracker, axis_07_prefetch_path_oriented_impl.hpp). None/Hardware erfüllen dies NICHT →
/// NoPrefetchTracker → kein Treiben → 0-Baseline (ehrlich).
template <class S>
concept HasPrefetchTracker = requires { typename S::impl_type; };

/// tracker_type_t<S> = der echte Tracker-Typ (PathOriented) bzw. NoPrefetchTracker (None/Hardware).
template <class S> struct tracker_type { using type = NoPrefetchTracker; };
template <HasPrefetchTracker S> struct tracker_type<S> { using type = typename S::impl_type; };
template <class S> using tracker_type_t = typename tracker_type<S>::type;
}  // namespace detail

/// ObservableAxis-Hülle: prefetch-Strategie + Per-Achsen-Mess-Mechanik (gegated). KEIN Aggregat (private
/// member + Methoden) → direkt als Anatomie-/abi_adapter-Member `ObservablePrefetch<S> m;` haltbar.
template <class Strategy>
    requires concepts::PrefetchStrategy<Strategy>
class ObservablePrefetch {
public:
    using strategy_type = Strategy;

    // Transparenter Decorator: die statische Strategie-Inspektion wird durchgereicht, damit die Hülle überall
    // als prefetch-Slot funktioniert (Composition-Inspektoren rufen C::prefetch::name()/is_active()).
    [[nodiscard]] static constexpr bool             is_active()   noexcept { return Strategy::is_active(); }
    [[nodiscard]] static constexpr std::string_view name()        noexcept { return Strategy::name(); }
    [[nodiscard]] static constexpr std::string_view family_name()
        noexcept requires requires { Strategy::family_name(); } { return Strategy::family_name(); }
    [[nodiscard]] static constexpr std::string_view flag_suffix()
        noexcept requires requires { Strategy::flag_suffix(); } { return Strategy::flag_suffix(); }

    /// Mess-Kopplung (der eigentliche „Driver"): der Tier-insert/lookup reicht die berührte Adresse herein.
    /// Bei einer Tracking-Strategie (PathOriented) wird der echte Tracker getrieben (enqueue + suggest_next),
    /// und die realen Zähler werden gespiegelt. Bei None/Hardware (kein Tracker) ist es bewusst ein no-op →
    /// PrefetchStatistics bleiben 0 (ehrliche Baseline). raw_bytes != nullptr ⇒ zusätzlich Hot-Path-Hint.
    void observe_prefetch(std::uint64_t addr, std::byte const* raw_bytes = nullptr, std::size_t n = 0) noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if constexpr (detail::HasPrefetchTracker<Strategy>) {
            tracker_.enqueue(addr);
            ++stats_.trigger_count;
            if constexpr (requires { tracker_.note_hot_path_bytes(raw_bytes, n); }) {
                if (raw_bytes != nullptr && n > 0) tracker_.note_hot_path_bytes(raw_bytes, n);
            }
            if constexpr (requires { tracker_.suggest_next(); }) {
                (void)tracker_.suggest_next();
                ++stats_.suggestions_made;
            }
            sync_from_tracker_();
        } else {
            (void)addr; (void)raw_bytes; (void)n;   // None/Hardware: kein Software-Tracker → 0-Baseline (ehrlich)
        }
#else
        (void)addr; (void)raw_bytes; (void)n;
#endif
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = PrefetchStatistics;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    // reset() = Statistik-Reset (Memory-Regel) + Tracker-Reset (frischer Pfad-Zustand je Messung).
    void reset() noexcept {
        stats_ = {};
        if constexpr (requires { tracker_.reset(); }) tracker_.reset();
    }

private:
    /// Spiegelt die ECHTEN Tracker-Zähler in die statistics() (kein erfundener Wert — alle Tracker-real).
    void sync_from_tracker_() noexcept {
        if constexpr (requires { tracker_.total_enqueued(); }) {
            stats_.total_addresses_enqueued = tracker_.total_enqueued();
        }
        if constexpr (requires { tracker_.total_hot_path_hints(); }) {
            stats_.hot_path_hints = tracker_.total_hot_path_hints();
        }
        if constexpr (requires { tracker_.queue_depth(); }) {
            auto const d = static_cast<std::uint64_t>(tracker_.queue_depth());
            if (d > stats_.max_queue_depth) stats_.max_queue_depth = d;
        }
    }

    // Tracker-ORGAN (NICHT die Strategie — deren CRTP-Base hat einen protected ctor): PathOrientedImpl bei
    // PathOriented (echter, public-default-konstruierbarer Tracker), sonst NoPrefetchTracker (leer, no-op).
    detail::tracker_type_t<Strategy> tracker_{};
    snapshot_t                       stats_{};
#endif
};

}  // namespace comdare::cache_engine::prefetch_axis
