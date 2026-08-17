#pragma once
// V41.F.6.1 axis_q1_queuing PriorityHeapBuffer Q-PRIO (2026-05-26)
//
// @topic queuing @achse Q1 @family Q06 PriorityHeapBuffer (Hot-Key Promotion)
// @subaxis QS2 ordered_access
//
// Priority-basierter Buffer: get() liefert IMMER hoechste Prioritaet (max-heap).
// Implementation via std::priority_queue<std::uint64_t>. Anwendung: LRU-Approximation,
// Hot-Key-Promotion (z.B. fuer Eviction-Queues, wo der "wichtigste" Eintrag zuerst
// geflusht/erhalten werden soll).
//
// **Erste Strategie mit supports_priority_ordering()=true** — Markant fuer
// PermutationEngine: nur Strategien mit dieser Property koennen in
// Hot-Key/Eviction-Pfaden eingesetzt werden.
//
// Allocation: A8-S5 SCHNITT-FORM (B), 2026-08-05 -- der Heap-Speicher haengt an der Allokator-ACHSE
// (axis_06). FORM-ENTSCHEID am Objekt: Form A (heap-frei) scheidet aus -- unbounded
// (is_bounded()==false); ein inline-Heap fester Groesse waere ein anderes Organ.
// MECHANIK, die diesen Fall von den Vektor-Organen unterscheidet: std::priority_queue nimmt KEINEN
// Allokator-Parameter, sondern einen CONTAINER-Parameter. Der Achsen-Adapter wird deshalb eine Ebene
// tiefer gesetzt (std::vector<element_type, Achsen-Adapter> als Traeger-Container) und der Heap mit
// (Compare, Container) konstruiert -- der einzige Weg, ohne die Adapter-Zeile in den Heap selbst zu
// erfinden. Die Max-Heap-Semantik und der O(1)-Min-Tracker (F57/Muster B) bleiben unberuehrt.
// [[allocation-failure-exception]]: der Wurf kommt seit diesem Schnitt vom StdAllocatorAdapter der
// Achse (Posten 64), nicht mehr vom Default-Allokator.

#include "axis_q1_queuing_axis_storage.hpp"
#include "axis_q1_queuing_base.hpp"
#include "axis_q1_queuing_subaxes_qs1_to_qs6.hpp"
#include "concepts/axis_q1_queuing_concept.hpp"
#include "concepts/axis_q1_queuing_cache_engine_permutation_concept.hpp"
#include <topics/queuing/concepts/topic_queuing_concept.hpp>

#include <organ_axes/axis_q1_queuing/axis_q1_queuing_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <queue>
#include <string_view>
#include <type_traits>
#include <vector>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::queuing::axis_q1_queuing {

class PriorityHeapBuffer : public BufferStrategyBase<PriorityHeapBuffer> {
public:
    static constexpr bool enabled = flags::priority_heap_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::ordered_access_tag;
    using family_id    = std::integral_constant<int, 6>; // Q06

