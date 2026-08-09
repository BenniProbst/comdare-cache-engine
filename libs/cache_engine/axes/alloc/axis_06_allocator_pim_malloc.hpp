#pragma once
// V41.F.6.1 Batch 6 PIMMallocAllocator A16 (2026-05-26)
//
// @topic allocator @achse 6 @family A16 (PIM-Malloc — VIA-Research, HPCA 2026 / arXiv:2505.13002)
// @subaxis AA5 allocation_policy (Processing-In-Memory Hardware-Awareness)
//
// **SONDERFALL PIM-Malloc:** Verlangt PIM-Hardware (UPMEM DPUs, Samsung HBM-PIM,
// SK Hynix AiM). API hat DPU-ID-Parameter analog NUMA-Node bei NUMAlloc.
// On non-PIM-systems: Fallback auf portable_aligned_alloc (Host-Memory).
//
// SONDERFALL-PROPERTY (User-Direktive 2026-05-26, Concept-Erweiterung):
//   requires_specialized_hardware() = true
//   CacheEngineBuilder kann pro Plattform Allocator-Subsets bilden — auf einem
//   reinem CPU-System wuerde dieser Allocator nicht permutiert werden.

#include "axis_06_allocator_strategy_base.hpp"
#include "axis_06_allocator_subaxes_aa1_to_aa7.hpp"
#include "concepts/axis_06_allocator_concept.hpp"
#include "concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"
#include "concepts/axis_06_allocator_zeroing_strategy_concept.hpp"
#include "concepts/axis_06_allocator_reallocating_strategy_concept.hpp"
#include <topics/allocator/concepts/topic_allocator_concept.hpp>

#include <axes/alloc/axis_06_allocator_flags.hpp>
#include "vendor_includes/pim_malloc_include.hpp"

