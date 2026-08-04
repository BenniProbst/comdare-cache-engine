#pragma once
// Adapter-Tier-Unterklasse der CONTAINER-Gattung (2026-06-03, #87+#90, AUTORITATIV Doku 14 §28 Invertebrate-Spalte
// + §26.4 + Doc 30 §8.0/§8.1). Ebene 2 unter dem Container-Außen-Interface (Ebene 1, AnatomyGattung::Container),
// gleichrangig zu Set/Sequence/View. Gebaut EXAKT analog SequenceComposition (10 geteilt + growth):
//
//   AdapterComposition<T0..T10, Inner>  =  11 geteilte/delegierte Achsen (§28; INC-2c)  +  inner_container (NEU axis_inner).
//
// §28-Invertebrate-Achsen (12; INC-2c): delegiert (9) search_algo, cache_traversal, memory_layout, allocator, prefetch,
// concurrency, isa, io_dispatch, migration_policy + aktiv (2) serialization, value_handle + spezifisch (1)
// inner_container. §26.4: stack/queue→deque, priority_queue→vector+Compare; Pflicht-API push/pop/top/front/back
// (KEIN begin/end). Die Disziplin (FIFO/LIFO/Priority) liegt in der API-NUTZUNG (front vs back), NICHT in einer Achse —
// §28 kennt KEINE „ordering"-Achse (frühere inner+ordering-Version war ein geratener Ebenen-/Achsen-Fehler, verworfen).
//
// NAMEN (#90-Sweep abgeschlossen): Datei adapter_anatomy.hpp + Typen AdapterComposition/AdapterAnatomy
// (konsistent mit set_/sequence_/view_; historisch container_anatomy.hpp / Container*). C++23, header-only.

// E-24 C3 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C3):
// PRODUKTIONSTIEFE, Schnitt identisch zu set_/sequence_/view_anatomy.hpp. Bis C2 hielt diese Anatomie genau
// ein reales Achsen-Organ (inner_container); die uebrigen zehn Slots existierten nur als Kompositions-TYPEN.
// C3 macht daraus reale Organ-Member mit Accessoren + die per-Achsen-Einsammlung nach dem SA-Muster
// (search_algorithm_anatomy.hpp:65-115).
//
// ABI-NEUTRAL (a-Teil): observe_all() liefert UNVERAENDERT den flachen AdapterObserverSnapshot, den
// adapter_abi_adapter.hpp in den Wire-POD spiegelt. Die per-Achsen-Sicht kommt ADDITIV als observe_axes()
// hinzu; ihre Promotion in die Wire-Ebene ist der ABI-Schritt C6 des b-Teils. kAdapterCompositionSlotCount
// (== 13, frozen legacy, vs. live slot_count == 11) bleibt BEWUSST unangetastet -- Aufraeumpass-Kandidat.

#include "anatomy_base.hpp"       // AnatomyGenus (Tier-Unterklasse) / AnatomyGattung
#include "observer_aggregate.hpp" // E-24 C3: ObservableAxis / snapshot_of_t (die ACHSEN-Ebene, SA-Muster)
#include "organ_concept.hpp"      // E-24 C1: OrganGuard (CRTP-Wache)

#include <algorithm> // std::push_heap / std::pop_heap (HeapInner = priority_queue-Disziplin)
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional> // std::less (HeapInner Default-Compare)
#include <optional>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::anatomy {

// ════════════════════════════════════════════════════════════════════════════════════════════════════════
// axis_inner (die EINE Adapter-spezifische Achse, §28) — das dekorierte Inner-Substrat. Pflicht-Ops für die
// §26.4-Adapter-API: push_back (ablegen), front/back (beide Enden lesen), pop_front/pop_back, size, clear.
// FIFO (queue) nutzt front+pop_front; LIFO (stack) nutzt back+pop_back — die Disziplin ist die API-Nutzung.
// ════════════════════════════════════════════════════════════════════════════════════════════════════════

