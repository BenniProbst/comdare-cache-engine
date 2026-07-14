#pragma once
// V41.F.6.1 axis_q1_queuing DeltaChainBuffer Q-DELTA (2026-05-26)
//
// @topic queuing @achse Q1 @family Q07 DeltaChainBuffer
// @subaxis QS4 versioned_access
//
// Delta-Chain Buffer im Bw-Tree-Stil (Levandoski/Lomet/Sengupta, ICDE 2013):
// jeder put() haengt einen Delta-Record mit aufsteigender Version-ID an die
// Spitze der Chain. get() liefert das neueste Delta zurueck und entfernt es
// (LIFO-Drain mit Versions-ID).
//
// **Erste Strategie mit is_versioned()=true** — Markant fuer PermutationEngine:
// nur versionierte Strategien koennen in Snapshot/MVCC-Pfaden eingesetzt werden
// (z.B. fuer Q-EPOCH-Konsumenten, Snapshot-Isolation).
//
// Allocation: std::vector basiert — Heap-Wachstum wirft std::bad_alloc bei OOM
// ([[allocation-failure-exception]]).

#include "axis_q1_queuing_base.hpp"
#include "axis_q1_queuing_subaxes_qs1_to_qs6.hpp"
#include "concepts/axis_q1_queuing_concept.hpp"
#include "concepts/axis_q1_queuing_cache_engine_permutation_concept.hpp"
#include "concepts/axis_q1_queuing_versioned_strategy_concept.hpp"
#include "../concepts/topic_queuing_concept.hpp"

#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::queuing::axis_q1_queuing {

class DeltaChainBuffer : public BufferStrategyBase<DeltaChainBuffer> {
public:
    static constexpr bool enabled = flags::delta_chain_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::versioned_access_tag;
    using family_id    = std::integral_constant<int, 7>; // Q07

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr bool             is_bounded() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      default_capacity() noexcept { return 0; } // unbounded
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "delta_chain"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::queuing::axis_q1_queuing::DeltaChainBuffer",
                                  "topics/queuing/axis_q1_queuing/axis_q1_queuing_delta_chain.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "DeltaChainBuffer (Bw-Tree, Levandoski/Lomet/Sengupta ICDE 2013)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "DELTA_CHAIN"; }

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_priority_ordering() noexcept { return false; }
    /// SONDERFALL: erste Q1-Strategie mit Versionierung=TRUE.
    [[nodiscard]] static constexpr bool                        is_versioned() noexcept { return true; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    [[nodiscard]] bool operator==(DeltaChainBuffer const& other) const noexcept {
        return chain_.size() == other.chain_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: push_back kann std::bad_alloc werfen.
    void put(element_type v) {
        chain_.push_back(v);
        ++next_version_id_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        if (chain_.size() > stats_.peak_size) stats_.peak_size = chain_.size();
        observer_.notify(stats_);
#endif
    }

    /// LIFO-Drain (jüngstes Delta zuerst).
    [[nodiscard]] std::optional<element_type> get() {
        if (chain_.empty()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.underflow_count;
            observer_.notify(stats_);
#endif
            return std::nullopt;
        }
        element_type v = chain_.back();
        chain_.pop_back();
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_get_count;
        observer_.notify(stats_);
#endif
        return v;
    }

    [[nodiscard]] size_type size() const noexcept { return chain_.size(); }
    [[nodiscard]] bool      is_empty() const noexcept { return chain_.empty(); }
    void                    clear() noexcept { chain_.clear(); }

    /// Versionsspezifisch: peek_front=neuestes Delta (top of chain), peek_back=aeltestes.
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept {
        if (chain_.empty()) return std::nullopt;
        return chain_.back();
    }
    [[nodiscard]] std::optional<element_type> peek_back() const noexcept {
        if (chain_.empty()) return std::nullopt;
        return chain_.front();
    }
    void emplace(element_type v) { put(v); }

    /// VersionedBufferStrategy [[versioned-strategy]]: monoton steigender Delta-Counter.
    /// Inkrementiert bei jedem put() (Append-Versioning, Bw-Tree-Stil).
    [[nodiscard]] std::uint64_t version_id() const noexcept { return next_version_id_; }

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
    std::vector<element_type> chain_;
    std::uint64_t             next_version_id_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t                 observer_{};
#endif
};

} // namespace comdare::cache_engine::queuing::axis_q1_queuing

namespace comdare::cache_engine::queuing::axis_q1_queuing {
static_assert(concepts::BufferStrategy<DeltaChainBuffer>);
static_assert(concepts::CacheEngineBufferPermutationStrategy<DeltaChainBuffer>);
static_assert(concepts::VersionedBufferStrategy<DeltaChainBuffer>);
} // namespace comdare::cache_engine::queuing::axis_q1_queuing
