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
// Allocation: A8-S5 SCHNITT-FORM (B), 2026-08-05 -- der Slot-Speicher haengt an der Allokator-ACHSE
// (axis_06). FORM-ENTSCHEID am Objekt: Form A (heap-frei) scheidet aus -- die Slot-Reihe waechst mit
// jedem put() und schrumpft erst beim Inline-Compact; is_bounded()==false. BESONDERHEIT dieses
// Organs: der Element-Typ des Containers ist std::optional<element_type> (der Tombstone IST das
// leere optional), der Adapter wird also auf std::optional<element_type> gebunden, nicht auf
// element_type -- die Tombstone-Semantik bleibt unberuehrt.
// [[allocation-failure-exception]]: der Wurf kommt seit diesem Schnitt vom StdAllocatorAdapter der
// Achse (Posten 64), nicht mehr vom Default-Allokator.

#include "axis_q1_queuing_axis_storage.hpp"
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

    /// A8-S5 SCHNITT-FORM (B): DIESE Zeile ist der Ausweis, den die Familien-Konformitaets-Wache
    /// liest (tests/unit/s5_family_alloc_conformance.hpp). Achsen-Default: axis_q1_queuing_axis_storage.hpp.
    using allocator_type = queuing_buffer_allocator_t;

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
    static constexpr std::string_view algo_version = "1.0.0.c";

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_priority_ordering() noexcept { return false; }
    /// SONDERFALL: versioned via Tombstone-Marker (anders als DeltaChainBuffer Append-Version).
    [[nodiscard]] static constexpr bool                        is_versioned() noexcept { return true; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    /// Die Slot-Reihe wird an das EIGENE allocator_ gebunden (der Adapter haelt &allocator_).
    TombstoneBuffer() : slots_(allocator_.template as_std_allocator<slot_type>()) {}

    /// COW-SICHERHEIT (Memento-Muster, Praezedenz btree_node_pool_store.hpp:86): Copy-Ctor/Assign
    /// rebinden an das EIGENE allocator_ und verwerfen die transiente Kopier-Pollution per
    /// restore_statistics. MOVE bewusst NICHT deklariert (Fremd-Zeiger im Adapter).
    TombstoneBuffer(TombstoneBuffer const& o)
        : allocator_(o.allocator_), slots_(o.slots_, allocator_.template as_std_allocator<slot_type>()),
          drain_pos_(o.drain_pos_), live_count_(o.live_count_), version_counter_(o.version_counter_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_    = o.stats_;
        observer_ = o.observer_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    TombstoneBuffer& operator=(TombstoneBuffer const& o) {
        if (this != &o) {
            slots_           = o.slots_; // POCCA=false -> eigener Adapter bleibt erhalten
            drain_pos_       = o.drain_pos_;
            live_count_      = o.live_count_;
            version_counter_ = o.version_counter_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_    = o.stats_;
            observer_ = o.observer_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~TombstoneBuffer() = default;

    [[nodiscard]] bool operator==(TombstoneBuffer const& other) const noexcept {
        return live_count_ == other.live_count_;
    }

    /// SONDERFALL [[allocation-failure-exception]]: push_back kann werfen -- seit dem A8-S5-Schnitt aus
    /// dem StdAllocatorAdapter der Allokator-ACHSE (Posten 64), nicht mehr vom Default-Allokator.
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

    /// EINSAMMEL-NAHT der T6-Durchbindung (Owner-KERN abend-11) -- EIGENER Name, DISJUNKT zum
    /// konstitutiven Store-Snapshot; die Summierungs-Frage gehoert ins Mess-Schnitt-Fenster.
    using allocator_snapshot_t = typename allocator_type::snapshot_t;
    [[nodiscard]] allocator_snapshot_t buffer_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    /// Der Tombstone IST das leere optional -- deshalb bindet der Achsen-Adapter auf DIESEN Typ.
    using slot_type  = std::optional<element_type>;
    using slots_type = std::vector<slot_type, queuing_buffer_alloc_t<slot_type>>;

    // allocator_ MUSS VOR slots_ stehen (Adapter haelt &allocator_) -- Ordnung wie 01a/01c/01d.
    allocator_type allocator_{};
    slots_type     slots_;
    std::size_t    drain_pos_       = 0;
    std::size_t    live_count_      = 0;
    std::uint64_t  version_counter_ = 0;
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
