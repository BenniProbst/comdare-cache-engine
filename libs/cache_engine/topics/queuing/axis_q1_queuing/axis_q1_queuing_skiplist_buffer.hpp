#pragma once
// V41.F.6.1 axis_q1_queuing SkiplistBuffer Q-SKIP (2026-05-26)
//
// @topic queuing @achse Q1 @family Q08 SkiplistBuffer
// @subaxis QS2 ordered_access
//
// Sortierter Buffer im LSM-MemTable-Stil (RocksDB, LevelDB): jeder put() fuegt
// in sortierter Reihenfolge ein. get() liefert das KLEINSTE Element (ascending
// drain — typisch fuer LSM-Compact-Output).
//
// Implementation via std::set<element_type> (sortiert+unique). Eine echte
// Skiplist-Implementation (Pugh CACM 1990) ist im Pilot abstrahiert — die
// Concept-Pflicht-API ist identisch, nur die O(log N)-Asymptotik wird durch
// Red-Black-Tree statt Skiplist erbracht.
//
// Cross-Constraint mit PriorityHeapBuffer: beide haben supports_priority_ordering=true,
// aber SkiplistBuffer ist ASCENDING (min first), PriorityHeapBuffer ist DESCENDING
// (max first). Q-SKIP fuer LSM-Compact, Q-PRIO fuer Hot-Key-Eviction.
//
// Allocation: A8-S5 SCHNITT-FORM (B), 2026-08-05 -- die Tree-KNOTEN kommen aus der Allokator-ACHSE
// (axis_06), nicht mehr aus std::allocator. FORM-ENTSCHEID am Objekt: Form A (heap-frei) scheidet
// zweifach aus -- der Puffer ist unbounded (is_bounded()==false), UND std::set hat ueberhaupt keine
// inline-Entsprechung: der Zustand ist eine Knoten-Struktur, kein zusammenhaengender Block.
// MECHANIK, die diesen Fall von den Vektor-Organen unterscheidet: std::set alloziert nie den
// Element-Typ, sondern seinen internen Knoten-Typ. Der Adapter wird deshalb ueber
// allocator_traits::rebind_alloc auf den Knoten umgebunden -- das leistet der StdAllocatorAdapter
// dank seines konvertierenden Template-Ctors ohne eine einzige Zeile Sonderbehandlung hier.
// [[allocation-failure-exception]]: der Wurf kommt seit diesem Schnitt vom StdAllocatorAdapter der
// Achse (Posten 64), nicht mehr vom Default-Allokator.

#include "axis_q1_queuing_axis_storage.hpp"
#include "axis_q1_queuing_base.hpp"
#include "axis_q1_queuing_subaxes_qs1_to_qs6.hpp"
#include "concepts/axis_q1_queuing_concept.hpp"
#include "concepts/axis_q1_queuing_cache_engine_permutation_concept.hpp"
#include "../concepts/topic_queuing_concept.hpp"

#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::queuing::axis_q1_queuing {

class SkiplistBuffer : public BufferStrategyBase<SkiplistBuffer> {
public:
    static constexpr bool enabled = flags::skiplist_buffer_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::ordered_access_tag;
    using family_id    = std::integral_constant<int, 8>; // Q08

