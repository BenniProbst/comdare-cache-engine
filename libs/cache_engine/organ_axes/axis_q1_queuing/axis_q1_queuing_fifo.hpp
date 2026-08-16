#pragma once
// V41.F.6.1 axis_q1_queuing FIFOQueueBuffer Q-FIFO (2026-05-26)
//
// @topic queuing @achse Q1 @family Q03 FIFOQueueBuffer (Ring/Deque)
// @subaxis QS1 sequential_access
//
// First-In-First-Out: klassische LSM-MemTable + Write-Coalescing-Pattern.
// Unbounded — std::deque-basiert.
//
// A8-S5 SCHNITT-FORM (B), 2026-08-05: der Deque-Speicher haengt an der Allokator-ACHSE (axis_06).
// FORM-ENTSCHEID am Objekt: Form A (heap-frei) scheidet aus -- der Puffer ist ERKLAERT unbounded
// (is_bounded()==false, default_capacity()==0), eine Compile-Time-Kappe waere keine staerkere
// Aussage, sondern eine Verhaltens-Aenderung. Deque statt Vektor bleibt der Familien-Unterschied
// zu AppendOnlyBuffer und wird NICHT eingeebnet; nur die Speicher-Quelle wandert an die Achse.
// [[allocation-failure-exception]]: der Wurf kommt seit diesem Schnitt NICHT mehr vom
// Default-Allokator, sondern vom StdAllocatorAdapter der Achse (Posten 64) -- s. put().

#include "axis_q1_queuing_axis_storage.hpp"
#include "axis_q1_queuing_base.hpp"
#include "axis_q1_queuing_subaxes_qs1_to_qs6.hpp"
#include "concepts/axis_q1_queuing_concept.hpp"
#include "concepts/axis_q1_queuing_cache_engine_permutation_concept.hpp"
#include <topics/queuing/concepts/topic_queuing_concept.hpp>

#include <organ_axes/axis_q1_queuing/axis_q1_queuing_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::queuing::axis_q1_queuing {

class FIFOQueueBuffer : public BufferStrategyBase<FIFOQueueBuffer> {
public:
    static constexpr bool enabled = flags::fifo_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::sequential_access_tag;
    using family_id    = std::integral_constant<int, 3>; // Q03

