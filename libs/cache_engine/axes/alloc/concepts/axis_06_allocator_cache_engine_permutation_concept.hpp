#pragma once
// V41.F.6.1.A cache-engine-spezifisches Pflicht-Concept (2026-05-25, revidiert nach User-Klarstellung)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A
//
// **Parallel zu AllocatorStrategy:** cache-engine-spezifische Pflicht-API die
// jede Allokator-Variante anbieten muss, damit sie als Permutations-Variante
// in der cache-engine-Pipeline registrierbar ist.
//
// Trennung zum Standard-Concept (AllocatorStrategy):
//   - AllocatorStrategy = strikt nach Standard (ISO C/C++ + PMR)
//   - CacheEnginePermutationStrategy = cache-engine-Permutationen-Identitaet
//
// Eine Klasse erfuellt typischerweise BEIDE Concepts.
//
// **CMake-Flag COMDARE_CE_ENABLE_STATISTICS (User-Direktive 2026-05-25):**
//   - ON (Default): jede Achse MUSS statistics() + reset() bieten — Pflicht-API.
//     Mess-Reihen + Welch-Test koennen dann auf konsistente Mess-Daten zugreifen.
//   - OFF: Concept verlangt KEINE statistics()/reset(). Topic+Achsen-Implementierungen
//     #ifdef'n diese Methoden komplett aus dem Binary. Production-Export OHNE
//     Mess-Overhead — z.B. "Beste gefundene Permutation als externer Suchalgorithmus".
//
// **Semantik reset() (User-Klarstellung):**
//   reset() ist die **Zuruecksetzbarkeit der Statistik** zwischen Mess-Permutationen.
//   NICHT zu verwechseln mit Pool-/Arena-Reset (das ist ResettableStrategy).

#include "axis_06_allocator_concept.hpp"
#include "../axis_06_allocator_subaxes_aa1_to_aa7.hpp"

