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
// Allocation: A8-S5 SCHNITT-FORM (B), 2026-08-05 -- BEIDE Allokationen dieses Organs haengen an der
// Allokator-ACHSE (axis_06). FORM-ENTSCHEID am Objekt: Form A (heap-frei) scheidet aus -- der Zustand
// ist per Konstruktion ein geteilter Heap-Snapshot, das IST das Organ.
//
// ZWEI Allokationen, nicht eine -- das ist der Befund, den make_shared verdeckt hatte:
//   (1) der SNAPSHOT-Block selbst (Kontrollblock + Vektor-Objekt) -> std::allocate_shared mit dem
//       Achsen-Adapter auf den Snapshot-Typ statt std::make_shared (das nimmt IMMER ::operator new);
//   (2) der ELEMENT-Puffer INNERHALB des Vektors -> der Vektor wird selbst mit dem Achsen-Adapter
//       auf element_type konstruiert. Ohne (2) haette (1) allein nur die Huelle gebunden und der
//       eigentliche Inhalt liefe weiter am T6-Zaehler vorbei.
//
// COW-KOPIE (die Falle dieses Organs): eine Organ-Kopie darf den Snapshot NICHT einfach TEILEN. Der
// geteilte Block gehoert dem allocator_ der QUELLE; ueberlebte die Kopie die Quelle, deallozierte der
// letzte shared_ptr ueber eine bereits zerstoerte Strategie. Der Copy-Ctor legt deshalb einen EIGENEN
// Snapshot an -- die Snapshot-Semantik nach aussen (gleicher Inhalt, gleiche Version) ist identisch.
// [[allocation-failure-exception]]: der Wurf kommt seit diesem Schnitt vom StdAllocatorAdapter der
// Achse (Posten 64), nicht mehr vom Default-Allokator.

#include "axis_q1_queuing_axis_storage.hpp"
#include "axis_q1_queuing_base.hpp"
#include "axis_q1_queuing_subaxes_qs1_to_qs6.hpp"
#include "concepts/axis_q1_queuing_concept.hpp"
#include "concepts/axis_q1_queuing_cache_engine_permutation_concept.hpp"
#include "concepts/axis_q1_queuing_versioned_strategy_concept.hpp"
#include <topics/queuing/concepts/topic_queuing_concept.hpp>

