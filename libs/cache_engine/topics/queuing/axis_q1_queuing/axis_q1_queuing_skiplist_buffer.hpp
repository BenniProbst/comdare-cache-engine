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
// Allocation: std::set Tree-Knoten via std::allocator — wirft std::bad_alloc
// bei OOM ([[allocation-failure-exception]]).

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
    static constexpr std::string_view algo_version = "v1";

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    /// SONDERFALL: ordered_access mit min-first Drain (LSM-Compact-Semantik).
    [[nodiscard]] static constexpr bool                        supports_priority_ordering() noexcept { return true; }
    [[nodiscard]] static constexpr bool                        is_versioned() noexcept { return false; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

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
#endif

private:
    std::set<element_type> data_;
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