    /// A8-S5 SCHNITT-FORM (B): DIESE Zeile ist der Ausweis, den die Familien-Konformitaets-Wache
    /// liest (tests/unit/s5_family_alloc_conformance.hpp). Achsen-Default: axis_q1_queuing_axis_storage.hpp.
    using allocator_type = queuing_buffer_allocator_t;

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr bool             is_bounded() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      default_capacity() noexcept { return 0; } // unbounded
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "skiplist_buffer"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::queuing::axis_q1_queuing::SkiplistBuffer",
                                  "topics/queuing/axis_q1_queuing/axis_q1_queuing_skiplist_buffer.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "SkiplistBuffer (LSM MemTable — RocksDB/LevelDB, Pugh CACM 1990)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "SKIPLIST_BUFFER"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    /// SONDERFALL: ordered_access mit min-first Drain (LSM-Compact-Semantik).
    [[nodiscard]] static constexpr bool                        supports_priority_ordering() noexcept { return true; }
    [[nodiscard]] static constexpr bool                        is_versioned() noexcept { return false; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    /// Der Baum wird an das EIGENE allocator_ gebunden (der Adapter haelt &allocator_); std::set
    /// rebindet ihn intern selbst auf seinen Knoten-Typ.
    SkiplistBuffer() : data_(std::less<element_type>{}, allocator_.template as_std_allocator<element_type>()) {}

    /// COW-SICHERHEIT (Memento-Muster, Praezedenz btree_node_pool_store.hpp:86): Copy-Ctor/Assign
    /// rebinden an das EIGENE allocator_ und verwerfen die transiente Kopier-Pollution per
    /// restore_statistics. MOVE bewusst NICHT deklariert (Fremd-Zeiger im Adapter).
    SkiplistBuffer(SkiplistBuffer const& o)
        : allocator_(o.allocator_), data_(o.data_, allocator_.template as_std_allocator<element_type>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_    = o.stats_;
        observer_ = o.observer_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    SkiplistBuffer& operator=(SkiplistBuffer const& o) {
        if (this != &o) {
            data_ = o.data_; // POCCA=false -> eigener Adapter bleibt erhalten
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_    = o.stats_;
            observer_ = o.observer_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~SkiplistBuffer() = default;

    [[nodiscard]] bool operator==(SkiplistBuffer const& other) const noexcept {
        return data_.size() == other.data_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: std::set::insert wirft
    /// std::bad_alloc bei Tree-Knoten-OOM.
    /// HINWEIS: Duplikate werden silent ignoriert (set-Semantik). LSM-typisch
    /// werden Schluessel sowieso ueber Versions-ID disambiguiert.
    void put(element_type v) {
        data_.insert(v);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        if (data_.size() > stats_.peak_size) stats_.peak_size = data_.size();
        observer_.notify(stats_);
#endif
    }

    /// Entfernt + liefert MIN-Element (ascending drain).
    [[nodiscard]] std::optional<element_type> get() {
        if (data_.empty()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.underflow_count;
            observer_.notify(stats_);
#endif
            return std::nullopt;
        }
        auto         it = data_.begin();
        element_type v  = *it;
        data_.erase(it);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_get_count;
        observer_.notify(stats_);
#endif
        return v;
    }

    [[nodiscard]] size_type size() const noexcept { return data_.size(); }
    [[nodiscard]] bool      is_empty() const noexcept { return data_.empty(); }
    void                    clear() noexcept { data_.clear(); }

    // Skiplist-spezifisch: peek_front=min (was als naechstes get() liefert), peek_back=max.
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept {
        if (data_.empty()) return std::nullopt;
        return *data_.begin();
    }
    [[nodiscard]] std::optional<element_type> peek_back() const noexcept {
        if (data_.empty()) return std::nullopt;
        return *data_.rbegin();
    }
    void emplace(element_type v) { put(v); }

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
    using data_type = std::set<element_type, std::less<element_type>, queuing_buffer_alloc_t<element_type>>;

    // allocator_ MUSS VOR data_ stehen (Adapter haelt &allocator_) -- Ordnung wie 01a/01c/01d.
    allocator_type allocator_{};
    data_type      data_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t                 observer_{};
#endif
};

} // namespace comdare::cache_engine::queuing::axis_q1_queuing

namespace comdare::cache_engine::queuing::axis_q1_queuing {
static_assert(concepts::BufferStrategy<SkiplistBuffer>);
static_assert(concepts::CacheEngineBufferPermutationStrategy<SkiplistBuffer>);
} // namespace comdare::cache_engine::queuing::axis_q1_queuing