/// DequeInner — Inner-Substrat über std::deque (Default für stack/queue, §26.4). Beide Enden O(1).
template <class T = std::uint64_t>
struct DequeInner {
    using element_type                     = T;
    static constexpr std::string_view name = "DequeInner";
    void                              push_back(element_type v) { d_.push_back(v); }
    [[nodiscard]] std::size_t         size() const noexcept { return d_.size(); }
    [[nodiscard]] element_type        front() const { return d_.front(); }
    [[nodiscard]] element_type        back() const { return d_.back(); }
    void                              pop_front() { d_.pop_front(); }
    void                              pop_back() { d_.pop_back(); }
    void                              clear() noexcept { d_.clear(); }

private:
    std::deque<element_type> d_{};
};

/// VectorInner — Inner-Substrat über std::vector (Default für priority_queue-Substrat, §26.4). pop_back O(1).
template <class T = std::uint64_t>
struct VectorInner {
    using element_type                     = T;
    static constexpr std::string_view name = "VectorInner";
    void                              push_back(element_type v) { v_.push_back(v); }
    [[nodiscard]] std::size_t         size() const noexcept { return v_.size(); }
    [[nodiscard]] element_type        front() const { return v_.front(); }
    [[nodiscard]] element_type        back() const { return v_.back(); }
    void                              pop_front() { v_.erase(v_.begin()); }
    void                              pop_back() { v_.pop_back(); }
    void                              clear() noexcept { v_.clear(); }

    /// E-24 C6-V: die ZAEHLENDE Variante von pop_front (Muster Strategy::index_org_scan_counted,
    /// axis_01_index_organization_observable.hpp:52-58). erase(begin) verschiebt REAL die uebrigen
    /// size()-1 Elemente um eine Position -- das ist die O(n)-Kostenklasse dieses Substrats, und genau
    /// diese real bewegte Element-Zahl wird dem Aufrufer zurueckgemeldet (honest-100%, #24 Option A).
    /// Nur die Huelle ObservableInnerContainer ruft sie; das Verhalten ist identisch zu pop_front().
    void pop_front_counted(std::uint64_t& elements_shifted) {
        if (!v_.empty()) elements_shifted += static_cast<std::uint64_t>(v_.size() - 1);
        v_.erase(v_.begin());
    }

private:
    std::vector<element_type> v_{};
};

/// HeapInner — Inner-Substrat mit echter PRIORITY-Disziplin (std::priority_queue, §26.4: vector + Compare). Hält
/// einen Max-Heap über std::vector via std::push_heap/std::pop_heap (Stand der Technik; Default-Compare std::less →
/// Max-Heap, front()==Maximum). Die Priority-Disziplin lebt damit INNERHALB der inner_container-Achse (§28) — KEINE
/// neue Achse. Priority-Nutzung: push + front()(=Max) + pop_front()(=Extract-Max). back()/pop_back() = Roh-Blatt
/// (Heap bleibt gültig, da pop_back ein Blatt entfernt) — nicht priority-relevant.
template <class T = std::uint64_t, class Compare = std::less<T>>
struct HeapInner {
    using element_type                     = T;
    static constexpr std::string_view name = "HeapInner";
    void                              push_back(element_type v) {
        v_.push_back(v);
        std::push_heap(v_.begin(), v_.end(), comp_);
    }
    [[nodiscard]] std::size_t  size() const noexcept { return v_.size(); }
    [[nodiscard]] element_type front() const { return v_.front(); } // Heap-Top = Maximum
    [[nodiscard]] element_type back() const { return v_.back(); }   // Blatt (nicht priority-relevant)
    void                       pop_front() {
        std::pop_heap(v_.begin(), v_.end(), comp_);
        v_.pop_back();
    } // Extract-Max
    void pop_back() { v_.pop_back(); } // entfernt ein Blatt (Heap bleibt gültig)
    void clear() noexcept { v_.clear(); }