#include <cache_engine/allocators/portable_aligned_alloc.hpp>
#include <measurement/measurable_concept.hpp>
#include <array> // FK-6: der algorithmen-eigene Fehlerraum-Satz (2 Etiketten)
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::alloc {

class PIMMallocAllocator : public AllocatorStrategyBase<PIMMallocAllocator> {
public:
    static constexpr bool enabled = flags::pim_malloc_enabled;

    /// SONDERFALL: PIM-DPU-Default (-1 = primaerer DPU)
    static constexpr int kDefaultDpuId = -1;

    using value_type = std::byte;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::allocator::concepts::AllocatorTopicTag;
    using axis_tag   = subaxes::allocation_policy_tag;
    using family_id  = std::integral_constant<int, 16>;

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return true; }
    [[nodiscard]] static constexpr bool        supports_pmr() noexcept { return true; }
    [[nodiscard]] static constexpr std::size_t max_alignment() noexcept { return alignof(std::max_align_t); }

    [[nodiscard]] static constexpr std::string_view name() noexcept {
        if constexpr (enabled) {
            return "pim_malloc";
        } else {
            return "pim_malloc(real=std)";
        }
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "PIM-Malloc Processing-In-Memory (VIA-Research, HPCA 2026 / arXiv:2505.13002)";
    }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    /// A1-WURF-VERTRAG (2026-08-06), 1. Bump: v1.0.0c -> v1.0.1c, weil der FEHLSCHLAG-Vertrag der Achse sich
    /// geaendert hat, ohne eine Registry-/XML-Flaeche zu bewegen -- ohne Bump wuerde der inkrementelle
    /// Tier-Binary-Cache alte Binaries weiterverwenden. Volle Begruendung samt Frozen-Neutralitaets-Beweis:
    /// axis_06_allocator_strategy_base.hpp, Abschnitt "A1-VERSIONS-BUMP".
    /// 2. Bump (2026-08-06): v1.0.1c -> v1.0.2c -- die reallocate()-Statistik-Korrektur bekam
    /// nachtraeglich einen Bump (Owner-Entscheid: "heute unerreichbar" entlastet nicht, s. dort).
    /// Volle Begruendung: axis_06_allocator_strategy_base.hpp, Abschnitt "A1-VERSIONS-BUMP, 2. BUMP".
    static constexpr std::string_view               algo_version = "1.0.2.c";
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "PIM_MALLOC"; }

    // V41.F.6.1 Vendor-Sonderfall-Properties (Pflicht)
    [[nodiscard]] static constexpr bool has_native_aligned_alloc() noexcept {
        return true;
    } // pim_alloc(size, alignment, dpu)
    [[nodiscard]] static constexpr bool requires_explicit_init() noexcept { return false; }  // pim_detect_hardware lazy
    [[nodiscard]] static constexpr bool supports_numa_node_hint() noexcept { return false; } // PIM != NUMA
    [[nodiscard]] static constexpr bool supports_thread_local_cache() noexcept {
        return false;
    } // single-DPU per allocation
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }
    [[nodiscard]] static constexpr bool requires_specialized_hardware() noexcept {
        return true;
    } // SONDERFALL: PIM-Hardware-Pflicht

    /// FK-6 (A15 EBENE 4, ALGORITHMEN-Ebene) -- DER EINE Fall im Bestand, in dem der Fehlerraum des
    /// ALGORITHMUS ueber den seiner ACHSE hinausgeht. Gemessen 09.08.2026: von 126 registrierten
    /// Organ-Varianten tragen 26 ueberhaupt requires_specialized_hardware(), und GENAU DIESE EINE
    /// meldet true. Fuer die uebrigen 125 deckt die Achse den Raum vollstaendig ab -- deshalb steht
    /// hier eine Deklaration und nicht in 126 Dateien eine Abschrift.
    ///
    /// WARUM 'quelle_nicht_verfuegbar' UND NICHT 'mess_fehler': pim_alloc() misst gegen PIM-DPU-
    /// Hardware (UPMEM, Samsung HBM-PIM, SK Hynix AiM). Auf einer CPU-Flotte ist die schlicht NICHT
    /// DA -- der Vendor-Shim liefert dann einen nullptr-Stub (vendor_includes/pim_malloc_include.hpp:
    /// pim_detect_hardware() == 0). Das ist D2 SourceUnavailable und KEIN Defekt des Algorithmus.
    /// Waere nur der Achsen-Boden deklariert, laese derselbe Lauf als "mess_fehler" -- also als
    /// gescheiterter Allokator, obwohl nie eine PIM-Einheit vorhanden war. Genau diese Verwechslung
    /// trennt axis_error.hpp seit INC-29.0 auseinander (Kopf: "ehrliches n/a, KEIN Algo-Fehler").
    ///
    /// DER BODEN BLEIBT DRIN: eine PIM-Allokation auf ECHTER PIM-Hardware kann sehr wohl real
    /// scheitern (DPU-OOM). Der Satz ERGAENZT den Achsen-Boden, er ERSETZT ihn nicht -- sonst waere
    /// ein echter Algo-Fehler auf PIM-Hardware ploetzlich unbenennbar.
    [[nodiscard]] static constexpr auto error_classes() noexcept {
        return std::array<std::string_view, 2>{::comdare::cache_engine::topics::kOrganErrMessFehler,
                                               ::comdare::cache_engine::topics::kOrganErrQuelleNichtVerfuegbar};
    }

    PIMMallocAllocator() noexcept : dpu_id_(kDefaultDpuId) {}
    explicit PIMMallocAllocator(int dpu_id) noexcept : dpu_id_(dpu_id) {}

    [[nodiscard]] bool operator==(PIMMallocAllocator const& other) const noexcept { return dpu_id_ == other.dpu_id_; }

    [[nodiscard]] void* allocate(std::size_t bytes, std::size_t alignment) {
        void* p;
        if constexpr (enabled) {
            p = ::pim_alloc(bytes, alignment, dpu_id_);
        } else {
            p = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, bytes);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += aligned_bytes;
            stats_.total_bytes_in_use += aligned_bytes;
        } else {
            ++stats_.failure_count;
        }
        observer_.notify(stats_);