    /// A8-S5 SCHNITT-FORM (B): DIESE Zeile ist der Ausweis, den die Familien-Konformitaets-Wache
    /// liest (tests/unit/s5_family_alloc_conformance.hpp). Achsen-Default: axis_q1_queuing_axis_storage.hpp.
    using allocator_type = queuing_buffer_allocator_t;

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr bool             is_bounded() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      default_capacity() noexcept { return 0; } // unbounded
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "priority_heap"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::queuing::axis_q1_queuing::PriorityHeapBuffer",
                                  "organ_axes/axis_q1_queuing/axis_q1_queuing_priority_heap.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "PriorityHeapBuffer (Max-Heap, LRU-Approx + Hot-Key Promotion)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "PRIORITY_HEAP"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    /// SONDERFALL: erste Q1-Strategie mit Priority-Ordering=TRUE.
    [[nodiscard]] static constexpr bool                        supports_priority_ordering() noexcept { return true; }
    [[nodiscard]] static constexpr bool                        is_versioned() noexcept { return false; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    /// Der Traeger-Container des Heaps wird an das EIGENE allocator_ gebunden und dem Heap als
    /// (Compare, Container) uebergeben -- std::priority_queue selbst kennt keinen Allokator-Parameter.
    PriorityHeapBuffer() : heap_(compare_type{}, make_bound_container()) {}

    /// COW-SICHERHEIT (Memento-Muster, Praezedenz btree_node_pool_store.hpp:86) -- und der Grund,
    /// warum der Copy-Ctor den Heap ERST leer und gebunden baut und die Elemente DANACH zuweist:
    /// eine Heap-Copy-KONSTRUKTION haette den Traeger-Container copy-konstruiert, und dabei kopiert
    /// std::vector seinen Allokator mit (select_on_container_copy_construction) -- der Adapter der
    /// QUELLE landete in unserem Heap. Die ZUWEISUNG dagegen laesst den Adapter unangetastet
    /// (propagate_on_container_copy_assignment ist false) und re-alloziert ueber UNSER allocator_.
    /// MOVE bewusst NICHT deklariert (er zoege den Adapter mitsamt Fremd-Zeiger).
    PriorityHeapBuffer(PriorityHeapBuffer const& o)
        : allocator_(o.allocator_), heap_(compare_type{}, make_bound_container()), min_(o.min_) {
        heap_ = o.heap_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_    = o.stats_;
        observer_ = o.observer_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    PriorityHeapBuffer& operator=(PriorityHeapBuffer const& o) {
        if (this != &o) {
            heap_ = o.heap_; // POCCA=false -> der Traeger-Container behaelt unseren Adapter
            min_  = o.min_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_    = o.stats_;
            observer_ = o.observer_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~PriorityHeapBuffer() = default;

    [[nodiscard]] bool operator==(PriorityHeapBuffer const& other) const noexcept {
        return heap_.size() == other.heap_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: priority_queue::push wirft
    /// std::bad_alloc bei OOM. Nicht noexcept.
    void put(element_type v) {
        heap_.push(v);
        // (F57/Muster B, WP-5 2026-07-16): Min-Tracker NACH erfolgreichem push aktualisieren — macht
        // peek_back() allokationsfrei O(1) (s. dort). Invariante: min_ = Minimum aller Heap-Elemente.
        if (!min_.has_value() || v < *min_) min_ = v;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        if (heap_.size() > stats_.peak_size) stats_.peak_size = heap_.size();
        observer_.notify(stats_);
#endif
    }

    /// Entfernt + liefert MAX-Element (Hot-Key first).
    [[nodiscard]] std::optional<element_type> get() {
        if (heap_.empty()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.underflow_count;
            observer_.notify(stats_);
#endif
            return std::nullopt;
        }
        element_type v = heap_.top();
        heap_.pop();
        // (F57/Muster B): Min-Tracker-Erhalt — das Minimum verlaesst den Max-Heap nur als top(), also wenn
        // ALLE verbliebenen Elemente == min sind (dann bleibt min_ korrekt) oder der Heap leer wird (reset).
        if (heap_.empty()) min_.reset();
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_get_count;
        observer_.notify(stats_);
#endif
        return v;
    }

    [[nodiscard]] size_type size() const noexcept { return heap_.size(); }
    [[nodiscard]] bool      is_empty() const noexcept { return heap_.empty(); }
    /// Bleibt noexcept wie zuvor: der frisch gebundene Traeger-Container ist LEER und alloziert
    /// deshalb nicht; die Move-Zuweisung zwischen zwei gleich vergleichenden Adaptern ist ein reiner
    /// Zeiger-Transfer. Frueher stand hier `decltype(heap_){}` -- das geht seit dem Schnitt nicht
    /// mehr, weil der Achsen-Adapter (bewusst) keinen Default-Konstruktor hat: ein default-gebauter
    /// Heap haette gar keine Achse, an der er haengt.
    void clear() noexcept {
        heap_ = heap_type{compare_type{}, make_bound_container()};
        min_.reset();
    }

    // std::queue-API auf max-heap:
    //   peek_front=highest priority (top, was als naechstes get() liefert)
    //   peek_back=lowest priority (Approximation — std::priority_queue exponiert kein min direkt)
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept {
        if (heap_.empty()) return std::nullopt;
        return heap_.top();
    }
    /// (F57/Muster B, WP-5 2026-07-16): der Gattungs-Concept (BufferStrategy) verlangt peek_back() noexcept —
    /// statt noexcept zu entfernen wird die Allokation HERAUSGEHOBEN: der fruehere O(N)-Drain kopierte den
    /// GANZEN Heap (`auto copy = heap_` = Container-Vollkopie unter noexcept = terminate-on-OOM, Muster B).
    /// Jetzt haelt ein O(1)-Min-Tracker (min_, gepflegt im nicht-noexcept put() und in get()/clear()) das
    /// exakte Minimum: das Minimum verlaesst den Max-Heap nur, wenn es selbst top() ist (= alle Elemente
    /// gleich, min_ bleibt korrekt) oder der Heap leer wird (reset). std::priority_queue exponiert weiterhin
    /// keinen Iterator; die unsichere c-Member-Konvention wird weiterhin NICHT genutzt.
    [[nodiscard]] std::optional<element_type> peek_back() const noexcept { return min_; }
    void                                      emplace(element_type v) { put(v); }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::BufferStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }

    /// EINSAMMEL-NAHT der T6-Durchbindung (Owner-KERN abend-11) -- EIGENER Name, DISJUNKT zum
    /// konstitutiven Store-Snapshot; die Summierungs-Frage gehoert ins Mess-Schnitt-Fenster.
    using allocator_snapshot_t = typename allocator_type::snapshot_t;
    [[nodiscard]] allocator_snapshot_t buffer_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    using compare_type   = std::less<element_type>;
    using container_type = std::vector<element_type, queuing_buffer_alloc_t<element_type>>;
    using heap_type      = std::priority_queue<element_type, container_type, compare_type>;

    /// Ein LEERER Traeger-Container, gebunden an das EIGENE allocator_. Leer heisst: keine Allokation.
    [[nodiscard]] container_type make_bound_container() {
        return container_type(allocator_.template as_std_allocator<element_type>());
    }

    // allocator_ MUSS VOR heap_ stehen (der Traeger-Container haelt &allocator_) -- Ordnung wie 01a/01c/01d.
    allocator_type allocator_{};
    heap_type      heap_;
    // (F57/Muster B): exaktes Heap-Minimum fuer das allokationsfreie noexcept-peek_back() (s. dort).
    std::optional<element_type> min_{};
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t                 observer_{};
#endif
};

} // namespace comdare::cache_engine::queuing::axis_q1_queuing

namespace comdare::cache_engine::queuing::axis_q1_queuing {
static_assert(concepts::BufferStrategy<PriorityHeapBuffer>);
static_assert(concepts::CacheEngineBufferPermutationStrategy<PriorityHeapBuffer>);
} // namespace comdare::cache_engine::queuing::axis_q1_queuing
