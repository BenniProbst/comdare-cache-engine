#pragma once
// K9-Fix (User-Direktive §4.4 2026-06-04 + User-Entscheid 2026-06-18) — REALER Software-Prefetch auf die
// TATSAECHLICHEN Speicheradressen des Such-Descents (axis_07, T7). Ersetzt die K9-Pseudo-Adress-Logik
// (Schluessel-Werte-als-Adresse, le_limitierung Z.9): die prefetch-Achse setzt jetzt `_mm_prefetch`
// (bzw. __builtin_prefetch) auf REALE Slot-Adressen ab, die der Store (LayoutAwareChunkedStore) im
// allozierten Chunk-Backing haelt — NICHT auf erfundene/Schluessel-abgeleitete Adressen.
//
// @topic prefetch @achse 07 @saeule 2 (Per-Achsen-Observer) @k9-fix real-address
//
// **Lehrbuch-Pattern (Strategy, web-verifiziert):** Software-Controlled Data Prefetching nach
//   - Mowry, T.C. "Tolerating Latency Through Software-Controlled Data Prefetching." Stanford
//     Tech-Report CSL-TR-94-628, 1994 (Software-Prefetch-Distanz: N Cache-Lines im Voraus).
//   - Chen, S., Gibbons, P.B., Mowry, T.C. "Improving Index Performance through Prefetching."
//     ACM SIGMOD 2001 (pfad-orientiertes Prefetching entlang des Such-Descents).
// Jede der 4 prefetch-Strategien ist ein DISTINKTES, real differenzierendes Descent-Prefetch-Verhalten
// (Strategy je Variante, zero-cost via `if constexpr (Strategy::family_id::value)`-Selektion):
//   • family_id 0  NonePrefetch          → KEIN _mm_prefetch (0-Overhead-Baseline; 0 reale Prefetches).
//   • family_id 2  HardwarePrefetch       → _mm_prefetch(_MM_HINT_T0) auf den AKTUELLEN Slot (CPU-managed
//                                           distance; ein Prefetch je beruehrtem Slot, Wormhole-Muster).
//   • family_id 1  DistanceEstimatorPrefetch → _mm_prefetch auf den Slot `i + distance` (distance = die
//                                           density-/latenz-basierte Heuristik-Distanz, ART/Leis 2013) →
//                                           prefetch laeuft VORAUS, andere Zieladressen als HW.
//   • family_id 3  PathOrientedPrefetch    → _mm_prefetch entlang des Descent-PFADS (prefetcht die naechsten
//                                           kBundle Slots des Pfads, Bundle-Granularitaet; PRT-ART/Chen 2001) →
//                                           mehrere reale Adressen je Trigger, distinkt zu HW + Distance.
//
// **KEIN OOB / HW-safe ABER aus realer Traversierung:** Es werden NUR Adressen `store.slot_address(j)` mit
// `j < store.slot_count()` geprefetcht (die Funktion liefert nullptr fuer OOB-Indizes → uebersprungen).
// `_mm_prefetch` ist selbst auf garbage HW-safe, aber hier stammt JEDE Zieladresse aus dem real allozierten
// Store-Backing → kein erfundener Pointer. Distance-/Path-Strategien clampen ihre Voraus-Indizes hart auf
// `slot_count()-1` (mind. der letzte reale Slot wird geprefetcht, nie jenseits des Backings).
//
// @related [[reference_axis_gold_standard_checklist]] [[feedback_lehrbuch_design_patterns_only_zero_cost_metaprog]]

#include "concepts/axis_07_prefetch_concept.hpp"
#include "axis_07_prefetch_distance_estimator_impl.hpp" // DistanceEstimatorImpl::estimate (Distance-Heuristik)
#include <cstddef>
#include <cstdint>
#include <type_traits>

#if defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__)
#include <xmmintrin.h> // _mm_prefetch + _MM_HINT_T0 (MSVC/GCC/Clang x86)
#define COMDARE_PREFETCH_HAVE_MM 1
#else
#define COMDARE_PREFETCH_HAVE_MM 0
#endif