    /// E-24 C6-V: die ZAEHLENDEN Varianten von push_back/pop_front. Die Sift-Arbeit von std::push_heap/
    /// std::pop_heap wird ueber einen zaehlenden Komparator gemessen: JEDER gezaehlte Schritt ist ein vom
    /// Heap-Algorithmus REAL ausgefuehrter Vergleich -- KEIN aus log2(n) geschaetzter Wert (#24 Option A).
    /// Das Ergebnis der Ops ist identisch zu push_back()/pop_front() (der Komparator delegiert an comp_).
    void push_back_counted(element_type v, std::uint64_t& sift_ops) {
        v_.push_back(v);
        std::uint64_t comparisons = 0;
        auto          counting    = [&](element_type const& a, element_type const& b) {
            ++comparisons;
            return comp_(a, b);
        };
        std::push_heap(v_.begin(), v_.end(), counting);
        sift_ops += comparisons;
    }
    void pop_front_counted(std::uint64_t& sift_ops) {
        std::uint64_t comparisons = 0;
        auto          counting    = [&](element_type const& a, element_type const& b) {
            ++comparisons;
            return comp_(a, b);
        };
        std::pop_heap(v_.begin(), v_.end(), counting);
        v_.pop_back();
        sift_ops += comparisons;
    }

private:
    std::vector<element_type> v_{};
    Compare                   comp_{};
};

// ════════════════════════════════════════════════════════════════════════════════════════════════════════
// Adapter-Observer (gattungs-eigen) — flacher uint64-POD, getrennt vom SearchAlgorithm-ObserverAggregate<17>.
// Felder spiegeln den Antrieb des inner_container (die real getriebene spezifische Achse, §28).
// ════════════════════════════════════════════════════════════════════════════════════════════════════════
struct
    AdapterObserverSnapshot { // Adapter-Observer (gattungs-eigen, getrennt vom SearchAlgorithm-ObserverAggregate<17>)
    std::uint64_t push_count        = 0; // push → inner_container
    std::uint64_t pop_count         = 0; // erfolgreiche pop_front/pop_back
    std::uint64_t front_reads       = 0; // front()-Zugriffe (FIFO-Disziplin)
    std::uint64_t back_reads        = 0; // back()/top()-Zugriffe (LIFO-Disziplin)
    std::uint64_t current_occupancy = 0; // aktuelle inner_container-Größe
    std::uint64_t peak_occupancy    = 0; // maximale inner_container-Größe
};

/// AdapterComposition — 11 geteilte/delegierte §28-Achsen + inner_container (INC-2d: isa raus).
/// Reihenfolge T0..T9 = §28-Invertebrate (delegiert + aktiv), dann Inner (spezifisch). Analog SequenceComposition.
template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9,
          class Inner = DequeInner<>>
struct AdapterComposition {
    using search_algo      = T0;    // axis_03a (delegated an inner)
    using cache_traversal  = T1;    // axis_03b (delegated)
    using memory_layout    = T2;    // axis_05  (delegated)
    using allocator        = T3;    // axis_06  (delegated)
    using prefetch         = T4;    // axis_07  (delegated)
    using concurrency      = T5;    // axis_08  (delegated)
    using serialization    = T6;    // axis_10  (aktiv)
    using value_handle     = T7;    // axis_14  (aktiv)
    using io_dispatch      = T8;    // axis_io  (delegated; INC-2d: isa raus, war T9)
    using migration_policy = T9;    // axis_migration (delegated; INC-2d: war T10)
    using inner_container  = Inner; // NEU axis_inner (Adapter-spezifisch, §28)

    static constexpr std::size_t      slot_count = 11; // 10 geteilt/delegiert + inner_container (INC-2d: isa raus)
    static constexpr std::string_view name       = "AdapterComposition";
    static constexpr std::string_view paper_id   = "P00 Adapter (Container-Tier-Unterklasse, Doku 14 §28 Invertebrate)";
};

