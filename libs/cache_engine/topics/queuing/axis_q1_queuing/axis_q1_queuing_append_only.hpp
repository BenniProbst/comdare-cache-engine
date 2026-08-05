#pragma once
// V41.F.6.1 axis_q1_queuing AppendOnlyBuffer Q-APP (2026-05-26)
//
// @topic queuing @achse Q1 @family Q02 AppendOnlyBuffer (Linear)
// @subaxis QS1 sequential_access
//
// Append-Only Buffer: monoton wachsender Linear-Buffer ohne mittiges Loeschen.
// Anwendung: LSM-MemTable (Write-Buffer vor Compact), Bw-Tree Delta-Chain
// (Levandoski 2013). get() entfernt vom Anfang (FIFO-Drain). put() hat amortisiert
// O(1) — bei Heap-Wachstum kann std::bad_alloc fliegen ([[allocation-failure-exception]]).
//
// Unterschied zu FIFOQueueBuffer:
//   - FIFOQueueBuffer: std::deque, kann mittig effizient erweitert/entleert werden
//   - AppendOnlyBuffer: std::vector, OPTIMIERT fuer reine Append + Bulk-Drain (cache-freundlicher,
//     bessere Locality bei Sequenz-Scan; nicht fuer haeufige Single-get() optimiert)
//
// A8-S5 SCHNITT-FORM (B), 2026-08-05: der Append-Speicher haengt an der Allokator-ACHSE (axis_06).
// FORM-ENTSCHEID am Objekt: Form A (heap-frei) scheidet aus -- monoton wachsender Append-Puffer,
// erklaert unbounded. GRENZE DES SCHNITTS, ausdruecklich deklariert: drain_all() gibt weiterhin
// einen std::vector mit DEFAULT-Allokator zurueck. Das ist KEIN Organ-Zustand, sondern ein
// AUSGABE-Wert, der das Organ verlaesst und dessen Lebensdauer der Aufrufer bestimmt; er an die
// Organ-Strategie zu binden hiesse, dem Aufrufer einen Zeiger auf unser allocator_ mitzugeben, der
// das Organ ueberleben kann (genau die COW-Falle, gegen die die Copy-Ctoren unten stehen).
// [[allocation-failure-exception]]: der Wurf des BUFFERS kommt seit diesem Schnitt vom
// StdAllocatorAdapter der Achse (Posten 64); der Wurf der drain_all-AUSGABE bleibt Default-Allokator.

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
#include <string_view>
#include <type_traits>
#include <vector>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::queuing::axis_q1_queuing {

class AppendOnlyBuffer : public BufferStrategyBase<AppendOnlyBuffer> {
public:
    static constexpr bool enabled = flags::append_only_enabled;

    using element_type = std::uint64_t;
    using size_type    = std::size_t;
    using topic_tag    = ::comdare::cache_engine::queuing::concepts::QueuingTopicTag;
    using axis_tag     = subaxes::sequential_access_tag;
    using family_id    = std::integral_constant<int, 2>; // Q02

