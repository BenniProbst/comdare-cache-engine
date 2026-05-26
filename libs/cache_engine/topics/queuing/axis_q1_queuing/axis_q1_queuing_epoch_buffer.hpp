#pragma once
// V41.F.6.1 axis_q1_queuing EpochBuffer Q-EPOCH (2026-05-26)
//
// @topic queuing @achse Q1 @family Q11 EpochBuffer
// @subaxis QS4 versioned_access
//
// QSBR-Buffer (Quiescent-State-Based-Reclamation, McKenney OLS 2001 + Masstree
// EuroSys 2012 + SMART ART OSDI 2023): Eintraege werden in der aktuellen Epoche
// akkumuliert; nach Erreichen des epoch_threshold rueckt die Epoche vor und
// die vorherige Epoche kann reclaimed werden (quiescent).
//
// **iterable_aspect_t Sonderfall:** epoch_threshold ist iterable Aspekt fuer
// hybride Laufzeit-Permutation (Doku §15.5). PermutationEngine erkennt
// HasIterableAspect<EpochBuffer> und generiert 1 Binary mit Runtime-Loop ueber
// kIterableEpochThresholds statt 5 separate Binaries. **2. Strategie mit
// iterable_aspect_t** (nach BoundedRing-Capacity).
//
// **4. Strategie mit is_versioned()=true** (Epoch-Versioning, anders als
// DeltaChain/Tombstone/CoW). Konzeptioneller Unterschied:
//   - DeltaChain     = Append-Versioning
//   - TombstoneBuffer = Erase-Versioning
//   - CopyOnWrite     = Snapshot-Versioning
//   - EpochBuffer     = Reclamation-Window-Versioning (zeitlich begrenzt)
//
// Allocation: std::vector-basierte Epochen-Slots — wirft std::bad_alloc bei OOM
// ([[allocation-failure-exception]]).

#include "axis_q1_queuing_base.hpp"
#include "axis_q1_queuing_subaxes_qs1_to_qs6.hpp"
#include "concepts/axis_q1_queuing_concept.hpp"
#include "concepts/axis_q1_queuing_cache_engine_permutation_concept.hpp"
#include "concepts/axis_q1_queuing_versioned_strategy_concept.hpp"
#include "concepts/axis_q1_queuing_iterable_aspect_strategy_concept.hpp"
#include "../concepts/topic_queuing_concept.hpp"

#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::queuing::axis_q1_queuing {

class EpochBuffer : public BufferStrategyBase<EpochBuffer> {
public:
    static constexpr bool enabled = flags::epoch_buffer_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::versioned_access_tag;
    using family_id    = std::integral_constant<int, 11>;  // Q11

    /// iterable_aspect_t (F.6.1.E hybride Laufzeit-Permutation, [[no-runtime-switch]] Ausnahme)
    /// PermutationEngine erkennt via HasIterableAspect<V> und generiert
    /// 1 Binary mit Runtime-Loop ueber kIterableEpochThresholds.
    using iterable_aspect_t = std::size_t;
    static constexpr std::array<std::size_t, 5> kIterableEpochThresholds{2u, 4u, 8u, 16u, 64u};
    [[nodiscard]] static constexpr std::span<std::size_t const> iterable_values() noexcept {
        return std::span<std::size_t const>{kIterableEpochThresholds.data(), kIterableEpochThresholds.size()};
    }