    /// A8-S5 SCHNITT-FORM (B): DIESE Zeile ist der Ausweis, den die Familien-Konformitaets-Wache
    /// liest (tests/unit/s5_family_alloc_conformance.hpp). Der Achsen-Default steht EINMAL in
    /// axis_q1_queuing_axis_storage.hpp, nicht hier -- Umbinden der Achse ist eine Zeile, nicht 13.
    using allocator_type = queuing_buffer_allocator_t;

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr bool             is_bounded() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      default_capacity() noexcept { return 0; } // unbounded
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "fifo_queue"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::queuing::axis_q1_queuing::FIFOQueueBuffer",
                                  "organ_axes/axis_q1_queuing/axis_q1_queuing_fifo.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "FIFOQueueBuffer (Ring/Deque — LSM MemTable + Write-Coalescing)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "FIFO"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_priority_ordering() noexcept { return false; }
    [[nodiscard]] static constexpr bool is_versioned() noexcept { return false; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    /// Der Deque wird an das EIGENE allocator_ gebunden (der Adapter haelt &allocator_).
    FIFOQueueBuffer() : data_(allocator_.template as_std_allocator<element_type>()) {}

    /// COW-SICHERHEIT (Memento-Muster, Praezedenz btree_node_pool_store.hpp:86 / linear_scan:167):
    /// der StdAllocatorAdapter haelt einen Zeiger auf die Strategie-INSTANZ. Eine implizite Kopie
    /// zoege den Adapter der QUELLE mit -- die Kopie allozierte/deallozierte dann ueber fremden,
    /// potentiell schon zerstoerten Speicher. Copy-Ctor/Assign rebinden deshalb an das EIGENE
    /// allocator_ und verwerfen die durch die Vollkopie entstandene transiente Allokations-Pollution
    /// per restore_statistics. MOVE ist BEWUSST NICHT deklariert (der user-definierte Copy
    /// unterdrueckt ihn implizit): jede Bewegung faellt auf den REBINDENDEN Copy zurueck.
    FIFOQueueBuffer(FIFOQueueBuffer const& o)
        : allocator_(o.allocator_), data_(o.data_, allocator_.template as_std_allocator<element_type>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_    = o.stats_;
        observer_ = o.observer_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    FIFOQueueBuffer& operator=(FIFOQueueBuffer const& o) {
        if (this != &o) {
            // propagate_on_container_copy_assignment ist false -> data_ BEHAELT seinen eigenen
            // Adapter (auf unser allocator_); genau das ist hier gewollt.
            data_ = o.data_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_    = o.stats_;
            observer_ = o.observer_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~FIFOQueueBuffer() = default;

    [[nodiscard]] bool operator==(FIFOQueueBuffer const& other) const noexcept {
        return data_.size() == other.data_.size(); // gleicher Inhalt zu pruefen waere teuer
    }

    /// SONDERFALL [[allocation-failure-exception]]: das Deque-Wachstum kann werfen. KAUSALITAET seit
    /// dem A8-S5-Schnitt (Posten 64/70): der Wurf kommt vom StdAllocatorAdapter der Allokator-ACHSE
    /// (Strategie meldet OOM per nullptr, der Adapter uebersetzt an EINER Stelle in std::bad_alloc),
    /// nicht mehr vom Default-Allokator. Fehlerklasse bleibt der FK-5-Boden der Allokator-Achse.
    void put(element_type v) {
        data_.push_back(v);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        if (data_.size() > stats_.peak_size) stats_.peak_size = data_.size();
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<element_type> get() {
        if (data_.empty()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.underflow_count;
            observer_.notify(stats_);
#endif
            return std::nullopt;
        }
        element_type v = data_.front();
        data_.pop_front();
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_get_count;
        observer_.notify(stats_);
#endif
        return v;
    }

    [[nodiscard]] size_type size() const noexcept { return data_.size(); }
    [[nodiscard]] bool      is_empty() const noexcept { return data_.empty(); }
    void                    clear() noexcept { data_.clear(); }

    // std::queue-API: peek_front=oldest (deque::front), peek_back=newest (deque::back)
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept {
        if (data_.empty()) return std::nullopt;
        return data_.front();
    }
    [[nodiscard]] std::optional<element_type> peek_back() const noexcept {
        if (data_.empty()) return std::nullopt;
        return data_.back();
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

    /// EINSAMMEL-NAHT der T6-Durchbindung (Owner-KERN LEDGER 04.08.2026 abend-11, Pflicht (a)).
    /// BEWUSST EIN EIGENER NAME neben store_/traversal_/search_allocator_statistics: jede Organ-Instanz
    /// haelt ihre EIGENE Strategie-Instanz, die Snapshots sind also DISJUNKT zum konstitutiven
    /// Store-Snapshot. Ob und wie sie in die T6-CSV-Spalte summiert werden (Doppelzaehlungs-Regel), ist
    /// der EXPLIZITE Schritt des Mess-Schnitt-Fensters VOR Messbeginn -- nicht dieser Scheibe. Die
    /// Namens-Trennung IST die Absicherung dagegen, dass ein generischer Leser still doppelt zaehlt.
    using allocator_snapshot_t = typename allocator_type::snapshot_t;
    [[nodiscard]] allocator_snapshot_t buffer_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    using data_type = std::deque<element_type, queuing_buffer_alloc_t<element_type>>;

    // allocator_ MUSS VOR data_ stehen: der Adapter haelt &allocator_, und die Member-
    // Initialisierungs-/Zerstoerungsreihenfolge ist die Deklarationsreihenfolge (data_ muss VOR der
    // Strategie sterben). Dieselbe Reihenfolge wie in den Pool-Stores (01a) und in 01c/01d.
    allocator_type allocator_{};
    data_type      data_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t                 observer_{};
#endif
};

} // namespace comdare::cache_engine::queuing::axis_q1_queuing

namespace comdare::cache_engine::queuing::axis_q1_queuing {
static_assert(concepts::BufferStrategy<FIFOQueueBuffer>);
static_assert(concepts::CacheEngineBufferPermutationStrategy<FIFOQueueBuffer>);
} // namespace comdare::cache_engine::queuing::axis_q1_queuing