#include <organ_axes/axis_q1_queuing/axis_q1_queuing_flags.hpp>
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

    /// A8-S5 SCHNITT-FORM (B): DIESE Zeile ist der Ausweis, den die Familien-Konformitaets-Wache
    /// liest (tests/unit/s5_family_alloc_conformance.hpp). Achsen-Default: axis_q1_queuing_axis_storage.hpp.
    using allocator_type = queuing_buffer_allocator_t;

    /// Der Snapshot-Typ ist Teil der oeffentlichen Flaeche (snapshot_ptr()) -- deshalb hier, nicht
    /// unten: er traegt den Achsen-Adapter sichtbar, statt ihn im privaten Teil zu verstecken.
    using snapshot_vector_type = std::vector<element_type, queuing_buffer_alloc_t<element_type>>;

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr bool             is_bounded() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      default_capacity() noexcept { return 0; } // unbounded
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "copy_on_write"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::queuing::axis_q1_queuing::CopyOnWriteBuffer",
                                  "organ_axes/axis_q1_queuing/axis_q1_queuing_copy_on_write.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "CopyOnWriteBuffer (Persistent ART, RCU-Tries — Driscoll/Sarnak/Sleator/Tarjan JCSS 1989)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "COPY_ON_WRITE"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

    [[nodiscard]] static constexpr bool supports_concurrent_producers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_concurrent_consumers() noexcept { return false; }
    [[nodiscard]] static constexpr bool supports_priority_ordering() noexcept { return false; }
    /// SONDERFALL: 3. Strategie mit Versionierung=TRUE (Snapshot-Versioning).
    [[nodiscard]] static constexpr bool                        is_versioned() noexcept { return true; }
    [[nodiscard]] static constexpr concepts::ProgressGuarantee progress_guarantee() noexcept {
        return concepts::ProgressGuarantee::Blocking;
    }

    // (F57/Muster B, WP-5 2026-07-16): der EINE unveraenderliche Leer-Snapshot wird HIER (nicht-noexcept)
    // vorab alloziert — clear() teilt ihn nur noch (shared_ptr-Copy-Assign, allokationsfrei). Teilen ist
    // CoW-korrekt: put()/get() mutieren *current_ NIE in place (immer make_shared-Neukopie).
    CopyOnWriteBuffer() : empty_snapshot_(make_snapshot()), current_(empty_snapshot_) {}

    /// COW-SICHERHEIT -- hier woertlich (Memento-Muster, Praezedenz btree_node_pool_store.hpp:86):
    /// die Kopie bekommt EIGENE Snapshots aus dem EIGENEN allocator_ statt die der Quelle zu teilen.
    /// Teilen waere die Falle: der geteilte Block gehoert der Quell-Strategie, und ueberlebte die
    /// Kopie die Quelle, deallozierte der letzte shared_ptr ueber eine zerstoerte Strategie.
    /// Nach aussen ist die Snapshot-Semantik unveraendert (gleicher Inhalt, gleiche Version).
    /// MOVE bewusst NICHT deklariert (Fremd-Zeiger im Adapter).
    CopyOnWriteBuffer(CopyOnWriteBuffer const& o)
        : allocator_(o.allocator_), empty_snapshot_(make_snapshot()),
          current_(o.current_->empty() ? empty_snapshot_ : make_snapshot(*o.current_)),
          snapshot_version_(o.snapshot_version_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_    = o.stats_;
        observer_ = o.observer_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    CopyOnWriteBuffer& operator=(CopyOnWriteBuffer const& o) {
        if (this != &o) {
            current_          = o.current_->empty() ? empty_snapshot_ : make_snapshot(*o.current_);
            snapshot_version_ = o.snapshot_version_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_    = o.stats_;
            observer_ = o.observer_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~CopyOnWriteBuffer() = default;

    [[nodiscard]] bool operator==(CopyOnWriteBuffer const& other) const noexcept {
        return current_->size() == other.current_->size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: shared_ptr-Allokation +
    /// vector-Kopie koennen std::bad_alloc werfen.
    /// CoW-Semantik: erstelle neuen Snapshot durch Vollkopie des aktuellen + Append.
    void put(element_type v) {
        auto next = make_snapshot(*current_);
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
        auto         next = make_snapshot(current_->begin() + 1, current_->end());
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
    // (F57/Muster B, WP-5 2026-07-16): der Gattungs-Concept (BufferStrategy) verlangt clear() noexcept —
    // statt noexcept zu entfernen wird die Allokation HERAUSGEHOBEN: der unveraenderliche Leer-Snapshot
    // ist im Konstruktor vorab alloziert; clear() ist eine allokationsfreie shared_ptr-Zuweisung
    // (vorher: make_shared unter noexcept = terminate-on-OOM, Muster B).
    void clear() noexcept {
        current_ = empty_snapshot_;
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
    /// A8-S5: der Rueckgabe-Typ traegt jetzt den Achsen-Adapter (snapshot_vector_type). Das ist die
    /// EINE bewusste Aenderung an der oeffentlichen Flaeche dieses Organs -- sie ist unvermeidlich,
    /// weil der Snapshot selbst der Achsen-Speicher IST. In-Tree gibt es genau NULL weitere
    /// Konsumenten (grep 'snapshot_ptr' ueber das ganze Repo: nur diese Zeile).
    [[nodiscard]] std::shared_ptr<snapshot_vector_type const> snapshot_ptr() const noexcept { return current_; }

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

#ifdef COMDARE_CE_ENABLE_STATISTICS
    /// EINSAMMEL-NAHT der T6-Durchbindung (Owner-KERN abend-11) -- EIGENER Name, DISJUNKT zum
    /// konstitutiven Store-Snapshot; die Summierungs-Frage gehoert ins Mess-Schnitt-Fenster.
    /// Der Alias liegt IM Statistik-Block, weil die Achsen-Strategie ihren snapshot_t auch nur dort
    /// fuehrt -- ausserhalb gibt es schlicht keine Statistik, die man einsammeln koennte.
    using allocator_snapshot_t = typename allocator_type::snapshot_t;
    [[nodiscard]] allocator_snapshot_t buffer_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    using snapshot_ptr_type = std::shared_ptr<snapshot_vector_type>;

    /// BEIDE Allokationen ueber die Achse: allocate_shared bindet den Snapshot-BLOCK (Kontrollblock +
    /// Vektor-Objekt), das durchgereichte as_std_allocator<element_type>() bindet den ELEMENT-Puffer
    /// im Vektor. std::make_shared koennte nur das erste -- und auch das nur ueber ::operator new.
    [[nodiscard]] snapshot_ptr_type make_snapshot() {
        return std::allocate_shared<snapshot_vector_type>(allocator_.template as_std_allocator<snapshot_vector_type>(),
                                                          allocator_.template as_std_allocator<element_type>());
    }
    [[nodiscard]] snapshot_ptr_type make_snapshot(snapshot_vector_type const& src) {
        return make_snapshot(src.begin(), src.end());
    }
    template <class It>
    [[nodiscard]] snapshot_ptr_type make_snapshot(It first, It last) {
        return std::allocate_shared<snapshot_vector_type>(allocator_.template as_std_allocator<snapshot_vector_type>(),
                                                          first, last,
                                                          allocator_.template as_std_allocator<element_type>());
    }

    // allocator_ MUSS VOR den Snapshots stehen (die Adapter halten &allocator_, und die Bloecke
    // muessen VOR der Strategie sterben) -- Ordnung wie 01a/01c/01d.
    allocator_type allocator_{};
    // (F57/Muster B): unveraenderlicher, im ctor vorab allozierter Leer-Snapshot fuer das noexcept-clear().
    // VOR current_ deklariert (Initialisierungsreihenfolge: current_ startet als Alias des Leer-Snapshots).
    snapshot_ptr_type empty_snapshot_;
    snapshot_ptr_type current_;
    std::uint64_t     snapshot_version_ = 0;
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