#include <measurement/measurable_concept.hpp>   // V41.F.6.1 Stufe 3 LIVE: MeasurableObserver Template
#include <concepts/legacy_original_code_strategy_concept.hpp>   // V41.F.6.1.P2.C Habich-Compliance Pflicht-API

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::alloc::concepts {

/**
 * @brief AllocationStatistics - Pflicht-Felder fuer Permutations-Mess-Reihen
 * @topic allocator
 * @achse 6
 *
 * Wird vom CacheEngineBuilder fuer Welch's t-Test ausgelesen. Format ist
 * cache-engine-spezifisch (Vendor-statistics sind NICHT standardisierbar —
 * siehe Web-Recherche W1).
 *
 * Achsen-spezifische Statistics (allocation/dealloc Counts + Fragmentation)
 * — andere Achsen haben andere Statistics-Structs unter ihrem eigenen Topic.
 * Allgemeines Mess-Konzept (Topic-uebergreifend) liegt in src/measurement/.
 */
struct AllocationStatistics {
    std::uint64_t total_bytes_allocated  = 0;
    std::uint64_t total_bytes_in_use     = 0;
    std::uint64_t allocation_count       = 0;
    std::uint64_t deallocation_count     = 0;
    std::uint64_t failure_count          = 0;
    double        external_fragmentation = 0.0;
    double        internal_fragmentation = 0.0;
};

/**
 * @brief ProgressGuarantee — Klassifikation der Synchronisations-Stufen
 *
 * V41.F.6.1 Batch 7 Refactoring (User-Direktive 2026-05-26
 * [[vendor-sonderfaelle-als-pflicht-property]] Stufen-Pattern):
 * Ersetzt frueheres is_lock_free()+is_wait_free()-Bool-Paar durch ordinale
 * Klassifikation. Hoehere Stufen IMPLIZIEREN niedrigere Stufen (wait-free ist
 * staerker als lock-free, lock-free ist staerker als obstruction-free).
 *
 * CacheEngineBuilder kann mit Vergleich `>= LockFree` Subsets bilden, ohne
 * mehrere bools synchron halten zu muessen.
 */
enum class ProgressGuarantee : int {
    Blocking        = 0,  // Mainstream: Mutex/Lock, kein Progress-Guarantee
    ObstructionFree = 1,  // Herlihy/Luchangco/Moir 2003: progress nur ohne contention
    LockFree        = 2,  // klassisch: mindestens 1 Thread garantiert progress
    WaitFree        = 3,  // Herlihy 1991: ALLE Threads garantiert progress in endlicher Zeit
    // Reserve: BoundedWaitFree=4, AmortizedWaitFree=5, ...
};

/**
 * @brief ResourceOwnership — Besitz-Modell des zugrundeliegenden std::pmr::memory_resource
 *
 * V41.F.6.1.R7.4 ([[vendor-sonderfaelle-als-pflicht-property]]): macht die bislang nur per
 * Kommentar getrennte PMR-Familie (A22) typsicher unterscheidbar. Der CacheEngineBuilder kann
 * daraus Lebensdauer-/Konfigurations-Constraints ableiten (geborgte Resource = Aufrufer muss sie
 * ueberleben lassen; eigene Resource = Wrapper-Instanz haelt sie selbst).
 *
 * - None     : Allokator verwaltet KEIN std::pmr::memory_resource-Objekt (libc malloc + alle
 *              Vendor-Allokatoren rufen direkt malloc/vendor-API; Default).
 * - Owned    : Wrapper BESITZT seine memory_resource (Lebensdauer an die Instanz gebunden, z.B.
 *              PoolResourceAllocator haelt einen eigenen unsynchronized_pool_resource).
 * - Borrowed : Wrapper LEITET eine extern besessene memory_resource WEITER (roher Zeiger, Lebens-
 *              dauer liegt beim Aufrufer, z.B. PmrResourceAllocator ueber new_delete_resource()).
 *
 * Orthogonal zu supports_pmr(): supports_pmr() == "kann als pmr-Resource genutzt werden" (POOL/PMR
 * UND z.B. jemalloc == true), waehrend resource_ownership() das BESITZMODELL des Resource-Objekts
 * angibt (nur POOL=Owned, PMR=Borrowed, malloc-Familie=None).
 */
enum class ResourceOwnership : int {
    None     = 0,  // kein eigenes/geborgtes memory_resource-Objekt (Default, malloc-Familie)
    Owned    = 1,  // eigene memory_resource (Lebensdauer an die Wrapper-Instanz gebunden)
    Borrowed = 2,  // externe memory_resource durchgereicht (Lebensdauer beim Aufrufer)
};

/**
 * @brief CacheEnginePermutationStrategy - cache-engine-Pflicht-API
 * @topic allocator
 * @achse 6
 *
 * **Klassifikation (Pflicht IMMER):**
 *   - typename axis_tag       (AA1-AA7 Sub-Achsen-Tag)
 *   - typename family_id      (A01-A23 Compile-Time-ID)
 *
 * **Compile-Time-Eigenschaften (Pflicht IMMER):**
 *   - static constexpr bool is_thread_safe()
 *   - static constexpr bool supports_pmr()
 *   - static constexpr std::size_t max_alignment()
 *
 * **Identifikation (Pflicht IMMER):**
 *   - static constexpr std::string_view name()
 *   - static constexpr std::string_view family_name()
 *
 * **Mess-API (Pflicht nur wenn COMDARE_CE_ENABLE_STATISTICS=ON):**
 *   - statistics()       -> AllocationStatistics noexcept
 *   - reset()            -> void noexcept  (Statistik-Reset zwischen Mess-Permutationen)
 *
 * Beispiel-Klasse:
 *
 *   namespace comdare::cache_engine::alloc {
 *       struct StdMalloc : public AllocatorStrategyBase<StdMalloc> {
 *           using topic_tag  = concepts::AllocatorTopicTag;
 *           using axis_tag   = subaxes::size_class_schema_tag;     // AA2
 *           using family_id  = std::integral_constant<int, 22>;    // A22
 *
 *           static constexpr bool is_thread_safe()         { return true; }
 *           static constexpr bool supports_pmr()           { return true; }
 *           static constexpr std::size_t max_alignment()   { return alignof(std::max_align_t); }
 *           static constexpr std::string_view name()        { return "std_malloc"; }
 *           static constexpr std::string_view family_name() { return "Standard libc malloc"; }
 *
 *           void* allocate(std::size_t bytes, std::size_t align);
 *           void  deallocate(void* p, std::size_t bytes, std::size_t align) noexcept;
 *           bool  operator==(StdMalloc const&) const noexcept { return true; }
 *
 *       #ifdef COMDARE_CE_ENABLE_STATISTICS
 *           concepts::AllocationStatistics statistics() const noexcept;
 *           void reset() noexcept;   // Statistik zuruecksetzen
 *       #endif
 *       };
 *   }
 */
template <typename A>
concept CacheEnginePermutationStrategy =
    ::comdare::cache_engine::allocator::concepts::AllocatorComponent<A>
    && requires {
        typename A::axis_tag;
        typename A::family_id;
        { A::is_thread_safe() } -> std::convertible_to<bool>;
        { A::supports_pmr()   } -> std::convertible_to<bool>;
        { A::max_alignment()  } -> std::convertible_to<std::size_t>;
        { A::name()           } -> std::convertible_to<std::string_view>;
        { A::family_name()    } -> std::convertible_to<std::string_view>;
        // V41.F.6.1 Batch 4 Konsolidierung 2026-05-26 (User-Direktive
        // [[vendor-sonderfaelle-als-pflicht-property]]):
        // Sonderfaelle der Vendors als abfragbare static-constexpr-Properties.
        // Jeder Wrapper muss antworten — der Wert kann negativ sein (false), aber
        // die Methode MUSS existieren. CacheEngineBuilder kann pro Permutation
        // daraus Constraints + Pruefling-Filter ableiten.
        { A::has_native_aligned_alloc()    } -> std::convertible_to<bool>;
        { A::requires_explicit_init()      } -> std::convertible_to<bool>;
        { A::supports_numa_node_hint()     } -> std::convertible_to<bool>;
        { A::supports_thread_local_cache() } -> std::convertible_to<bool>;
        { A::requires_specialized_hardware() } -> std::convertible_to<bool>;
        // V41.F.6.1 Batch 7 Refactoring (User-Direktive 2026-05-26 Stufen-Pattern):
        // progress_guarantee() — Stufen-Klassifikation der Synchronisations-Garantie.
        // ERSETZT frueher: is_lock_free() + is_wait_free() (2 bools mit implizitem
        // Constraint wait-free => lock-free). Jetzt 1 ordinaler Enum-Wert.
        // CacheEngineBuilder: `level() >= LockFree` deckt lock-free + wait-free.
        { A::progress_guarantee() } -> std::convertible_to<ProgressGuarantee>;
        // V41.F.6.1.R7.4 ([[vendor-sonderfaelle-als-pflicht-property]]): Besitz-Modell des
        // memory_resource-Objekts (None/Owned/Borrowed). Grenzt POOL (Owned) gegen PMR (Borrowed)
        // gegen malloc-Familie (None) TYPSICHER ab. Default None liefert AllocatorStrategyBase —
        // nur POOL/PMR ueberschreiben (kein Bloat: nicht 25x hardcoded).
        { A::resource_ownership() } -> std::convertible_to<ResourceOwnership>;
    }
#ifdef COMDARE_CE_ENABLE_STATISTICS
    // V41.F.6.1 Stufe 3 LIVE (Doku §15.3 + §15.8 / User-Direktive [[statistics-observer-pflicht]]):
    // Bei STATISTICS=ON ist variadische Statistik-Observer-Auswertungsklasse Pflicht.
    // observer_t = MeasurableObserver<snapshot_t> ueber die achs-spezifische
    // Statistics-Struktur (z.B. AllocationStatistics fuer Allocator-Achse).
    && requires(A a, A const& ac) {
        { ac.statistics() } noexcept;
        { a.reset() }      noexcept;
    }
    && requires {
        typename A::snapshot_t;
        typename A::observer_t;
    }
    && std::same_as<typename A::observer_t,
                    ::comdare::cache_engine::measurement::MeasurableObserver<typename A::snapshot_t>>
    && requires(A const& ac) {
        { ac.observer() } noexcept -> std::same_as<typename A::observer_t const&>;
    }
#endif
    // V41.F.6.1.P2.C Habich-Compliance Pflicht: get_compiler + has_original_paper_code + is_original_module
    && ::comdare::cache_engine::concepts::LegacyOriginalCodePflicht<A>
    ;

}  // namespace comdare::cache_engine::alloc::concepts
