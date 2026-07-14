#pragma once
// V41.F.6.1 axis_q1_queuing CopyOnWriteBuffer Q-COW (2026-05-26)
//
// @topic queuing @achse Q1 @family Q10 CopyOnWriteBuffer
// @subaxis QS4 versioned_access
//
// Snapshot-Buffer im Persistent-Data-Structure-Stil (Driscoll/Sarnak/Sleator/
// Tarjan JCSS 1989 — Persistent ART, RCU-Tries). Jeder put()/get()
// inkrementiert die Snapshot-Version, wodurch frühere Snapshots logisch
// erhalten bleiben (in dieser vereinfachten Implementation via shared_ptr
// auf den aktuellen Inhalt — der GC erledigt die nicht-mehr-referenzierten
// Snapshots automatisch).
//
// **3. Strategie mit is_versioned()=true** (nach DeltaChainBuffer + TombstoneBuffer).
// Konzeptioneller Unterschied:
//   - DeltaChainBuffer    = Append-Versioning (LIFO der Deltas)
//   - TombstoneBuffer = Erase-Versioning (Marker bleiben bis Compact)
//   - CopyOnWriteBuffer     = Snapshot-Versioning (gesamter State pro Version)
//
// Allocation: shared_ptr<vector> + Konstruktor — wirft std::bad_alloc bei OOM
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
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::queuing::axis_q1_queuing {

class CopyOnWriteBuffer : public BufferStrategyBase<CopyOnWriteBuffer> {
public:
    static constexpr bool enabled = flags::copy_on_write_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::versioned_access_tag;
    using family_id    = std::integral_constant<int, 10>; // Q10

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr bool             is_bounded() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      default_capacity() noexcept { return 0; } // unbounded
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "copy_on_write"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::queuing::axis_q1_queuing::CopyOnWriteBuffer",
                                  "topics/queuing/axis_q1_queuing/axis_q1_queuing_copy_on_write.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "CopyOnWriteBuffer (Persistent ART, RCU-Tries — Driscoll/Sarnak/Sleator/Tarjan JCSS 1989)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "COPY_ON_WRITE"; }

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_priority_ordering() noexcept { return false; }
    /// SONDERFALL: 3. Strategie mit Versionierung=TRUE (Snapshot-Versioning).
    [[nodiscard]] static constexpr bool                        is_versioned() noexcept { return true; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    CopyOnWriteBuffer() : current_(std::make_shared<std::vector<element_type>>()) {}

    [[nodiscard]] bool operator==(CopyOnWriteBuffer const& other) const noexcept {
        return current_->size() == other.current_->size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: shared_ptr-Allokation +
    /// vector-Kopie koennen std::bad_alloc werfen.
    /// CoW-Semantik: erstelle neuen Snapshot durch Vollkopie des aktuellen + Append.
    void put(element_type v) {
        auto next = std::make_shared<std::vector<element_type>>(*current_);
        next->push_back(v);
        current_ = std::move(next);
        ++snapshot_version_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        if (current_->size() > stats_.peak_size) stats_.peak_size = current_->size();
        observer_.notify(stats_);
#endif
    }

    /// FIFO-Drain mit CoW: neuer Snapshot ohne Front-Element.
    [[nodiscard]] std::optional<element_type> get() {
        if (current_->empty()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.underflow_count;
            observer_.notify(stats_);
#endif
            return std::nullopt;
        }
        element_type v    = current_->front();
        auto         next = std::make_shared<std::vector<element_type>>(current_->begin() + 1, current_->end());
        current_          = std::move(next);
        ++snapshot_version_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_get_count;
        observer_.notify(stats_);
#endif
        return v;
    }

    [[nodiscard]] size_type size() const noexcept { return current_->size(); }
    [[nodiscard]] bool      is_empty() const noexcept { return current_->empty(); }
    void                    clear() noexcept {
        current_ = std::make_shared<std::vector<element_type>>();
        ++snapshot_version_;
    }

    // std::queue-API: peek_front=oldest, peek_back=newest.
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept {
        if (current_->empty()) return std::nullopt;
        return current_->front();
    }
    [[nodiscard]] std::optional<element_type> peek_back() const noexcept {
        if (current_->empty()) return std::nullopt;
        return current_->back();
    }
    void emplace(element_type v) { put(v); }

    /// VersionedBufferStrategy [[versioned-strategy]]: monoton steigender Snapshot-Counter.
    /// Inkrementiert bei jedem put()/get()/clear() (Snapshot-Versioning).
    [[nodiscard]] std::uint64_t version_id() const noexcept { return snapshot_version_; }

    /// CoW-spezifisch (nicht im Sub-Concept): aktueller Snapshot als shared_ptr
    /// fuer Reader-Snapshot-Isolation (RCU-Tries-Pattern).
    [[nodiscard]] std::shared_ptr<std::vector<element_type> const> snapshot_ptr() const noexcept { return current_; }

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
    std::shared_ptr<std::vector<element_type>> current_;
    std::uint64_t                              snapshot_version_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t                 observer_{};
#endif
};

} // namespace comdare::cache_engine::queuing::axis_q1_queuing

namespace comdare::cache_engine::queuing::axis_q1_queuing {
static_assert(concepts::BufferStrategy<CopyOnWriteBuffer>);
static_assert(concepts::CacheEngineBufferPermutationStrategy<CopyOnWriteBuffer>);
static_assert(concepts::VersionedBufferStrategy<CopyOnWriteBuffer>);
} // namespace comdare::cache_engine::queuing::axis_q1_queuing