/// IsAdapterComposition — Concept: 11 geteilte named Achsen + inner_container (§28 Invertebrate; INC-2d ohne isa).
template <class C>
concept IsAdapterComposition = requires {
    typename C::search_algo;
    typename C::cache_traversal;
    typename C::memory_layout;
    typename C::allocator;
    typename C::prefetch;
    typename C::concurrency;
    typename C::serialization;
    typename C::value_handle;
    typename C::io_dispatch;
    typename C::migration_policy;
    typename C::inner_container;
    { C::slot_count } -> std::convertible_to<std::size_t>;
};

inline constexpr std::size_t kAdapterCompositionSlotCount = 13; // frozen legacy §28-Count (nicht live slot_count)

/// AdapterAxisObservation<Composition> -- E-24 C3: die PER-ACHSEN-Sicht der Adapter-Tier-Unterklasse,
/// gebaut nach dem SearchAlgorithm-Vorbild ObserverAggregate<Composition> (observer_aggregate.hpp:93-146).
/// Reihenfolge der Member == GenusBindingTraits<Adapter>::axis_names() (Kreuz-Wache in der Test-TU
/// tests/unit/test_e24_c3_adapter_anatomy.cpp; hier waere sie ein Include-Zyklus).
/// ABGRENZUNG zu C6: in-process-Sicht, KEIN Wire-POD.
template <class Composition>
struct AdapterAxisObservation {
    snapshot_of_t<typename Composition::search_algo>      search_algo;
    snapshot_of_t<typename Composition::cache_traversal>  cache_traversal;
    snapshot_of_t<typename Composition::memory_layout>    memory_layout;
    snapshot_of_t<typename Composition::allocator>        allocator;
    snapshot_of_t<typename Composition::prefetch>         prefetch;
    snapshot_of_t<typename Composition::concurrency>      concurrency;
    snapshot_of_t<typename Composition::serialization>    serialization;
    snapshot_of_t<typename Composition::value_handle>     value_handle;
    snapshot_of_t<typename Composition::io_dispatch>      io_dispatch;
    snapshot_of_t<typename Composition::migration_policy> migration_policy;
    snapshot_of_t<typename Composition::inner_container>  inner_container;

    /// Wie viele Slots liefern echte Snapshots (Rest = EmptyAxisSnapshot)?
    [[nodiscard]] static constexpr std::size_t observable_count() noexcept {
        std::size_t n = 0;
        if constexpr (ObservableAxis<typename Composition::search_algo>) ++n;
        if constexpr (ObservableAxis<typename Composition::cache_traversal>) ++n;
        if constexpr (ObservableAxis<typename Composition::memory_layout>) ++n;
        if constexpr (ObservableAxis<typename Composition::allocator>) ++n;
        if constexpr (ObservableAxis<typename Composition::prefetch>) ++n;
        if constexpr (ObservableAxis<typename Composition::concurrency>) ++n;
        if constexpr (ObservableAxis<typename Composition::serialization>) ++n;
        if constexpr (ObservableAxis<typename Composition::value_handle>) ++n;
        if constexpr (ObservableAxis<typename Composition::io_dispatch>) ++n;
        if constexpr (ObservableAxis<typename Composition::migration_policy>) ++n;
        if constexpr (ObservableAxis<typename Composition::inner_container>) ++n;
        return n;
    }

    /// Slot-Zahl aus der LIVE-Komposition gerechnet (11), NICHT aus dem frozen legacy
    /// kAdapterCompositionSlotCount (13) -- die beiden sind am Ist verschieden und bleiben es hier.
    [[nodiscard]] static constexpr std::size_t total_slots() noexcept { return Composition::slot_count; }
};

