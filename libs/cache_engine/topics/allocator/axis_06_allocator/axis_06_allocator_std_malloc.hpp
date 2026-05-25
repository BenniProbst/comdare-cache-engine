#pragma once
// V41.F.6.1.A Beispiel-Klasse StdMalloc fuer Allocator-Achse 6 (2026-05-25)
//
// @topic allocator
// @achse 6
// @family A22 (ptmalloc2 / glibc malloc)
// @subaxis AA2 size_class_schema
// @stand V41.F.6.1.A Pilot
//
// **Concept-Beweis:** erste konkrete Klasse die das AllocatorStrategy-Concept
// vollstaendig erfuellt und von der CRTP-Basis erbt. Demonstriert dass das
// neue Topic+Achsen+Concept+CRTP-Pattern funktioniert.
//
// Implementation: std::aligned_alloc + std::free (libc malloc-Pfad).
// Statistik: thread-local counter (kein atomic, da raw_allocate per-Thread aufgerufen wird;
// fuer echte concurrent-Allokation muesste das auf std::atomic umgestellt werden).
//
// **WICHTIG:** Dies ist ein Concept-Validations-Beispiel, NICHT die produktive
// std::malloc-Wrapper-Familie. Die echte Migration der 23 bestehenden Familien
// (a01-a23 unter libs/cache_engine/include/cache_engine/allocators/families/)
// in das neue Topic+Achsen-Pattern erfolgt in F.6.2.

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "../concepts/topic_allocator_concept.hpp"

#include <cstdlib>     // std::aligned_alloc, std::free
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::allocator::axis_06_allocator {

/**
 * @brief StdMalloc - libc malloc-Wrapper als Concept-Beweis-Klasse
 * @topic allocator
 * @achse 6
 * @subaxis AA2 size_class_schema_tag (libc malloc nutzt Bin-basierte Size-Classes)
 * @family A22 (ptmalloc2 / glibc)
 * @reuse_status (b) Eigenstaendige Wrapper-Klasse
 *
 * Erfuellt das AllocatorStrategy-Concept vollstaendig:
 *
 * **(1) Runtime-API:**
 *   - raw_allocate(bytes, align)        -> std::aligned_alloc
 *   - raw_deallocate(p, bytes, align)   -> std::free
 *   - statistics()                      -> internal counter snapshot
 *   - reset()                           -> counter clear
 *
 * **(2) Compile-Time-Eigenschaften:**
 *   - topic_tag  = AllocatorTopicTag
 *   - axis_tag   = size_class_schema_tag (AA2)
 *   - family_id  = std::integral_constant<int, 22>
 *   - is_thread_safe()    = true   (glibc malloc ist thread-safe)
 *   - supports_pmr()      = true   (kann via CacheEnginePmrResource benutzt werden)
 *   - max_alignment()     = alignof(std::max_align_t)
 *
 * **(3) Identifikation:**
 *   - name()         = "std_malloc"
 *   - family_name()  = "Standard libc malloc (ptmalloc2 / glibc)"
 *
 * Erbt von AllocatorStrategyBase<StdMalloc> (CRTP) — damit verfuegbar:
 *   - allocate(bytes, align) / deallocate(p, bytes, align)
 *   - statistics() / reset()
 *   - as_std_allocator<T>() / as_pmr_resource()  (TODO F.6.1.B)
 */
class StdMalloc : public AllocatorStrategyBase<StdMalloc> {
public:
    // ───────────────────────────────────────────────────────────────────────
    // (2) Compile-Time-Eigenschaften (Pflicht-API-Gruppe 2)
    // ───────────────────────────────────────────────────────────────────────
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = subaxes::size_class_schema_tag;
    using family_id  = std::integral_constant<int, 22>;   // A22 ptmalloc2/glibc

    [[nodiscard]] static constexpr bool        is_thread_safe()   noexcept { return true; }
    [[nodiscard]] static constexpr bool        supports_pmr()     noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t max_alignment()    noexcept { return alignof(std::max_align_t); }

    // ───────────────────────────────────────────────────────────────────────
    // (3) Identifikation (Pflicht-API-Gruppe 3)
    // ───────────────────────────────────────────────────────────────────────
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "std_malloc"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "Standard libc malloc (ptmalloc2 / glibc)"; }

    // ───────────────────────────────────────────────────────────────────────
    // (1) Runtime-Allocation-API (Pflicht-API-Gruppe 1)
    // ───────────────────────────────────────────────────────────────────────

    [[nodiscard]] void* raw_allocate(std::size_t bytes, std::size_t alignment) {
        // std::aligned_alloc verlangt dass bytes ein Multiple von alignment ist
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        void* p = std::aligned_alloc(alignment, aligned_bytes);
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += aligned_bytes;
            stats_.total_bytes_in_use    += aligned_bytes;
        } else {
            ++stats_.failure_count;
        }
        return p;
    }

    void raw_deallocate(void* p, std::size_t bytes, std::size_t alignment) {
        if (p == nullptr) return;
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        std::free(p);
        ++stats_.deallocation_count;
        if (aligned_bytes <= stats_.total_bytes_in_use) {
            stats_.total_bytes_in_use -= aligned_bytes;
        } else {
            stats_.total_bytes_in_use = 0;
        }
    }

    [[nodiscard]] concepts::AllocationStatistics statistics() const noexcept {
        return stats_;
    }

    void reset() noexcept {
        stats_ = {};
    }

private:
    concepts::AllocationStatistics stats_{};
};

}  // namespace comdare::cache_engine::allocator::axis_06_allocator

// ───────────────────────────────────────────────────────────────────────────
// Compile-Time-Beweis: StdMalloc erfuellt das AllocatorStrategy-Concept
// ───────────────────────────────────────────────────────────────────────────
namespace comdare::cache_engine::allocator::axis_06_allocator {
    static_assert(concepts::AllocatorStrategy<StdMalloc>,
        "V41.F.6.1.A Beweis: StdMalloc muss das AllocatorStrategy-Concept erfuellen "
        "(Pflicht-API-Gruppen 1+2+3). Falls dieser static_assert fehlschlaegt: "
        "Pruefe ob alle Methoden + typename axis_tag/family_id + static constexpr Eigenschaften "
        "korrekt definiert sind.");
}  // namespace