namespace comdare::cache_engine::prefetch_axis {

/// Ergebnis EINES Descent-Prefetch-Triggers (real, kein erfundener Wert): wie viele reale `_mm_prefetch`-
/// Instruktionen abgesetzt wurden + die letzte real geprefetchte Adresse + die genutzte Voraus-Distanz.
struct DescentPrefetchResult {
    std::uint64_t        prefetches_issued = 0;       ///< Anzahl real abgesetzter _mm_prefetch (0 fuer None)
    unsigned char const* last_address      = nullptr; ///< letzte REAL geprefetchte Slot-Adresse (im Store-Backing)
    std::uint64_t last_distance = 0; ///< genutzte Voraus-Distanz in Slots (0=HW/aktuell, N=distance, Bundle=path)
};

namespace detail_real {

/// family_id einer prefetch-Strategie (constexpr int): 0=None, 1=DistanceEstimator, 2=Hardware, 3=PathOriented.
template <class S>
inline constexpr int prefetch_family_v = [] {
    if constexpr (requires { S::family_id::value; })
        return static_cast<int>(S::family_id::value);
    else
        return 0;
}();

/// Real-issuing _mm_prefetch (T0) — HW-safe; die Adresse stammt IMMER aus store.slot_address (real alloziert).
inline void issue_prefetch_t0(unsigned char const* addr) noexcept {
#if COMDARE_PREFETCH_HAVE_MM
    _mm_prefetch(reinterpret_cast<char const*>(addr), _MM_HINT_T0);
#elif defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(addr, 0 /*read*/, 3 /*high locality ~T0*/);
#else
    // Portabler Fallback (kein HW-Prefetch verfuegbar): ein volatile-Read als minimale Touch-Annaeherung.
    volatile unsigned char sink = *addr;
    (void)sink;
#endif
}

/// Default-Bundle (Cache-Lines/Slots voraus) fuer PathOriented (Chen 2001 Bundle-Granularitaet).
inline constexpr std::size_t kPathBundle = 4;

} // namespace detail_real

/// PrefetchDescentPolicy<Strategy> — die zero-cost Strategy-Dispatch-Policy: gegeben der reale Store + der
/// gerade beruehrte Slot-Index `i`, setzt sie die strategie-distinkten realen `_mm_prefetch` ab. Templatisiert
/// auf den Store-Typ (Duck-Typing: braucht slot_count() + slot_address(i)); zero-cost (alles inline + constexpr).
template <class Strategy>
struct PrefetchDescentPolicy {
    static constexpr int family = detail_real::prefetch_family_v<Strategy>;

    /// Treibt EINEN Descent-Prefetch-Trigger auf REALE Store-Adressen. `i` = der Slot, den der Descent gerade
    /// beruehrt (aus der echten Traversierung). Liefert das reale Ergebnis (Zaehler + letzte Adresse + Distanz).
    template <class Store>
    static DescentPrefetchResult drive(Store const& store, std::size_t i,
                                       std::uint32_t runtime_distance = 0) noexcept {
        DescentPrefetchResult r{};
        std::size_t const     n = store.slot_count();
        if (n == 0) return r;  // leerer Store → nichts real zu prefetchen
        if (i >= n) i = n - 1; // i in den realen Bereich klemmen (kein OOB-Index)
        (void)runtime_distance;

        if constexpr (family == 0) {
            // NonePrefetch: KEIN Prefetch (0-Overhead-Baseline) → 0 reale Prefetches, keine Adresse.
            (void)store;
            (void)i;
            return r;
        } else if constexpr (family == 2) {
            // HardwarePrefetch (Wormhole): T0-Hint auf den AKTUELLEN Slot — CPU schaetzt Distance autonom.
            unsigned char const* a = store.slot_address(i);
            if (a != nullptr) {
                detail_real::issue_prefetch_t0(a);
                r.prefetches_issued = 1;
                r.last_address      = a;
                r.last_distance     = 0;
            }
            return r;
        } else if constexpr (family == 1) {
            // DistanceEstimatorPrefetch (ART): density-/latenz-basierte Distanz voraus, optional durch den
            // RC-Laufzeitknopf ueberschrieben. Ohne Override bleibt die bisherige estimate()-Distanz exakt gleich.
            double const density = (n >= 1) ? (100.0 * static_cast<double>(n) / static_cast<double>(n + 1)) : 0.0;
            std::uint32_t const dist =
                (runtime_distance != 0)
                    ? runtime_distance
                    : static_cast<std::uint32_t>(
                          impl::DistanceEstimatorImpl::estimate(density, /*tier_latency_cycles=*/30.0));
            std::size_t const ahead  = static_cast<std::size_t>(dist);
            std::size_t       target = (ahead > ((n - 1) - i)) ? (n - 1) : (i + ahead);
            unsigned char const* a = store.slot_address(target);
            if (a != nullptr) {
                detail_real::issue_prefetch_t0(a);
                r.prefetches_issued = 1;
                r.last_address      = a;
                r.last_distance     = dist;
            }
            return r;
        } else {
            // PathOrientedPrefetch (PRT-ART/Chen 2001): Bundle entlang des Descent-Pfads — prefetcht die naechsten
            // kBundle Slots ab i+1 (mehrere reale Adressen je Trigger; Bundle-Granularitaet). Alle Ziele real geklemmt.
            std::uint64_t        issued = 0;
            unsigned char const* last   = nullptr;
            for (std::size_t b = 1; b <= detail_real::kPathBundle; ++b) {
                std::size_t target = i + b;
                if (target >= n) break; // nicht ueber das reale Backing hinaus
                unsigned char const* a = store.slot_address(target);
                if (a != nullptr) {
                    detail_real::issue_prefetch_t0(a);
                    ++issued;
                    last = a;
                }
            }
            r.prefetches_issued = issued;
            r.last_address      = last;
            r.last_distance     = detail_real::kPathBundle;
            return r;
        }
    }
};

} // namespace comdare::cache_engine::prefetch_axis