/// AdapterAnatomy — die Container-Gattung, Adapter-Tier-Unterklasse
/// (genus()==Adapter, gattung_of→Container). Treibt die spezifische Achse inner_container REAL über die
/// §26.4-Adapter-API (push/pop/top/front/back); die 12 geteilten/delegierten Achsen werden getragen (im
/// Komposition-Typ; analog SequenceAnatomy, die growth real treibt + die 10 geteilten trägt).
/// E-24 C1 (Luecke L5): erbt die CRTP-Wache OrganGuard (Goldstandard axis_io_strategy_base.hpp:15-21) --
/// die erste Konstruktion prueft compile-hart gegen OrganConcept. Zero-cost, kein ABI-Ereignis.
template <class Composition>
class AdapterAnatomy : public OrganGuard<AdapterAnatomy<Composition>> {
public:
    using composition_t = Composition;
    using inner_t       = typename Composition::inner_container;
    using element_type  = typename inner_t::element_type;
    /// E-24 C3: der per-Achsen-Beobachtungs-Typ dieser Tier-Unterklasse (in-process, kein Wire-POD -- C6).
    using axis_observation_t = AdapterAxisObservation<Composition>;

    static constexpr std::string_view composition_name() noexcept { return Composition::name; }
    static constexpr std::string_view paper_id() noexcept { return Composition::paper_id; }
    static constexpr AnatomyGenus     genus() noexcept { return AnatomyGenus::Adapter; }         // Tier-Unterklasse
    static constexpr AnatomyGattung   gattung() noexcept { return AnatomyGattung::Container; }   // Außen-Interface
    static constexpr std::size_t      organ_count() noexcept { return Composition::slot_count; } // 11

    AdapterAnatomy() = default;
    /// capacity wird für ABI-ctor-Kompatibilität akzeptiert, aber ignoriert (unbeschränkter Adapter).
    explicit AdapterAnatomy(std::size_t /*capacity*/) noexcept {}

    // ── §26.4 Adapter-API (push/pop/top/front/back) — treibt das inner_container-Organ + Observer ──
    void put(element_type v) { push(v); } // Alias (Bestands-Aufrufe); push = die §26.4-Operation
    void push(element_type v) {
        axis_inner_container_.push_back(v);
        ++obs_.push_count;
        obs_.current_occupancy = static_cast<std::uint64_t>(axis_inner_container_.size());
        if (obs_.current_occupancy > obs_.peak_occupancy) obs_.peak_occupancy = obs_.current_occupancy;
    }
    /// FIFO-Entnahme (queue): vorderstes Element.
    [[nodiscard]] std::optional<element_type> pop_front() {
        if (axis_inner_container_.size() == 0) {
            // E-24 C6-V (Ehrlichkeits-Schliessung, Katalog C-E): pop auf LEER ist ein REALES Ereignis der
            // inner_container-Achse und war bisher unbeobachtet. Traegt der Slot eine beobachtende Huelle,
            // bekommt sie es gemeldet; eine nackte Substrat-Belegung hat den Member nicht -> `if constexpr`
            // blendet die Meldung restlos aus (zero-cost, Verhalten unveraendert).
            if constexpr (requires(inner_t& i) { i.note_underflow(); }) { axis_inner_container_.note_underflow(); }
            return std::nullopt;
        }
        element_type const v = axis_inner_container_.front();
        ++obs_.front_reads;
        axis_inner_container_.pop_front();
        ++obs_.pop_count;
        obs_.current_occupancy = static_cast<std::uint64_t>(axis_inner_container_.size());
        return v;
    }
    /// LIFO-Entnahme (stack): hinterstes Element.
    [[nodiscard]] std::optional<element_type> pop_back() {
        if (axis_inner_container_.size() == 0) {
            // E-24 C6-V: identische Ehrlichkeits-Schliessung wie in pop_front (s. dort).
            if constexpr (requires(inner_t& i) { i.note_underflow(); }) { axis_inner_container_.note_underflow(); }
            return std::nullopt;
        }
        element_type const v = axis_inner_container_.back();
        ++obs_.back_reads;
        axis_inner_container_.pop_back();
        ++obs_.pop_count;
        obs_.current_occupancy = static_cast<std::uint64_t>(axis_inner_container_.size());
        return v;
    }
    /// Bestands-Alias: get() == FIFO-Entnahme (queue-Default, §26.4 Default-Inner deque).
    [[nodiscard]] std::optional<element_type> get() { return pop_front(); }
    [[nodiscard]] std::optional<element_type> front() const {
        if (axis_inner_container_.size() == 0) return std::nullopt;
        return axis_inner_container_.front();
    }
    [[nodiscard]] std::optional<element_type> back() const {
        if (axis_inner_container_.size() == 0) return std::nullopt;
        return axis_inner_container_.back();
    }
    [[nodiscard]] std::optional<element_type> top() const { return back(); } // stack-top
    [[nodiscard]] std::size_t                 size() const noexcept { return axis_inner_container_.size(); }
    void                                      clear() noexcept {
        axis_inner_container_.clear();
        obs_.current_occupancy = 0;
    }

