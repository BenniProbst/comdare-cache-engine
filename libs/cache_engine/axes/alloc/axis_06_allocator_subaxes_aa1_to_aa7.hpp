#pragma once
// V41.F.6.1.A Sub-Achsen-Tags AA1-AA7 fuer Achse 6 Allocator (2026-05-25)
//
// @topic allocator
// @achse 6
// @stand V41.F.6.1.A Pilot
//
// Pflicht-Klassifikation jeder Allokator-Familie nach UML REV7 §2.5.
// Die 7 Sub-Achsen AA1-AA7 stehen ORTHOGONAL zueinander — eine Familie kann
// ihre primaere Charakteristik genau einem Tag zuordnen (typename axis_tag).
//
// Permutations-Konsequenz: der CacheEngineBuilder kann Familien nach
// Sub-Achsen gruppieren, um cross-orthogonale Permutationen zu erzeugen
// (z.B. Sub-Achse AA1 freelist x AA4 sync ueber alle Familien).
//
// Verwendet in:
//   topics/allocator/axis_06_allocator/concepts/axis_06_allocator_concept.hpp
//     (AllocatorStrategy verlangt `typename axis_tag`)

namespace comdare::cache_engine::alloc::subaxes {

/**
 * @brief AA1 Free-List-Topologie
 * @subaxis AA1
 *
 * Wie sind freie Blocks organisiert?
 * Beispiele: Per-Page-Sharding (mimalloc A04), Per-Thread-Caching (snmalloc A07),
 *            Buddy-Tree (A19), single-Global-Free-List (dlmalloc A20).
 */
struct freelist_topology_tag {};

/**
 * @brief AA2 Size-Class-Schema
 * @subaxis AA2
 *
 * Wie werden Block-Groessen klassifiziert?
 * Beispiele: Slab-per-Objektgroesse (A02), dlmalloc bins (A20), jemalloc size-classes (A05),
 *            scalloc spans (A08), Buddy-power-of-2 (A19).
 */
struct size_class_schema_tag {};

/**
 * @brief AA3 Thread-Locality
 * @subaxis AA3
 *
 * Wie lokal pro Thread/Core ist die Allokation?
 * Beispiele: tcmalloc thread-local-caches (A06), rpmalloc thread-caching (A10),
 *            snmalloc message-passing (A07), Hoard per-heap-locks (A01).
 */
struct thread_locality_tag {};

/**
 * @brief AA4 Synchronization
 * @subaxis AA4
 *
 * Welcher Concurrency-Mechanismus?
 * Beispiele: lock-free CAS (A03 Michael), LRMalloc (A11), mimalloc-CAS-on-non-local-free (A04),
 *            per-heap-locks (A01 Hoard), wait-free (A17 Crystalline).
 */
struct synchronization_tag {};

/**
 * @brief AA5 Allocation-Policy
 * @subaxis AA5
 *
 * Wie werden Allokationsanfragen gerouted?
 * Beispiele: NUMA-origin-aware (A09 NUMAlloc), cache-set-aware (A12 CAMA),
 *            NUCA-aware (A14 TCMalloc-warehouse), PIM-aware (A16 PIM-malloc).
 */
struct allocation_policy_tag {};

/**
 * @brief AA6 Reclamation
 * @subaxis AA6
 *
 * Wie werden geloeschte Blocks zurueckgegeben?
 * Beispiele: magazine cache (A02), page-heap-release (A06), lock-free-reclamation (A17),
 *            deferred-free (A04 mimalloc), RCU/epoch (V42.P1.2 comdare-rcu).
 */
struct reclamation_tag {};

/**
 * @brief AA7 Fragmentation-Strategy
 * @subaxis AA7
 *
 * Wie wird Fragmentierung vermieden?
 * Beispiele: dlmalloc coalescing (A20), object-coloring (A02), buddy-splitting (A19),
 *            page-local-sharding (A04 mimalloc), span-pooling (A08 scalloc).
 */
struct fragmentation_strategy_tag {};

}  // namespace comdare::cache_engine::alloc::subaxes
