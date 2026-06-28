#pragma once
// V41.F.6.1.A Mess-Concept mit Observer-Pattern (Topic-uebergreifend, 2026-05-25 revidiert)
//
// @stand V41.F.6.1.A
//
// **Architektur-Ebene (User-Direktive 2026-05-25):**
//   src/measurement/ = allgemeines Mess-Concept (Topic-uebergreifend)
//
// User-Direktive:
//   "Die Statistik sollte jeweils Observer bereitstellen koennen, um extern ganz
//    einfach verschiedene Achsen auswerten zu koennen. Dabei verwenden die Observer
//    Instanziierungen das Template Pattern der jeweiligen Achse, weil schliesslich
//    jede Achse denselben Typ an statistischer Auswertung verwendet."
//
// **CMake-Flag COMDARE_CE_ENABLE_STATISTICS:**
//   ON  (Default): MeasurableComponent ist Pflicht — alle Topic+Achsen-Wrapper
//                  bieten record()/snapshot()/reset()/observer().
//   OFF: MeasurableComponent ist trivial erfuellt — Topic+Achsen-Wrapper #ifdef'n
//        Mess-API komplett raus (kein Overhead).
//
// **Verwendung extern:**
//   for (auto* topic : topics) {
//       auto obs = topic->observer();    // Template ueber snapshot_t
//       obs.on_event([](auto const& snap) { /* auswerten */ });
//   }

#include <concepts>
#include <functional>
#include <type_traits>

namespace comdare::cache_engine::measurement {

#ifdef COMDARE_CE_ENABLE_STATISTICS

// ───────────────────────────────────────────────────────────────────────────
// (1) MeasurableObserver - Template Pattern fuer Achs-spezifische Auswertung
// ───────────────────────────────────────────────────────────────────────────

/**
 * @brief MeasurableObserver<Snapshot> - Template-parametrisierter Single-Slot-Notify-Hook (KEIN GoF-one-to-many)
 *
 * Jede Achse instanziiert dieses Template mit ihrer konkreten Snapshot-Struktur
 * (z.B. AllocationStatistics fuer allocator-Topic, BufferStats fuer queuing, ...).
 *
 * MUSTER-EHRLICHKEIT (K10-PMAJOR-08, 2026-06-18, code-verifiziert): Dies ist ein SINGLE-SLOT-Notify-Hook
 * (NotifyPolicy-Sink) — EIN einziger callback_ (s.u.), den `on_event` ERSETZT (kein subscriber-Container,
 * kein Multicast). Es ist daher NICHT das klassische GoF-Observer-Muster (one-to-many subject↔observers).
 * Frühere Behauptung „der Builder kann MEHRERE Observer pro Achse registrieren" war falsch und wurde entfernt:
 * ein zweiter `on_event`-Aufruf überschreibt den ersten. Wer Fan-out braucht (Welch-Test + LaTeX-Report +
 * Live-Telemetry), setzt EINEN Callback, der intern fan-out't — oder migriert bewusst auf einen Subscriber-Vektor.
 */
// AUDIT #212 / K5b (P5/P8, 2026-06-28): Der Push-Sink (std::function callback_ + on_event/notify/has_callback)
// ist jetzt OPT-IN ueber `COMDARE_CE_ENABLE_OBSERVER_PUSH`. Default AUS => zero-cost NullNotify: KEIN
// std::function-Member (kein ~32-B-Slot je Achse) und KEIN per-Op-Branch im Mess-/Produktiv-Hot-Pfad
// (Apparat-Reinheit Meta #6). Ueber die extern-C-Modulgrenze wird NIE ein Subscriber gesetzt -- die Auswertung
// laeuft rein PULL (snapshot()/statistics() + observer()/Concept). NUR die Observer-Mechanik-Unit-Tests
// (test_v41_topic_allocator_axis_06/_queuing/_traversal) setzen das Flag, um den Push zu pruefen. Das
// `callback_ (s.u.)` im Kommentar oben existiert daher nur im Opt-in-Push-Build.
template <typename Snapshot>
class MeasurableObserver {
public:
    using snapshot_t = Snapshot;
    using callback_t = std::function<void(snapshot_t const&)>;

#ifdef COMDARE_CE_ENABLE_OBSERVER_PUSH
    void on_event(callback_t cb) { callback_ = std::move(cb); }

    void notify(snapshot_t const& snap) const {
        if (callback_) callback_(snap);
    }

    [[nodiscard]] bool has_callback() const noexcept {
        return static_cast<bool>(callback_);
    }

private:
    callback_t callback_{};
#else
public:   // explizit: der private:-Block oben liegt im #ifdef-Zweig und entfaellt im Default-Build => notify ist public.
    // DEFAULT: NullNotify -- notify() ist ein leerer NullObject (der Compiler elidiert ihn vollstaendig); KEIN
    // std::function-Member, KEIN per-Op-Branch. Die Achsen-Call-Sites `observer_.notify(stats_)` bleiben
    // UNVERAENDERT und kompilieren hier zu nichts. on_event/has_callback existieren nur im Push-Build (s.o.).
    void notify(snapshot_t const&) const noexcept {}
#endif
};

namespace concepts {

// ───────────────────────────────────────────────────────────────────────────
// (2) MeasurableComponent - Pflicht-Concept fuer alle messbaren Topic-Achsen
// ───────────────────────────────────────────────────────────────────────────

/**
 * @brief MeasurableComponent - Topic-uebergreifendes Mess-Concept
 *
 * Pflicht-API (WENN COMDARE_CE_ENABLE_STATISTICS=ON):
 *   - typename snapshot_t          Achs-spezifische Statistics-Struct
 *                                   (z.B. AllocationStatistics fuer allocator-Topic)
 *   - typename observer_t          == MeasurableObserver<snapshot_t> (Pflicht-Alias)
 *   - snapshot() const noexcept    liefert aktuellen Snapshot
 *   - reset() noexcept             Statistik zwischen Mess-Permutationen reset
 *   - observer() const noexcept    liefert observer_t const& (no-copy, kein Allokations-Overhead)
 *
 * Beispiel-Klasse die das Concept erfuellt (StdMalloc in axis_06_allocator):
 *   struct StdMalloc {
 *       using snapshot_t = AllocationStatistics;
 *       using observer_t = MeasurableObserver<snapshot_t>;
 *       snapshot_t snapshot() const noexcept { return stats_; }
 *       void reset() noexcept { stats_ = {}; }
 *       observer_t const& observer() const noexcept { return observer_; }
 *       // ... allocate/deallocate, jeder Aufruf -> observer_.notify(stats_)
 *   };
 */
template <typename T>
concept MeasurableComponent = requires(T t, T const& tc) {
    typename T::snapshot_t;
    typename T::observer_t;
    { tc.snapshot() } noexcept -> std::same_as<typename T::snapshot_t>;
    { t.reset() }    noexcept;
    { tc.observer() } noexcept -> std::same_as<typename T::observer_t const&>;
} && std::same_as<typename T::observer_t, MeasurableObserver<typename T::snapshot_t>>;

}  // namespace concepts

#else  // COMDARE_CE_ENABLE_STATISTICS

namespace concepts {

/**
 * @brief MeasurableComponent (NULL-Variante bei STATISTICS=OFF)
 *
 * Trivial true — Production-Build entfernt Mess-Overhead komplett aus Binary.
 * Vendor-Wrapper #ifdef'n snapshot/reset/observer-Methoden weg.
 */
template <typename T>
concept MeasurableComponent = true;

}  // namespace concepts

#endif  // COMDARE_CE_ENABLE_STATISTICS

}  // namespace comdare::cache_engine::measurement