    /// observe_all() — EIGENER Adapter-Observer (NICHT der SearchAlgorithm-ObserverAggregate<17>).
    /// UNVERAENDERT gegenueber C2 (Wire-Spiegelung im adapter_abi_adapter); die per-Achsen-Sicht steht
    /// daneben in observe_axes().
    [[nodiscard]] AdapterObserverSnapshot observe_all() const noexcept { return obs_; }

    // -- E-24 C3: ORGAN-ACCESSOREN (einer je Slot, Reihenfolge == axis_names()) --------------------
    // Bewusst AUSGESCHRIEBEN statt makro-generiert (grep-Doktrin). Muster identisch zu set_anatomy.hpp.
    [[nodiscard]] typename Composition::search_algo&       search_algo_organ() noexcept { return axis_search_algo_; }
    [[nodiscard]] typename Composition::search_algo const& search_algo_organ() const noexcept {
        return axis_search_algo_;
    }
    [[nodiscard]] typename Composition::cache_traversal& cache_traversal_organ() noexcept {
        return axis_cache_traversal_;
    }
    [[nodiscard]] typename Composition::cache_traversal const& cache_traversal_organ() const noexcept {
        return axis_cache_traversal_;
    }
    [[nodiscard]] typename Composition::memory_layout& memory_layout_organ() noexcept { return axis_memory_layout_; }
    [[nodiscard]] typename Composition::memory_layout const& memory_layout_organ() const noexcept {
        return axis_memory_layout_;
    }
    [[nodiscard]] typename Composition::allocator&         allocator_organ() noexcept { return axis_allocator_; }
    [[nodiscard]] typename Composition::allocator const&   allocator_organ() const noexcept { return axis_allocator_; }
    [[nodiscard]] typename Composition::prefetch&          prefetch_organ() noexcept { return axis_prefetch_; }
    [[nodiscard]] typename Composition::prefetch const&    prefetch_organ() const noexcept { return axis_prefetch_; }
    [[nodiscard]] typename Composition::concurrency&       concurrency_organ() noexcept { return axis_concurrency_; }
    [[nodiscard]] typename Composition::concurrency const& concurrency_organ() const noexcept {
        return axis_concurrency_;
    }
    [[nodiscard]] typename Composition::serialization& serialization_organ() noexcept { return axis_serialization_; }
    [[nodiscard]] typename Composition::serialization const& serialization_organ() const noexcept {
        return axis_serialization_;
    }
    [[nodiscard]] typename Composition::value_handle&       value_handle_organ() noexcept { return axis_value_handle_; }
    [[nodiscard]] typename Composition::value_handle const& value_handle_organ() const noexcept {
        return axis_value_handle_;
    }
    [[nodiscard]] typename Composition::io_dispatch&       io_dispatch_organ() noexcept { return axis_io_dispatch_; }
    [[nodiscard]] typename Composition::io_dispatch const& io_dispatch_organ() const noexcept {
        return axis_io_dispatch_;
    }
    [[nodiscard]] typename Composition::migration_policy& migration_policy_organ() noexcept {
        return axis_migration_policy_;
    }
    [[nodiscard]] typename Composition::migration_policy const& migration_policy_organ() const noexcept {
        return axis_migration_policy_;
    }
    [[nodiscard]] typename Composition::inner_container& inner_container_organ() noexcept {
        return axis_inner_container_;
    }
    [[nodiscard]] typename Composition::inner_container const& inner_container_organ() const noexcept {
        return axis_inner_container_;
    }