    [[nodiscard]] static constexpr bool        is_thread_safe()    noexcept { return false; }
    [[nodiscard]] static constexpr bool        is_bounded()        noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t default_capacity()  noexcept { return 0; }
    [[nodiscard]] static constexpr std::string_view name()         noexcept { return "epoch_buffer"; }
    [[nodiscard]] static constexpr std::string_view family_name()  noexcept { return "EpochBuffer (QSBR — McKenney OLS 2001, SMART ART OSDI 2023, Masstree EuroSys 2012)"; }
    [[nodiscard]] static constexpr std::string_view flag_suffix()  noexcept { return "EPOCH_BUFFER"; }

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_priority_ordering()    noexcept { return false; }
    /// SONDERFALL: 4. Strategie mit Versionierung=TRUE (Reclamation-Window-Versioning).
    [[nodiscard]] static constexpr bool is_versioned()                  noexcept { return true; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    EpochBuffer() : epoch_threshold_(kIterableEpochThresholds[2]) {}  // Default = 8
    explicit EpochBuffer(std::size_t threshold) : epoch_threshold_(threshold == 0 ? 1u : threshold) {}

    [[nodiscard]] bool operator==(EpochBuffer const& other) const noexcept {
        return current_epoch_.size() == other.current_epoch_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: push_back wirft std::bad_alloc bei OOM.
    /// Epoche rueckt bei Erreichen des Thresholds vor — vorige Epoche wird reclaimed.
    void put(element_type v) {
        current_epoch_.push_back(v);
        if (current_epoch_.size() >= epoch_threshold_) {
            // Epoch-Advance: vorige Epoche wird reclaimed (Drain liegt jetzt in retired_epoch_)
            retired_epoch_ = std::move(current_epoch_);
            current_epoch_.clear();
            ++epoch_id_;
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        std::size_t total = current_epoch_.size() + retired_epoch_.size();
        if (total > stats_.peak_size) stats_.peak_size = total;
        observer_.notify(stats_);
#endif
    }

    /// Drain zuerst aus retired_epoch_, dann aus current_epoch_ (FIFO ueber Epochen-Grenzen).
    [[nodiscard]] std::optional<element_type> get() {
        if (!retired_epoch_.empty()) {
            element_type v = retired_epoch_.front();
            retired_epoch_.erase(retired_epoch_.begin());
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.total_get_count;
            observer_.notify(stats_);
#endif
            return v;
        }
        if (current_epoch_.empty()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.underflow_count;
            observer_.notify(stats_);
#endif
            return std::nullopt;
        }
        element_type v = current_epoch_.front();
        current_epoch_.erase(current_epoch_.begin());
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_get_count;
        observer_.notify(stats_);
#endif
        return v;
    }

    [[nodiscard]] size_type size()     const noexcept { return current_epoch_.size() + retired_epoch_.size(); }
    [[nodiscard]] bool      is_empty() const noexcept { return current_epoch_.empty() && retired_epoch_.empty(); }
    void                    clear()          noexcept { current_epoch_.clear(); retired_epoch_.clear(); }

    // std::queue-API: peek_front=erstes aus retired (oder current), peek_back=letztes current.
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept {
        if (!retired_epoch_.empty()) return retired_epoch_.front();
        if (!current_epoch_.empty()) return current_epoch_.front();
        return std::nullopt;
    }
    [[nodiscard]] std::optional<element_type> peek_back() const noexcept {
        if (!current_epoch_.empty()) return current_epoch_.back();
        if (!retired_epoch_.empty()) return retired_epoch_.back();
        return std::nullopt;
    }
    void emplace(element_type v) { put(v); }

    /// VersionedBufferStrategy [[versioned-strategy]]: monoton steigender Epoch-Counter.
    /// Inkrementiert bei jedem Epoch-Advance (Reclamation-Window-Versioning).
    [[nodiscard]] std::uint64_t version_id() const noexcept { return epoch_id_; }

    /// Setter fuer Runtime-Threshold-Switch ([[iterable-aspect-strategy]] Sub-Concept).
    /// Konsolidierter Name `set_iterable_aspect` analog allen anderen iterable Strategien.
    /// Akzeptiert 0 als Sentinel — wird auf 1 normalisiert (keine Exception bei Epoch).
    void set_iterable_aspect(std::size_t new_threshold) noexcept {
        epoch_threshold_ = (new_threshold == 0 ? 1u : new_threshold);
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::BufferStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot()   const noexcept { return stats_; }
    void reset() noexcept { stats_ = {}; observer_.notify(stats_); }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer()       noexcept { return observer_; }
#endif

private:
    std::vector<element_type> current_epoch_;
    std::vector<element_type> retired_epoch_;
    std::size_t epoch_threshold_;
    std::uint64_t epoch_id_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t observer_{};
#endif
};

}  // namespace

namespace comdare::cache_engine::queuing::axis_q1_queuing {
    static_assert(concepts::BufferStrategy<EpochBuffer>);
    static_assert(concepts::CacheEngineBufferPermutationStrategy<EpochBuffer>);
    static_assert(concepts::VersionedBufferStrategy<EpochBuffer>);
    static_assert(concepts::IterableAspectStrategy<EpochBuffer>);
}
