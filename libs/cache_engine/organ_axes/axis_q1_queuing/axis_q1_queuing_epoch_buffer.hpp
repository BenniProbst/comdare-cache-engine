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
// iterable_aspect_t** (nach BoundedRingBuffer-Capacity).
//
// **4. Strategie mit is_versioned()=true** (Epoch-Versioning, anders als
// DeltaChainBuffer/Tombstone/CoW). Konzeptioneller Unterschied:
//   - DeltaChainBuffer     = Append-Versioning
//   - TombstoneBuffer = Erase-Versioning
//   - CopyOnWriteBuffer     = Snapshot-Versioning
//   - EpochBuffer     = Reclamation-Window-Versioning (zeitlich begrenzt)
//
// Allocation: A8-S5 SCHNITT-FORM (B), 2026-08-05 -- BEIDE Epochen-Slots haengen an der
// Allokator-ACHSE (axis_06). FORM-ENTSCHEID am Objekt: Form A (heap-frei) scheidet aus -- die
// aktuelle Epoche waechst bis zum epoch_threshold_, und der ist ein LAUFZEIT-Aspekt
// (iterable_aspect_t, per set_iterable_aspect frei setzbar); eine Compile-Time-Kappe gibt es hier
// also nicht, sie waere eine Erfindung. [[allocation-failure-exception]]: der Wurf kommt seit
// diesem Schnitt vom StdAllocatorAdapter der Achse (Posten 64), nicht mehr vom Default-Allokator.
// BEIDE Epochen haengen am SELBEN allocator_ -- nur deshalb bleibt der Epoch-Advance
// (retired_epoch_ = std::move(current_epoch_)) ein reiner Zeiger-Transfer: die Adapter beider
// Vektoren vergleichen gleich, der Move stiehlt den Block statt elementweise zu kopieren.

#include "axis_q1_queuing_axis_storage.hpp"
#include "axis_q1_queuing_base.hpp"
#include "axis_q1_queuing_subaxes_qs1_to_qs6.hpp"
#include "concepts/axis_q1_queuing_concept.hpp"
#include "concepts/axis_q1_queuing_cache_engine_permutation_concept.hpp"
#include "concepts/axis_q1_queuing_versioned_strategy_concept.hpp"
#include "concepts/axis_q1_queuing_iterable_aspect_strategy_concept.hpp"
#include <topics/queuing/concepts/topic_queuing_concept.hpp>

#include <organ_axes/axis_q1_queuing/axis_q1_queuing_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::queuing::axis_q1_queuing {

class EpochBuffer : public BufferStrategyBase<EpochBuffer> {
public:
    static constexpr bool enabled = flags::epoch_buffer_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::versioned_access_tag;
    using family_id    = std::integral_constant<int, 11>; // Q11

    /// A8-S5 SCHNITT-FORM (B): DIESE Zeile ist der Ausweis, den die Familien-Konformitaets-Wache
    /// liest (tests/unit/s5_family_alloc_conformance.hpp). Achsen-Default: axis_q1_queuing_axis_storage.hpp.
    using allocator_type = queuing_buffer_allocator_t;

    /// iterable_aspect_t (F.6.1.E hybride Laufzeit-Permutation, [[no-runtime-switch]] Ausnahme)
    /// PermutationEngine erkennt via HasIterableAspect<V> und generiert
    /// 1 Binary mit Runtime-Loop ueber kIterableEpochThresholds.
    using iterable_aspect_t = std::size_t;
    static constexpr std::array<std::size_t, 5>                 kIterableEpochThresholds{2u, 4u, 8u, 16u, 64u};
    [[nodiscard]] static constexpr std::span<std::size_t const> iterable_values() noexcept {
        return std::span<std::size_t const>{kIterableEpochThresholds.data(), kIterableEpochThresholds.size()};
    }

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr bool             is_bounded() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      default_capacity() noexcept { return 0; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "epoch_buffer"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::queuing::axis_q1_queuing::EpochBuffer",
                                  "organ_axes/axis_q1_queuing/axis_q1_queuing_epoch_buffer.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "EpochBuffer (QSBR — McKenney OLS 2001, SMART ART OSDI 2023, Masstree EuroSys 2012)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "EPOCH_BUFFER"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_priority_ordering() noexcept { return false; }
    /// SONDERFALL: 4. Strategie mit Versionierung=TRUE (Reclamation-Window-Versioning).
    [[nodiscard]] static constexpr bool                        is_versioned() noexcept { return true; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    /// Beide Epochen-Vektoren werden an das EIGENE allocator_ gebunden (der Adapter haelt &allocator_).
    EpochBuffer() : EpochBuffer(kIterableEpochThresholds[2]) {} // Default = 8
    explicit EpochBuffer(std::size_t threshold)
        : current_epoch_(allocator_.template as_std_allocator<element_type>()),
          retired_epoch_(allocator_.template as_std_allocator<element_type>()),
          epoch_threshold_(threshold == 0 ? 1u : threshold) {}

    /// COW-SICHERHEIT (Memento-Muster, Praezedenz btree_node_pool_store.hpp:86): Copy-Ctor/Assign
    /// rebinden BEIDE Epochen an das EIGENE allocator_ und verwerfen die transiente Kopier-Pollution
    /// per restore_statistics. MOVE bewusst NICHT deklariert (Fremd-Zeiger im Adapter).
    EpochBuffer(EpochBuffer const& o)
        : allocator_(o.allocator_),
          current_epoch_(o.current_epoch_, allocator_.template as_std_allocator<element_type>()),
          retired_epoch_(o.retired_epoch_, allocator_.template as_std_allocator<element_type>()),
          epoch_threshold_(o.epoch_threshold_), epoch_id_(o.epoch_id_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_    = o.stats_;
        observer_ = o.observer_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    EpochBuffer& operator=(EpochBuffer const& o) {
        if (this != &o) {
            current_epoch_   = o.current_epoch_; // POCCA=false -> eigene Adapter bleiben erhalten
            retired_epoch_   = o.retired_epoch_;
            epoch_threshold_ = o.epoch_threshold_;
            epoch_id_        = o.epoch_id_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_    = o.stats_;
            observer_ = o.observer_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~EpochBuffer() = default;

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

    [[nodiscard]] size_type size() const noexcept { return current_epoch_.size() + retired_epoch_.size(); }
    [[nodiscard]] bool      is_empty() const noexcept { return current_epoch_.empty() && retired_epoch_.empty(); }
    void                    clear() noexcept {
        current_epoch_.clear();
        retired_epoch_.clear();
    }

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
    using epoch_type = std::vector<element_type, queuing_buffer_alloc_t<element_type>>;

    // allocator_ MUSS VOR den Epochen stehen (Adapter haelt &allocator_) -- Ordnung wie 01a/01c/01d.
    allocator_type allocator_{};
    epoch_type     current_epoch_;
    epoch_type     retired_epoch_;
    std::size_t    epoch_threshold_;
    std::uint64_t  epoch_id_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t                 observer_{};
#endif
};

} // namespace comdare::cache_engine::queuing::axis_q1_queuing

namespace comdare::cache_engine::queuing::axis_q1_queuing {
static_assert(concepts::BufferStrategy<EpochBuffer>);
static_assert(concepts::CacheEngineBufferPermutationStrategy<EpochBuffer>);
static_assert(concepts::VersionedBufferStrategy<EpochBuffer>);
static_assert(concepts::IterableAspectStrategy<EpochBuffer>);
} // namespace comdare::cache_engine::queuing::axis_q1_queuing