    /// axis_organ_names() -- Praesenz-DEKLARATION der real gehaltenen Organ-Member in Slot-Reihenfolge.
    /// Die WACHE dagegen lebt in der Test-TU (Kreuz gegen GenusBindingTraits<Adapter>::axis_names()).
    [[nodiscard]] static constexpr std::array<std::string_view, 11> const& axis_organ_names() noexcept {
        static constexpr std::array<std::string_view, 11> kNames = {
            "search_algo",   "cache_traversal", "memory_layout", "allocator",        "prefetch",       "concurrency",
            "serialization", "value_handle",    "io_dispatch",   "migration_policy", "inner_container"};
        return kNames;
    }

    /// observe_axes() -- E-24 C3 / Luecke-L2-Rest: sammelt die Sub-Organ-Beobachtung Slot fuer Slot ein,
    /// exakt nach dem SA-Muster. Ein Slot ohne statistics() bleibt EmptyAxisSnapshot.
    [[nodiscard]] axis_observation_t observe_axes() const noexcept {
        axis_observation_t agg{};
        if constexpr (ObservableAxis<typename Composition::search_algo>) {
            agg.search_algo = axis_search_algo_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::cache_traversal>) {
            agg.cache_traversal = axis_cache_traversal_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::memory_layout>) {
            agg.memory_layout = axis_memory_layout_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::allocator>) { agg.allocator = axis_allocator_.statistics(); }
        if constexpr (ObservableAxis<typename Composition::prefetch>) { agg.prefetch = axis_prefetch_.statistics(); }
        if constexpr (ObservableAxis<typename Composition::concurrency>) {
            agg.concurrency = axis_concurrency_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::serialization>) {
            agg.serialization = axis_serialization_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::value_handle>) {
            agg.value_handle = axis_value_handle_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::io_dispatch>) {
            agg.io_dispatch = axis_io_dispatch_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::migration_policy>) {
            agg.migration_policy = axis_migration_policy_.statistics();
        }
        if constexpr (ObservableAxis<typename Composition::inner_container>) {
            agg.inner_container = axis_inner_container_.statistics();
        }
        return agg;
    }

    /// Diagnose: wie viele Slots liefern echte Snapshots? (Namensgleich zum SA-Vorbild.)
    [[nodiscard]] static constexpr std::size_t observable_axis_count() noexcept {
        return axis_observation_t::observable_count();
    }

private:
    // E-24 C3: die 11 Slots als REALE Organ-Member. inner_container ist der getriebene Slot (war bis C2 als
    // `inner_` das einzige Achsen-Member); die uebrigen zehn werden GETRAGEN und ueber ihre Accessoren
    // getrieben -- R5.B-Grenze ehrlich. Init-Stil wie SearchAlgorithmAnatomy:194-221 (default-init OHNE
    // `{}` fuer die getragenen; inner_container behaelt sein `{}`, weil es seit #87 so gebaut ist).
    typename Composition::search_algo      axis_search_algo_;
    typename Composition::cache_traversal  axis_cache_traversal_;
    typename Composition::memory_layout    axis_memory_layout_;
    typename Composition::allocator        axis_allocator_;
    typename Composition::prefetch         axis_prefetch_;
    typename Composition::concurrency      axis_concurrency_;
    typename Composition::serialization    axis_serialization_;
    typename Composition::value_handle     axis_value_handle_;
    typename Composition::io_dispatch      axis_io_dispatch_;
    typename Composition::migration_policy axis_migration_policy_;
    inner_t                                axis_inner_container_{};

    AdapterObserverSnapshot obs_{};
};

} // namespace comdare::cache_engine::anatomy