#endif
        return p;
    }

    void deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept {
        if (p == nullptr) return;
        if constexpr (enabled) {
            ::pim_free(p, dpu_id_);
        } else {
            ::comdare::cache_engine::allocator::portable_aligned_free(p);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::size_t aligned_bytes = ((bytes + alignment - 1) / alignment) * alignment;
        ++stats_.deallocation_count;
        if (aligned_bytes <= stats_.total_bytes_in_use)
            stats_.total_bytes_in_use -= aligned_bytes;
        else
            stats_.total_bytes_in_use = 0;
        observer_.notify(stats_);
#else
        (void)bytes;
        (void)alignment;
#endif
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::AllocationStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    }
    // Phase 0.3 (Memento, VERVOLLSTAENDIGUNG 2026-08-05 / A8-S5 01c Vorlauf 0): Statistik auf einen zuvor
    // via statistics() gezogenen Snapshot zuruecksetzen -- spiegelbildlich zu reset(). Die Deklaration ist
    // PFLICHT, kein Komfort: AllocatorStrategyBase::restore_statistics ist ein CRTP-WEITERLEITER
    // (derived().restore_statistics(s)). Fehlt der eigene Member, findet der Weiterleiter nur sich SELBST
    // wieder -- unbeschraenkte Selbst-Rekursion, die g++ 15.3 unter -O3 als Endlosschleife ('jmp .')
    // emittiert und unter -O1 als rekursiven Selbstaufruf. Bis 2026-08-05 trug ausschliesslich
    // ExgenAllocator diesen Member; jede andere Strategie haette den Memento-Pfad (Copy-Ctor der
    // strategie-besitzenden Stores/Puffer) zum Haenger gemacht. Die Basis pinnt das jetzt zusaetzlich
    // compile-hart (self-proving static_assert im Weiterleiter).
    void restore_statistics(snapshot_t const& s) noexcept {
        stats_ = s;
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }
#endif

    [[nodiscard]] void* zero_allocate(std::size_t n, std::size_t size) {
        std::size_t bytes = n * size;
        void*       p;
        if constexpr (enabled) {
            p = ::pim_calloc(n, size, dpu_id_);
        } else {
            p = std::calloc(n, size);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if (p != nullptr) {
            ++stats_.allocation_count;
            stats_.total_bytes_allocated += bytes;
            stats_.total_bytes_in_use += bytes;
        } else {
            ++stats_.failure_count;
        }
        observer_.notify(stats_);
#endif
        return p;
    }

    [[nodiscard]] void* reallocate(void* p, std::size_t old_bytes, std::size_t new_bytes, std::size_t alignment) {
        void* np;
        if constexpr (enabled) {
            np = ::pim_realloc(p, new_bytes, dpu_id_);
            (void)alignment;
        } else {
            np = ::comdare::cache_engine::allocator::portable_aligned_alloc(alignment, new_bytes);
            if (np != nullptr && p != nullptr) {
                std::size_t copy_bytes = (old_bytes < new_bytes) ? old_bytes : new_bytes;
                std::memcpy(np, p, copy_bytes);
                ::comdare::cache_engine::allocator::portable_aligned_free(p);
            }
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        if (np == nullptr) {
            ++stats_.failure_count;
            observer_.notify(stats_);
            return nullptr;
        }
        if (p != nullptr) {
            // A1-Nachbesserung 2026-08-06 (Statistik-Symmetrie der Achse): die Gegenbuchung des ALTEN
            // Blocks rechnet ALIGNED -- genau wie seine Buchung in allocate() und wie die Gegenbuchung
            // in deallocate(). Die rohe old_bytes liess je reallocate Phantom-Bytes stehen. Volle
            // Begruendung: axis_06_allocator_pool_resource.hpp, Abschnitt reallocate.
            std::size_t aligned_old = ((old_bytes + alignment - 1) / alignment) * alignment;
            if (aligned_old <= stats_.total_bytes_in_use)
                stats_.total_bytes_in_use -= aligned_old;
            else
                stats_.total_bytes_in_use = 0;
            ++stats_.deallocation_count;
        }
        std::size_t aligned_new = ((new_bytes + alignment - 1) / alignment) * alignment;
        stats_.total_bytes_in_use += aligned_new;
        stats_.total_bytes_allocated += aligned_new;
        ++stats_.allocation_count;
        observer_.notify(stats_);
#endif
        return np;
    }

    /// PIM-spezifisch: DPU-ID Setter/Getter
    [[nodiscard]] int dpu_id() const noexcept { return dpu_id_; }
    void              set_dpu_id(int dpu) noexcept { dpu_id_ = dpu; }

private:
    int dpu_id_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::AllocationStatistics stats_{};
    observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::alloc

namespace comdare::cache_engine::alloc {
static_assert(concepts::AllocatorStrategy<PIMMallocAllocator>);
static_assert(concepts::CacheEnginePermutationStrategy<PIMMallocAllocator>);
static_assert(concepts::ZeroingStrategy<PIMMallocAllocator>);
static_assert(concepts::ReallocatingStrategy<PIMMallocAllocator>);
} // namespace comdare::cache_engine::alloc
