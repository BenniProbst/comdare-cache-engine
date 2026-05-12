#pragma once
// IAllocationStrategy - Concept-Wurzel der Allokator-Familien (REV 7 §1.4)
//
// Jeder Comdare-Allokator-Baustein implementiert dieses Concept und
// faellt damit unter eine der 7 Achsen AA1-AA7 (siehe Allokator_Matrix.txt).

#include <concepts>
#include <cstddef>
#include <new>

namespace comdare::cache_engine::allocator {

// AA-Achsen-Tags zur Compile-time-Permutation (REV 7 §2.5)
namespace axes {
    struct freelist_topology_tag      {};   // AA1
    struct size_class_schema_tag       {};   // AA2
    struct thread_locality_tag         {};   // AA3
    struct synchronization_tag         {};   // AA4
    struct allocation_policy_tag       {};   // AA5
    struct reclamation_tag             {};   // AA6
    struct fragmentation_strategy_tag  {};   // AA7
}  // namespace axes

// Concept-Wurzel: jede Allokator-Familie muss diese minimalen Operationen bieten
template <typename A>
concept IAllocationStrategy = requires(A a, std::size_t n, void* p, std::size_t align) {
    { a.raw_allocate(n, align)   } -> std::same_as<void*>;
    { a.raw_deallocate(p, n, align) } -> std::same_as<void>;
    { a.statistics()              } noexcept;
    typename A::axis_tag;        // Pflicht: Welche AA-Achse charakterisiert sie?
    typename A::family_id;       // Pflicht: A01...A23 als compile-time integer constant
};

// Statistics-Pflicht-Felder (alle Allokatoren)
struct AllocationStatistics {
    std::size_t total_bytes_allocated = 0;
    std::size_t total_bytes_in_use     = 0;
    std::size_t allocation_count       = 0;
    std::size_t deallocation_count     = 0;
    std::size_t failure_count          = 0;          // alloc returned nullptr / threw
    double      external_fragmentation = 0.0;        // [0..1]
    double      internal_fragmentation = 0.0;        // [0..1]
};

}  // namespace comdare::cache_engine::allocator
