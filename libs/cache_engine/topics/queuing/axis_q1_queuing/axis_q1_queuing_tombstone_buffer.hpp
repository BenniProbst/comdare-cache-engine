#pragma once
// V41.F.6.1 axis_q1_queuing TombstoneBuffer Q-TOMB (2026-05-26)
//
// @topic queuing @achse Q1 @family Q09 TombstoneBuffer
// @subaxis QS4 versioned_access
//
// Versionierter Buffer mit Tombstone-Marker-Pattern (LSM + MVCC + ART-Optimistik):
// jeder put() fuegt einen Live-Eintrag an; deleted-Eintraege bleiben als
// Tombstones (std::nullopt) bestehen, bis ein Compact-Lauf sie eliminiert.
//
// get() ueberspringt Tombstones automatisch und liefert das aelteste Live-Element
// (FIFO-Drain ueber Live-Slots). size() zaehlt nur Live-Slots; tombstone_count()
// liefert die zusaetzlichen Tombstone-Bytes (Compact-Trigger-Metrik).
//
// **2. Strategie mit is_versioned()=true** (nach DeltaChainBuffer) — anders aber:
// DeltaChainBuffer = Append-Versioning, TombstoneBuffer = Erase-Versioning.
//
// Allocation: std::vector<std::optional<...>> — wirft std::bad_alloc bei OOM
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

class TombstoneBuffer : public BufferStrategyBase<TombstoneBuffer> {
public:
    static constexpr bool enabled = flags::tombstone_buffer_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::versioned_access_tag;
    using family_id    = std::integral_constant<int, 9>; // Q09

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr bool             is_bounded() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      default_capacity() noexcept { return 0; } // unbounded
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "tombstone_buffer"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::queuing::axis_q1_queuing::TombstoneBuffer",
                                  "topics/queuing/axis_q1_queuing/axis_q1_queuing_tombstone_buffer.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "TombstoneBuffer (LSM + MVCC + ART-Optimistik)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "TOMBSTONE_BUFFER"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_priority_ordering() noexcept { return false; }
    /// SONDERFALL: versioned via Tombstone-Marker (anders als DeltaChainBuffer Append-Version).
    [[nodiscard]] static constexpr bool                        is_versioned() noexcept { return true; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    [[nodiscard]] bool operator==(TombstoneBuffer const& other) const noexcept {
        return live_count_ == other.live_count_;
    }

    /// SONDERFALL [[allocation-failure-exception]]: push_back kann std::bad_alloc werfen.
    void put(element_type v) {
        slots_.push_back(std::optional<element_type>{v});
        ++live_count_;
        ++version_counter_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        if (live_count_ > stats_.peak_size) stats_.peak_size = live_count_;
        observer_.notify(stats_);
#endif
    }

    /// FIFO-Drain ueber Live-Slots; Tombstones werden uebersprungen + bei
    /// vollem Drain entfernt (Compact-Inline).
    [[nodiscard]] std::optional<element_type> get() {
        while (drain_pos_ < slots_.size() && !slots_[drain_pos_].has_value()) {
            ++drain_pos_; // Tombstone ueberspringen
        }
        if (drain_pos_ >= slots_.size()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.underflow_count;
            observer_.notify(stats_);
#endif
            return std::nullopt;
        }
        element_type v = *slots_[drain_pos_];
        slots_[drain_pos_].reset(); // markiere als Tombstone NACH get()
        ++drain_pos_;
        --live_count_;
        ++version_counter_;
        // Inline-Compact bei vollem Drain
        if (drain_pos_ >= slots_.size()) {
            slots_.clear();
            drain_pos_ = 0;
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_get_count;
        observer_.notify(stats_);
#endif
        return v;
    }

    [[nodiscard]] size_type size() const noexcept { return live_count_; }
    [[nodiscard]] bool      is_empty() const noexcept { return live_count_ == 0; }
    void                    clear() noexcept {
        slots_.clear();
        drain_pos_  = 0;
        live_count_ = 0;
    }

    // std::queue-API: peek_front=erstes Live, peek_back=letztes Live.
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept {
        for (std::size_t i = drain_pos_; i < slots_.size(); ++i) {
            if (slots_[i].has_value()) return slots_[i];
        }
        return std::nullopt;
    }
    [[nodiscard]] std::optional<element_type> peek_back() const noexcept {
        for (std::size_t i = slots_.size(); i > drain_pos_; --i) {
            if (slots_[i - 1].has_value()) return slots_[i - 1];
        }
        return std::nullopt;
    }
    void emplace(element_type v) { put(v); }

    /// Tombstone-spezifisch: aktuelle Anzahl ungelebter Tombstone-Slots.
    [[nodiscard]] std::size_t tombstone_count() const noexcept { return (slots_.size() - drain_pos_) - live_count_; }

    /// VersionedBufferStrategy [[versioned-strategy]]: monoton steigender Operation-Counter.
    /// Inkrementiert bei jedem put()/get() (Tombstone-Operation-Versioning, MVCC-Snapshot-Marker).
    [[nodiscard]] std::uint64_t version_id() const noexcept { return version_counter_; }

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
    std::vector<std::optional<element_type>> slots_;
    std::size_t                              drain_pos_       = 0;
    std::size_t                              live_count_      = 0;
    std::uint64_t                            version_counter_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t                 observer_{};
#endif
};

} // namespace comdare::cache_engine::queuing::axis_q1_queuing

namespace comdare::cache_engine::queuing::axis_q1_queuing {
static_assert(concepts::BufferStrategy<TombstoneBuffer>);
static_assert(concepts::CacheEngineBufferPermutationStrategy<TombstoneBuffer>);
static_assert(concepts::VersionedBufferStrategy<TombstoneBuffer>);
} // namespace comdare::cache_engine::queuing::axis_q1_queuing
