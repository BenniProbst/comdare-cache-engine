#pragma once
// V41.F.6.1 axis_q1_queuing BatchedInsertBuffer Q-BATCH (2026-05-26)
//
// @topic queuing @achse Q1 @family Q12 BatchedInsertBuffer
// @subaxis QS5 batched_access  (ERSTE QS5-Strategie)
//
// Batch-orientierter Buffer fuer OLAP-Indexe + ART-Bulk-Insert (Leis ICDE 2013
// §6 Bulk-Insertion-Optimization; vektorisiertes OLAP-Batching ist eine
// Re-Implementierung/Baseline im LSM-Batching-Kontext ohne verifiziertes
// Ursprungspaper):
// put() puffert in Sub-Batches der Groesse batch_size_; vollstaendige Batches
// werden in den Output-Bereich uebergeben (bulk-Drain). Ziel: amortisierter
// Insert-Cost durch Bulk-Operations (SIMD-Vectorization + Cache-Locality).
//
// **Erste Q1-Strategie mit Subaxis QS5 batched_access.** Spezifische Methode:
//   bulk_insert(std::span<element_type const>) — append eines kompletten
//   Sub-Batches in einem Schritt (effizienter als N×put()).
//
// Allocation: std::vector-basierte Batches — wirft std::bad_alloc bei OOM
// ([[allocation-failure-exception]]).

#include "axis_q1_queuing_base.hpp"
#include "axis_q1_queuing_subaxes_qs1_to_qs6.hpp"
#include "concepts/axis_q1_queuing_concept.hpp"
#include "concepts/axis_q1_queuing_cache_engine_permutation_concept.hpp"
#include "concepts/axis_q1_queuing_batched_insertable_strategy_concept.hpp"
#include "../concepts/topic_queuing_concept.hpp"

#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::queuing::axis_q1_queuing {

class BatchedInsertBuffer : public BufferStrategyBase<BatchedInsertBuffer> {
public:
    static constexpr bool enabled = flags::batched_insert_buffer_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::batched_access_tag;
    using family_id    = std::integral_constant<int, 12>; // Q12

    static constexpr std::size_t kDefaultBatchSize = 64; // Cache-Line-tauglich

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr bool             is_bounded() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      default_capacity() noexcept { return 0; } // unbounded
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "batched_insert_buffer"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::queuing::axis_q1_queuing::BatchedInsertBuffer",
                                  "topics/queuing/axis_q1_queuing/axis_q1_queuing_batched_insert_buffer.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "BatchedInsertBuffer (OLAP-Index, ART-Bulk-Insert — Leis ICDE 2013 §6)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "BATCHED_INSERT_BUFFER"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1";

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_priority_ordering() noexcept { return false; }
    [[nodiscard]] static constexpr bool is_versioned() noexcept { return false; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    BatchedInsertBuffer() : batch_size_(kDefaultBatchSize) {}
    explicit BatchedInsertBuffer(std::size_t batch_size)
        : batch_size_(batch_size == 0 ? kDefaultBatchSize : batch_size) {}

    [[nodiscard]] bool operator==(BatchedInsertBuffer const& other) const noexcept { return size() == other.size(); }

    /// SONDERFALL [[allocation-failure-exception]]: push_back wirft std::bad_alloc bei OOM.
    /// Bei Erreichen der Batch-Groesse: aktuellen Sub-Batch in completed_ verschieben.
    void put(element_type v) {
        current_batch_.push_back(v);
        if (current_batch_.size() >= batch_size_) {
            completed_batches_.emplace_back(std::move(current_batch_));
            current_batch_.clear();
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        std::size_t total = size();
        if (total > stats_.peak_size) stats_.peak_size = total;
        observer_.notify(stats_);
#endif
    }

    /// QS5-spezifisch: append eines kompletten Sub-Batches in einem Schritt
    /// (amortisiert effizienter als span.size() × put()).
    /// SONDERFALL [[allocation-failure-exception]]: vector::insert + emplace_back
    /// koennen std::bad_alloc werfen.
    void bulk_insert(std::span<element_type const> batch) {
        if (batch.empty()) return;
        current_batch_.insert(current_batch_.end(), batch.begin(), batch.end());
        while (current_batch_.size() >= batch_size_) {
            std::vector<element_type> overflow{current_batch_.begin() + batch_size_, current_batch_.end()};
            current_batch_.resize(batch_size_);
            completed_batches_.emplace_back(std::move(current_batch_));
            current_batch_ = std::move(overflow);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_.total_put_count += batch.size();
        std::size_t total = size();
        if (total > stats_.peak_size) stats_.peak_size = total;
        observer_.notify(stats_);
#endif
    }

    /// Drain: zuerst aus completed_batches_, dann aus current_batch_ (FIFO).
    [[nodiscard]] std::optional<element_type> get() {
        if (!completed_batches_.empty()) {
            auto& head_batch = completed_batches_.front();
            if (!head_batch.empty()) {
                element_type v = head_batch.front();
                head_batch.erase(head_batch.begin());
                if (head_batch.empty()) { completed_batches_.erase(completed_batches_.begin()); }
#ifdef COMDARE_CE_ENABLE_STATISTICS
                ++stats_.total_get_count;
                observer_.notify(stats_);
#endif
                return v;
            }
            completed_batches_.erase(completed_batches_.begin());
        }
        if (current_batch_.empty()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.underflow_count;
            observer_.notify(stats_);
#endif
            return std::nullopt;
        }
        element_type v = current_batch_.front();
        current_batch_.erase(current_batch_.begin());
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_get_count;
        observer_.notify(stats_);
#endif
        return v;
    }

    [[nodiscard]] size_type size() const noexcept {
        std::size_t total = current_batch_.size();
        for (auto const& b : completed_batches_) total += b.size();
        return total;
    }
    [[nodiscard]] bool is_empty() const noexcept { return size() == 0; }
    void               clear() noexcept {
        current_batch_.clear();
        completed_batches_.clear();
    }

    // std::queue-API: peek_front=erstes aus completed (oder current), peek_back=letztes current.
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept {
        for (auto const& b : completed_batches_) {
            if (!b.empty()) return b.front();
        }
        if (!current_batch_.empty()) return current_batch_.front();
        return std::nullopt;
    }
    [[nodiscard]] std::optional<element_type> peek_back() const noexcept {
        if (!current_batch_.empty()) return current_batch_.back();
        for (auto it = completed_batches_.rbegin(); it != completed_batches_.rend(); ++it) {
            if (!it->empty()) return it->back();
        }
        return std::nullopt;
    }
    void emplace(element_type v) { put(v); }

    /// Batch-spezifisch: Anzahl der vollstaendig befuellten Sub-Batches (bereit fuer Bulk-Drain).
    [[nodiscard]] std::size_t completed_batch_count() const noexcept { return completed_batches_.size(); }
    [[nodiscard]] std::size_t batch_size() const noexcept { return batch_size_; }

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
    std::vector<element_type>              current_batch_;
    std::vector<std::vector<element_type>> completed_batches_;
    std::size_t                            batch_size_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t                 observer_{};
#endif
};

} // namespace comdare::cache_engine::queuing::axis_q1_queuing

namespace comdare::cache_engine::queuing::axis_q1_queuing {
static_assert(concepts::BufferStrategy<BatchedInsertBuffer>);
static_assert(concepts::CacheEngineBufferPermutationStrategy<BatchedInsertBuffer>);
static_assert(concepts::BatchedInsertableStrategy<BatchedInsertBuffer>);
} // namespace comdare::cache_engine::queuing::axis_q1_queuing