    /// A8-S5 SCHNITT-FORM (B): DIESE Zeile ist der Ausweis, den die Familien-Konformitaets-Wache
    /// liest (tests/unit/s5_family_alloc_conformance.hpp). Achsen-Default: axis_q1_queuing_axis_storage.hpp.
    using allocator_type = queuing_buffer_allocator_t;

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr bool             is_bounded() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      default_capacity() noexcept { return 0; } // unbounded
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "append_only"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::queuing::axis_q1_queuing::AppendOnlyBuffer",
                                  "topics/queuing/axis_q1_queuing/axis_q1_queuing_append_only.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "AppendOnlyBuffer (LSM-MemTable + Bw-Tree Delta-Chain, Levandoski 2013)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "APPEND_ONLY"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0c";

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_priority_ordering() noexcept { return false; }
    [[nodiscard]] static constexpr bool is_versioned() noexcept { return false; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    /// Der Vektor wird an das EIGENE allocator_ gebunden (der Adapter haelt &allocator_).
    AppendOnlyBuffer() : data_(allocator_.template as_std_allocator<element_type>()) {}

    /// COW-SICHERHEIT (Memento-Muster, Praezedenz btree_node_pool_store.hpp:86): Copy-Ctor/Assign
    /// rebinden an das EIGENE allocator_ und verwerfen die transiente Kopier-Pollution per
    /// restore_statistics. MOVE bewusst NICHT deklariert (Fremd-Zeiger im Adapter).
    AppendOnlyBuffer(AppendOnlyBuffer const& o)
        : allocator_(o.allocator_), data_(o.data_, allocator_.template as_std_allocator<element_type>()),
          drain_pos_(o.drain_pos_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_    = o.stats_;
        observer_ = o.observer_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    AppendOnlyBuffer& operator=(AppendOnlyBuffer const& o) {
        if (this != &o) {
            data_      = o.data_; // POCCA=false -> eigener Adapter bleibt erhalten
            drain_pos_ = o.drain_pos_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_    = o.stats_;
            observer_ = o.observer_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~AppendOnlyBuffer() = default;

    [[nodiscard]] bool operator==(AppendOnlyBuffer const& other) const noexcept {
        return data_.size() == other.data_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: push_back kann bei Reallocation werfen. Nicht
    /// noexcept. KAUSALITAET seit dem A8-S5-Schnitt (Posten 64/70): der Wurf kommt aus dem
    /// StdAllocatorAdapter der Allokator-ACHSE, nicht mehr vom Default-Allokator.
    void put(element_type v) {
        data_.push_back(v);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_put_count;
        if (data_.size() > stats_.peak_size) stats_.peak_size = data_.size();
        observer_.notify(stats_);
#endif
    }

    /// Drain vom Anfang (FIFO-Semantik). O(1) amortisiert via drain_pos_-Cursor (KEIN Element-Shift);
    /// bei vollstaendigem Drain wird der Vector freigegeben. Fuer Bulk-Drain siehe drain_all().
    [[nodiscard]] std::optional<element_type> get() {
        if (drain_pos_ >= data_.size()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.underflow_count;
            observer_.notify(stats_);
#endif
            return std::nullopt;
        }
        element_type v = data_[drain_pos_++];
        // Garbage-Collection bei vollem Drain: vector resetten um Speicher freizugeben
        if (drain_pos_ >= data_.size()) {
            data_.clear();
            drain_pos_ = 0;
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_get_count;
        observer_.notify(stats_);
#endif
        return v;
    }

    /// Bulk-Drain (Vollausbau): liefert ALLE noch nicht gedrainten Elemente in Append-Reihenfolge und
    /// leert den Buffer. O(M) fuer M = size() verbleibende Elemente in EINEM Zug statt Mx get()
    /// (cache-freundlich, erfuellt den dokumentierten Bulk-Drain-Zweck -- vgl. LSM-MemTable-Flush).
    ///
    /// A8-S5-FOLGE, DEKLARIERT STATT STILL (2026-08-05): der frueher vorhandene O(1)-Sonderweg fuer
    /// drain_pos_==0 (`out = std::move(data_)`, ganzer Buffer-Transfer) ENTFAELLT. Er war ein Move
    /// zwischen zwei Vektoren DESSELBEN Typs; seit dem Schnitt haengt data_ am Achsen-Adapter, die
    /// Rueckgabe bewusst NICHT (s. Datei-Kopf: der Aufrufer soll keinen Zeiger auf unser allocator_
    /// erben). Zwischen zwei verschiedenen Allokator-Typen gibt es keinen Buffer-Transfer -- beide
    /// Zweige kopieren jetzt elementweise. Das ist ein PERF-Pfad-Wechsel ohne Semantik-Aenderung
    /// (gleiche Elemente, gleiche Reihenfolge, gleicher Leer-Zustand danach); algo_version bleibt
    /// deshalb unveraendert, exakt nach der 01c-/S5-04-Praezedenz (Bump-Entscheid im Mess-/A13-Fenster).
    /// [[allocation-failure-exception]]: die AUSGABE waechst am Default-Allokator und kann werfen.
    [[nodiscard]] std::vector<element_type> drain_all() {
        std::vector<element_type> out;
        if (drain_pos_ < data_.size()) {
            out.assign(data_.begin() + static_cast<std::ptrdiff_t>(drain_pos_), data_.end());
        }
        data_.clear();
        drain_pos_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_.total_get_count += out.size();
        observer_.notify(stats_);
#endif
        return out;
    }

    [[nodiscard]] size_type size() const noexcept { return data_.size() - drain_pos_; }
    [[nodiscard]] bool      is_empty() const noexcept { return drain_pos_ >= data_.size(); }
    void                    clear() noexcept {
        data_.clear();
        drain_pos_ = 0;
    }

    // std::queue-API: peek_front=oldest non-drained, peek_back=newest
    [[nodiscard]] std::optional<element_type> peek_front() const noexcept {
        if (drain_pos_ >= data_.size()) return std::nullopt;
        return data_[drain_pos_];
    }
    [[nodiscard]] std::optional<element_type> peek_back() const noexcept {
        if (drain_pos_ >= data_.size()) return std::nullopt;
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

    /// EINSAMMEL-NAHT der T6-Durchbindung (Owner-KERN abend-11) -- EIGENER Name, DISJUNKT zum
    /// konstitutiven Store-Snapshot; die Summierungs-Frage gehoert ins Mess-Schnitt-Fenster.
    using allocator_snapshot_t = typename allocator_type::snapshot_t;
    [[nodiscard]] allocator_snapshot_t buffer_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    using data_type = std::vector<element_type, queuing_buffer_alloc_t<element_type>>;

    // allocator_ MUSS VOR data_ stehen (Adapter haelt &allocator_) -- Ordnung wie 01a/01c/01d.
    allocator_type allocator_{};
    data_type      data_;
    std::size_t    drain_pos_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    concepts::BufferStatistics stats_{};
    observer_t                 observer_{};
#endif
};

} // namespace comdare::cache_engine::queuing::axis_q1_queuing

namespace comdare::cache_engine::queuing::axis_q1_queuing {
static_assert(concepts::BufferStrategy<AppendOnlyBuffer>);
static_assert(concepts::CacheEngineBufferPermutationStrategy<AppendOnlyBuffer>);
} // namespace comdare::cache_engine::queuing::axis_q1_queuing
